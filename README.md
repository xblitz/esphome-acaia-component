# esphome-acaia-component

An ESPHome external component that connects to **Acaia Lunar** scales over BLE and exposes weight, flow, timer, battery, mode, and scale settings as native ESPHome entities.

Maintained fork of [variegated-coffee/esphome-acaia-component](https://github.com/variegated-coffee/esphome-acaia-component), modernized for current ESPHome and extended with multi-scale support, settings entities, actions, and triggers.

## Features

- **Sensors:** weight (g), flow rate (g/s), shot timer (s), battery (%), auto-off timeout (min)
- **Binary sensors:** BLE connected, timer running, beep enabled
- **Text sensors:** weight units (`g`/`oz`), scale mode (`Weighing`, `Droplet`, …)
- **Controls:** sound switch, unit select, sleep-timeout select (all follow the scale's own settings)
- **Actions:** tare, timer start/stop/reset
- **Triggers:** `on_tare`, `on_timer_start`, `on_timer_stop`, `on_timer_reset`
- **Multi-scale:** run one hub per scale (e.g. Lunar + Sette) on a single ESP32
- Full 24-bit weight decode — no rollover above 650 g

## Compatibility

- Verified against an Acaia Lunar (AL001, pre-2021, Bluetooth 4.0). Other IPS-protocol Acaia scales may work.
- The scale's Bluetooth setting (`bt SEt`) must be on (default).
- Any ESP32 with BLE; tested on ESP32-C6 (see below).

## Installation

Add this repository as an external component:

```yaml
external_components:
  - source: github://xblitz/esphome-acaia-component
    components: [acaia]
```

Pin to a release (e.g. `@v0.1.0`) so later changes on `main` never alter your build.

## Configuration

A complete working config is also shipped as [`example.yaml`](example.yaml).

### Minimal example

```yaml
esp32_ble_tracker:

ble_client:
  - id: ble_lunar
    mac_address: "00:1C:97:1B:CD:B0"

acaia:
  - id: lunar
    ble_client_id: ble_lunar

sensor:
  - platform: acaia
    acaia_id: lunar
    weight:
      name: "Lunar Weight"
```

Find the scale's MAC address with an `esp32_ble_tracker:` scan (watch the logs while the scale is on).

### Full example

```yaml
esp32_ble_tracker:

ble_client:
  - id: ble_lunar
    mac_address: "00:1C:97:1B:CD:B0"

acaia:
  - id: lunar
    ble_client_id: ble_lunar
    on_tare:
      then:
        - logger.log: "Lunar tare pressed"
    on_timer_start:
      then:
        - logger.log: "Lunar timer started"
    on_timer_stop:
      then:
        - logger.log: "Lunar timer stopped"
    on_timer_reset:
      then:
        - logger.log: "Lunar timer reset"

sensor:
  - platform: acaia
    acaia_id: lunar
    weight:
      name: "Lunar Weight"
    flow:
      name: "Lunar Flow"
    timer:
      name: "Lunar Timer"
    battery:
      name: "Lunar Battery"
    auto_off:
      name: "Lunar Auto-Off"

binary_sensor:
  - platform: acaia
    acaia_id: lunar
    connected:
      name: "Lunar Connected"
    timer_running:
      name: "Lunar Timer Running"
    beep:
      name: "Lunar Beep"

text_sensor:
  - platform: acaia
    acaia_id: lunar
    units:
      name: "Lunar Units"
    mode:
      name: "Lunar Mode"

switch:
  - platform: acaia
    acaia_id: lunar
    name: "Lunar Sound"

select:
  - platform: acaia
    acaia_id: lunar
    type: unit
    name: "Lunar Unit"
  - platform: acaia
    acaia_id: lunar
    type: sleep
    name: "Lunar Sleep Timeout"
```

For a second scale, add another `ble_client` entry, another `acaia:` hub with its own `id`, and point more entities at it via `acaia_id`.

### Hub options (`acaia:`)

| Key | Description |
|---|---|
| `id` | Hub ID referenced by entities as `acaia_id` and by actions. |
| `ble_client_id` / `mac_address` | Attach to a `ble_client` (standard BLE client options). |
| `on_tare` | Automation fired when the tare button is pressed. |
| `on_timer_start` / `on_timer_stop` / `on_timer_reset` | Automations fired on timer button presses. |

### Entities

All entity platforms take `acaia_id` to select their scale. Every child key is optional — only configure what you need.

**`sensor.acaia`:** `weight` (g), `flow` (g/s), `timer` (s), `battery` (%), `auto_off` (min).

**`binary_sensor.acaia`:** `connected`, `timer_running`, `beep`.

**`text_sensor.acaia`:** `units` (`g` or `oz`, as set on the scale), `mode` (e.g. `Weighing`, `Dual Display`, `Droplet`, `Droplet and Square`, `Triangle and Square`, `Auto-Tare`).

**`switch.acaia`:** sound on/off. Standard switch options (e.g. `restore_mode`) apply.

**`select.acaia`:** requires `type: unit` (`g`/`oz`) or `type: sleep` (`off`/`5`/`10`/`20`/`30`/`60` minutes, `off` = never).

### Actions

Usable from ESPHome automations and Home Assistant services:

```yaml
- acaia.tare:
    id: lunar
- acaia.start_timer:
    id: lunar
- acaia.stop_timer:
    id: lunar
- acaia.reset_timer:
    id: lunar
```

## Notes and quirks

- **Mode is read-only.** The scale announces mode changes but accepts no mode command over BLE, so the mode sensor detects only. It publishes at connect (from the settings frame) and on every mode press.
- **Sound/unit/sleep are optimistic.** Operating them sends the change immediately and updates the UI; the scale's own settings frames (pushed on connect and after changes) correct them if they drift.
- **Flow is computed on the ESP**, from the ~10 Hz weight stream over a ~0.5 s window — the Lunar does not report flow itself.
- **Weight is always grams** in ESPHome: if the scale is set to ounces, readings are converted. The `units` sensor shows the scale-side setting.
- The component sends a 1 Hz heartbeat to keep the BLE session alive, as the protocol requires.
- Unknown messages and records are logged with their raw bytes at warning level, to support further protocol reverse-engineering. Debug logs include the raw settings frame on every connect.

## Tested environment

- ESPHome 2026.9.1, ESP-IDF framework
- ESP32-C6 DevKitC-1 (8 MB flash)
- Acaia Lunar AL001, Bluetooth 4.0

## License

MIT — see [LICENSE](LICENSE).
