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
  StaticJsonDocument<512> doc;
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

void register_usb_pd_controller_init_and_routes_tests() {
  // handle() timing test
  RUN_TEST(test_handle_early_return_due_to_interval);
  // Route handler tests
  RUN_TEST(test_mainPageHandler_sets_progmem);
  RUN_TEST(test_availableVoltagesHandler_lists_values);
  RUN_TEST(test_availableCurrentsHandler_lists_values);
  RUN_TEST(test_pdoProfilesHandler_disconnected_503);
  RUN_TEST(test_pdoProfilesHandler_connected_lists_pdos);
  RUN_TEST(test_setPDConfigHandler_invalid_json_400);
  RUN_TEST(test_setPDConfigHandler_invalid_values_400);
  RUN_TEST(test_setPDConfigHandler_not_connected_503);
  RUN_TEST(test_setPDConfigHandler_success_200);
}

#endif // NATIVE_PLATFORM
