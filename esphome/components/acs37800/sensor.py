import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CURRENT,
    CONF_ID,
    CONF_POWER,
    CONF_SHUNT_RESISTANCE,
    CONF_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_VOLT,
    UNIT_WATT,
)

DEPENDENCIES = ["i2c"]

CONF_DIVIDER_RESISTANCE = "divider_resistance"
CONF_SAMPLE_NUMBER = "number_of_saples"

acs37800_ns = cg.esphome_ns.namespace("acs37800")
ACS37800Sensor = acs37800_ns.class_(
    "ACS37800Sensor", cg.PollingComponent, i2c.I2CDevice
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ACS37800Sensor),
            cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_POWER): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_SHUNT_RESISTANCE, default=82000.0): cv.All(
                cv.resistance, cv.Range(min=0.0)
            ),
            cv.Optional(CONF_DIVIDER_RESISTANCE, default=2000000.0): cv.All(
                cv.resistance, cv.Range(min=0.0)
            ),
            cv.Optional(CONF_SAMPLE_NUMBER, default=1023): cv.int_range(
                min=0, max=1023
            ),
        }
    )
    .extend(cv.polling_component_schema("2s"))
    .extend(i2c.i2c_device_schema(0x60)),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.setSenseRes(config[CONF_SHUNT_RESISTANCE]))
    cg.add(var.setDividerRes(config[CONF_DIVIDER_RESISTANCE]))
    cg.add(var.setNumberOfSamples(config[CONF_SAMPLE_NUMBER]))

    if CONF_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(var.set_voltage_sensor(sens))

    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_current_sensor(sens))

    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(sens))
