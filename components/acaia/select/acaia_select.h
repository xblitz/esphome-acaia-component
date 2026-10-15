#pragma once

#include "../acaia.h"
#include "../acaia_message.h"
#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

#include <string>

namespace esphome {
    namespace acaia {
        class AcaiaUnitSelect : public esphome::select::Select,
                                public esphome::Component,
                                public esphome::Parented<Acaia> {
          public:
            void setup() override;
            void control(const std::string &value) override;
            void handle_message(AcaiaMessage message);
            void dump_config() override;
        };
    } // namespace acaia
} // namespace esphome
