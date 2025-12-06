#ifndef STUSB4500_CHIP_ADAPTER_H
#define STUSB4500_CHIP_ADAPTER_H

#include <usb_pd_chip.h>

// Adapter around SparkFun STUSB4500 library.
// Only compiled for Arduino/ESP32 targets.
class STUSB4500Chip : public IUsbPdChip {
public:
  STUSB4500Chip();
  ~STUSB4500Chip() override = default;

  bool probe(uint8_t i2cAddress) override;
  bool begin() override;
  void read() override;

  int getPdoNumber() const override;
  float getVoltage(int pdoIndex) const override;
  float getCurrent(int pdoIndex) const override;

  void setVoltage(int pdoIndex, float volts) override;
  void setCurrent(int pdoIndex, float amps) override;
  void setPdoNumber(int pdoIndex) override;

  void write() override;
  void softReset() override;

private:
  // Forward-declared in cpp to avoid leaking Arduino headers here
  class Impl;
  Impl *impl; // PIMPL to keep headers Arduino-free
};

#endif // STUSB4500_CHIP_ADAPTER_H
