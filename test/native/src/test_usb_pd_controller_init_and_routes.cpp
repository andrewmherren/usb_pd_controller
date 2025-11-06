#include <unity.h>

#ifdef NATIVE_PLATFORM
#include "fakes/fake_usb_pd_chip.h"
#include <ArduinoFake.h>
#include <ArduinoJson.h>
#include <interface/core/web_request_core.h>
#include <interface/core/web_response_core.h>
#include <usb_pd_controller.h>
using namespace fakeit;

// ---- Initialization paths  (parseConfig and readPDConfig already tested in
// other suite) ---- Note: We avoid calling begin() directly as it requires
// Serial.println stubbing which is challenging with overloaded methods. Instead
// we test component behaviors.

// ---- handle() timing - millis stubbed in setUp, so no special stub needed
// here ----
static void test_handle_early_return_due_to_interval() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);

  //  millis returns 0 from setUp, so first call is always <30s and should
  //  early-return
  ctrl.handle();

  // No crash and still not connected
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
}

// ---- Route/handler behaviors ----
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
  TEST_ASSERT_EQUAL(5.0, arr[0].as<double>());
  TEST_ASSERT_EQUAL(20.0, arr[4].as<double>());
}

static void test_availableCurrentsHandler_lists_values() {
  FakeUsbPdChip chip;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  ctrl.availableCurrentsHandler(req, res);
  StaticJsonDocument<512> doc; // Increased buffer for array of 9 values
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE_MESSAGE(err, "Currents JSON parse error");
  TEST_ASSERT_TRUE(doc.containsKey("currents"));
  JsonArray arr = doc["currents"].as<JsonArray>();
  TEST_ASSERT_TRUE(arr.size() >= 9);
  TEST_ASSERT_EQUAL(0.5, arr[0].as<double>());
  TEST_ASSERT_EQUAL(3.0, arr[arr.size() - 1].as<double>());
}

static void test_pdoProfilesHandler_disconnected_503() {
  FakeUsbPdChip chip;
  chip.present = false;
  USBPDController ctrl(chip);
  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdoProfilesHandler(req, res);
  TEST_ASSERT_EQUAL(503, res.getStatus());
  // Should still be JSON body with error and empty pdos
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
  // Verify JSON response with 3 PDO profiles and activePDO field
  StaticJsonDocument<1024> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_TRUE(doc.containsKey("pdos"));
  JsonArray pdos = doc["pdos"].as<JsonArray>();
  TEST_ASSERT_EQUAL(3, pdos.size());
  TEST_ASSERT_TRUE(doc.containsKey("activePDO"));
}

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
  // Ensure connected
  TEST_ASSERT_TRUE(ctrl.readPDConfig());

  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":12,\"current\":2}");
  ctrl.setPDConfigHandler(req, res);

  // Should be OK with success JSON
  TEST_ASSERT_EQUAL(200, res.getStatus());
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_TRUE(doc["success"].as<bool>());
  TEST_ASSERT_EQUAL(12.0, doc["voltage"].as<double>());
}

// ============================================================================
// Additional handle() tests for complete coverage
// ============================================================================

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

static void test_handle_disconnect_detected() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);

  // Avoid ctrl.begin() to prevent Serial/Wire side-effects in native tests.
  // Establish "connected" state via readPDConfig() instead.
  When(Method(ArduinoFake(), delay)).AlwaysReturn();
  // Ensure handle() passes the 30s interval check on first call
  When(Method(ArduinoFake(), millis)).Return(31001, 31001);

  // Establish connection without invoking begin()
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

  // Establish initial connection via readPDConfig instead of begin()
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());

  // Simulate disconnection
  chip.present = false;

  // Stub millis to trigger check after 30s interval (first call returns 0,
  // second returns 31001)
  // Return >30s on both calls so handle() runs and detects the disconnect
  When(Method(ArduinoFake(), millis)).Return(31001, 31001);
  ctrl.handle(); // Should detect disconnection
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());

  // Reconnect chip
  chip.present = true;

  // Stub millis for next handle() call (needs fresh interval)
  // lastCheckTime from previous handle() is 31001, so return > 61001 twice
  // to pass the 30s interval gate and then set lastCheckTime
  When(Method(ArduinoFake(), millis)).Return(62002, 62002);
  // With Serial output removed from begin() and Wire.begin stubbed,
  // calling handle() to reconnect should be safe in native tests
  ctrl.handle();
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
}

// ============================================================================
// Additional handler tests for complete coverage
// ============================================================================

static void test_pdStatusHandler_reconnect_during_status_check() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);

  When(Method(ArduinoFake(), delay)).AlwaysReturn();

  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());

  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdStatusHandler(req, res);

  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_TRUE(doc["connected"].as<bool>());
  TEST_ASSERT_TRUE(doc["success"].as<bool>());
  TEST_ASSERT_GREATER_THAN(0, doc["voltage"].as<double>());
}

static void test_pdStatusHandler_refresh_when_already_connected() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);

  // Establish connection via readPDConfig instead of begin()
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());

  chip.volt[chip.getPdoNumber()] = 15.0f;

  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdStatusHandler(req, res);

  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_TRUE(doc["connected"].as<bool>());
  TEST_ASSERT_FLOAT_WITHIN(0.1, 15.0, doc["voltage"].as<double>());
}

static void test_pdStatusHandler_board_disconnected() {
  FakeUsbPdChip chip;
  chip.present = false;
  USBPDController ctrl(chip);

  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdStatusHandler(req, res);

  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_FALSE(doc["connected"].as<bool>());
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(doc.containsKey("message"));
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

static void test_setPDConfigHandler_parse_current_field() {
  FakeUsbPdChip chip;
  chip.present = true;
  USBPDController ctrl(chip);

  // Establish connection via readPDConfig instead of begin()
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

  // Establish connection via readPDConfig instead of begin()
  TEST_ASSERT_TRUE(ctrl.readPDConfig());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());

  // Enable write failure simulation - chip.write() will corrupt values to 0
  chip.simulateWriteFailure = true;

  WebRequestCore req;
  WebResponseCore res;
  req.setBody("{\"voltage\":12.0,\"current\":2.0}");
  ctrl.setPDConfigHandler(req, res);

  // The handler should return 500 when setPDConfig fails
  TEST_ASSERT_EQUAL(500, res.getStatus());
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(doc.containsKey("error"));
}

void register_usb_pd_controller_init_and_routes_tests() {
  // handle() timing test
  RUN_TEST(test_handle_early_return_due_to_interval);
  RUN_TEST(test_handle_check_after_30_seconds_no_change);
  RUN_TEST(test_handle_disconnect_detected);
  RUN_TEST(test_handle_reconnection_detected);

  // Route handler tests
  RUN_TEST(test_mainPageHandler_sets_progmem);
  RUN_TEST(test_availableVoltagesHandler_lists_values);
  RUN_TEST(test_availableCurrentsHandler_lists_values);
  RUN_TEST(test_pdoProfilesHandler_disconnected_503);
  RUN_TEST(test_pdoProfilesHandler_connected_lists_pdos);
  RUN_TEST(test_pdoProfilesHandler_complete_profile_data);

  // setPDConfigHandler tests
  RUN_TEST(test_setPDConfigHandler_invalid_json_400);
  RUN_TEST(test_setPDConfigHandler_invalid_values_400);
  RUN_TEST(test_setPDConfigHandler_not_connected_503);
  RUN_TEST(test_setPDConfigHandler_success_200);
  RUN_TEST(test_setPDConfigHandler_parse_current_field);
  RUN_TEST(test_setPDConfigHandler_failure_returns_500);

  // pdStatusHandler tests
  RUN_TEST(test_pdStatusHandler_reconnect_during_status_check);
  RUN_TEST(test_pdStatusHandler_refresh_when_already_connected);
  RUN_TEST(test_pdStatusHandler_board_disconnected);
}

#endif // NATIVE_PLATFORM
