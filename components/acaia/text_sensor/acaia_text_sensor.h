#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "../acaia.h"
#include "../acaia_message.h"

namespace esphome {
    namespace acaia {
        class AcaiaTextSensor
                : public esphome::Component, public esphome::Parented<Acaia> {
        public:
            void setup() override;
            void handle_message(AcaiaMessage message);
            void set_units(esphome::text_sensor::TextSensor *sens);
            void dump_config() override;

        protected:
            esphome::text_sensor::TextSensor *units_{nullptr};
        };
    }
}
