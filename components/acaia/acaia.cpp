#include "acaia.h"

#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/core/log.h"

#include <esp_gattc_api.h>
#include <esp_timer.h>

#include <cmath>
#include <cstring>

static const char* TAG = "ACAIA";

constexpr const char *SCALE_SERVICE_UUID = "00001820-0000-1000-8000-00805f9b34fb";
constexpr const char *SCALE_CHARACTERISTIC_UUID = "00002a80-0000-1000-8000-00805f9b34fb";
constexpr uint8_t MAGIC1 = 0xef;
constexpr uint8_t MAGIC2 = 0xdd;
constexpr uint8_t PACKET_TYPE_MESSAGE = 0x0c;
constexpr uint8_t PACKET_TYPE_SETTINGS = 0x08;
constexpr uint8_t ACAIA_MESSAGE_TYPE_IDENT = 11;
constexpr uint8_t ACAIA_MESSAGE_TYPE_NOTIFICATION_REQUEST = 12;
constexpr uint8_t ACAIA_MESSAGE_TYPE_HEARTBEAT = 0;
constexpr uint8_t ACAIA_MESSAGE_TYPE_TARE = 4;
constexpr uint8_t ACAIA_MESSAGE_TYPE_TIMER_MANIPULATION = 13;
constexpr uint8_t ACAIA_EVENT_WEIGHT = 0x05;
constexpr uint8_t ACAIA_EVENT_TIMER = 0x07;
constexpr uint8_t ACAIA_EVENT_BUTTON = 0x08;
constexpr uint8_t ACAIA_EVENT_HEARTBEAT = 0x0B;
constexpr uint8_t ACAIA_RECORD_WEIGHT = 5;
constexpr uint8_t ACAIA_RECORD_BATTERY = 6;
constexpr uint8_t ACAIA_RECORD_TIMER = 7;
constexpr uint8_t ACAIA_RECORD_BUTTON = 8;
constexpr uint8_t ACAIA_RECORD_ACK = 11;
constexpr uint8_t ACAIA_MESSAGE_TYPE_SETTING_CHANGE = 10;
constexpr uint8_t SETTING_ITEM_UNIT = 0;
constexpr uint8_t SETTING_ITEM_SLEEP = 1;
constexpr uint8_t SETTING_ITEM_SOUND = 5;
constexpr uint8_t SETTINGS_UNIT_GRAMS = 2;
constexpr uint8_t SETTINGS_UNIT_OUNCES = 5;
constexpr float OUNCES_TO_GRAMS = 28.349523125f;

// Mode key codes, shared by button announcements and settings byte 6.
static const char *mode_name_for_key(uint8_t key) {
    switch (key) {
        case 0x02:
            return "Weighing";
        case 0x04:
            return "Dual Display";
        case 0x03:
            return "Droplet";
        case 0x0F:
            return "Droplet and Square";
        case 0x10:
            return "Triangle and Square";
        case 0x11:
            return "Auto-Tare";
        default:
            return nullptr;
    }
}

namespace esphome {
    namespace acaia {
void Acaia::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTC_CONNECT_EVT:
            ESP_LOGD(TAG, "Connected");
        break;
        case ESP_GATTC_OPEN_EVT: {
            ESP_LOGD(TAG, "ESP_GATTC_OPEN_EVT");
            connected_ = true;

            AcaiaMessage message;
            message.timestamp = esp_timer_get_time();
            message.connected = connected_;
            message.has_timer_running = true;
            message.timer_running = timer_running_;
            send_message_to_handlers(message);

            break;
        }
        case ESP_GATTC_SEARCH_CMPL_EVT: {
            ESP_LOGD(TAG, "Registering for notifications");

            auto *chr = this->parent_->get_characteristic(
                    esphome::esp32_ble::ESPBTUUID::from_raw(SCALE_SERVICE_UUID),
                    esphome::esp32_ble::ESPBTUUID::from_raw(SCALE_CHARACTERISTIC_UUID)
            );
            if (chr == nullptr) {
                ESP_LOGW(TAG, "[%s] Scale service not found on device, is it an Acaia scale?", this->parent_->address_str());
                break;
            }
            this->char_handle_ = chr->handle;

            auto status = this->parent_->register_for_notify(chr->handle);
            if (status) {
                ESP_LOGW(TAG, "[%s] register_for_notify failed, status=%d", this->parent_->address_str(), status);
            }
            break;
        }
        case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
            ESP_LOGD(TAG, "Registered for notifications, getting client characteristic");

            auto *chr = this->parent_->get_config_descriptor(
                    this->char_handle_
            );
            if (chr == nullptr) {
                ESP_LOGW(TAG, "[%s] No client config descriptor found", this->parent_->address_str());
                break;
            }

            uint16_t notify_en = 1;
            esp_err_t status =
                    esp_ble_gattc_write_char_descr(this->parent_->get_gattc_if(), this->parent_->get_conn_id(), chr->handle, sizeof(notify_en),
                                                   (uint8_t *) &notify_en, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
            if (status) {
                ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char_descr error, status=%d", this->parent_->address_str(), status);
            }

            ident();
            notification_request();
            last_heartbeat_ = esp_timer_get_time();
            // Done with the parent's GATT cache; allow it to be released.
            this->node_state = esphome::esp32_ble_tracker::ClientState::ESTABLISHED;
            break;
        }
        case ESP_GATTC_NOTIFY_EVT: {
            if (param->notify.handle != this->char_handle_)
                break;

            if (packet_buffer_head_ + param->notify.value_len >= PACKET_BUFFER_SIZE) {
                ESP_LOGE(TAG, "[%s] Received packet would overflow the packet buffer. Flushing.", this->parent_->address_str());
                packet_buffer_head_ = 0;
                break;
            }
            memcpy(packet_buffer_ + packet_buffer_head_, param->notify.value, param->notify.value_len);
            packet_buffer_head_ += param->notify.value_len;

            // A single notification may carry more than one frame; drain them all.
            while (true) {
                if (packet_buffer_head_ < 2) {
                    break;
                }

                if (packet_buffer_[0] != MAGIC1 || packet_buffer_[1] != MAGIC2) {
                    ESP_LOGW(TAG, "[%s] Packet doesn't start with the magic numbers. Flushing.", this->parent_->address_str());
                    packet_buffer_head_ = 0;
                    break;
                }

                if (packet_buffer_head_ < 4) {
                    break;
                }

                auto packet_len = packet_buffer_[3] + 5;

                if (packet_len > PACKET_BUFFER_SIZE) {
                    ESP_LOGW(TAG, "[%s] Impossible packet length %d, flushing.", this->parent_->address_str(), packet_len);
                    packet_buffer_head_ = 0;
                    break;
                }

                if (packet_buffer_head_ < packet_len) {
                    break;
                }

                if (!verify_checksum(packet_len)) {
                    ESP_LOGW(TAG, "[%s] Checksum mismatch, discarding packet.", this->parent_->address_str());
                    packet_buffer_head_ = 0;
                    break;
                }

                auto packet_type = packet_buffer_[2];

                if (packet_type == PACKET_TYPE_SETTINGS) {
                    // Battery byte top bit mirrors the timer state.
                    timer_running_ = (packet_buffer_[4] & 0x80) != 0;

                    AcaiaMessage message;
                    message.timestamp = esp_timer_get_time();
                    message.connected = connected_;
                    message.has_timer_running = true;
                    message.timer_running = timer_running_;
                    message.has_battery = true;
                    message.battery = packet_buffer_[4] & 0x7F;
                    if (packet_len >= 6) {
                        const uint8_t units_code = packet_buffer_[5];
                        if (units_code == SETTINGS_UNIT_GRAMS || units_code == SETTINGS_UNIT_OUNCES) {
                            weight_units_ = units_code == SETTINGS_UNIT_OUNCES ? WeightUnits::OUNCES
                                                                              : WeightUnits::GRAMS;
                            message.has_units = true;
                            message.units = units_code == SETTINGS_UNIT_OUNCES ? "oz" : "g";
                        } else {
                            ESP_LOGW(TAG, "[%s] Unknown weight unit code %u, keeping previous.", this->parent_->address_str(), units_code);
                        }
                    }
                    if (packet_len >= 7) {
                        const char *mode = mode_name_for_key(packet_buffer_[6]);
                        if (mode != nullptr) {
                            message.has_mode = true;
                            message.mode = mode;
                        } else {
                            ESP_LOGW(TAG, "[%s] Unknown mode key %02X in settings, ignoring.",
                                     this->parent_->address_str(), packet_buffer_[6]);
                        }
                    }
                    if (packet_len >= 10) {
                        message.has_auto_off = true;
                        message.auto_off = packet_buffer_[7] * 5;
                        message.has_beep = true;
                        message.beep = packet_buffer_[9] == 1;
                    }
                    ESP_LOGI(TAG, "Settings packet, battery %d%%", (int) message.battery);
                    {
                        char hex[format_hex_pretty_size(PACKET_BUFFER_SIZE)];
                        ESP_LOGD(TAG, "[%s] Settings frame: %s", this->parent_->address_str(),
                                 format_hex_pretty_to(hex, packet_buffer_, packet_len));
                    }
                    send_message_to_handlers(message);
                } else if (packet_type == PACKET_TYPE_MESSAGE) {
                    // Message
                    auto message_type = packet_buffer_[4];

                    AcaiaMessage message;

                    message.timestamp = esp_timer_get_time();

                    message.connected = connected_;
                    message.has_timer_running = true;
                    message.timer_running = timer_running_;

                    switch (message_type) {
                        case ACAIA_EVENT_WEIGHT: {
                            if (packet_len < 13) {
                                ESP_LOGW(TAG, "[%s] Truncated weight message, ignoring.", this->parent_->address_str());
                                break;
                            }
                            message.type = AcaiaMessageType::weight;
                            message.has_weight = true;
                            message.weight = decode_weight((packet_buffer_[7] << 16) | (packet_buffer_[6] << 8) | packet_buffer_[5], packet_buffer_[9], packet_buffer_[10]);
                            // TEMP-DIAG: correlate weight-record byte 3 with flow; remove after the flow fix.
                            if (packet_buffer_[8] != 0x00) {
                                ESP_LOGD(TAG, "[%s] Weight byte3=%02X weight=%.2f", this->parent_->address_str(),
                                         packet_buffer_[8], message.weight);
                            }
                            parse_record_chain(11, packet_len - 2, message);

                            break;
                        }
                        case ACAIA_EVENT_TIMER: {
                            if (packet_len < 10) {
                                ESP_LOGW(TAG, "[%s] Truncated timer message, ignoring.", this->parent_->address_str());
                                break;
                            }
                            message.type = AcaiaMessageType::timer;
                            message.has_time = true;
                            message.time = decode_time(packet_buffer_[5], packet_buffer_[6], packet_buffer_[7]);
                            parse_record_chain(8, packet_len - 2, message);

                            break;
                        }
                        case ACAIA_EVENT_BUTTON: {
                            message.type = AcaiaMessageType::tare_start_stop_reset;

                            if (packet_len < 8) {
                                ESP_LOGW(TAG, "[%s] Truncated button message, ignoring.", this->parent_->address_str());
                                break;
                            }
                            const uint8_t key = packet_buffer_[5];
                            handle_button_key(key, message, packet_len);
                            parse_record_chain(6, packet_len - 2, message);

                            break;
                        }
                        case ACAIA_EVENT_HEARTBEAT: {
                            message.type = heartbeat;
                            parse_record_chain(7, packet_len - 2, message);

                            break;
                        }
                        default: {
                            char hex[format_hex_pretty_size(PACKET_BUFFER_SIZE)];
                            ESP_LOGW(TAG, "[%s] Unknown message type 0x%02X: %s", this->parent_->address_str(), message_type,
                                     format_hex_pretty_to(hex, packet_buffer_, packet_len));
                            break;
                        }
                    }

                    send_message_to_handlers(message);
                } else if (packet_type == 0x07) {
                    // Pre-auth oddity frame; known-benign, ignore.
                } else {
                    char hex[format_hex_pretty_size(PACKET_BUFFER_SIZE)];
                    ESP_LOGW(TAG, "[%s] Unknown packet type 0x%02X: %s", this->parent_->address_str(), packet_type,
                             format_hex_pretty_to(hex, packet_buffer_, packet_len));
                }

                // Drop the consumed packet, keep any pipelined remainder.
                packet_buffer_head_ -= packet_len;
                memmove(packet_buffer_, packet_buffer_ + packet_len, packet_buffer_head_);
            }

            break;
        }
        case ESP_GATTC_CLOSE_EVT: {
            connected_ = false;
            weight_units_ = WeightUnits::GRAMS;
            timer_running_ = false;
            char_handle_ = 0;
            last_heartbeat_ = 0;
            packet_buffer_head_ = 0;

            AcaiaMessage message;
            message.timestamp = esp_timer_get_time();
            message.connected = connected_;
            message.has_timer_running = true;
            message.timer_running = timer_running_;
            send_message_to_handlers(message);

            break;
        }
        default:
            ESP_LOGD(TAG, "GATTC Event incoming, type %u", event);
    }
}

void Acaia::add_on_message_callback(std::function<void(AcaiaMessage)> callback) {
    this->on_message_callback_.add(std::move(callback));
}
void Acaia::add_on_tare_callback(std::function<void()> callback) {
    this->on_tare_callback_.add(std::move(callback));
}
void Acaia::add_on_timer_start_callback(std::function<void()> callback) {
    this->on_timer_start_callback_.add(std::move(callback));
}
void Acaia::add_on_timer_stop_callback(std::function<void()> callback) {
    this->on_timer_stop_callback_.add(std::move(callback));
}
void Acaia::add_on_timer_reset_callback(std::function<void()> callback) {
    this->on_timer_reset_callback_.add(std::move(callback));
}

void Acaia::dump_config() {
    ESP_LOGCONFIG(TAG, "Acaia Scale:");
    ESP_LOGCONFIG(TAG, "  BLE Client Address: %s", this->parent_->address_str());
}

void Acaia::send_tare() {
    ESP_LOGD(TAG, "Sending Tare");
    uint8_t payload[] = {0x00};

    uint8_t encoded_payload[sizeof(payload) + 5];

    encode(ACAIA_MESSAGE_TYPE_TARE, payload, sizeof(payload), encoded_payload, sizeof(encoded_payload));

    write_to_scale_characteristic(encoded_payload, sizeof(encoded_payload));
}

void Acaia::reset_timer() {
    ESP_LOGD(TAG, "Resetting timer");
    timer_running_ = false;
    uint8_t payload[] = {0x00, 0x01};

    uint8_t encoded_payload[sizeof(payload) + 5];

    encode(ACAIA_MESSAGE_TYPE_TIMER_MANIPULATION, payload, sizeof(payload), encoded_payload, sizeof(encoded_payload));

    write_to_scale_characteristic(encoded_payload, sizeof(encoded_payload));
}

void Acaia::start_timer() {
    ESP_LOGD(TAG, "Starting timer");
    timer_running_ = true;
    uint8_t payload[] = {0x00, 0x00};

    uint8_t encoded_payload[sizeof(payload) + 5];

    encode(ACAIA_MESSAGE_TYPE_TIMER_MANIPULATION, payload, sizeof(payload), encoded_payload, sizeof(encoded_payload));

    write_to_scale_characteristic(encoded_payload, sizeof(encoded_payload));
}

void Acaia::stop_timer() {
    ESP_LOGD(TAG, "Stopping timer");
    timer_running_ = false;
    uint8_t payload[] = {0x00, 0x02};

    uint8_t encoded_payload[sizeof(payload) + 5];

    encode(ACAIA_MESSAGE_TYPE_TIMER_MANIPULATION, payload, sizeof(payload), encoded_payload, sizeof(encoded_payload));

    write_to_scale_characteristic(encoded_payload, sizeof(encoded_payload));
}

void Acaia::set_unit(const std::string &unit) {
    if (unit == "g") {
        send_setting_change(SETTING_ITEM_UNIT, SETTINGS_UNIT_GRAMS);
    } else if (unit == "oz") {
        send_setting_change(SETTING_ITEM_UNIT, SETTINGS_UNIT_OUNCES);
    } else {
        ESP_LOGW(TAG, "[%s] Unknown unit '%s', ignoring.", this->parent_->address_str(), unit.c_str());
    }
}
void Acaia::set_sleep(int minutes) {
    uint8_t code;
    switch (minutes) {
        case 0:
            code = 0;
            break;
        case 5:
            code = 1;
            break;
        case 10:
            code = 2;
            break;
        case 20:
            code = 3;
            break;
        case 30:
            code = 4;
            break;
        case 60:
            code = 5;
            break;
        default:
            ESP_LOGW(TAG, "[%s] Unsupported sleep timeout %d min, ignoring.", this->parent_->address_str(),
                     minutes);
            return;
    }
    send_setting_change(SETTING_ITEM_SLEEP, code);
}
void Acaia::set_sound(bool enabled) {
    send_setting_change(SETTING_ITEM_SOUND, enabled ? 1 : 0);
}
void Acaia::send_setting_change(uint8_t item, uint8_t value) {
    ESP_LOGD(TAG, "Sending setting change, item %u value %u", item, value);
    uint8_t payload[] = {0x00, item, value};

    uint8_t encoded_payload[sizeof(payload) + 5];

    encode(ACAIA_MESSAGE_TYPE_SETTING_CHANGE, payload, sizeof(payload), encoded_payload, sizeof(encoded_payload));

    write_to_scale_characteristic(encoded_payload, sizeof(encoded_payload));
}

void Acaia::send_message_to_handlers(AcaiaMessage message) {
    if (message.has_weight && this->weight_units_ == WeightUnits::OUNCES) {
        message.weight *= OUNCES_TO_GRAMS;
    }
    this->on_message_callback_.call(message);
}

void Acaia::parse_record_chain(size_t offset, size_t end, AcaiaMessage &message) {
    size_t pos = offset;
    while (pos < end) {
        const uint8_t tag = packet_buffer_[pos];
        size_t body_len;
        switch (tag) {
            case ACAIA_RECORD_WEIGHT:
                body_len = 6;
                break;
            case ACAIA_RECORD_BATTERY:
                body_len = 1;
                break;
            case ACAIA_RECORD_TIMER:
                body_len = 3;
                break;
            case ACAIA_RECORD_BUTTON:
                body_len = 1;
                break;
            case ACAIA_RECORD_ACK:
                body_len = 2;
                break;
            default: {
                char hex[format_hex_pretty_size(PACKET_BUFFER_SIZE)];
                ESP_LOGW(TAG, "[%s] Unknown record tag %02X: %s", this->parent_->address_str(), tag,
                         format_hex_pretty_to(hex, packet_buffer_, end + 2));
                return;
            }
        }
        if (pos + 1 + body_len > end) {
            ESP_LOGW(TAG, "[%s] Truncated record tag %02X, ignoring.", this->parent_->address_str(), tag);
            return;
        }
        const size_t body = pos + 1;
        switch (tag) {
            case ACAIA_RECORD_WEIGHT:
                message.has_weight = true;
                message.weight = decode_weight((packet_buffer_[body + 2] << 16) | (packet_buffer_[body + 1] << 8) |
                                                     packet_buffer_[body],
                                                 packet_buffer_[body + 4], packet_buffer_[body + 5]);
                // TEMP-DIAG: correlate weight-record byte 3 with flow; remove after the flow fix.
                if (packet_buffer_[body + 3] != 0x00) {
                    ESP_LOGD(TAG, "[%s] Chained weight byte3=%02X weight=%.2f", this->parent_->address_str(),
                             packet_buffer_[body + 3], message.weight);
                }
                break;
            case ACAIA_RECORD_BATTERY:
                // Assume the record shares the settings frame's encoding.
                message.has_battery = true;
                message.battery = packet_buffer_[body] & 0x7F;
                break;
            case ACAIA_RECORD_TIMER:
                message.has_time = true;
                message.time = decode_time(packet_buffer_[body], packet_buffer_[body + 1], packet_buffer_[body + 2]);
                break;
            case ACAIA_RECORD_BUTTON:
                handle_button_key(packet_buffer_[body], message, end + 2);
                break;
            case ACAIA_RECORD_ACK:
                break;
        }
        pos += 1 + body_len;
    }
}
void Acaia::handle_button_key(uint8_t key, AcaiaMessage &message, size_t packet_len) {
    switch (key) {
        case 0x00:
            message.button = tare;
            this->on_tare_callback_.call();
            break;
        case 0x08:
            message.button = start;
            timer_running_ = true;
            this->on_timer_start_callback_.call();
            break;
        case 0x0A:
            message.button = stop;
            timer_running_ = false;
            this->on_timer_stop_callback_.call();
            break;
        case 0x09:
            message.button = reset;
            timer_running_ = false;
            this->on_timer_reset_callback_.call();
            break;
        case 0x02:
        case 0x04:
        case 0x03:
        case 0x0F:
        case 0x10:
        case 0x11:
            // Key is one of the known mode keys here; never null.
            message.has_mode = true;
            message.mode = mode_name_for_key(key);
            break;
        default: {
            char hex[format_hex_pretty_size(PACKET_BUFFER_SIZE)];
            ESP_LOGW(TAG, "[%s] Unknown button key %02X: %s", this->parent_->address_str(), key,
                     format_hex_pretty_to(hex, packet_buffer_, packet_len));
            break;
        }
    }
}

float Acaia::decode_weight(uint32_t raw, uint8_t unit, uint8_t signifier) {
    float divisor = powf(10.f, (float)unit);
    float multiplier = ((signifier & 0x02) == 0x02) ? -1.f : 1.f;
    return ((float)raw / divisor) * multiplier;
}

float Acaia::decode_time(uint8_t minutes, uint8_t seconds, uint8_t tens) {
    float value = minutes * 60;
    value = value + seconds;
    return value + ((float)tens / 10.f);
}

void Acaia::loop() {
    if (!this->connected_ || this->char_handle_ == 0) {
        return;
    }
    if (esp_timer_get_time() - this->last_heartbeat_ > 1000 * 1000) {
        this->send_heartbeat();
    }
}

void Acaia::send_heartbeat() {
    ESP_LOGD(TAG, "Sending Heartbeat");

    uint8_t payload[2] = {0x02, 0x00};
    uint8_t encoded_payload[sizeof(payload) + 5];

    encode(ACAIA_MESSAGE_TYPE_HEARTBEAT, payload, sizeof(payload), encoded_payload, sizeof(encoded_payload));

    write_to_scale_characteristic(encoded_payload, sizeof(encoded_payload));
    last_heartbeat_ = esp_timer_get_time();
}

void Acaia::ident() {
    ESP_LOGD(TAG, "Sending Ident");
    uint8_t payload[15] = {0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d,
                         0x2d, 0x2d, 0x2d};

    uint8_t encoded_payload[sizeof(payload) + 5];

    encode(ACAIA_MESSAGE_TYPE_IDENT, payload, sizeof(payload), encoded_payload, sizeof(encoded_payload));

    write_to_scale_characteristic(encoded_payload, sizeof(encoded_payload));
}

void Acaia::notification_request() {
    ESP_LOGD(TAG, "Requesting notifications");

    uint8_t payload[] = {
            9, // Length of notification request
            0, // weight
            1, // weight argument
            1, // battery
            2, // battery argument
            2, // timer
            5, // timer argument (number heartbeats between timer messages)
            3, // key
            4, // setting
    };

    uint8_t encoded_buf[sizeof(payload) + 5];

    encode(ACAIA_MESSAGE_TYPE_NOTIFICATION_REQUEST, payload, sizeof(payload), encoded_buf, sizeof(encoded_buf));

    write_to_scale_characteristic(encoded_buf, sizeof(encoded_buf));
}

bool Acaia::verify_checksum(size_t packet_len) {
    // Inverse of encode(): checksums cover the length byte through the end
    // of the payload, split into even/odd index sums.
    uint8_t cksum1 = 0;
    uint8_t cksum2 = 0;
    for (size_t i = 0; i < packet_len - 5; i++) {
        if (i % 2 == 0) {
            cksum1 += packet_buffer_[3 + i];
        } else {
            cksum2 += packet_buffer_[3 + i];
        }
    }
    return cksum1 == packet_buffer_[packet_len - 2] && cksum2 == packet_buffer_[packet_len - 1];
}

size_t Acaia::encode(uint8_t message_type, uint8_t* in_buf, size_t len, uint8_t* out_buf, size_t out_buf_len)
{
    if (out_buf_len < len + 5) {
        ESP_LOGE(TAG, "[%s] Invalid size on output buffer for encoding message of type %u", this->parent_->address_str(), message_type);
        return 0;
    }

    out_buf[0] = MAGIC1;
    out_buf[1] = MAGIC2;
    out_buf[2] = message_type;
    uint8_t cksum1 = 0;
    uint8_t cksum2 = 0;
    for (uint16_t i = 0; i < len; i++) {
        uint8_t val = in_buf[i] & 0xff;
        out_buf[3 + i] = val;
        if (i % 2 == 0) {
            cksum1 += val;
        } else {
            cksum2 += val;
        }
    }
    out_buf[len + 3] = cksum1 & 0xff;
    out_buf[len + 4] = cksum2 & 0xff;

    return len + 5;
}

void Acaia::write_to_scale_characteristic(uint8_t* buf, size_t len)
{
    auto status =
        esp_ble_gattc_write_char(this->parent_->get_gattc_if(), this->parent_->get_conn_id(), this->char_handle_,
                             len, buf, ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
    if (status) {
        ESP_LOGW(TAG, "[%s] esp_ble_gattc_write_char failed, status=%d", this->parent_->address_str(),
                 status);
    }
}

    }
}
