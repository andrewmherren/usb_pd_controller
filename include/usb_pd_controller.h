#ifndef USB_PD_CONTROLLER_H
#define USB_PD_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <interface/auth_types.h>
#include <interface/core/web_request_core.h>
#include <interface/core/web_response_core.h>
#include <interface/openapi_factory.h>
#include <interface/openapi_types.h>
#include <interface/utils/route_variant.h>
#include <interface/web_module_interface.h>
#include <usb_pd_chip.h>
#include <usb_pd_core.h>
#include <web_platform_interface.h>


// DEFAULT macro conflict handling not needed now that SparkFun headers are
// isolated behind an adapter

class USBPDController : public IWebModule {
public:
  // Initialize the PD controller with a chip implementation
  explicit USBPDController(IUsbPdChip &chip);

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
  void mainPageHandler(WebRequestCore &req, WebResponseCore &res);
  void pdStatusHandler(WebRequestCore &req, WebResponseCore &res);
  void availableVoltagesHandler(WebRequestCore &req, WebResponseCore &res);
  void availableCurrentsHandler(WebRequestCore &req, WebResponseCore &res);
  void pdoProfilesHandler(WebRequestCore &req, WebResponseCore &res);
  void setPDConfigHandler(WebRequestCore &req, WebResponseCore &res);

private:
  IUsbPdChip &pdController;
  USBPDCore core;

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