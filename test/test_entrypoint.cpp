#include <unity.h>

// Include native test headers for interface/type tests
// These are pure C++ tests with mocked Arduino APIs via ArduinoFake
#ifdef NATIVE_PLATFORM
#include <ArduinoFake.h>
#include <testing/testing_platform_provider.h>
using namespace fakeit;

// Forward declarations from included sources
void register_usb_pd_core_tests();
void register_usb_pd_core_negative_tests();
void register_usb_pd_controller_tests();
void register_usb_pd_controller_init_and_routes_tests();

// Global provider that persists across tests (but gets reset in setUp)
static MockWebPlatformProvider *globalProvider = nullptr;

extern "C" void setUp(void) {
  ArduinoFakeReset();
  // Create a fresh mock platform instance for each test
  if (globalProvider) {
    delete globalProvider;
  }
  globalProvider = new MockWebPlatformProvider();
  IWebPlatformProvider::instance = globalProvider;
  // Stub Wire.begin to avoid any side effects in native tests
  When(OverloadedMethod(ArduinoFake(Wire), begin, void())).AlwaysDo([]() {});
  // Stub delay to avoid timing side-effects in native tests
  When(Method(ArduinoFake(), delay)).AlwaysReturn();
  // Stub millis for timing-related tests
  When(Method(ArduinoFake(), millis)).AlwaysReturn(0UL);
}

extern "C" void tearDown(void) {
  // Reset platform instance to avoid static destructor order issues
  IWebPlatformProvider::instance = nullptr;
  // Don't delete here - will be deleted in next setUp or at program end
}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Register and run native tests
  register_usb_pd_core_tests();
  register_usb_pd_core_negative_tests();
  register_usb_pd_controller_tests();
  register_usb_pd_controller_init_and_routes_tests();

  UNITY_END();

  // Clean up global provider
  if (globalProvider) {
    delete globalProvider;
    globalProvider = nullptr;
  }

  return 0;
}

// ESP32 entrypoint - basic compilation test
#else
#include <Arduino.h>

// ESP32 test entrypoint: runs on-device tests located under test/esp32 (recursive)
// Each on-device test file should expose a registrar function that runs its
// own RUN_TEST calls. We invoke those here.

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

// Optional: still allow a minimal sanity test
static void test_esp32_sanity_compiles_and_runs() { TEST_ASSERT_TRUE(true); }

// Forward declarations for on-device tests (defined under test/esp32)
void test_esp32_begin_initializeHardware_does_not_crash();
void test_esp32_handle_interval_no_crash();

void setup() {
  // Allow USB CDC/Serial to enumerate
  delay(2000);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  UNITY_BEGIN();
  // Give the serial monitor a moment to attach before printing results
  delay(500);

  RUN_TEST(test_esp32_sanity_compiles_and_runs);
  RUN_TEST(test_esp32_begin_initializeHardware_does_not_crash);
  RUN_TEST(test_esp32_handle_interval_no_crash);

  UNITY_END();
}

void loop() {
  // No-op
}
#endif
