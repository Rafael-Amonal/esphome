#pragma once

#include <list>

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"
#include "acs37800_defines.h"

namespace esphome {
namespace acs37800 {

class ACS37800Sensor : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;
  void loop() override;

  void set_voltage_sensor(sensor::Sensor *voltage_sensor) { voltage_sensor_ = voltage_sensor; }
  void set_current_sensor(sensor::Sensor *current_sensor) { current_sensor_ = current_sensor; }
  void set_power_sensor(sensor::Sensor *power_sensor) { power_sensor_ = power_sensor; }
  // Change the parameters
  void setSenseRes(float newRes) { _senseResistance = newRes; }      // Change the value of _senseResistance (Ohms)
  void setDividerRes(float newRes) { _dividerResistance = newRes; }  // Change the value of _dividerResistance (Ohms)
  void setNumberOfSamples(uint32_t numberOfSamples) { _numberOfSamples = numberOfSamples; }

 protected:
  // The value of the sense resistor for voltage measurement in Ohms
  float _senseResistance = ACS37800_DEFAULT_SENSE_RES;

  // The value of the divider resistance for voltage measurement in Ohms
  float _dividerResistance = ACS37800_DEFAULT_DIVIDER_RES;

  // The ACS37800's current sensing range
  float _currentSensingRange = ACS37800_DEFAULT_CURRENT_RANGE;

  // The ACS37800's coarse current gain - needed by the current calculations
  float _currentCoarseGain;

  uint32_t _numberOfSamples = 1023;
  sensor::Sensor *voltage_sensor_{nullptr};
  sensor::Sensor *current_sensor_{nullptr};
  sensor::Sensor *power_sensor_{nullptr};

  // Configurable Settings
  // By default, settings are written to the shadow registers only. Set _eeprom to true to write to EEPROM too.
  // Set/Get the number of samples for RMS calculations. Bypass_N_Enable must be set/true for this to have effect.
  ACS37800ERR _setNumberOfSamples(uint32_t numberOfSamples, bool _eeprom = false);
  ACS37800ERR _getNumberOfSamples(
      uint32_t *numberOfSamples);  // Read and return the number of samples (from _shadow_ memory)
  // Set/Clear the Bypass_N_Enable flag
  ACS37800ERR _setBypassNenable(bool bypass, bool _eeprom = false);
  ACS37800ERR _getBypassNenable(bool *bypass);  // Read and return the bypass_n_en flag (from _shadow_ memory)

  // Basic methods for accessing the volatile registers
  ACS37800ERR _readRMS(float *vRMS, float *iRMS);  // Read volatile register 0x20. Return the vRMS and iRMS.
  ACS37800ERR _readPowerActiveReactive(
      float *pActive, float *pReactive);  // Read volatile register 0x21. Return the pactive and pimag (reactive)
  ACS37800ERR _readPowerFactor(float *pApparent, float *pFactor, bool *posangle,
                               bool *pospf);  // Read volatile register 0x22. Return the apparent power, power factor,
                                              // leading / lagging, generated / consumed
  ACS37800ERR _readInstantaneous(
      float *vInst, float *iInst,
      float *pInst);  // Read volatile registers 0x2A and 0x2C. Return the vInst, iInst and pInst.
  ACS37800ERR _readErrorFlags(
      ACS37800_REGISTER_2D_t *errorFlags);  // Read volatile register 0x2D. Return its contents in errorFlags.

  // Basic methods for accessing registers
  ACS37800ERR readRegister(uint32_t *data, uint8_t address);
  ACS37800ERR writeRegister(uint32_t data, uint8_t address);

  bool write_byte_32(uint8_t a_register, uint32_t data) { return write_bytes_16(a_register, (uint16_t *) &data, 2); }
  bool read_byte_32(uint8_t a_register, uint16_t *data) { return read_bytes_16(a_register, data, 2); }
};

}  // namespace acs37800
}  // namespace esphome
