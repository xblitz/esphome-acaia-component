import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_FLOW,
    CONF_ID,
    CONF_WEIGHT,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_WEIGHT,
    ICON_BATTERY,
    ICON_SCALE,
    ICON_TIMER,
    ICON_WATER,
    STATE_CLASS_MEASUREMENT,
    UNIT_MINUTE,
    UNIT_PERCENT,
    UNIT_SECOND,
)

from .. import Acaia, acaia_ns

CONF_ACAIA_ID = "acaia_id"

CONF_TIMER = "timer"
CONF_BATTERY = "battery"
CONF_AUTO_OFF = "auto_off"

UNIT_GRAM = "g"
UNIT_GRAM_PER_SECOND = "g/s"

AcaiaSensor = acaia_ns.class_(
    "AcaiaSensor",
    cg.Component,
    cg.Parented.template(Acaia),
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(AcaiaSensor),
        cv.GenerateID(CONF_ACAIA_ID): cv.use_id(Acaia),
        cv.Optional(CONF_WEIGHT): sensor.sensor_schema(
            unit_of_measurement=UNIT_GRAM,
            icon=ICON_SCALE,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_WEIGHT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FLOW): sensor.sensor_schema(
            unit_of_measurement=UNIT_GRAM_PER_SECOND,
            icon=ICON_WATER,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TIMER): sensor.sensor_schema(
            unit_of_measurement=UNIT_SECOND,
            icon=ICON_TIMER,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_BATTERY): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            icon=ICON_BATTERY,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AUTO_OFF): sensor.sensor_schema(
            unit_of_measurement=UNIT_MINUTE,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_ACAIA_ID])

    if weight_config := config.get(CONF_WEIGHT):
        sens = await sensor.new_sensor(weight_config)
        cg.add(var.set_weight(sens))

    if flow_config := config.get(CONF_FLOW):
        sens = await sensor.new_sensor(flow_config)
        cg.add(var.set_flow(sens))

    if timer_config := config.get(CONF_TIMER):
        sens = await sensor.new_sensor(timer_config)
        cg.add(var.set_timer(sens))

    if battery_config := config.get(CONF_BATTERY):
        sens = await sensor.new_sensor(battery_config)
        cg.add(var.set_battery(sens))

    if auto_off_config := config.get(CONF_AUTO_OFF):
        sens = await sensor.new_sensor(auto_off_config)
        cg.add(var.set_auto_off(sens))
