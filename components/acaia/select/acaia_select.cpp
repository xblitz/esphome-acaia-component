#include "acaia_select.h"

#include "esphome/core/log.h"

static const char* TAG = "ACAIA";

namespace esphome {
    namespace acaia {
void AcaiaUnitSelect::setup() {
    get_parent()->add_on_message_callback([this](AcaiaMessage msg) { this->handle_message(msg); });
}
void AcaiaUnitSelect::control(const std::string &value) {
    get_parent()->set_unit(value);
    if (value == "g" || value == "oz") {
        publish_state(value);
    }
}
void AcaiaUnitSelect::handle_message(AcaiaMessage message) {
    if (message.has_units) {
        publish_state(message.units);
    }
}
void AcaiaUnitSelect::dump_config() {
    LOG_SELECT("", "Acaia Unit Select", this);
}
    }
}
