#ifndef USB_PD_CONTROLLER_H
#define USB_PD_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <Wire.h> // Save Arduino's DEFAULT definition before including SparkFun library
#ifdef DEFAULT
#define SAVED_DEFAULT DEFAULT
#undef DEFAULT
#endif

// Include the SparkFun library
#include <SparkFun_STUSB4500.h>

// Restore Arduino's DEFAULT definition after including SparkFun library
#ifdef SAVED_DEFAULT
#define DEFAULT SAVED_DEFAULT
#undef SAVED_DEFAULT
#endif

class USBPDController {
public:
  USBPDController();

  // Initialize the PD controller with optional I2C address (default 0x28)
  void begin(uint8_t i2cAddress = 0x28);

  // Handle periodic operations (should be called in loop)
  void handle();

  // Register web handlers to the provided server
  void setupWebHandlers(ESP8266WebServer &server);

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
  void handlePDStatus(ESP8266WebServer &server);
  void handleAvailableVoltages(ESP8266WebServer &server);
  void handleAvailableCurrents(ESP8266WebServer &server);
  void handlePDOProfiles(ESP8266WebServer &server);
  void handleSetPDConfig(ESP8266WebServer &server);
  void handleRoot(ESP8266WebServer &server);
  void handleSetup(ESP8266WebServer &server);
};

// Global instance
extern USBPDController usbPDController;

#endif // USB_PD_CONTROLLER_H