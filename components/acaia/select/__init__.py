import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ID, CONF_TYPE

from .. import Acaia, acaia_ns

CONF_ACAIA_ID = "acaia_id"

AcaiaUnitSelect = acaia_ns.class_(
    "AcaiaUnitSelect",
    select.Select,
    cg.Component,
    cg.Parented.template(Acaia),
)
AcaiaSleepSelect = acaia_ns.class_(
    "AcaiaSleepSelect",
    select.Select,
    cg.Component,
    cg.Parented.template(Acaia),
)

SELECT_CLASSES = {
    "unit": AcaiaUnitSelect,
    "sleep": AcaiaSleepSelect,
}
SELECT_OPTIONS = {
    "unit": ["g", "oz"],
    # "off" displays for the 0 (disabled) timeout; order must match SLEEP_MINUTES.
    "sleep": ["off", "5", "10", "20", "30", "60"],
}

CONFIG_SCHEMA = (
    select.select_schema(AcaiaUnitSelect)
    .extend(
        {
            cv.GenerateID(CONF_ACAIA_ID): cv.use_id(Acaia),
            cv.Required(CONF_TYPE): cv.one_of("unit", "sleep"),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    kind = config[CONF_TYPE]
    config[CONF_ID].type = SELECT_CLASSES[kind]
    var = await select.new_select(config, options=SELECT_OPTIONS[kind])
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_ACAIA_ID])
