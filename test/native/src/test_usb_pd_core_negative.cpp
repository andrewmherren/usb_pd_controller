#include <unity.h>

#ifdef NATIVE_PLATFORM
#include "fakes/fake_usb_pd_chip.h"
#include <usb_pd_core.h>

static void test_readConfig_fails_when_values_zero() {
  FakeUsbPdChip chip;
  chip.active = 1;
  chip.volt[1] = 0.0f; // Force invalid reading
  USBPDCore core(chip);
  float v, c;
  int p;
  TEST_ASSERT_FALSE(core.readConfig(v, c, p));
}

// Chip that causes read-back to fail after configuration
class FailingReadBackChip : public FakeUsbPdChip {
public:
  void softReset() override {
    // After soft reset, simulate that voltage/current read as zero
    volt[active] = 0.0f;
  }
};

static void test_setConfig_returns_false_on_readback_failure() {
  FailingReadBackChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(12.0f, 1.5f);
  TEST_ASSERT_FALSE(ok);
}

void register_usb_pd_core_negative_tests() {
  RUN_TEST(test_readConfig_fails_when_values_zero);
  RUN_TEST(test_setConfig_returns_false_on_readback_failure);
}

#endif // NATIVE_PLATFORM
