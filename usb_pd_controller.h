#ifndef USB_PD_CONTROLLER_H
#define USB_PD_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <web_module_interface.h>

// Handle DEFAULT definition conflicts with SparkFun library
// The SparkFun STUSB4500 library defines DEFAULT as 0xFF in
// stusb4500_register_map.h We need to be careful with any DEFAULT definitions
// in the Arduino core
#pragma push_macro("DEFAULT")
#undef DEFAULT

// Include the SparkFun library
#include <SparkFun_STUSB4500.h>

// Restore the original DEFAULT definition if it existed
#pragma pop_macro("DEFAULT")

class USBPDController : public IWebModule {
public:
  USBPDController(); // Initialize the PD controller with optional I2C address
                     // (default 0x28)
  void begin(uint8_t i2cAddress = 0x28);

  // Handle periodic operations (should be called in loop)
  void handle();

  // IWebModule interface implementation
  std::vector<WebRoute> getHttpRoutes() override;
  std::vector<WebRoute> getHttpsRoutes() override;
  String getModuleName() const override { return "USBPDController"; }
  String getModuleVersion() const override { return "2.1.0"; }

  // Check if PD board is connected
  bool isPDBoardConnected();

  // Read current PD configuration
  bool readPDConfig();

  // Set new PD configuration
  bool setPDConfig(float voltage, float current);

  // Get all PDO profiles as JSON string
  String getAllPDOProfiles();

  // Get the main HTML content for this module
  String getMainPageHtml() const;

  // API handler methods (for use by web router)
  String handlePDStatusAPI();
  String handleAvailableVoltagesAPI();
  String handleAvailableCurrentsAPI();
  String handlePDOProfilesAPI();
  String handleSetPDConfigAPI(const String &jsonBody);

private:
  STUSB4500 pdController;

  // Current PD settings
  float currentVoltage;
  float currentCurrent;
  bool pdBoardConnected;
  unsigned long lastCheckTime;
  uint8_t i2cAddress;
};

// Global instance
extern USBPDController usbPDController;

#endif // USB_PD_CONTROLLER_H