#ifndef USB_PD_CHIP_H
#define USB_PD_CHIP_H

#include <stdint.h>

// Minimal abstraction for a USB-PD controller chip (e.g., STUSB4500)
// This allows native tests to use a fake implementation while ESP32 uses
// a real adapter around the SparkFun library.
class IUsbPdChip {
public:
  virtual ~IUsbPdChip() = default;

  // Probe for device presence on the I2C bus at the given address
  virtual bool probe(uint8_t i2cAddress) = 0;

  // Initialize the device after presence detected
  virtual bool begin() = 0;

  // Read current configuration/state from the device
  virtual void read() = 0;

  // Getters for current active PDO number, voltage and current
  virtual int getPdoNumber() const = 0;
  virtual float getVoltage(int pdoIndex) const = 0;
  virtual float getCurrent(int pdoIndex) const = 0;

  // Setters for configuration
  virtual void setVoltage(int pdoIndex, float volts) = 0;
  virtual void setCurrent(int pdoIndex, float amps) = 0;
  virtual void setPdoNumber(int pdoIndex) = 0;

  // Persist configuration and apply immediately
  virtual void write() = 0;
  virtual void softReset() = 0;
};

#endif // USB_PD_CHIP_H
