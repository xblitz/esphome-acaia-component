#include "acaia_switch.h"

#include "esphome/core/log.h"

static const char *TAG = "ACAIA";

namespace esphome {
    namespace acaia {
        void AcaiaSoundSwitch::setup() {
            get_parent()->add_on_message_callback([this](AcaiaMessage msg) { this->handle_message(msg); });
        }
        void AcaiaSoundSwitch::write_state(bool state) {
            get_parent()->set_sound(state);
            publish_state(state);
        }
        void AcaiaSoundSwitch::handle_message(AcaiaMessage message) {
            if (message.has_beep) {
                publish_state(message.beep);
            }
        }
        void AcaiaSoundSwitch::dump_config() { LOG_SWITCH("", "Acaia Sound Switch", this); }
    } // namespace acaia
} // namespace esphome
