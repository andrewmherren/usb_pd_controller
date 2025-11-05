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

// Minimal ESP32 test to verify the library compiles on hardware
// The interface library doesn't need extensive ESP32-specific tests since
// it's primarily type definitions and pure C++ interfaces

extern "C" void setUp(void) {}
extern "C" void tearDown(void) {}

void test_interface_library_compiles_on_esp32(void) {
  // Simple test to verify library compiles and links on ESP32
  TEST_ASSERT_TRUE(true);
}

void setup() {
  // Allow USB CDC/Serial to enumerate
  delay(2000);
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  UNITY_BEGIN();

  UNITY_END();
}

void loop() {
  // No-op
}
#endif
