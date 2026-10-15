import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select

from .. import acaia_ns, Acaia

CONF_ACAIA_ID = "acaia_id"

AcaiaUnitSelect = acaia_ns.class_(
    "AcaiaUnitSelect",
    select.Select,
    cg.Component,
    cg.Parented.template(Acaia),
)

CONFIG_SCHEMA = (
    select.select_schema(AcaiaUnitSelect)
    .extend(
        {
            cv.GenerateID(CONF_ACAIA_ID): cv.use_id(Acaia),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await select.new_select(config, options=["g", "oz"])
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_ACAIA_ID])
