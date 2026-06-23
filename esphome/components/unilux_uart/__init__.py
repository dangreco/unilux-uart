import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

from esphome.components import climate, sensor, uart

CODEOWNERS = ["@dangreco"]
DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor", "climate"]

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


def _channel_schema(key):
    """Sensor schema for one temperature channel.

    The channel defaults to enabled with its key as the sensor name, so omitting
    the key still produces a published sensor. Set ``disabled: true`` to suppress
    the channel entirely.
    """
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
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


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UniluxUartComponent),
            cv.Optional(CONF_T1, default={}): _channel_schema(CONF_T1),
            cv.Optional(CONF_T2, default={}): _channel_schema(CONF_T2),
            # Two-way target-temperature control, exposed as a climate entity by
            # default. Set "disabled: true" to suppress it.
            cv.Optional(CONF_CLIMATE, default={}): _climate_schema(),
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

    cg.add_library("unilux-uart", "main", "https://github.com/dangreco/unilux-uart.git")
