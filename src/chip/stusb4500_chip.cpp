#if defined(ARDUINO) || defined(ESP_PLATFORM)

#include "stusb4500_chip.h"

#include <SparkFun_STUSB4500.h>
#include <Wire.h>

class STUSB4500Chip::Impl {
public:
  STUSB4500 chip;
};

STUSB4500Chip::STUSB4500Chip() : impl(new Impl()) {}

bool STUSB4500Chip::probe(uint8_t i2cAddress) {
  Wire.beginTransmission(i2cAddress);
  uint8_t err = Wire.endTransmission();
  return err == 0;
}

bool STUSB4500Chip::begin() { return impl->chip.begin(); }

void STUSB4500Chip::read() { impl->chip.read(); }

int STUSB4500Chip::getPdoNumber() const { return impl->chip.getPdoNumber(); }

float STUSB4500Chip::getVoltage(int pdoIndex) const {
  return impl->chip.getVoltage(pdoIndex);
}

float STUSB4500Chip::getCurrent(int pdoIndex) const {
  return impl->chip.getCurrent(pdoIndex);
}

void STUSB4500Chip::setVoltage(int pdoIndex, float volts) {
  impl->chip.setVoltage(pdoIndex, volts);
}

void STUSB4500Chip::setCurrent(int pdoIndex, float amps) {
  impl->chip.setCurrent(pdoIndex, amps);
}

void STUSB4500Chip::setPdoNumber(int pdoIndex) {
  impl->chip.setPdoNumber(pdoIndex);
}

void STUSB4500Chip::write() { impl->chip.write(); }

void STUSB4500Chip::softReset() { impl->chip.softReset(); }

#endif // ARDUINO || ESP_PLATFORM
