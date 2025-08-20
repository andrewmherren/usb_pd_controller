#include "usb_pd_controller.h"
#include "assets/usb_pd_html.h"
#include "assets/usb_pd_js.h"

// Create global instance of USBPDController
USBPDController usbPDController;

// USBPDController implementation
USBPDController::USBPDController()
    : currentVoltage(0.0), currentCurrent(0.0), pdBoardConnected(false),
      lastCheckTime(0), i2cAddress(0x28) {}

void USBPDController::begin(uint8_t i2cAddress) {
  // Set the I2C address
  this->i2cAddress = i2cAddress;

  Serial.println("Initializing USB PD Controller...");
  Serial.print("Using I2C address: 0x");
  Serial.println(i2cAddress, HEX);

  // Check if PD board is connected
  pdBoardConnected = isPDBoardConnected();

  // Initialize USB-PD controller if board is connected
  if (pdBoardConnected) {
    pdBoardConnected = pdController.begin();
    if (pdBoardConnected) {
      Serial.println("STUSB4500 initialized successfully");
      readPDConfig();
    } else {
      Serial.println("Failed to initialize STUSB4500");
    }
  } else {
    Serial.println("STUSB4500 not detected on I2C bus");
  }
  
  // Register static assets
  IWebModule::addStaticAsset("/assets/usb-pd-controller.js", String(FPSTR(USB_PD_JS)), "application/javascript", true);
}

void USBPDController::handle() {
  // Periodically check if PD board connection status has changed
  if (millis() - lastCheckTime >
      30000) { // Check every 30 seconds to reduce I2C spam
    lastCheckTime = millis();

    bool connected = isPDBoardConnected();
    if (connected != pdBoardConnected) {
      if (connected) {
        Serial.println("PD board connected");
        pdBoardConnected = pdController.begin();
        if (pdBoardConnected) {
          readPDConfig();
        }
      } else {
        Serial.println("PD board disconnected");
        pdBoardConnected = false;
      }
    }
  }
}

std::vector<WebRoute> USBPDController::getHttpRoutes() {
  std::vector<WebRoute> routes; // Main page route
  routes.push_back({"/", WebModule::WM_GET,
                    [this](const String &requestBody,
                           const std::map<String, String> &params) -> String {
                      // Set current path for navigation menu
                      IWebModule::setCurrentPath("/usb_pd/");

                      // Use navigation menu injection
                      String htmlContent =
                          String(FPSTR(USB_PD_HTML));
                      htmlContent =
                          IWebModule::injectNavigationMenu(htmlContent);

                      return htmlContent;
                    },
                    "text/html", "Main USB PD control page"});

  // API endpoints
  routes.push_back({"/pd-status", WebModule::WM_GET,
                    [this](const String &requestBody,
                           const std::map<String, String> &params) -> String {
                      return this->handlePDStatusAPI();
                    },
                    "application/json", "Get PD status"});

  routes.push_back({"/available-voltages", WebModule::WM_GET,
                    [this](const String &requestBody,
                           const std::map<String, String> &params) -> String {
                      return this->handleAvailableVoltagesAPI();
                    },
                    "application/json", "Get available voltages"});

  routes.push_back({"/available-currents", WebModule::WM_GET,
                    [this](const String &requestBody,
                           const std::map<String, String> &params) -> String {
                      return this->handleAvailableCurrentsAPI();
                    },
                    "application/json", "Get available currents"});

  routes.push_back({"/pdo-profiles", WebModule::WM_GET,
                    [this](const String &requestBody,
                           const std::map<String, String> &params) -> String {
                      return this->handlePDOProfilesAPI();
                    },
                    "application/json", "Get PDO profiles"});

  routes.push_back({"/set-pd-config", WebModule::WM_POST,
                    [this](const String &requestBody,
                           const std::map<String, String> &params) -> String {
                      return this->handleSetPDConfigAPI(requestBody);
                    },
                    "application/json", "Set PD configuration"});

  return routes;
}

std::vector<WebRoute> USBPDController::getHttpsRoutes() {
  // For now, use same routes for HTTPS as HTTP
  return getHttpRoutes();
}

bool USBPDController::isPDBoardConnected() {
  Wire.beginTransmission(i2cAddress); // STUSB4500 I2C address
  byte error = Wire.endTransmission();
  return (error == 0);
}

bool USBPDController::readPDConfig() {
  if (!pdBoardConnected) {
    // Try to reconnect
    pdBoardConnected = pdController.begin();
    if (!pdBoardConnected) {
      return false;
    }
  }

  // Read current configuration
  pdController.read();
  int pdoNumber = pdController.getPdoNumber();
  currentVoltage = pdController.getVoltage(pdoNumber);
  currentCurrent = pdController.getCurrent(pdoNumber);

  return true;
}

bool USBPDController::setPDConfig(float voltage, float current) {
  if (!pdBoardConnected) {
    Serial.println("Cannot set PD config: board not connected");
    return false;
  }

  // Find appropriate PDO to modify
  int pdoToModify = 0;

  if (voltage == 5.0) {
    // Use PDO1 which is fixed at 5V
    pdoToModify = 1;
    pdController.setCurrent(1, current);
  } else if (voltage <= 12.0) {
    // Use PDO2 for voltages up to 12V
    pdoToModify = 2;
    pdController.setVoltage(2, voltage);
    pdController.setCurrent(2, current);
  } else {
    // Use PDO3 for higher voltages
    pdoToModify = 3;
    pdController.setVoltage(3, voltage);
    pdController.setCurrent(3, current);
  }

  // Set this PDO as highest priority
  pdController.setPdoNumber(pdoToModify);

  // Write the settings to the device
  pdController.write();

  // Soft reset to apply changes immediately
  pdController.softReset();

  // Read back the actual values from the board to confirm
  if (readPDConfig()) {
    Serial.println("PD configuration updated successfully");
    return true;
  } else {
    Serial.println("Failed to read back PD configuration");
    return false;
  }
}

String USBPDController::getAllPDOProfiles() {
  if (!pdBoardConnected) {
    return "{\"error\":\"PD board not connected\"}";
  }

  String json = "{\"pdos\":[";

  for (int i = 1; i <= 3; i++) {
    if (i > 1)
      json += ",";

    float voltage = pdController.getVoltage(i);
    float current = pdController.getCurrent(i);
    bool isActive = (pdController.getPdoNumber() == i);

    json += "{";
    json += "\"number\":" + String(i) + ",";
    json += "\"voltage\":" + String(voltage) + ",";
    json += "\"current\":" + String(current) + ",";
    json += "\"power\":" + String(voltage * current) + ",";
    json += "\"active\":" + String(isActive ? "true" : "false");
    if (i == 1) {
      json += ",\"fixed\":true"; // PDO1 is always fixed at 5V
    }
    json += "}";
  }

  json += "],\"activePDO\":" + String(pdController.getPdoNumber()) + "}";
  return json;
}

String USBPDController::getMainPageHtml() const {
  return String(FPSTR(USB_PD_HTML));
}

String USBPDController::handlePDStatusAPI() {
  // Check if PD board is connected
  bool connected = isPDBoardConnected();

  // Try to read fresh values if connected
  if (connected && !pdBoardConnected) {
    pdBoardConnected = pdController.begin();
    if (pdBoardConnected) {
      readPDConfig();
    }
  } else if (connected && pdBoardConnected) {
    // Refresh values if already connected
    readPDConfig();
  } else {
    pdBoardConnected = false;
  }

  String response;
  if (pdBoardConnected && currentVoltage > 0) {
    response = "{\"success\":true,\"connected\":true,\"voltage\":" +
               String(currentVoltage) +
               ",\"current\":" + String(currentCurrent) + "}";
  } else {
    response = "{\"success\":false,\"connected\":" +
               String(connected ? "true" : "false") + ",\"message\":\"" +
               (connected ? "Board initialized but values not read"
                          : "PD board not connected") +
               "\"}";
  }

  return response;
}

String USBPDController::handleAvailableVoltagesAPI() {
  return "[5.0, 9.0, 12.0, 15.0, 20.0]";
}

String USBPDController::handleAvailableCurrentsAPI() {
  return "[0.5, 1.0, 1.5, 2.0, 2.5, 3.0]";
}

String USBPDController::handlePDOProfilesAPI() {
  if (!isPDBoardConnected()) {
    return "{\"success\":false,\"message\":\"PD board not connected\"}";
  }

  return getAllPDOProfiles();
}

String USBPDController::handleSetPDConfigAPI(const String &jsonBody) {

  // Parse JSON
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, jsonBody);

  if (error) {
    return "{\"success\":false,\"message\":\"Invalid JSON\"}";
  }

  float voltage = doc["voltage"];
  float current = doc["current"];

  // Validate values
  if (voltage < 5.0 || voltage > 20.0 || current < 0.5 || current > 3.0) {
    return "{\"success\":false,\"message\":\"Invalid values\"}";
  }

  // Check if PD board is connected
  if (!isPDBoardConnected()) {
    return "{\"success\":false,\"message\":\"PD board not connected\"}";
  }

  // Apply configuration
  bool success = setPDConfig(voltage, current);

  if (success) {
    return "{\"success\":true,\"voltage\":" + String(currentVoltage) +
           ",\"current\":" + String(currentCurrent) + "}";
  } else {
    return "{\"success\":false,\"message\":\"Failed to set configuration\"}";
  }
}