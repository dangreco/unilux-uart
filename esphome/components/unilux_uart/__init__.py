import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)

from esphome.components import climate, number, select, sensor, uart

CODEOWNERS = ["@dangreco"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "climate", "number", "select"]

CONF_T1 = "t1"
CONF_T2 = "t2"
CONF_CLIMATE = "climate"
CONF_DISABLED = "disabled"

CHANNELS = (CONF_T1, CONF_T2)

unilux_uart_ns = cg.esphome_ns.namespace("unilux_uart")
UniluxUartComponent = unilux_uart_ns.class_(
    "UniluxUartComponent", cg.Component, uart.UARTDevice
)
UniluxUartClimate = unilux_uart_ns.class_(
    "UniluxUartClimate", climate.Climate, cg.Component
)
UniluxNumber = unilux_uart_ns.class_("UniluxNumber", number.Number, cg.Component)
UniluxSelect = unilux_uart_ns.class_("UniluxSelect", select.Select, cg.Component)

# Per-setting number entities (scalar temperatures, i16 BE tenths of a degree).
# key -> (msg_id, default name, min, max, step, initial_value)
NUMBER_SETTINGS = {
    "temperature_offset": (0x56, "Temperature Offset", -5.0, 5.0, 0.5, 0.0),
    "switching_diff_heat": (0x58, "Switching Differential (Heat)", 2.0, 4.0, 0.5, 2.0),
    "switching_diff_cool": (0x59, "Switching Differential (Cool)", 2.0, 4.0, 0.5, 2.0),
    "changeover_heat": (0x5D, "Heating Changeover Temperature", 10.0, 25.0, 0.5, 18.0),
    "changeover_cool": (0x5E, "Cooling Changeover Temperature", 20.0, 40.0, 0.5, 23.0),
}

# Per-setting select entities (enum bytes). key -> (msg_id, default name, options,
# restore, initial_index) where options is an ordered mapping of displayed label
# -> on-wire byte.
SELECT_SETTINGS = {
    "system_mode": (
        0x23,
        "System Mode",
        {"2 Pipe": 0x00, "4 Pipe": 0x01, "2 Pipe Auto": 0x02, "2 Pipe Heat": 0x03},
        True,
        0,
    ),
    "display_unit": (
        0x2C,
        "Temperature Display Unit",
        {"Celsius": 0x02, "Fahrenheit": 0x03},
        False,
        0,
    ),
}


def _channel_schema(key):
    """Sensor schema for one temperature channel.

    The channel defaults to enabled with its key as the sensor name, so omitting
    the key still produces a published sensor. Set ``disabled: true`` to suppress
    the channel entirely.
    """
    return sensor.sensor_schema(
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ).extend(
        {
            cv.Optional(CONF_NAME, default=key): cv.string,
            cv.Optional(CONF_DISABLED, default=False): cv.boolean,
        }
    )


def _climate_schema():
    """Climate schema for the two-way target-temperature control.

    Defaults to enabled with the name "Thermostat", so omitting the key still
    exposes the climate entity. Set ``disabled: true`` to suppress it.
    """
    return (
        climate.climate_schema(UniluxUartClimate)
        .extend(cv.COMPONENT_SCHEMA)
        .extend(
            {
                cv.Optional(CONF_NAME, default="Thermostat"): cv.string,
                cv.Optional(CONF_DISABLED, default=False): cv.boolean,
            }
        )
    )


def _number_schema(default_name):
    """Number schema for one scalar-temperature setting.

    Enabled by default with the given name; set ``disabled: true`` to suppress.
    """
    return number.number_schema(
        UniluxNumber,
        device_class=DEVICE_CLASS_TEMPERATURE,
    ).extend(
        {
            cv.Optional(CONF_NAME, default=default_name): cv.string,
            cv.Optional(CONF_DISABLED, default=False): cv.boolean,
        }
    )


def _select_schema(default_name):
    """Select schema for one enum-byte setting.

    Enabled by default with the given name; set ``disabled: true`` to suppress.
    """
    return select.select_schema(UniluxSelect).extend(
        {
            cv.Optional(CONF_NAME, default=default_name): cv.string,
            cv.Optional(CONF_DISABLED, default=False): cv.boolean,
        }
    )


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UniluxUartComponent),
            cv.Optional(CONF_T1, default={}): _channel_schema(CONF_T1),
            cv.Optional(CONF_T2, default={}): _channel_schema(CONF_T2),
            # Two-way target-temperature control, exposed as a climate entity by
            # default. Set "disabled: true" to suppress it.
            cv.Optional(CONF_CLIMATE, default={}): _climate_schema(),
            # Per-setting number and select entities, all enabled by default.
            **{
                cv.Optional(key, default={}): _number_schema(name)
                for key, (_, name, _, _, _, _) in NUMBER_SETTINGS.items()
            },
            **{
                cv.Optional(key, default={}): _select_schema(name)
                for key, (_, name, _, _, _) in SELECT_SETTINGS.items()
            },
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    for key in CHANNELS:
        conf = config[key]
        if conf[CONF_DISABLED]:
            continue
        sens = await sensor.new_sensor(conf)
        cg.add(getattr(var, f"set_{key}_sensor")(sens))

    clim_conf = config[CONF_CLIMATE]
    if not clim_conf[CONF_DISABLED]:
        clim = cg.new_Pvariable(clim_conf[CONF_ID])
        await cg.register_component(clim, clim_conf)
        await climate.register_climate(clim, clim_conf)
        cg.add(clim.set_parent(var))
        cg.add(var.set_climate(clim))

    for key, (msg_id, _, lo, hi, step, initial_value) in NUMBER_SETTINGS.items():
        conf = config[key]
        if conf[CONF_DISABLED]:
            continue
        num = await number.new_number(
            conf, min_value=lo, max_value=hi, step=step
        )
        await cg.register_component(num, conf)
        cg.add(num.set_parent(var))
        cg.add(num.set_msg_id(msg_id))
        cg.add(num.set_initial_value(initial_value))
        cg.add(var.add_number(num))

    for key, (msg_id, _, options, restore, initial_index) in SELECT_SETTINGS.items():
        conf = config[key]
        if conf[CONF_DISABLED]:
            continue
        sel = await select.new_select(conf, options=list(options.keys()))
        await cg.register_component(sel, conf)
        cg.add(sel.set_parent(var))
        cg.add(sel.set_msg_id(msg_id))
        cg.add(sel.set_option_bytes(list(options.values())))
        cg.add(sel.set_restore(restore))
        cg.add(sel.set_initial_index(initial_index))
        cg.add(var.add_select(sel))

    cg.add_library("unilux-uart", "main", "https://github.com/dangreco/unilux-uart.git")
