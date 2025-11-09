#include <unity.h>

#ifdef NATIVE_PLATFORM
#include "fakes/fake_usb_pd_chip.h"
#include <ArduinoFake.h>
#include <ArduinoJson.h>
#include <interface/core/web_request_core.h>
#include <interface/core/web_response_core.h>
#include <usb_pd_controller.h>
using namespace fakeit;

static void test_module_metadata() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  TEST_ASSERT_EQUAL_STRING("USB PD Controller", ctrl.getModuleName().c_str());
  TEST_ASSERT_EQUAL_STRING(WEB_MODULE_VERSION_STR, module.getModuleVersion().c_str());
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

static void test_readPDConfig_core_read_failure() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  
  // Connect the board
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  
  // Corrupt chip state to make core.readConfig fail
  chip.volt[chip.getPdoNumber()] = 0.0f;
  
  // This should return false on read failure
  TEST_ASSERT_FALSE(ctrl.readPDConfig());
}

static void test_readPDConfig_reconnects_when_disconnected() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  
  // Initially not connected
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
  
  // readPDConfig should trigger reconnection (line 218)
  bool result = ctrl.readPDConfig();
  
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  TEST_ASSERT_GREATER_THAN(0, ctrl.getCurrentVoltage());
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

static void test_setPDConfig_calls_delay_on_success() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  
  When(Method(ArduinoFake(), delay)).AlwaysReturn();
  
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.setPDConfig(9.0f, 1.5f));
  
  // Verify delay was called (line 227)
  Verify(Method(ArduinoFake(), delay).Using(100)).Once();
}

static void test_getAllPDOProfiles_behaviour() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  // Not connected yet -> error string
  auto notConn = ctrl.getAllPDOProfiles();
  TEST_ASSERT_NOT_EQUAL(-1, notConn.indexOf("error"));
  TEST_ASSERT_NOT_EQUAL(-1, notConn.indexOf("not connected"));
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

// ============================================================================
// Additional coverage tests for begin() and initializeHardware()
// ============================================================================

static void test_begin_calls_initializeHardware() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);

  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const char *)))
      .AlwaysReturn(1);
  When(Method(ArduinoFake(), millis)).AlwaysReturn(0);
  When(Method(ArduinoFake(), delay)).AlwaysReturn();

  ctrl.begin();

  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  TEST_ASSERT_GREATER_THAN(0, ctrl.getCurrentVoltage());
}

static void test_begin_with_config_variant() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);

  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const char *)))
      .AlwaysReturn(1);
  When(Method(ArduinoFake(), millis)).AlwaysReturn(0);
  When(Method(ArduinoFake(), delay)).AlwaysReturn();

  DynamicJsonDocument doc(256);
  doc["SDA"] = 21;
  doc["SCL"] = 22;
  doc["i2cAddress"] = 0x29;

  ctrl.begin(doc.as<JsonVariant>());

  TEST_ASSERT_EQUAL(21, ctrl.getSdaPin());
  TEST_ASSERT_EQUAL(22, ctrl.getSclPin());
  TEST_ASSERT_EQUAL_UINT8(0x29, ctrl.getI2cAddress());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
}

static void test_initializeHardware_chip_not_present() {
  FakeUsbPdChip chip;
  chip.present = false;
  USBPDController ctrl(chip);

  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const char *)))
      .AlwaysReturn(1);
  When(Method(ArduinoFake(), millis)).AlwaysReturn(0);
  When(Method(ArduinoFake(), delay)).AlwaysReturn();

  ctrl.begin();

  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
  TEST_ASSERT_EQUAL(0, ctrl.getCurrentVoltage());
}

// Helper chip that fails begin() even when present
class ChipFailsBegin : public FakeUsbPdChip {
public:
  ChipFailsBegin() { present = true; }
  bool begin() override { return false; } // Fail initialization
};

static void test_initializeHardware_chip_present_but_begin_fails() {
  ChipFailsBegin chip;
  USBPDController ctrl(chip);

  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const char *)))
      .AlwaysReturn(1);
  When(Method(ArduinoFake(), millis)).AlwaysReturn(0);
  When(Method(ArduinoFake(), delay)).AlwaysReturn();

  ctrl.begin();

  // Chip is detected (probe returns true) but begin() failed (line 55 false branch)
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
  TEST_ASSERT_EQUAL(0, ctrl.getCurrentVoltage());
}

// ============================================================================
// handle() timing and state tests
// ============================================================================

static void test_handle_early_return_due_to_interval() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  ctrl.handle();
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
}

static void test_handle_check_after_30_seconds_no_change() {
  FakeUsbPdChip chip;
  chip.present = false;
  USBPDController ctrl(chip);
  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(const char *)))
      .AlwaysReturn(1);
  When(Method(ArduinoFake(), delay)).AlwaysReturn();
  When(Method(ArduinoFake(), millis)).Return(0, 31000, 31000);
  ctrl.handle();
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
}

  static void test_handle_connected_stays_connected() {
    FakeUsbPdChip chip;
    chip.present = true;
    USBPDController ctrl(chip);
    When(Method(ArduinoFake(), delay)).AlwaysReturn();
  
    // Connect initially
    TEST_ASSERT_TRUE(ctrl.readPDConfig());
    TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  
    // Fast-forward past interval and check again - should stay connected (line 80)
    When(Method(ArduinoFake(), millis)).Return(31001, 31001);
    ctrl.handle();
  
    // Still connected, no state change
    TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  }

static void test_handle_disconnect_detected() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  When(Method(ArduinoFake(), delay)).AlwaysReturn();
  When(Method(ArduinoFake(), millis)).Return(31001, 31001);
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  chip.present = false;
  ctrl.handle();
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
}

static void test_handle_reconnection_detected() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  chip.present = false;
  When(Method(ArduinoFake(), millis)).Return(31001, 31001);
  ctrl.handle();
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
  chip.present = true;
  When(Method(ArduinoFake(), millis)).Return(62002, 62002);
  ctrl.handle();
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
}

// ============================================================================
// Route handler tests
// ============================================================================

static void test_pdStatusHandler_json_fields_when_connected() {
  FakeUsbPdChip chip;
  chip.present = true;
  chip.volt[1] = 9.0f;
  chip.amps[1] = 1.5f;
  USBPDController ctrl(chip);
  
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  
  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdStatusHandler(req, res);
  
  StaticJsonDocument<512> doc;
  deserializeJson(doc, res.getContent());
  
  TEST_ASSERT_TRUE(doc.containsKey("success"));
  TEST_ASSERT_TRUE(doc.containsKey("connected"));
  TEST_ASSERT_TRUE(doc.containsKey("voltage"));
  TEST_ASSERT_TRUE(doc.containsKey("current"));
  TEST_ASSERT_TRUE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(doc["connected"].as<bool>());
  TEST_ASSERT_EQUAL(9.0, doc["voltage"].as<double>());
  TEST_ASSERT_EQUAL(1.5, doc["current"].as<double>());
}

static void test_pdStatusHandler_reconnection_path() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  
  When(Method(ArduinoFake(), delay)).AlwaysReturn();
  
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
  
  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdStatusHandler(req, res);
  
  StaticJsonDocument<256> doc;
  deserializeJson(doc, res.getContent());
  
  TEST_ASSERT_TRUE(doc["connected"].as<bool>());
  TEST_ASSERT_TRUE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
}

static void test_pdStatusHandler_disconnected_shows_message() {
  FakeUsbPdChip chip;
  chip.present = false;
  USBPDController ctrl(chip);
  
  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdStatusHandler(req, res);
  
  StaticJsonDocument<256> doc;
  deserializeJson(doc, res.getContent());
  
  TEST_ASSERT_FALSE(doc["connected"].as<bool>());
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(doc.containsKey("message"));
  TEST_ASSERT_EQUAL_STRING("PD board not connected", doc["message"].as<const char*>());
}

static void test_pdStatusHandler_connected_but_no_values() {
  FakeUsbPdChip chip;
  chip.present = true;
  chip.volt[chip.getPdoNumber()] = 0.0f;
  USBPDController ctrl(chip);
  
  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdStatusHandler(req, res);
  
  StaticJsonDocument<256> doc;
  deserializeJson(doc, res.getContent());
  
  TEST_ASSERT_TRUE(doc["connected"].as<bool>());
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(doc.containsKey("message"));
  TEST_ASSERT_NOT_EQUAL(-1, String(doc["message"].as<const char*>()).indexOf("initialized"));
}

static void test_pdoProfilesHandler_disconnected_503() {
  FakeUsbPdChip chip;
  chip.present = false;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdoProfilesHandler(req, res);
  TEST_ASSERT_EQUAL(503, res.getStatus());
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(doc["pdos"].is<JsonArray>());
}

static void test_pdoProfilesHandler_connected_lists_pdos() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdoProfilesHandler(req, res);
  StaticJsonDocument<1024> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_TRUE(doc.containsKey("pdos"));
  JsonArray pdos = doc["pdos"].as<JsonArray>();
  TEST_ASSERT_EQUAL(3, pdos.size());
  TEST_ASSERT_TRUE(doc.containsKey("activePDO"));
}

static void test_pdoProfilesHandler_complete_profile_data() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  When(Method(ArduinoFake(), delay)).AlwaysReturn();
  chip.active = 2;
  chip.volt[1] = 5.0f;
  chip.volt[2] = 12.0f;
  chip.volt[3] = 20.0f;
  chip.amps[1] = 1.5f;
  chip.amps[2] = 2.0f;
  chip.amps[3] = 3.0f;
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdoProfilesHandler(req, res);
  StaticJsonDocument<2048> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  JsonArray pdos = doc["pdos"].as<JsonArray>();
  TEST_ASSERT_EQUAL(3, pdos.size());
  JsonObject pdo1 = pdos[0];
  TEST_ASSERT_EQUAL(1, pdo1["number"].as<int>());
  TEST_ASSERT_EQUAL(5.0, pdo1["voltage"].as<double>());
  TEST_ASSERT_EQUAL(1.5, pdo1["current"].as<double>());
  TEST_ASSERT_EQUAL(7.5, pdo1["power"].as<double>());
  TEST_ASSERT_FALSE(pdo1["active"].as<bool>());
  TEST_ASSERT_TRUE(pdo1["fixed"].as<bool>());
  JsonObject pdo2 = pdos[1];
  TEST_ASSERT_EQUAL(2, pdo2["number"].as<int>());
  TEST_ASSERT_TRUE(pdo2["active"].as<bool>());
  TEST_ASSERT_FALSE(pdo2.containsKey("fixed"));
  TEST_ASSERT_EQUAL(2, doc["activePDO"].as<int>());
}

// ============================================================================
// setPDConfigHandler validation tests
// ============================================================================

static void test_setPDConfigHandler_invalid_json_400() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("not-json");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(400, res.getStatus());
}

static void test_setPDConfigHandler_invalid_values_400() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":1,\"current\":10}");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(400, res.getStatus());
}

static void test_setPDConfigHandler_voltage_too_low() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":4.0,\"current\":2.0}");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(400, res.getStatus());
}

static void test_setPDConfigHandler_voltage_too_high() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":21.0,\"current\":2.0}");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(400, res.getStatus());
}

static void test_setPDConfigHandler_current_too_low() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":12.0,\"current\":0.3}");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(400, res.getStatus());
}

static void test_setPDConfigHandler_current_too_high() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":12.0,\"current\":3.5}");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(400, res.getStatus());
}

static void test_setPDConfigHandler_not_connected_503() {
  FakeUsbPdChip chip;
  chip.present = false;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":12,\"current\":2}");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(503, res.getStatus());
}

static void test_setPDConfigHandler_success_200() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":12,\"current\":2}");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(200, res.getStatus());
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_TRUE(doc["success"].as<bool>());
  TEST_ASSERT_EQUAL(12.0, doc["voltage"].as<double>());
}

static void test_setPDConfigHandler_parse_current_field() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":9.0,\"current\":1.5}");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(200, res.getStatus());
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_TRUE(doc["success"].as<bool>());
  TEST_ASSERT_EQUAL(1.5, doc["current"].as<double>());
}

static void test_setPDConfigHandler_failure_returns_500() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
  chip.simulateWriteFailure = true;
  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":12.0,\"current\":2.0}");
  ctrl.setPDConfigHandler(req, res);
  TEST_ASSERT_EQUAL(500, res.getStatus());
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(doc.containsKey("error"));
}

static void test_readPDConfig_when_disconnected_returns_false() {
  FakeUsbPdChip chip;
  chip.present = false;
  USBPDController ctrl(chip);

  TEST_ASSERT_FALSE(ctrl.readPDConfig());
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
}

static void test_parseConfig_with_null_config() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);

  JsonVariant nullConfig;
  ctrl.__test_applyConfig(nullConfig);

  TEST_ASSERT_EQUAL(4, ctrl.getSdaPin());
  TEST_ASSERT_EQUAL(5, ctrl.getSclPin());
  TEST_ASSERT_EQUAL_STRING("sparkfun", ctrl.getBoardType().c_str());
}

static void test_parseConfig_with_all_fields() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);

  DynamicJsonDocument doc(256);
  doc["SDA"] = 21;
  doc["SCL"] = 22;
  doc["board"] = "sparkfun";
  doc["i2cAddress"] = 0x29;

  ctrl.__test_applyConfig(doc.as<JsonVariant>());

  TEST_ASSERT_EQUAL(21, ctrl.getSdaPin());
  TEST_ASSERT_EQUAL(22, ctrl.getSclPin());
  TEST_ASSERT_EQUAL_STRING("sparkfun", ctrl.getBoardType().c_str());
  TEST_ASSERT_EQUAL_UINT8(0x29, ctrl.getI2cAddress());
}

static void test_parseConfig_invalid_board_type_fallback() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);

  DynamicJsonDocument doc(256);
  doc["board"] = "invalid_board";

  ctrl.__test_applyConfig(doc.as<JsonVariant>());

  TEST_ASSERT_EQUAL_STRING("sparkfun", ctrl.getBoardType().c_str());
}

static void test_parseConfig_only_SDA_pin() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);

  DynamicJsonDocument doc(256);
  doc["SDA"] = 25;

  ctrl.__test_applyConfig(doc.as<JsonVariant>());

  TEST_ASSERT_EQUAL(25, ctrl.getSdaPin());
  TEST_ASSERT_EQUAL(5, ctrl.getSclPin()); // Default
}

static void test_parseConfig_only_SCL_pin() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);

  DynamicJsonDocument doc(256);
  doc["SCL"] = 26;

  ctrl.__test_applyConfig(doc.as<JsonVariant>());

  TEST_ASSERT_EQUAL(4, ctrl.getSdaPin()); // Default
  TEST_ASSERT_EQUAL(26, ctrl.getSclPin());
}

static void test_parseConfig_only_board_type() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);

  DynamicJsonDocument doc(256);
  doc["board"] = "sparkfun";

  ctrl.__test_applyConfig(doc.as<JsonVariant>());

  TEST_ASSERT_EQUAL_STRING("sparkfun", ctrl.getBoardType().c_str());
}

static void test_parseConfig_only_i2c_address() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);

  DynamicJsonDocument doc(256);
  doc["i2cAddress"] = 0x28;

  ctrl.__test_applyConfig(doc.as<JsonVariant>());

  TEST_ASSERT_EQUAL_UINT8(0x28, ctrl.getI2cAddress());
}

void register_usb_pd_controller_tests() {
  RUN_TEST(test_module_metadata);
  RUN_TEST(test_isPDBoardConnected_reflects_probe);
  RUN_TEST(test_readPDConfig_reconnect_failure_returns_false);
  RUN_TEST(test_readPDConfig_success_updates_cache);
  RUN_TEST(test_readPDConfig_core_read_failure);
  RUN_TEST(test_readPDConfig_reconnects_when_disconnected);
  RUN_TEST(test_setPDConfig_requires_connection);
  RUN_TEST(test_setPDConfig_success_updates_cache);
  RUN_TEST(test_setPDConfig_calls_delay_on_success);
  RUN_TEST(test_getAllPDOProfiles_behaviour);
  RUN_TEST(test_routes_built_and_sizes);
  RUN_TEST(test_pdStatusHandler_builds_json);
  RUN_TEST(test_parseConfig_updates_fields_via_test_helper);
  RUN_TEST(test_mainPageHandler_sets_progmem);
  RUN_TEST(test_availableVoltagesHandler_lists_values);
  RUN_TEST(test_availableCurrentsHandler_lists_values);

  // Additional coverage tests
  RUN_TEST(test_begin_calls_initializeHardware);
  RUN_TEST(test_begin_with_config_variant);
  RUN_TEST(test_initializeHardware_chip_not_present);
  RUN_TEST(test_initializeHardware_chip_present_but_begin_fails);
  
  // handle() timing and state
  RUN_TEST(test_handle_early_return_due_to_interval);
  RUN_TEST(test_handle_check_after_30_seconds_no_change);
    RUN_TEST(test_handle_connected_stays_connected);
  RUN_TEST(test_handle_disconnect_detected);
  RUN_TEST(test_handle_reconnection_detected);
  
  // Route handlers
  RUN_TEST(test_pdStatusHandler_json_fields_when_connected);
  RUN_TEST(test_pdStatusHandler_reconnection_path);
  RUN_TEST(test_pdStatusHandler_disconnected_shows_message);
  RUN_TEST(test_pdStatusHandler_connected_but_no_values);
  RUN_TEST(test_pdoProfilesHandler_disconnected_503);
  RUN_TEST(test_pdoProfilesHandler_connected_lists_pdos);
  RUN_TEST(test_pdoProfilesHandler_complete_profile_data);
  
  // setPDConfigHandler validation
  RUN_TEST(test_setPDConfigHandler_invalid_json_400);
  RUN_TEST(test_setPDConfigHandler_invalid_values_400);
  RUN_TEST(test_setPDConfigHandler_voltage_too_low);
  RUN_TEST(test_setPDConfigHandler_voltage_too_high);
  RUN_TEST(test_setPDConfigHandler_current_too_low);
  RUN_TEST(test_setPDConfigHandler_current_too_high);
  RUN_TEST(test_setPDConfigHandler_not_connected_503);
  RUN_TEST(test_setPDConfigHandler_success_200);
  RUN_TEST(test_setPDConfigHandler_parse_current_field);
  RUN_TEST(test_setPDConfigHandler_failure_returns_500);
  
  // parseConfig tests
  RUN_TEST(test_readPDConfig_when_disconnected_returns_false);
  RUN_TEST(test_parseConfig_with_null_config);
  RUN_TEST(test_parseConfig_with_all_fields);
  RUN_TEST(test_parseConfig_invalid_board_type_fallback);
  RUN_TEST(test_parseConfig_only_SDA_pin);
  RUN_TEST(test_parseConfig_only_SCL_pin);
  RUN_TEST(test_parseConfig_only_board_type);
  RUN_TEST(test_parseConfig_only_i2c_address);
}

#endif // NATIVE_PLATFORM
