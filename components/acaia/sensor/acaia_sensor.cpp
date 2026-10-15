#include "acaia_sensor.h"

#include "esphome/core/log.h"

#include <esp_timer.h>

#include <cmath>

static const char* TAG = "ACAIA";

constexpr size_t FLOW_MOVING_AVERAGE_SIZE = 20;

namespace esphome {
    namespace acaia {
void AcaiaSensor::setup() {
    flows_.reserve(FLOW_MOVING_AVERAGE_SIZE - 1);
    get_parent()->add_on_message_callback([this](AcaiaMessage msg) { this->handle_message(msg); });
}

void AcaiaSensor::handle_message(AcaiaMessage message) {
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

    if (message.has_time) {
        if (should_set(timer_, message.time, 0.05)) {
            timer_->publish_state(message.time);
        }
    }

    if (message.has_battery) {
        if (should_set(battery_, message.battery, 0.5)) {
            battery_->publish_state(message.battery);
        }
    }

    float avg_flow = calculate_avg_flow();

    if (!std::isnan(avg_flow) && should_set(flow_, avg_flow, 0.1)) {
        flow_->publish_state(avg_flow);
    }
}

float AcaiaSensor::calculate_avg_flow()
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

bool AcaiaSensor::should_set(esphome::sensor::Sensor *sensor, float value, float sigma)
{
    return sensor != nullptr && (!sensor->has_state() || fabs(sensor->get_raw_state() - value) >= sigma);
}

void AcaiaSensor::set_weight(esphome::sensor::Sensor *sens) { weight_ = sens; }

void AcaiaSensor::set_flow(esphome::sensor::Sensor *sens) { flow_ = sens; }
void AcaiaSensor::set_timer(esphome::sensor::Sensor *sens) { timer_ = sens; }
void AcaiaSensor::set_battery(esphome::sensor::Sensor *sens) { battery_ = sens; }

void AcaiaSensor::dump_config() {
    LOG_SENSOR("  ", "Weight", this->weight_);
    LOG_SENSOR("  ", "Flow", this->flow_);
    LOG_SENSOR("  ", "Timer", this->timer_);
    LOG_SENSOR("  ", "Battery", this->battery_);
}

    }
}
