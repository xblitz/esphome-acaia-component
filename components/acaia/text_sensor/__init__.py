import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ICON_SCALE

from .. import acaia_ns, Acaia

CONF_ACAIA_ID = "acaia_id"

CONF_UNITS = "units"

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
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_ACAIA_ID])

    if units_config := config.get(CONF_UNITS):
        sens = await text_sensor.new_text_sensor(units_config)
        cg.add(var.set_units(sens))
