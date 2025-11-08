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

// ============================================================================
// Additional branch coverage tests for buildPdoProfilesJson
// ============================================================================

// Test with PDO2 active to ensure active flag branches are covered
static void test_buildPdoProfilesJson_pdo2_active_branches() {
  FakeUsbPdChip chip;
  chip.active = 2;
  chip.volt[1] = 5.0f;
  chip.volt[2] = 12.0f;
  chip.volt[3] = 20.0f;
  chip.amps[1] = 1.0f;
  chip.amps[2] = 2.0f;
  chip.amps[3] = 3.0f;

  USBPDCore core(chip);
  String json = core.buildPdoProfilesJson();

  // PDO1 should have active:false
  int pdo1Start = json.indexOf("\"number\":1");
  TEST_ASSERT_NOT_EQUAL(-1, pdo1Start);
  int pdo1End = json.indexOf("}", pdo1Start);
  String pdo1Section = json.substring(pdo1Start, pdo1End);
  TEST_ASSERT_NOT_EQUAL(-1, pdo1Section.indexOf("\"active\":false"));

  // PDO2 should have active:true
  int pdo2Start = json.indexOf("\"number\":2");
  TEST_ASSERT_NOT_EQUAL(-1, pdo2Start);
  int pdo2End = json.indexOf("}", pdo2Start);
  String pdo2Section = json.substring(pdo2Start, pdo2End);
  TEST_ASSERT_NOT_EQUAL(-1, pdo2Section.indexOf("\"active\":true"));

  // PDO3 should have active:false
  int pdo3Start = json.indexOf("\"number\":3");
  TEST_ASSERT_NOT_EQUAL(-1, pdo3Start);
  int pdo3End = json.indexOf("}", pdo3Start);
  String pdo3Section = json.substring(pdo3Start, pdo3End);
  TEST_ASSERT_NOT_EQUAL(-1, pdo3Section.indexOf("\"active\":false"));
}

// Test PDO loop iterations - verify all 3 PDOs are processed
static void test_buildPdoProfilesJson_processes_all_three_pdos() {
  FakeUsbPdChip chip;
  chip.active = 1;
  chip.volt[1] = 5.1f;
  chip.volt[2] = 9.5f;
  chip.volt[3] = 15.3f;
  chip.amps[1] = 1.1f;
  chip.amps[2] = 2.2f;
  chip.amps[3] = 3.3f;

  USBPDCore core(chip);
  String json = core.buildPdoProfilesJson();

  // Count occurrences of "number" field - should be exactly 3
  int count = 0;
  int pos = 0;
  while ((pos = json.indexOf("\"number\":", pos)) != -1) {
    count++;
    pos++;
  }
  TEST_ASSERT_EQUAL(3, count);

  // Verify each PDO has all expected fields
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"voltage\":5."));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"voltage\":9."));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"voltage\":15."));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"current\":1."));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"current\":2."));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"current\":3."));
}

// Test comma insertion logic (i > 1) - only PDO2 and PDO3 should have commas before them
static void test_buildPdoProfilesJson_comma_logic() {
  FakeUsbPdChip chip;
  chip.active = 1;

  USBPDCore core(chip);
  String json = core.buildPdoProfilesJson();

  // The JSON should have structure: {"pdos":[{PDO1},{PDO2},{PDO3}],"activePDO":1}
  // Count commas within the pdos array
  int pdosStart = json.indexOf("[");
  int pdosEnd = json.indexOf("]");
  String pdosArray = json.substring(pdosStart, pdosEnd);

  // Should have exactly 2 commas between the 3 PDO objects
  int commaCount = 0;
  for (int i = 0; i < pdosArray.length(); i++) {
    // Count commas at the top level (between {} objects)
    if (pdosArray.charAt(i) == '}') {
      // Check if next non-whitespace char is comma
      for (int j = i + 1; j < pdosArray.length(); j++) {
        if (pdosArray.charAt(j) == ',') {
          commaCount++;
          break;
        } else if (pdosArray.charAt(j) == '{' || pdosArray.charAt(j) == ']') {
          break;
        }
      }
    }
  }
  TEST_ASSERT_EQUAL(2, commaCount); // Should have 2 commas separating 3 objects
}

// Test fixed flag only on PDO1
static void test_buildPdoProfilesJson_fixed_flag_only_on_pdo1() {
  FakeUsbPdChip chip;
  chip.active = 2;

  USBPDCore core(chip);
  String json = core.buildPdoProfilesJson();

  // Count occurrences of "fixed" - should be exactly 1
  int count = 0;
  int pos = 0;
  while ((pos = json.indexOf("\"fixed\"", pos)) != -1) {
    count++;
    pos++;
  }
  TEST_ASSERT_EQUAL(1, count);

  // Verify it's in the PDO1 section
  int pdo1Start = json.indexOf("\"number\":1");
  int pdo1End = json.indexOf("}", pdo1Start);
  String pdo1Section = json.substring(pdo1Start, pdo1End);
  TEST_ASSERT_NOT_EQUAL(-1, pdo1Section.indexOf("\"fixed\":true"));
}

// Test power calculation in JSON (voltage * current)
static void test_buildPdoProfilesJson_power_calculations() {
  FakeUsbPdChip chip;
  chip.active = 1;
  chip.volt[1] = 5.0f;
  chip.volt[2] = 12.0f;
  chip.volt[3] = 20.0f;
  chip.amps[1] = 2.0f;  // 5V * 2A = 10W
  chip.amps[2] = 3.0f;  // 12V * 3A = 36W
  chip.amps[3] = 5.0f;  // 20V * 5A = 100W

  USBPDCore core(chip);
  String json = core.buildPdoProfilesJson();

  // Verify power values are calculated correctly
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"power\":10"));   // 5*2
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"power\":36"));   // 12*3
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"power\":100"));  // 20*5
}

// ============================================================================
// Additional branch coverage tests for setConfig edge cases
// ============================================================================

// Test with voltage slightly less than 5.0 (should use PDO2 path)
static void test_setConfig_voltage_below_5v_uses_pdo2() {
  FakeUsbPdChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(4.9f, 1.0f);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(2, chip.getPdoNumber()); // Should take the <= 12.0 path
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.9f, chip.getVoltage(2));
}

// Test with voltage slightly above 12.0 (should use PDO3 path)
static void test_setConfig_voltage_above_12v_uses_pdo3() {
  FakeUsbPdChip chip;
  USBPDCore core(chip);
  bool ok = core.setConfig(12.1f, 2.0f);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL(3, chip.getPdoNumber());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.1f, chip.getVoltage(3));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, chip.getVoltage(2)); // middle fallback
}

// Test readConfig with positive values to cover success path
static void test_readConfig_succeeds_with_valid_values() {
  FakeUsbPdChip chip;
  chip.active = 2;
  chip.volt[2] = 12.0f;
  chip.amps[2] = 2.5f;

  USBPDCore core(chip);
  float v, c;
  int p;
  bool result = core.readConfig(v, c, p);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, v);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.5f, c);
  TEST_ASSERT_EQUAL(2, p);
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
  
  // Additional branch coverage tests - buildPdoProfilesJson
  RUN_TEST(test_buildPdoProfilesJson_pdo2_active_branches);
  RUN_TEST(test_buildPdoProfilesJson_processes_all_three_pdos);
  RUN_TEST(test_buildPdoProfilesJson_comma_logic);
  RUN_TEST(test_buildPdoProfilesJson_fixed_flag_only_on_pdo1);
  RUN_TEST(test_buildPdoProfilesJson_power_calculations);
  
  // Additional branch coverage tests - setConfig edge cases
  RUN_TEST(test_setConfig_voltage_below_5v_uses_pdo2);
  RUN_TEST(test_setConfig_voltage_above_12v_uses_pdo3);
  RUN_TEST(test_readConfig_succeeds_with_valid_values);
}

#endif // NATIVE_PLATFORM
