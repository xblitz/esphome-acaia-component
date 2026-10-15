import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ICON_SCALE

from .. import Acaia, acaia_ns

CONF_ACAIA_ID = "acaia_id"

CONF_UNITS = "units"
CONF_MODE = "mode"

AcaiaTextSensor = acaia_ns.class_(
    "AcaiaTextSensor",
    cg.Component,
    cg.Parented.template(Acaia),
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(AcaiaTextSensor),
        cv.GenerateID(CONF_ACAIA_ID): cv.use_id(Acaia),
        cv.Optional(CONF_UNITS): text_sensor.text_sensor_schema(
            icon=ICON_SCALE,
        ),
        cv.Optional(CONF_MODE): text_sensor.text_sensor_schema(),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_ACAIA_ID])

    if units_config := config.get(CONF_UNITS):
        sens = await text_sensor.new_text_sensor(units_config)
        cg.add(var.set_units(sens))

    if mode_config := config.get(CONF_MODE):
        sens = await text_sensor.new_text_sensor(mode_config)
        cg.add(var.set_mode(sens))
