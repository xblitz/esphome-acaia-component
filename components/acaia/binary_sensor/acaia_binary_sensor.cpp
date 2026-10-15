#include "acaia_binary_sensor.h"

#include "esphome/core/log.h"

static const char* TAG = "ACAIA";

namespace esphome {
    namespace acaia {
void AcaiaBinarySensor::setup() {
    get_parent()->add_on_message_callback([this](AcaiaMessage msg) { this->handle_message(msg); });
}

void AcaiaBinarySensor::handle_message(AcaiaMessage message) {
    if (connected_ != nullptr) {
        connected_->publish_state(message.connected);
    }
}

void AcaiaBinarySensor::set_connected(esphome::binary_sensor::BinarySensor *sens) { connected_ = sens; }

void AcaiaBinarySensor::dump_config() {
    LOG_BINARY_SENSOR("  ", "Connected", this->connected_);
}

    }
}
