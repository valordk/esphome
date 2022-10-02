import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_CLOCK_PIN,
    CONF_DATA_PIN,
    CONF_TRIGGER_PIN,
    DEVICE_CLASS_DISTANCE,
    STATE_CLASS_MEASUREMENT,
    UNIT_MILLIMETER,
    ICON_RULER,
)
from esphome.cpp_helpers import gpio_pin_expression

mitutoyo_ns = cg.esphome_ns.namespace("mitutoyo")
MitutoyoInstrument = mitutoyo_ns.class_("MitutoyoInstrument", sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        MitutoyoInstrument,
        unit_of_measurement=UNIT_MILLIMETER,
        icon=ICON_RULER,
        accuracy_decimals=3,
        device_class=DEVICE_CLASS_DISTANCE,
        state_class=STATE_CLASS_MEASUREMENT,
    ).extend(
        {
            cv.Required(CONF_CLOCK_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_DATA_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_TRIGGER_PIN): pins.gpio_output_pin_schema
        }
    ).extend(cv.polling_component_schema("60s"))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await sensor.register_sensor(var, config)

    pin_clock = await gpio_pin_expression(config[CONF_CLOCK_PIN])
    cg.add(var.set_pin_clock(pin_clock))
    pin_data = await gpio_pin_expression(config[CONF_DATA_PIN])
    cg.add(var.set_pin_data(pin_data))
    pin_trigger = await gpio_pin_expression(config[CONF_TRIGGER_PIN])
    cg.add(var.set_pin_trigger(pin_trigger))
