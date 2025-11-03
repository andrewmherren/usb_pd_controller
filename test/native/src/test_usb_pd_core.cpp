#include <unity.h>

#ifdef NATIVE_PLATFORM
#include "fakes/fake_usb_pd_chip.h"
#include <ArduinoFake.h>
#include <usb_pd_core.h>


// Pull in production core implementation for native build
#ifndef USB_PD_CORE_IMPL_INCLUDED
#define USB_PD_CORE_IMPL_INCLUDED
#include "../../../src/usb_pd_core.cpp"
#endif

static void test_set_5v_uses_pdo1_only() {
  FakeUsbPdChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(5.0f, 2.0f);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(1, chip.getPdoNumber());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, chip.getCurrent(1));
}

static void test_set_12v_prefers_pdo2_with_fallback() {
  FakeUsbPdChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(12.0f, 1.5f);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(2, chip.getPdoNumber());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, chip.getVoltage(2));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, chip.getCurrent(2));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, chip.getCurrent(1));
}

static void test_set_20v_uses_pdo3_with_middle_fallback() {
  FakeUsbPdChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(20.0f, 3.0f);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(3, chip.getPdoNumber());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, chip.getVoltage(3));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, chip.getVoltage(2));
}

void register_usb_pd_core_tests() {
  RUN_TEST(test_set_5v_uses_pdo1_only);
  RUN_TEST(test_set_12v_prefers_pdo2_with_fallback);
  RUN_TEST(test_set_20v_uses_pdo3_with_middle_fallback);
}

#endif // NATIVE_PLATFORM
