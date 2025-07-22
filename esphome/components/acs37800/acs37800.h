#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace acs37800 {

// Default current-sensing range
// ACS37800KMACTR-030B3-I2C is a 30.0 Amp part - as used on the SparkFun Qwiic Power Meter
// ACS37800KMACTR-090B3-I2C is a 90.0 Amp part
const float ACS37800_DEFAULT_CURRENT_RANGE = 30.0f;

class ACS37800Component : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void update() override;
  void loop() override;

  void set_shunt_resistance_ohm(float shunt_resistance_ohm) { shunt_resistance_ohm_ = shunt_resistance_ohm; }
  void set_divider_resistance_ohm(float divider_resistance_ohm) { divider_resistance_ohm_ = divider_resistance_ohm; }
  void set_number_of_samples(uint32_t samples) { number_of_samples_ = samples; }

  void set_voltage_sensor(sensor::Sensor *voltage_sensor) { voltage_sensor_ = voltage_sensor; }
  void set_current_sensor(sensor::Sensor *current_sensor) { current_sensor_ = current_sensor; }
  void set_power_sensor(sensor::Sensor *power_sensor) { power_sensor_ = power_sensor; }

 protected:
  // The value of the sense resistor for voltage measurement in Ohms
  float shunt_resistance_ohm_;
  // The value of the divider resistance for voltage measurement in Ohms
  float divider_resistance_ohm_;
  // The ACS37800's current sensing range
  float current_sensing_range_ = 30.0f;  // Default to 30.0A, can be overridden by set_max_current_a()
  // Set the number of samples for RMS calculations
  uint32_t number_of_samples_;
  // The ACS37800's coarse current gain - needed by the current calculations
  float current_coarse_gain_;
  sensor::Sensor *voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *power_sensor_{nullptr};
};

}  // namespace acs37800
}  // namespace esphome
