#ifndef USB_PD_CONTROLLER_H
#define USB_PD_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SparkFun_STUSB4500.h>
#include <Wire.h>
#include <web_platform.h>


// Forward declaration of WebPlatform (from web_platform.h)
extern class WebPlatform webPlatform;

// Handle DEFAULT definition conflicts with SparkFun library
// The SparkFun STUSB4500 library defines DEFAULT as 0xFF in
// stusb4500_register_map.h We need to be careful with any DEFAULT definitions
// in the Arduino core
#pragma push_macro("DEFAULT")

// Restore the original DEFAULT definition if it existed
#pragma pop_macro("DEFAULT")

class USBPDController : public IWebModule {
public:
  USBPDController(); // Initialize the PD controller

  // Module lifecycle methods (IWebModule interface)
  void begin() override;
  void begin(const JsonVariant &config) override;
  void handle() override;

  // IWebModule interface implementation
  std::vector<RouteVariant> getHttpRoutes() override;
  std::vector<RouteVariant> getHttpsRoutes() override;
  String getModuleName() const override { return "USB PD Controller"; }
  String getModuleVersion() const override { return "0.1.0"; }
  String getModuleDescription() const override {
    return "USB-C Power Delivery voltage and current control";
  }

  // Check if PD board is connected
  bool isPDBoardConnected();

  // Read current PD configuration
  bool readPDConfig();

  // Set new PD configuration
  bool setPDConfig(float voltage, float current);

  // Get all PDO profiles as JSON string
  String getAllPDOProfiles();

  // Route handler methods
  void mainPageHandler(WebRequest &req, WebResponse &res);
  void pdStatusHandler(WebRequest &req, WebResponse &res);
  void availableVoltagesHandler(WebRequest &req, WebResponse &res);
  void availableCurrentsHandler(WebRequest &req, WebResponse &res);
  void pdoProfilesHandler(WebRequest &req, WebResponse &res);
  void setPDConfigHandler(WebRequest &req, WebResponse &res);

private:
  STUSB4500 pdController;

  // Current PD settings
  float currentVoltage = 0.0;
  float currentCurrent = 0.0;
  bool pdBoardConnected = false;
  unsigned long lastCheckTime = 0;
  uint8_t i2cAddress = 0x28;

  // I2C configuration
  int sdaPin = 4;
  int sclPin = 5;
  String boardType = "sparkfun";

  // Initialize I2C and hardware with configuration
  void initializeHardware();
  void parseConfig(const JsonVariant &config);
};

// Global instance
extern USBPDController usbPDController;

#endif // USB_PD_CONTROLLER_H