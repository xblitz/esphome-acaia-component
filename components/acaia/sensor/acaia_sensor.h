#pragma once
//
// Created by Magnus Nordlander on 2023-10-26.
//

#include "../acaia.h"
#include "../acaia_message.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include <cstdint>
#include <deque>
#include <vector>

namespace esphome {
    namespace acaia {
        class AcaiaMeasurement {
          public:
            explicit AcaiaMeasurement(int64_t timestamp, float weight) : timestamp(timestamp), weight(weight) {}

            int64_t timestamp;
            float weight;
        };

        class AcaiaSensor : public esphome::Component, public esphome::Parented<Acaia> {
          public:
            void setup() override;
            void handle_message(AcaiaMessage message);
            float calculate_avg_flow();
            bool should_set(esphome::sensor::Sensor *sensor, float value, float sigma = 0.1);
            void set_weight(esphome::sensor::Sensor *sens);
            void set_flow(esphome::sensor::Sensor *sens);
            void set_timer(esphome::sensor::Sensor *sens);
            void set_battery(esphome::sensor::Sensor *sens);
            void set_auto_off(esphome::sensor::Sensor *sens);
            void dump_config() override;

          protected:
            esphome::sensor::Sensor *weight_{nullptr};
            esphome::sensor::Sensor *flow_{nullptr};
            esphome::sensor::Sensor *timer_{nullptr};
            esphome::sensor::Sensor *battery_{nullptr};
            esphome::sensor::Sensor *auto_off_{nullptr};

            std::deque<AcaiaMeasurement> weight_history_;
            std::vector<float> flows_;
        };
    } // namespace acaia
} // namespace esphome
