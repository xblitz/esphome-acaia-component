#pragma once

#include "../acaia.h"
#include "../acaia_message.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
    namespace acaia {
        class AcaiaSoundSwitch : public esphome::switch_::Switch,
                                 public esphome::Component,
                                 public esphome::Parented<Acaia> {
          public:
            void setup() override;
            void write_state(bool state) override;
            void handle_message(AcaiaMessage message);
            void dump_config() override;
        };
    } // namespace acaia
} // namespace esphome
