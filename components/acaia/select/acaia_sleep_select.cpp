#include "acaia_sleep_select.h"

#include "esphome/core/log.h"

namespace esphome {
namespace acaia {

static const char *const TAG = "acaia.sleep_select";

// Must match the options registered for type "sleep" in __init__.py.
static constexpr int SLEEP_MINUTES[] = {0, 5, 10, 20, 30, 60};
static constexpr size_t SLEEP_OPTION_COUNT = sizeof(SLEEP_MINUTES) / sizeof(SLEEP_MINUTES[0]);

void AcaiaSleepSelect::dump_config() { LOG_SELECT("", "Acaia Sleep Timeout", this); }

void AcaiaSleepSelect::control(size_t index) {
  if (index >= SLEEP_OPTION_COUNT)
    return;
  get_parent()->set_sleep(SLEEP_MINUTES[index]);
  publish_state(index);
}

void AcaiaSleepSelect::handle_message(AcaiaMessage message) {
  if (!message.has_auto_off)
    return;
  const int minutes = (int) message.auto_off;
  for (size_t i = 0; i < SLEEP_OPTION_COUNT; i++) {
    if (SLEEP_MINUTES[i] == minutes) {
      publish_state(i);
      break;
    }
  }
}

}  // namespace acaia
}  // namespace esphome
