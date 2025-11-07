#include <unity.h>

#ifdef NATIVE_PLATFORM
#include "fakes/fake_usb_pd_chip.h"
#include <ArduinoFake.h>
#include <ArduinoJson.h>
#include <interface/core/web_request_core.h>
#include <interface/core/web_response_core.h>
#include <usb_pd_controller.h>
using namespace fakeit;

// ---- handle() timing ----
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
  StaticJsonDocument<256> doc;
  auto err = deserializeJson(doc, res.getContent());
  TEST_ASSERT_FALSE(err);
  TEST_ASSERT_FALSE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(doc["pdos"].is<JsonArray>());
}

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
  
  // Verify all expected JSON fields are present
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
  
  // Initially not connected
  TEST_ASSERT_FALSE(ctrl.isPdBoardConnected());
  
  // Call pdStatusHandler which should trigger reconnection
  WebRequestCore req;
  WebResponseCore res;
  ctrl.pdStatusHandler(req, res);
  
  StaticJsonDocument<256> doc;
  deserializeJson(doc, res.getContent());
  
  // Should have connected and read config
  TEST_ASSERT_TRUE(doc["connected"].as<bool>());
  TEST_ASSERT_TRUE(doc["success"].as<bool>());
  TEST_ASSERT_TRUE(ctrl.isPdBoardConnected());
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

void register_usb_pd_controller_routes_tests() {
  // handle timing/state
  RUN_TEST(test_handle_early_return_due_to_interval);
  RUN_TEST(test_handle_check_after_30_seconds_no_change);
  RUN_TEST(test_handle_disconnect_detected);
  RUN_TEST(test_handle_reconnection_detected);
  // route handlers
  RUN_TEST(test_mainPageHandler_sets_progmem);
  RUN_TEST(test_availableVoltagesHandler_lists_values);
  RUN_TEST(test_availableCurrentsHandler_lists_values);
  RUN_TEST(test_pdoProfilesHandler_disconnected_503);
  RUN_TEST(test_pdStatusHandler_json_fields_when_connected);
  RUN_TEST(test_pdStatusHandler_reconnection_path);
  RUN_TEST(test_pdoProfilesHandler_connected_lists_pdos);
  RUN_TEST(test_pdoProfilesHandler_complete_profile_data);
  // setPDConfig
  RUN_TEST(test_setPDConfigHandler_invalid_json_400);
  RUN_TEST(test_setPDConfigHandler_invalid_values_400);
  RUN_TEST(test_setPDConfigHandler_not_connected_503);
  RUN_TEST(test_setPDConfigHandler_success_200);
  RUN_TEST(test_setPDConfigHandler_parse_current_field);
  RUN_TEST(test_setPDConfigHandler_failure_returns_500);
}

#endif // NATIVE_PLATFORM
