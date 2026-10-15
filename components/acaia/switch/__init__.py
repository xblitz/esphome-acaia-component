import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch

from .. import Acaia, acaia_ns

CONF_ACAIA_ID = "acaia_id"

AcaiaSoundSwitch = acaia_ns.class_(
    "AcaiaSoundSwitch",
    switch.Switch,
    cg.Component,
    cg.Parented.template(Acaia),
)

CONFIG_SCHEMA = (
    switch.switch_schema(AcaiaSoundSwitch)
    .extend(
        {
            cv.GenerateID(CONF_ACAIA_ID): cv.use_id(Acaia),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await switch.new_switch(config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_ACAIA_ID])
