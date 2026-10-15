#pragma once
//
// Created by Magnus Nordlander on 2023-10-25.
//

#include <cstdint>
#include <string>

namespace esphome {
    namespace acaia {
        enum AcaiaMessageType { none, weight, heartbeat, timer, tare_start_stop_reset };
        enum AcaiaButton { tare, start, stop, reset };

        class AcaiaMessage {
        public:
            AcaiaMessageType type = none;

            bool has_weight = false;
            float weight;

            bool has_time = false;
            float time;

            bool has_battery = false;
            float battery;

            bool has_units = false;
            std::string units;

            AcaiaButton button = tare;

            bool connected = false;

            bool has_timer_running = false;
            bool timer_running = false;

            int64_t timestamp;
        };
    }
}
