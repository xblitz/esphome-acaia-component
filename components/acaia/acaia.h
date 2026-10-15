#pragma once
//
// Created by Magnus Nordlander on 2023-10-24.
//

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/ble_client/ble_client.h"
#include "acaia_message.h"

#include <esp_gattc_api.h>

#include <cstdint>
#include <functional>

constexpr size_t PACKET_BUFFER_SIZE = 64;

namespace esphome {
    namespace acaia {
        class Acaia : public esphome::Component, public esphome::ble_client::BLEClientNode {
        public:
            void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;
            void loop() override;
            void add_on_message_callback(std::function<void(AcaiaMessage)> callback);
            void dump_config() override;
            void send_tare();
            void reset_timer();
            void start_timer();
            void stop_timer();

        protected:
            void send_message_to_handlers(AcaiaMessage message);
            float decode_weight(uint32_t raw, uint8_t unit, uint8_t signifier);
            float decode_time(uint8_t minutes, uint8_t seconds, uint8_t tens);
            void send_heartbeat();
            void ident();
            void notification_request();
            size_t encode(uint8_t message_type, uint8_t* in_buf, size_t len, uint8_t* out_buf, size_t out_buf_len);
            bool verify_checksum(size_t packet_len);
            void write_to_scale_characteristic(uint8_t* buf, size_t len);

            bool connected_ = false;

            uint16_t char_handle_ = 0;
            uint64_t last_heartbeat_ = 0;

            uint8_t packet_buffer_[PACKET_BUFFER_SIZE]{};
            uint16_t packet_buffer_head_ = 0;

            CallbackManager<void(AcaiaMessage)> on_message_callback_;
        };
    }
}
