#include "acs37800.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <cinttypes>

namespace esphome {
namespace acs37800 {

static const char *const TAG = "acs37800";

void ACS37800Component::setup() {
  ESP_LOGCONFIG(TAG, "Running setup");

  if (this->getCurrentCoarseGain(&current_coarse_gain_) != 0) {  // Get the current gain from shadow memory
    this->mark_failed();
    return;
  }
  if (this->setBypassNenable(true, false) != 0) {  // Set the Bypass_N_Enable flag to true, but don't write to EEPROM
    this->mark_failed();
    return;
  }
  if (this->setNumberOfSamples(number_of_samples_, false)) {  // Set the number of samples, but don't write to EEPROM
    this->mark_failed();
    return;
  }
}

void ACS37800Component::dump_config() {
  ESP_LOGCONFIG(TAG, "ACS37800:");
  LOG_I2C_DEVICE(this);

  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
    return;
  }
  LOG_UPDATE_INTERVAL(this);

  ESP_LOGCONFIG(TAG,
                "  Sense resistor for voltage measurement in Ohms: %d\n"
                "  Divider resistance for voltage measurement in Ohms: %d\n"
                "  Number of samples for RMS calculations: %d",
                shunt_resistance_ohm_, divider_resistance_ohm_, number_of_samples_);
  LOG_SENSOR(TAG,
                "  Sense resistor for voltage measurement in Ohms: %d\n"
                "  Divider resistance for voltage measurement in Ohms: %d\n"
                "  Number of samples for RMS calculations: %d",
                shunt_resistance_ohm_, divider_resistance_ohm_, number_of_samples_);

  LOG_SENSOR("  ", "Voltage", this->voltage_sensor_);
  LOG_SENSOR("  ", "Current", this->current_sensor_);
  LOG_SENSOR("  ", "Power", this->power_sensor_);
}

float ACS37800Component::get_setup_priority() const { return setup_priority::DATA; }

void ACS37800Component::update() {
  float v_rms, i_rms;
  if (this->readRMS(&v_rms, &i_rms) != ACS37800_SUCCESS) {
    return;
  }
  this->voltage_sensor_->publish_state(v_rms);
  this->current_sensor_->publish_state(i_rms);
  this->power_sensor_->publish_state(v_rms * i_rms);
  this->status_clear_warning();
}

//////////////// PRIVATE FUNCTIONS ///////////////////

// Read a register's contents. Contents are returned in data.
ACS37800ERR ACS37800Component::readRegister(uint32_t *data, uint8_t address) {
  if (!this->read_bytes_16(address, (uint16_t *) data, 2)) {
    this->status_set_warning();
    return (ACS37800_ERR_I2C_ERROR);
  }
  return (ACS37800_SUCCESS);
}

// Write data to the selected register
ACS37800ERR ACS37800Component::writeRegister(uint32_t data, uint8_t address) {
  if (!this->write_bytes_16(address, (uint16_t *) &data, 2)) {
    return (ACS37800_ERR_I2C_ERROR);  // Bail
  }
  return (ACS37800_SUCCESS);
}

// Set the number of samples for RMS calculations. Bypass_N_Enable must be set/true for this to have effect.
ACS37800ERR ACS37800Component::setNumberOfSamples(uint32_t numberOfSamples, bool _eeprom) {
  ACS37800ERR error =
      writeRegister(ACS37800_CUSTOMER_ACCESS_CODE, ACS37800_REGISTER_VOLATILE_2F);  // Set the customer access code

  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  ACS37800_REGISTER_0F_t store;
  error = readRegister(&store.data.all, ACS37800_REGISTER_SHADOW_1F);  // Read register 1F

  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  store.data.bits.n = numberOfSamples & 0x3FF;  // Adjust the number of samples (limit to 10 bits)

  error = writeRegister(store.data.all, ACS37800_REGISTER_SHADOW_1F);  // Write register 1F

  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  if (_eeprom)  // Check if user wants to set eeprom too
  {
    error = readRegister(&store.data.all, ACS37800_REGISTER_EEPROM_0F);  // Read register 0F

    if (error != ACS37800_SUCCESS) {
      return (error);  // Bail
    }

    store.data.bits.n = numberOfSamples & 0x3FF;  // Adjust the number of samples (limit to 10 bits)

    error = writeRegister(store.data.all, ACS37800_REGISTER_EEPROM_0F);  // Write register 0F
  }

  error = writeRegister(0, ACS37800_REGISTER_VOLATILE_2F);  // Clear the customer access code

  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  delay(100);  // Allow time for the shadow/eeprom memory to be updated - otherwise the next readRegister will return
               // zero...

  return (error);
}

// Read and return the number of samples from shadow memory
ACS37800ERR ACS37800Component::getNumberOfSamples(uint32_t *numberOfSamples) {
  ACS37800_REGISTER_0F_t store;
  ACS37800ERR error = readRegister(&store.data.all, ACS37800_REGISTER_SHADOW_1F);  // Read register 1F

  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  *numberOfSamples = store.data.bits.n;  // Return the number of samples

  return (error);
}

// Set/Clear the Bypass_N_Enable flag
ACS37800ERR ACS37800Component::setBypassNenable(bool bypass, bool _eeprom) {
  ACS37800ERR error =
      writeRegister(ACS37800_CUSTOMER_ACCESS_CODE, ACS37800_REGISTER_VOLATILE_2F);  // Set the customer access code

  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  ACS37800_REGISTER_0F_t store;
  error = readRegister(&store.data.all, ACS37800_REGISTER_SHADOW_1F);  // Read register 1F

  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  if (bypass)  // Adjust bypass_n_en
  {
    store.data.bits.bypass_n_en = 1;
  } else {
    store.data.bits.bypass_n_en = 0;
  }

  error = writeRegister(store.data.all, ACS37800_REGISTER_SHADOW_1F);  // Write register 1F

  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  if (_eeprom)  // Check if user wants to set eeprom too
  {
    error = readRegister(&store.data.all, ACS37800_REGISTER_EEPROM_0F);  // Read register 0F

    if (error != ACS37800_SUCCESS) {
      return (error);  // Bail
    }

    if (bypass)  // Adjust bypass_n_en
    {
      store.data.bits.bypass_n_en = 1;
    } else {
      store.data.bits.bypass_n_en = 0;
    }

    error = writeRegister(store.data.all, ACS37800_REGISTER_EEPROM_0F);  // Write register 0F
  }

  error = writeRegister(0, ACS37800_REGISTER_VOLATILE_2F);  // Clear the customer access code

  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  delay(100);  // Allow time for the shadow/eeprom memory to be updated - otherwise the next readRegister will return
               // zero...

  return (error);
}

//// Read and return the bypass_n_en flag from shadow memory
ACS37800ERR ACS37800Component::getBypassNenable(bool *bypass) {
  ACS37800_REGISTER_0F_t store;
  ACS37800ERR error = readRegister(&store.data.all, ACS37800_REGISTER_SHADOW_1F);  // Read register 1F
  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }
  *bypass = (bool) store.data.bits.bypass_n_en;  // Return bypass_n_en
  return (error);
}

// Get the coarse current gain from shadow memory
ACS37800ERR ACS37800Component::getCurrentCoarseGain(float *currentCoarseGain) {
  ACS37800_REGISTER_0B_t store;
  ACS37800ERR error = readRegister(&store.data.all, ACS37800_REGISTER_SHADOW_1B);  // Read register 1B
  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }
  float gain = ACS37800_CRS_SNS_GAINS[store.data.bits.crs_sns];
  *currentCoarseGain = gain;  // Return the gain
  return (error);
}

// Read volatile register 0x20. Return the vInst (Volts) and iInst (Amps).
ACS37800ERR ACS37800Component::readRMS(float *vRMS, float *iRMS) {
  ACS37800_REGISTER_20_t store;
  ACS37800ERR error = readRegister(&store.data.all, ACS37800_REGISTER_VOLATILE_20);  // Read register 20
  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }
  // Extract vrms. Convert to voltage in Volts.
  //  Note: datasheet says "RMS voltage output. This field is an unsigned 16-bit fixed point number with 16 fractional
  //  bits" Datasheet also says "Voltage Channel ADC Sensitivity: 110 LSB/mV"
  float volts = (float) store.data.bits.vrms;
  volts /= 55000.0;  // Convert from codes to the fraction of ADC Full Scale (16-bit)
  volts *= 250;      // Convert to mV (Differential Input Range is +/- 250mV)
  volts /= 1000;     // Convert to Volts
  // Correct for the voltage divider: (RISO1 + RISO2 + RSENSE) / RSENSE
  // Or:  (RISO1 + RISO2 + RISO3 + RISO4 + RSENSE) / RSENSE
  float resistorMultiplier = (divider_resistance_ohm_ + shunt_resistance_ohm_) / shunt_resistance_ohm_;
  volts *= resistorMultiplier;
  *vRMS = volts;

  // Extract the irms. Convert to current in Amps.
  // Datasheet says: "RMS current output. This field is a signed 16-bit fixed point number with 15 fractional bits"
  union {
    int16_t Signed;
    uint16_t unSigned;
  } signedUnsigned;  // Avoid any ambiguity when casting to signed int

  signedUnsigned.unSigned = store.data.bits.irms;  // Extract irms as signed int
  float amps = (float) signedUnsigned.Signed;
  amps /= 55000.0;                 // Convert from codes to the fraction of ADC Full Scale (16-bit)
  amps *= current_sensing_range_;  // Convert to Amps
  *iRMS = amps;
  return (error);
}

// Read volatile register 0x21. Return the pactive and pimag.
ACS37800ERR ACS37800Component::readPowerActiveReactive(float *pActive, float *pReactive) {
  ACS37800_REGISTER_21_t store;
  ACS37800ERR error = readRegister(&store.data.all, ACS37800_REGISTER_VOLATILE_21);  // Read register 21
  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }

  // Extract pactive. Convert to Watts
  // Note: datasheet says:
  // "Active power output. This field is a signed 16-bit fixed point
  //  number with 15 fractional bits, where positive MaxPow = 0.704,
  //  and negative MaxPow = –0.704. To convert the value (input
  //  power) to line power, divide the input power by the RSENSE and
  //  RISO voltage divider ratio using actual resistor values."
  // Datasheet also says:
  //  "3.08 LSB/mW for the 30A version and 1.03 LSB/mW for the 90A version"

  union {
    int16_t Signed;
    uint16_t unSigned;
  } signedUnsigned;  // Avoid any ambiguity when casting to signed int
  signedUnsigned.unSigned = store.data.bits.pactive;
  float power = (float) signedUnsigned.Signed;
  float LSBpermW = 3.08;                      // LSB per mW
  LSBpermW *= 30.0 / current_sensing_range_;  // Correct for sensor version
  power /= LSBpermW;                          // Convert from codes to mW
  // Correct for the voltage divider: (RISO1 + RISO2 + RSENSE) / RSENSE
  // Or:  (RISO1 + RISO2 + RISO3 + RISO4 + RSENSE) / RSENSE
  float resistorMultiplier = (divider_resistance_ohm_ + shunt_resistance_ohm_) / shunt_resistance_ohm_;
  power *= resistorMultiplier;
  power /= 1000;  // Convert from mW to W
  *pActive = power;

  // Extract pimag. Convert to VAR
  // Note: datasheet says:
  // "Reactive power output. This field is an unsigned 16-bit fixed
  //  point number with 16 fractional bits, where MaxPow = 0.704. To
  //  convert the value (input power) to line power, divide the input
  //  power by the RSENSE and RISO voltage divider ratio using actual
  //  resistor values."
  // Datasheet also says:
  //  "6.15 LSB/mVAR for the 30A version and 2.05 LSB/mVAR for the 90A version"
  power = (float) store.data.bits.pimag;
  float LSBpermVAR = 6.15;                      // LSB per mVAR
  LSBpermVAR *= 30.0 / current_sensing_range_;  // Correct for sensor version
  power /= LSBpermVAR;                          // Convert from codes to mVAR
  // Correct for the voltage divider: (RISO1 + RISO2 + RSENSE) / RSENSE
  // Or:  (RISO1 + RISO2 + RISO3 + RISO4 + RSENSE) / RSENSE
  power *= resistorMultiplier;
  power /= 1000;  // Convert from mVAR to VAR
  *pReactive = power;
  return (error);
}

// Read volatile register 0x22. Return the apparent power, power factor, leading / lagging, generated / consumed
ACS37800ERR ACS37800Component::readPowerFactor(float *pApparent, float *pFactor, bool *posangle, bool *pospf) {
  ACS37800_REGISTER_22_t store;
  ACS37800ERR error = readRegister(&store.data.all, ACS37800_REGISTER_VOLATILE_22);  // Read register 22
  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }
  // Extract papparent. Convert to VA
  // Note: datasheet says:
  // "Apparent power output magnitude. This field is an unsigned
  //  16-bit fixed point number with 16 fractional bits, where MaxPow
  //  = 0.704. To convert the value (input power) to line power, divide
  //  the input power by the RSENSE and RISO voltage divider ratio
  //  using actual resistor values."
  // Datasheet also says:
  //  "6.15 LSB/mVA for the 30A version and 2.05 LSB/mVA for the 90A version"
  float power = (float) store.data.bits.papparent;
  float LSBpermVA = 6.15;                      // LSB per mVA
  LSBpermVA *= 30.0 / current_sensing_range_;  // Correct for sensor version
  power /= LSBpermVA;                          // Convert from codes to mVA
  // Correct for the voltage divider: (RISO1 + RISO2 + RSENSE) / RSENSE
  // Or:  (RISO1 + RISO2 + RISO3 + RISO4 + RSENSE) / RSENSE
  float resistorMultiplier = (divider_resistance_ohm_ + shunt_resistance_ohm_) / shunt_resistance_ohm_;
  power *= resistorMultiplier;
  power /= 1000;  // Convert from mVAR to VAR
  *pApparent = power;

  // Extract power factor
  // Datasheet says:
  // "Power factor output. This field is a signed 11-bit fixed point number
  //  with 10 fractional bits. It ranges from –1 to ~1 with a step
  //  size of 2^-10."
  union {
    int16_t Signed;
    uint16_t unSigned;
  } signedUnsigned;                                         // Avoid any ambiguity when casting to signed int
  signedUnsigned.unSigned = store.data.bits.pfactor << 5;   // Move 11-bit number into 16-bits (signed)
  float pfactor = (float) signedUnsigned.Signed / 32768.0;  // Convert to +/- 1
  *pFactor = pfactor;
  // Extract posangle and pospf
  *posangle = store.data.bits.posangle & 0x1;
  *pospf = store.data.bits.pospf & 0x1;
  return (error);
}

// Read volatile registers 0x2A and 0x2C. Return the vInst (Volts), iInst (Amps) and pInst (VAR).
ACS37800ERR ACS37800Component::readInstantaneous(float *vInst, float *iInst, float *pInst) {
  ACS37800_REGISTER_2A_t store;
  ACS37800ERR error = readRegister(&store.data.all, ACS37800_REGISTER_VOLATILE_2A);  // Read register 2A
  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }
  // Extract the vcodes. Convert to voltage in Volts.
  union {
    int16_t Signed;
    uint16_t unSigned;
  } signedUnsigned;  // Avoid any ambiguity when casting to signed int
  // Extract vcodes as signed int
  // vcodes as actually int16_t but is stored in a uint32_t as a 16-bit bitfield
  signedUnsigned.unSigned = store.data.bits.vcodes;
  float volts = (float) signedUnsigned.Signed;
  // Datasheet says "Voltage Channel ADC Sensitivity: 110 LSB/mV"
  volts /= 27500.0;  // Convert from codes to the fraction of ADC Full Scale
  volts *= 250;      // Convert to mV (Differential Input Range is +/- 250mV)
  volts /= 1000;     // Convert to Volts
  // Correct for the voltage divider: (RISO1 + RISO2 + RSENSE) / RSENSE
  // Or:  (RISO1 + RISO2 + RISO3 + RISO4 + RSENSE) / RSENSE
  float resistorMultiplier = (divider_resistance_ohm_ + shunt_resistance_ohm_) / shunt_resistance_ohm_;
  volts *= resistorMultiplier;
  *vInst = volts;

  // Extract the icodes. Convert to current in Amps.
  signedUnsigned.unSigned = store.data.bits.icodes;  // Extract icodes as signed int
  float amps = (float) signedUnsigned.Signed;
  amps /= 27500.0;                 // Convert from codes to the fraction of ADC Full Scale
  amps *= current_sensing_range_;  // Convert to Amps
  *iInst = amps;

  ACS37800_REGISTER_2C_t pstore;
  error = readRegister(&pstore.data.all, ACS37800_REGISTER_VOLATILE_2C);  // Read register 2C
  if (error != ACS37800_SUCCESS) {
    return (error);  // Bail
  }
  // Extract pinstant as signed int. Convert to W
  // pinstant as actually int16_t but is stored in a uint32_t as a 16-bit bitfield
  signedUnsigned.unSigned = pstore.data.bits.pinstant;
  float power = (float) signedUnsigned.Signed;
  // Datasheet says: 3.08 LSB/mW for the 30A version and 1.03 LSB/mW for the 90A version
  float LSBpermW = 3.08;                      // LSB per mW
  LSBpermW *= 30.0 / current_sensing_range_;  // Correct for sensor version
  power /= LSBpermW;                          // Convert from codes to mW
  // Correct for the voltage divider: (RISO1 + RISO2 + RSENSE) / RSENSE
  // Or:  (RISO1 + RISO2 + RISO3 + RISO4 + RSENSE) / RSENSE
  power *= resistorMultiplier;
  power /= 1000;  // Convert from mW to W
  *pInst = power;
  return (error);
}

// Read volatile register 0x2D. Return the error flags.
ACS37800ERR ACS37800Component::readErrorFlags(ACS37800_REGISTER_2D_t *errorFlags) {
  ACS37800ERR error = readRegister(&errorFlags->data.all, ACS37800_REGISTER_VOLATILE_2D);  // Read register 2D
  return (error);
}

}  // namespace acs37800
}  // namespace esphome
