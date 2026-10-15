#pragma once
//
// Created by Magnus Nordlander on 2023-10-26.
//

#include <cmath>
#include <deque>
#include <vector>
#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/helpers.h"
#include "../Acaia.h"
#include "../AcaiaMessage.h"

constexpr size_t FLOW_MOVING_AVERAGE_SIZE = 20;

namespace esphome {
    namespace acaia {
        class AcaiaMeasurement {
        public:
            explicit AcaiaMeasurement(int64_t timestamp, float weight) : timestamp(timestamp), weight(weight) {}

            int64_t timestamp;
            float weight;
        };

        class AcaiaSensor
                : public esphome::Component, public esphome::Parented<Acaia> {
        public:
            void setup() override {
                flows_.reserve(FLOW_MOVING_AVERAGE_SIZE - 1);
                get_parent()->add_on_message_callback([this](AcaiaMessage msg) { this->handle_message(msg); });
            }

            void handle_message(AcaiaMessage message) {
                if (message.has_weight) {
                    weight_history_.emplace_back(message.timestamp, message.weight);

                    while (weight_history_.size() > FLOW_MOVING_AVERAGE_SIZE) {
                        weight_history_.pop_front();
                    }

                    
                    float weight_in_g = message.weight;
                    if (should_set(weight_, weight_in_g, 0.01)) {
                        weight_->publish_state(weight_in_g);
                    }
                }

                float avg_flow = calculate_avg_flow();

                if (!std::isnan(avg_flow) && should_set(flow_, avg_flow, 0.1)) {
                    flow_->publish_state(avg_flow);
                }
            }

            float calculate_avg_flow()
            {
                flows_.clear();
                int64_t now = esp_timer_get_time();

                for (uint16_t i = 3; i < weight_history_.size(); i++) {
                    auto prev = weight_history_.at(i - 3);
                    auto curr = weight_history_.at(i);

                    if ((now - prev.timestamp) > 5000000) {
                        continue;
                    }

                    float time_diff_s = ((float) (curr.timestamp - prev.timestamp)) / 1000000;
                    float weight_diff = curr.weight - prev.weight;

                    if (fabs(weight_diff) < 20 && time_diff_s >= 0.25 && time_diff_s < 0.35) {
                        flows_.push_back(weight_diff / time_diff_s);
                    }
                }

                if (flows_.empty()) {
                    return NAN;
                }

                float sum_flows = 0;
                for (auto f : flows_) {
                    sum_flows += f;
                }

                return sum_flows / (float)flows_.size();
            }

            bool should_set(esphome::sensor::Sensor *sensor, float value, float sigma = 0.1)
            {
                return sensor != nullptr && (!sensor->has_state() || fabs(sensor->get_raw_state() - value) >= sigma);
            }

            void set_weight(esphome::sensor::Sensor *sens) { weight_ = sens; }
            void set_flow(esphome::sensor::Sensor *sens) { flow_ = sens; }

            void dump_config() override {
                LOG_SENSOR("  ", "Weight", this->weight_);
                LOG_SENSOR("  ", "Flow", this->flow_);
            }
        protected:
            esphome::sensor::Sensor *weight_{nullptr};
            esphome::sensor::Sensor *flow_{nullptr};

            std::deque<AcaiaMeasurement> weight_history_;
            std::vector<float> flows_;
        };
    }
}
