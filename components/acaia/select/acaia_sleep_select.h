#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

#include "../acaia.h"
#include "../acaia_message.h"

namespace esphome {
namespace acaia {

/// Sleep timeout of the scale, driven by the auto-off setting (0 = off).
class AcaiaSleepSelect : public select::Select, public Component, public Parented<Acaia> {
 public:
  void dump_config() override;
  void control(size_t index) override;

  void handle_message(AcaiaMessage message);
};

}  // namespace acaia
}  // namespace esphome
