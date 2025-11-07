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

// Branch coverage: Test exact boundary value 5.0V (== condition)
static void test_set_exact_5v_uses_pdo1() {
  FakeUsbPdChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(5.0f, 1.0f);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(1, chip.getPdoNumber());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, chip.getCurrent(1));
}

// Branch coverage: Test value just above 5V (voltage <= 12.0f branch)
static void test_set_9v_uses_pdo2() {
  FakeUsbPdChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(9.0f, 1.5f);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(2, chip.getPdoNumber());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 9.0f, chip.getVoltage(2));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, chip.getCurrent(2));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, chip.getCurrent(1)); // fallback
}

// Branch coverage: Test exact 12V boundary (voltage <= 12.0f edge)
static void test_set_exact_12v_uses_pdo2() {
  FakeUsbPdChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(12.0f, 2.0f);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(2, chip.getPdoNumber());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, chip.getVoltage(2));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, chip.getCurrent(2));
}

// Branch coverage: Test value just above 12V (else branch - PDO3)
static void test_set_15v_uses_pdo3() {
  FakeUsbPdChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(15.0f, 2.5f);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(3, chip.getPdoNumber());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, chip.getVoltage(3));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, chip.getVoltage(2)); // middle fallback
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, chip.getCurrent(3));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, chip.getCurrent(2));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, chip.getCurrent(1)); // final fallback
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

// ============================================================================
// Negative/error path tests for readConfig and setConfig
// ============================================================================

static void test_readConfig_fails_when_voltage_zero() {
  FakeUsbPdChip chip;
  chip.active = 1;
  chip.volt[1] = 0.0f; // Force invalid reading
  USBPDCore core(chip);
  float v, c;
  int p;
  TEST_ASSERT_FALSE(core.readConfig(v, c, p));
}

static void test_readConfig_fails_when_current_zero() {
  FakeUsbPdChip chip;
  chip.active = 1;
  chip.volt[1] = 5.0f;
  chip.amps[1] = 0.0f; // Force invalid current reading
  USBPDCore core(chip);
  float v, c;
  int p;
  TEST_ASSERT_FALSE(core.readConfig(v, c, p));
}

static void test_readConfig_fails_when_voltage_negative() {
  FakeUsbPdChip chip;
  chip.active = 1;
  chip.volt[1] = -1.0f; // Negative voltage
  USBPDCore core(chip);
  float v, c;
  int p;
  TEST_ASSERT_FALSE(core.readConfig(v, c, p));
}

static void test_readConfig_fails_when_current_negative() {
  FakeUsbPdChip chip;
  chip.active = 1;
  chip.volt[1] = 5.0f;
  chip.amps[1] = -1.0f; // Negative current
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

void register_usb_pd_core_tests() {
  // Positive path tests - setConfig
  RUN_TEST(test_set_5v_uses_pdo1_only);
  RUN_TEST(test_set_12v_prefers_pdo2_with_fallback);
  RUN_TEST(test_set_20v_uses_pdo3_with_middle_fallback);
  RUN_TEST(test_set_exact_5v_uses_pdo1);
  RUN_TEST(test_set_9v_uses_pdo2);
  RUN_TEST(test_set_exact_12v_uses_pdo2);
  RUN_TEST(test_set_15v_uses_pdo3);
  
  // Positive path tests - buildPdoProfilesJson
  RUN_TEST(test_buildPdoProfilesJson_complete_structure);
  RUN_TEST(test_buildPdoProfilesJson_with_pdo1_active);
  RUN_TEST(test_buildPdoProfilesJson_with_pdo3_active);
  
  // Negative path tests - readConfig
  RUN_TEST(test_readConfig_fails_when_voltage_zero);
  RUN_TEST(test_readConfig_fails_when_current_zero);
  RUN_TEST(test_readConfig_fails_when_voltage_negative);
  RUN_TEST(test_readConfig_fails_when_current_negative);
  
  // Negative path tests - setConfig
  RUN_TEST(test_setConfig_returns_false_on_readback_failure);
}

#endif // NATIVE_PLATFORM
