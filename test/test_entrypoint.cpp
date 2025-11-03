#include <unity.h>

// Include native test headers for interface/type tests
// These are pure C++ tests with mocked Arduino APIs via ArduinoFake
#ifdef NATIVE_PLATFORM
#include <ArduinoFake.h>

// Bring in the native test sources so PlatformIO sees the tests
#include "native/src/test_usb_pd_core.cpp"

// Forward declarations from included sources
void register_usb_pd_core_tests();

extern "C" void setUp(void) { ArduinoFakeReset(); }

extern "C" void tearDown(void) {
  // Clean teardown - nothing needed currently
}

int main(int argc, char **argv) {
  UNITY_BEGIN();

  // Register and run native tests
  register_usb_pd_core_tests();

  UNITY_END();
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
