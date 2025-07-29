#ifndef USB_PD_CONTROLLER_H
#define USB_PD_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

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

// Use appropriate WebServer library based on board
#if defined(ESP32)
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include <WebServer.h>
typedef WebServer WebServerClass;
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServerClass;
#else
#include <WebServer.h>
typedef WebServer WebServerClass;
#endif
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServerClass;
#endif

class USBPDController {
public:
  USBPDController();

  // Initialize the PD controller with optional I2C address (default 0x28)
  void begin(uint8_t i2cAddress = 0x28);

  // Handle periodic operations (should be called in loop)
  void handle();

  // Register web handlers to the provided server
  void setupWebHandlers(WebServerClass &server);

  // Check if PD board is connected
  bool isPDBoardConnected();

  // Read current PD configuration
  bool readPDConfig();

  // Set new PD configuration
  bool setPDConfig(float voltage, float current);

  // Get all PDO profiles as JSON string
  String getAllPDOProfiles();

private:
  STUSB4500 pdController; // Current PD settings
  float currentVoltage;
  float currentCurrent;
  bool pdBoardConnected;
  unsigned long lastCheckTime;
  uint8_t i2cAddress;

  // Web handler methods
  void handlePDStatus(WebServerClass &server);
  void handleAvailableVoltages(WebServerClass &server);
  void handleAvailableCurrents(WebServerClass &server);
  void handlePDOProfiles(WebServerClass &server);
  void handleSetPDConfig(WebServerClass &server);
  void handleRoot(WebServerClass &server);
  void handleSetup(WebServerClass &server);
};

// Global instance
extern USBPDController usbPDController;

#endif // USB_PD_CONTROLLER_H