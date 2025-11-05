#include <unity.h>

#ifdef NATIVE_PLATFORM
#include "fakes/fake_usb_pd_chip.h"
#include <ArduinoFake.h>
#include <usb_pd_core.h>

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

static void test_buildPdoProfilesJson_complete_structure() {
  FakeUsbPdChip chip;
  chip.active = 2;
  chip.volt[1] = 5.0f;
  chip.volt[2] = 12.0f;
  chip.volt[3] = 20.0f;
  chip.amps[1] = 1.5f;
  chip.amps[2] = 2.0f;
  chip.amps[3] = 3.0f;

  USBPDCore core(chip);
  String json = core.buildPdoProfilesJson();

  // Verify JSON structure
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"pdos\""));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"activePDO\""));

  // Verify all PDOs are present
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"number\":1"));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"number\":2"));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"number\":3"));

  // Verify PDO1 is marked as fixed
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"fixed\":true"));

  // Verify active flag
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"active\":true"));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"active\":false"));

  // Verify voltage/current values
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"voltage\":5"));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"voltage\":12"));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"voltage\":20"));

  // Verify activePDO is 2
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"activePDO\":2"));
}

static void test_buildPdoProfilesJson_with_pdo1_active() {
  FakeUsbPdChip chip;
  chip.active = 1;
  chip.volt[1] = 5.0f;
  chip.volt[2] = 9.0f;
  chip.volt[3] = 15.0f;

  USBPDCore core(chip);
  String json = core.buildPdoProfilesJson();

  // Verify activePDO is 1
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"activePDO\":1"));
}

static void test_buildPdoProfilesJson_with_pdo3_active() {
  FakeUsbPdChip chip;
  chip.active = 3;
  chip.volt[1] = 5.0f;
  chip.volt[2] = 12.0f;
  chip.volt[3] = 20.0f;

  USBPDCore core(chip);
  String json = core.buildPdoProfilesJson();

  // Verify activePDO is 3
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"activePDO\":3"));
}

void register_usb_pd_core_tests() {
  RUN_TEST(test_set_5v_uses_pdo1_only);
  RUN_TEST(test_set_12v_prefers_pdo2_with_fallback);
  RUN_TEST(test_set_20v_uses_pdo3_with_middle_fallback);
  RUN_TEST(test_buildPdoProfilesJson_complete_structure);
  RUN_TEST(test_buildPdoProfilesJson_with_pdo1_active);
  RUN_TEST(test_buildPdoProfilesJson_with_pdo3_active);
}

#endif // NATIVE_PLATFORM
