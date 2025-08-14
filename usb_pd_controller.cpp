#include "usb_pd_controller.h"
#include "usb_pd_controller_web.h"

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

      Serial.println("Failed to initialize STUSB4500");
    }
  } else {
    Serial.println("STUSB4500 not detected on I2C bus");
  }
}
void USBPDController::handle() {
  // This method is kept for backward compatibility
  // The actual work is now done in handleLoop()
  handleLoop();
}

void USBPDController::handleLoop() {
  // Periodically check if PD board connection status has changed
  if (millis() - lastCheckTime > 5000) { // Check every 5 seconds
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
void USBPDController::registerRoutes(WebRouter &router, const char *basePath) {
  Serial.print("Registering USB PD Controller routes with base path: ");
  Serial.println(basePath);

  // Register main page route
  String mainPath = String(basePath);
  if (!mainPath.endsWith("/"))
    mainPath += "/";
  
  router.addRoute(mainPath.c_str(), HTTP_GET, [this](WebServerClass &server) {
    Serial.println("USB PD Controller main page requested");
    Serial.println("Main page HTML length: " + String(strlen_P(USB_PD_CONTROLLER_HTML)));
    
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send_P(200, "text/html", USB_PD_CONTROLLER_HTML);
    
    Serial.println("USB PD Controller main page sent");
  });

  // Register API endpoints with base path
  String pdStatusPath = String(basePath) + "/pd-status";
  router.addRoute(pdStatusPath.c_str(), HTTP_GET,
                  [this](WebServerClass &server) {
                    String response = this->handlePDStatusAPI();
                    server.send(200, "application/json", response);
                  });

  String voltagesPath = String(basePath) + "/available-voltages";
  router.addRoute(voltagesPath.c_str(), HTTP_GET,
                  [this](WebServerClass &server) {
                    String response = this->handleAvailableVoltagesAPI();
                    server.send(200, "application/json", response);
                  });

  String currentsPath = String(basePath) + "/available-currents";
  router.addRoute(currentsPath.c_str(), HTTP_GET,
                  [this](WebServerClass &server) {
                    String response = this->handleAvailableCurrentsAPI();
                    server.send(200, "application/json", response);
                  });

  String profilesPath = String(basePath) + "/pdo-profiles";
  router.addRoute(profilesPath.c_str(), HTTP_GET,
                  [this](WebServerClass &server) {
                    String response = this->handlePDOProfilesAPI();
                    server.send(200, "application/json", response);
                  });

  String configPath = String(basePath) + "/set-pd-config";
  router.addRoute(configPath.c_str(), HTTP_POST,
                  [this](WebServerClass &server) {
                    String jsonBody = server.arg("plain");
                    String response = this->handleSetPDConfigAPI(jsonBody);

                    // Determine response code based on response content
                    int responseCode = 200;
                    if (response.indexOf("\"success\":false") >= 0) {
                      if (response.indexOf("Invalid JSON") >= 0 ||
                          response.indexOf("Invalid values") >= 0) {
                        responseCode = 400;
                      } else if (response.indexOf("not connected") >= 0) {
                        responseCode = 503;
                      } else {
                        responseCode = 500;
                      }
                    }

                    server.send(responseCode, "application/json", response);
                  });
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

  Serial.print("Read from PD board - PDO: ");
  Serial.print(pdoNumber);
  Serial.print(", Voltage: ");
  Serial.print(currentVoltage);
  Serial.print("V, Current: ");
  Serial.print(currentCurrent);
  Serial.println("A");

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
  return String(FPSTR(USB_PD_CONTROLLER_HTML));
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
  Serial.println("Received config: " + jsonBody);

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