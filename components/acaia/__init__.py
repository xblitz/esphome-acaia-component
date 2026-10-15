import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.components import ble_client
from esphome.const import CONF_ID, CONF_TRIGGER_ID

CODEOWNERS = ["@magnusnordlander"]
MULTI_CONF = True
acaia_ns = cg.esphome_ns.namespace("esphome::acaia")

Acaia = acaia_ns.class_("Acaia", cg.Component, ble_client.BLEClientNode)

AcaiaTareAction = acaia_ns.class_("AcaiaTareAction", automation.Action)
AcaiaResetTimerAction = acaia_ns.class_("AcaiaResetTimerAction", automation.Action)
AcaiaStartTimerAction = acaia_ns.class_("AcaiaStartTimerAction", automation.Action)
AcaiaStopTimerAction = acaia_ns.class_("AcaiaStopTimerAction", automation.Action)
AcaiaRefreshSettingsAction = acaia_ns.class_("AcaiaRefreshSettingsAction", automation.Action)
AcaiaSetUnitAction = acaia_ns.class_("AcaiaSetUnitAction", automation.Action)
AcaiaSetSleepAction = acaia_ns.class_("AcaiaSetSleepAction", automation.Action)
AcaiaSetSoundAction = acaia_ns.class_("AcaiaSetSoundAction", automation.Action)

AcaiaTareTrigger = acaia_ns.class_("AcaiaTareTrigger", automation.Trigger)
AcaiaTimerStartTrigger = acaia_ns.class_("AcaiaTimerStartTrigger", automation.Trigger)
AcaiaTimerStopTrigger = acaia_ns.class_("AcaiaTimerStopTrigger", automation.Trigger)
AcaiaTimerResetTrigger = acaia_ns.class_("AcaiaTimerResetTrigger", automation.Trigger)

CONF_ACAIA_ID = "acaia_id"
CONF_UNIT = "unit"
CONF_MINUTES = "minutes"
CONF_ENABLED = "enabled"
CONF_ON_TARE = "on_tare"
CONF_ON_TIMER_START = "on_timer_start"
CONF_ON_TIMER_STOP = "on_timer_stop"
CONF_ON_TIMER_RESET = "on_timer_reset"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(Acaia),
    }
).extend(ble_client.BLE_CLIENT_SCHEMA).extend(
        {
            cv.Optional(CONF_ON_TARE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(AcaiaTareTrigger),
                }
            ),
            cv.Optional(CONF_ON_TIMER_START): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(AcaiaTimerStartTrigger),
                }
            ),
            cv.Optional(CONF_ON_TIMER_STOP): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(AcaiaTimerStopTrigger),
                }
            ),
            cv.Optional(CONF_ON_TIMER_RESET): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(AcaiaTimerResetTrigger),
                }
            ),
        }
    )

ACAIA_ACTION_NO_ARG_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.templatable(cv.use_id(Acaia)),
    }
)

@automation.register_action("acaia.tare", AcaiaTareAction, ACAIA_ACTION_NO_ARG_SCHEMA,
    synchronous=True,)
@automation.register_action("acaia.reset_timer", AcaiaResetTimerAction, ACAIA_ACTION_NO_ARG_SCHEMA,
    synchronous=True,)
@automation.register_action("acaia.start_timer", AcaiaStartTimerAction, ACAIA_ACTION_NO_ARG_SCHEMA,
    synchronous=True,)
@automation.register_action("acaia.stop_timer", AcaiaStopTimerAction, ACAIA_ACTION_NO_ARG_SCHEMA,
    synchronous=True,)
@automation.register_action("acaia.refresh_settings", AcaiaRefreshSettingsAction, ACAIA_ACTION_NO_ARG_SCHEMA,
    synchronous=True,)
async def no_arg_action(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action(
    "acaia.set_unit",
    AcaiaSetUnitAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(Acaia),
            cv.Required(CONF_UNIT): cv.templatable(cv.one_of("g", "oz", lower=True)),
        }
    ),
    synchronous=True,
)
async def set_unit_action(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    cg.add(var.set_unit(await cg.templatable(config[CONF_UNIT], args, cg.std_string)))
    return var


@automation.register_action(
    "acaia.set_sleep",
    AcaiaSetSleepAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(Acaia),
            cv.Required(CONF_MINUTES): cv.templatable(cv.one_of(0, 5, 10, 20, 30, 60)),
        }
    ),
    synchronous=True,
)
async def set_sleep_action(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    cg.add(var.set_minutes(await cg.templatable(config[CONF_MINUTES], args, cg.int_)))
    return var


@automation.register_action(
    "acaia.set_sound",
    AcaiaSetSoundAction,
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.use_id(Acaia),
            cv.Required(CONF_ENABLED): cv.templatable(cv.boolean),
        }
    ),
    synchronous=True,
)
async def set_sound_action(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    cg.add(var.set_enabled(await cg.templatable(config[CONF_ENABLED], args, cg.bool_)))
    return var


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)
    for conf in config.get(CONF_ON_TARE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_TIMER_START, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_TIMER_STOP, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
    for conf in config.get(CONF_ON_TIMER_RESET, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)