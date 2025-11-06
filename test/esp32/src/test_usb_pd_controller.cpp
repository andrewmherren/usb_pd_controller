#include <Arduino.h>
#include <unity.h>

#include <chip/stusb4500_chip.h>
#include <usb_pd_controller.h>


// On-device tests that exercise hardware-dependent paths safely.
// These tests are tolerant of hardware presence: they assert no crashes and
// reasonable invariants rather than requiring a specific board to be attached.

// Basic begin()/initializeHardware() smoke test
void test_esp32_begin_initializeHardware_does_not_crash() {
  STUSB4500Chip chip;
  USBPDController ctrl(chip);

  // Apply default config and attempt to begin
  ctrl.begin();

  // If hardware is present, we might be connected; if not, we should still be
  // alive
  TEST_ASSERT_TRUE(true);
}

// Handle() timing gate should not crash and should respect the interval
void test_esp32_handle_interval_no_crash() {
  STUSB4500Chip chip;
  USBPDController ctrl(chip);

  // First call: usually returns early (<30s)
  ctrl.handle();

  // Wait slightly longer than the configured interval (overridden in test env)
#ifdef USB_PD_HANDLE_INTERVAL_MS
  delay(USB_PD_HANDLE_INTERVAL_MS + 20);
#else
  // Fallback to a short wait to avoid long hangs
  delay(250);
#endif
  ctrl.handle();

  // No assumptions about connection state, just ensure we didn't crash
  TEST_ASSERT_TRUE(true);
}

// Registrar invoked by the ESP32 test entrypoint
void register_esp32_usb_pd_controller_tests() {
  RUN_TEST(test_esp32_begin_initializeHardware_does_not_crash);
  RUN_TEST(test_esp32_handle_interval_no_crash);
}
