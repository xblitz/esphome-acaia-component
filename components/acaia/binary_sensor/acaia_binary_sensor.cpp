#include "acaia_binary_sensor.h"

#include "esphome/core/log.h"

static const char *TAG = "ACAIA";

namespace esphome {
    namespace acaia {
        void AcaiaBinarySensor::setup() {
            get_parent()->add_on_message_callback([this](AcaiaMessage msg) { this->handle_message(msg); });
        }

        void AcaiaBinarySensor::handle_message(AcaiaMessage message) {
            if (connected_ != nullptr) {
                connected_->publish_state(message.connected);
            }
            if (timer_running_ != nullptr && message.has_timer_running) {
                timer_running_->publish_state(message.timer_running);
            }
            if (beep_ != nullptr && message.has_beep) {
                beep_->publish_state(message.beep);
            }
        }

        void AcaiaBinarySensor::set_connected(esphome::binary_sensor::BinarySensor *sens) { connected_ = sens; }
        void AcaiaBinarySensor::set_timer_running(esphome::binary_sensor::BinarySensor *sens) { timer_running_ = sens; }
        void AcaiaBinarySensor::set_beep(esphome::binary_sensor::BinarySensor *sens) { beep_ = sens; }

        void AcaiaBinarySensor::dump_config() {
            LOG_BINARY_SENSOR("  ", "Connected", this->connected_);
            LOG_BINARY_SENSOR("  ", "Timer Running", this->timer_running_);
            LOG_BINARY_SENSOR("  ", "Beep", this->beep_);
        }

    } // namespace acaia
} // namespace esphome
