import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, ICON_TIMER

from .. import Acaia, acaia_ns

CONF_ACAIA_ID = "acaia_id"

CONF_CONNECTED = "connected"
CONF_TIMER_RUNNING = "timer_running"
CONF_BEEP = "beep"

AcaiaBinarySensor = acaia_ns.class_(
    "AcaiaBinarySensor",
    cg.Component,
    cg.Parented.template(Acaia),
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(AcaiaBinarySensor),
        cv.GenerateID(CONF_ACAIA_ID): cv.use_id(Acaia),
        cv.Optional(CONF_CONNECTED): binary_sensor.binary_sensor_schema(
            icon="mdi:bluetooth-connect",
        ),
        cv.Optional(CONF_TIMER_RUNNING): binary_sensor.binary_sensor_schema(
            icon=ICON_TIMER,
        ),
        cv.Optional(CONF_BEEP): binary_sensor.binary_sensor_schema(),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_ACAIA_ID])

    if connected_config := config.get(CONF_CONNECTED):
        sens = await binary_sensor.new_binary_sensor(connected_config)
        cg.add(var.set_connected(sens))

    if timer_running_config := config.get(CONF_TIMER_RUNNING):
        sens = await binary_sensor.new_binary_sensor(timer_running_config)
        cg.add(var.set_timer_running(sens))

    if beep_config := config.get(CONF_BEEP):
        sens = await binary_sensor.new_binary_sensor(beep_config)
        cg.add(var.set_beep(sens))
