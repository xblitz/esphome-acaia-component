#pragma once
//
// Created by Magnus Nordlander on 2023-10-31.
//

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
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
        class AcaiaRefreshSettingsAction : public Action<Ts...> {
        public:
            explicit AcaiaRefreshSettingsAction(Acaia *acaia) : acaia_(acaia) {}

            void play(const Ts &...x) override { acaia_->refresh_settings(); }

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
