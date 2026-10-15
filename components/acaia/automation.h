#pragma once
//
// Created by Magnus Nordlander on 2023-10-31.
//

#include "esphome/core/component.h"
#include "esphome/core/automation.h"

#include <string>
#include "acaia.h"

namespace esphome {
    namespace acaia {
        template<typename... Ts>
        class AcaiaTareAction : public Action<Ts...> {
        public:
            explicit AcaiaTareAction(Acaia *acaia) : acaia_(acaia) {}

            void play(const Ts &...x) override { acaia_->send_tare(); }

        protected:
            Acaia *acaia_;
        };

        template<typename... Ts>
        class AcaiaResetTimerAction : public Action<Ts...> {
        public:
            explicit AcaiaResetTimerAction(Acaia *acaia) : acaia_(acaia) {}

            void play(const Ts &...x) override { acaia_->reset_timer(); }

        protected:
            Acaia *acaia_;
        };

        template<typename... Ts>
        class AcaiaStartTimerAction : public Action<Ts...> {
        public:
            explicit AcaiaStartTimerAction(Acaia *acaia) : acaia_(acaia) {}

            void play(const Ts &...x) override { acaia_->start_timer(); }

        protected:
            Acaia *acaia_;
        };

        template<typename... Ts>
        class AcaiaStopTimerAction : public Action<Ts...> {
        public:
            explicit AcaiaStopTimerAction(Acaia *acaia) : acaia_(acaia) {}

            void play(const Ts &...x) override { acaia_->stop_timer(); }

        protected:
            Acaia *acaia_;
        };

        template<typename... Ts>
        class AcaiaSetUnitAction : public Action<Ts...> {
        public:
            explicit AcaiaSetUnitAction(Acaia *acaia) : acaia_(acaia) {}

            TEMPLATABLE_VALUE(std::string, unit)

            void play(const Ts &...x) override { acaia_->set_unit(this->unit_.value(x...)); }

        protected:
            Acaia *acaia_;
        };

        template<typename... Ts>
        class AcaiaSetSleepAction : public Action<Ts...> {
        public:
            explicit AcaiaSetSleepAction(Acaia *acaia) : acaia_(acaia) {}

            TEMPLATABLE_VALUE(int, minutes)

            void play(const Ts &...x) override { acaia_->set_sleep(this->minutes_.value(x...)); }

        protected:
            Acaia *acaia_;
        };

        class AcaiaTareTrigger : public Trigger<> {
        public:
            explicit AcaiaTareTrigger(Acaia *parent) {
                parent->add_on_tare_callback([this]() { this->trigger(); });
            }
        };

        class AcaiaTimerStartTrigger : public Trigger<> {
        public:
            explicit AcaiaTimerStartTrigger(Acaia *parent) {
                parent->add_on_timer_start_callback([this]() { this->trigger(); });
            }
        };

        class AcaiaTimerStopTrigger : public Trigger<> {
        public:
            explicit AcaiaTimerStopTrigger(Acaia *parent) {
                parent->add_on_timer_stop_callback([this]() { this->trigger(); });
            }
        };

        class AcaiaTimerResetTrigger : public Trigger<> {
        public:
            explicit AcaiaTimerResetTrigger(Acaia *parent) {
                parent->add_on_timer_reset_callback([this]() { this->trigger(); });
            }
        };
    }
}
