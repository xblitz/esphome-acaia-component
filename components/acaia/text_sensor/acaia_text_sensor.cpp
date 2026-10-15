#include "acaia_text_sensor.h"

#include "esphome/core/log.h"

static const char* TAG = "ACAIA";

namespace esphome {
    namespace acaia {
void AcaiaTextSensor::setup() {
    get_parent()->add_on_message_callback([this](AcaiaMessage msg) { this->handle_message(msg); });
}
void AcaiaTextSensor::handle_message(AcaiaMessage message) {
    if (units_ != nullptr && message.has_units) {
        units_->publish_state(message.units);
    }
}
void AcaiaTextSensor::set_units(esphome::text_sensor::TextSensor *sens) { units_ = sens; }
void AcaiaTextSensor::dump_config() {
    LOG_TEXT_SENSOR("  ", "Units", this->units_);
}
    }
}
