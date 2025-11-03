#ifndef FAKE_USB_PD_CHIP_H
#define FAKE_USB_PD_CHIP_H

#include <array>
#include <usb_pd_chip.h>


class FakeUsbPdChip : public IUsbPdChip {
public:
  bool present = true;
  int active = 1;
  std::array<float, 4> volt{{0, 5.0f, 12.0f, 20.0f}};
  std::array<float, 4> amps{{0, 1.0f, 2.0f, 3.0f}};

  bool probe(uint8_t) override { return present; }
  bool begin() override { return present; }
  void read() override {}
  int getPdoNumber() const override { return active; }
  float getVoltage(int idx) const override { return volt[idx]; }
  float getCurrent(int idx) const override { return amps[idx]; }
  void setVoltage(int idx, float v) override { volt[idx] = v; }
  void setCurrent(int idx, float a) override { amps[idx] = a; }
  void setPdoNumber(int idx) override { active = idx; }
  void write() override {}
  void softReset() override {}
};

#endif // FAKE_USB_PD_CHIP_H
