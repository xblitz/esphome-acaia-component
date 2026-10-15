#pragma once
//
// Created by Magnus Nordlander on 2023-10-31.
//

#include "esphome/core/component.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "../acaia.h"
#include "../acaia_message.h"

namespace esphome {
    namespace acaia {
        class AcaiaBinarySensor
                : public esphome::Component, public esphome::Parented<Acaia> {
        public:
            void setup() override;
            void handle_message(AcaiaMessage message);
            void set_connected(esphome::binary_sensor::BinarySensor *sens);
            void set_timer_running(esphome::binary_sensor::BinarySensor *sens);
            void set_beep(esphome::binary_sensor::BinarySensor *sens);
            void dump_config() override;

        protected:
            esphome::binary_sensor::BinarySensor *connected_{nullptr};
            esphome::binary_sensor::BinarySensor *timer_running_{nullptr};
            esphome::binary_sensor::BinarySensor *beep_{nullptr};
        };
    }
}
