#include <unity.h>

#ifdef NATIVE_PLATFORM
#include "fakes/fake_usb_pd_chip.h"
#include <ArduinoFake.h>
#include <ArduinoJson.h>
#include <interface/core/web_request_core.h>
#include <interface/core/web_response_core.h>
#include <usb_pd_controller.h>

static void test_module_metadata() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  TEST_ASSERT_EQUAL_STRING("USB PD Controller", ctrl.getModuleName().c_str());
  TEST_ASSERT_EQUAL_STRING("0.1.0", ctrl.getModuleVersion().c_str());
  TEST_ASSERT_TRUE_MESSAGE(ctrl.getModuleDescription().length() > 0,
                           "Description should be non-empty");
}

static void test_isPDBoardConnected_reflects_probe() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  chip.present = true;
  TEST_ASSERT_TRUE(ctrl.isPDBoardConnected());
  chip.present = false;
  TEST_ASSERT_FALSE(ctrl.isPDBoardConnected());
}

static void test_readPDConfig_reconnect_failure_returns_false() {
  FakeUsbPdChip chip;
  chip.present = false; // begin() will fail
  USBPDController ctrl(chip);
  TEST_ASSERT_FALSE(ctrl.readPDConfig());
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
}

static void test_readPDConfig_success_updates_cache() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, chip.getVoltage(chip.getPdoNumber()),
                           ctrl.getCurrentVoltage());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, chip.getCurrent(chip.getPdoNumber()),
                           ctrl.getCurrentCurrent());
}

static void test_setPDConfig_requires_connection() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  // Not connected yet
  TEST_ASSERT_FALSE(ctrl.setPDConfig(12.0f, 2.0f));
}

static void test_setPDConfig_success_updates_cache() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  // Establish connection by reading current config
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.setPDConfig(12.0f, 2.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 12.0f, ctrl.getCurrentVoltage());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, ctrl.getCurrentCurrent());
}

static void test_getAllPDOProfiles_behaviour() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  // Not connected yet -> error string
  auto notConn = ctrl.getAllPDOProfiles();
  TEST_ASSERT_NOT_EQUAL(-1, notConn.indexOf("error"));
  // Connect then should return JSON with pdos
  chip.present = true;
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  auto json = ctrl.getAllPDOProfiles();
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"pdos\""));
  TEST_ASSERT_NOT_EQUAL(-1, json.indexOf("\"activePDO\""));
}

static void test_routes_built_and_sizes() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  auto http = ctrl.getHttpRoutes();
  auto https = ctrl.getHttpsRoutes();
  TEST_ASSERT_TRUE(http.size() >= 7);
  TEST_ASSERT_EQUAL(http.size(), https.size());
}

static void test_pdStatusHandler_builds_json() {
  FakeUsbPdChip chip;
  chip.present = false; // simulate not connected
  USBPDController ctrl(chip);

  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdStatusHandler(req, res);
  TEST_ASSERT_EQUAL_STRING("application/json", res.getMimeType().c_str());
  auto content = res.getContent();
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, content);
  TEST_ASSERT_FALSE_MESSAGE(err, "JSON parse error");
  TEST_ASSERT_TRUE(doc.containsKey("connected"));
  TEST_ASSERT_TRUE(doc.containsKey("success"));
}

static void test_parseConfig_updates_fields_via_test_helper() {
  FakeUsbPdChip chip;
  chip.present = false; // Avoid performing begin success path
  USBPDController ctrl(chip);

  DynamicJsonDocument doc(256);
  doc["SDA"] = 21;
  doc["SCL"] = 22;
  doc["board"] = "unknown"; // should fallback to sparkfun
  doc["i2cAddress"] = 0x29;

  ctrl.__test_applyConfig(doc.as<JsonVariant>());

  TEST_ASSERT_EQUAL(21, ctrl.getSdaPin());
  TEST_ASSERT_EQUAL(22, ctrl.getSclPin());
  TEST_ASSERT_EQUAL_STRING("sparkfun", ctrl.getBoardType().c_str());
  TEST_ASSERT_EQUAL_UINT8(0x29, ctrl.getI2cAddress());
}

static void test_mainPageHandler_sets_progmem() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  ctrl.mainPageHandler(req, res);
  TEST_ASSERT_TRUE(res.hasProgmemContent());
  TEST_ASSERT_EQUAL_STRING("text/html", res.getMimeType().c_str());
}

static void test_availableVoltagesHandler_lists_values() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  ctrl.availableVoltagesHandler(req, res);
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "Voltages JSON parse error");
  TEST_ASSERT_TRUE(doc.containsKey("voltages"));
  JsonArray arr = doc["voltages"].as<JsonArray>();
  TEST_ASSERT_EQUAL(5, arr.size());
}

static void test_availableCurrentsHandler_lists_values() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  ctrl.availableCurrentsHandler(req, res);
  StaticJsonDocument<512> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "Currents JSON parse error");
  TEST_ASSERT_TRUE(doc.containsKey("currents"));
  JsonArray arr = doc["currents"].as<JsonArray>();
  TEST_ASSERT_TRUE(arr.size() >= 9);
}

void register_usb_pd_controller_tests() {
  RUN_TEST(test_module_metadata);
  RUN_TEST(test_isPDBoardConnected_reflects_probe);
  RUN_TEST(test_readPDConfig_reconnect_failure_returns_false);
  RUN_TEST(test_readPDConfig_success_updates_cache);
  RUN_TEST(test_setPDConfig_requires_connection);
  RUN_TEST(test_setPDConfig_success_updates_cache);
  RUN_TEST(test_getAllPDOProfiles_behaviour);
  RUN_TEST(test_routes_built_and_sizes);
  RUN_TEST(test_pdStatusHandler_builds_json);
  RUN_TEST(test_parseConfig_updates_fields_via_test_helper);
  RUN_TEST(test_mainPageHandler_sets_progmem);
  RUN_TEST(test_availableVoltagesHandler_lists_values);
  RUN_TEST(test_availableCurrentsHandler_lists_values);
}

#endif // NATIVE_PLATFORM
