#include "usb_pd_controller.h"
#include "usb_pd_controller_web.h"
#include <wifi_ap.h>

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

void USBPDController::setupWebHandlers(WebServerClass &server) {
  Serial.println("Setting up USB PD web handlers...");

  // Main device control interface
  server.on("/", [this, &server]() { this->handleRoot(server); });

  // API endpoints for USB-PD control
  server.on("/pd-status", HTTP_GET,
            [this, &server]() { this->handlePDStatus(server); });

  server.on("/available-voltages", HTTP_GET,
            [this, &server]() { this->handleAvailableVoltages(server); });

  server.on("/available-currents", HTTP_GET,
            [this, &server]() { this->handleAvailableCurrents(server); });

  // API endpoint for PDO profiles
  server.on("/pdo-profiles", HTTP_GET,
            [this, &server]() { this->handlePDOProfiles(server); });

  server.on("/set-pd-config", HTTP_POST,
            [this, &server]() { this->handleSetPDConfig(server); });

  // Access to WiFi setup from normal operation
  server.on("/setup", [this, &server]() { this->handleSetup(server); });
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

// Web handler implementations

void USBPDController::handleRoot(WebServerClass &server) {
  // Add headers to prevent browser caching and HSTS issues when switching
  // between HTTP/HTTPS
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");

  server.send_P(200, "text/html", USB_PD_CONTROLLER_HTML);
}

void USBPDController::renderHomePage(String &output) {
  // Copy the HTML content from PROGMEM to a String
  output = FPSTR(USB_PD_CONTROLLER_HTML);
}

void USBPDController::handlePDStatus(WebServerClass &server) {
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

  server.send(200, "application/json", response);
}

void USBPDController::handleAvailableVoltages(WebServerClass &server) {
  server.send(200, "application/json", "[5.0, 9.0, 12.0, 15.0, 20.0]");
}

void USBPDController::handleAvailableCurrents(WebServerClass &server) {
  server.send(200, "application/json", "[0.5, 1.0, 1.5, 2.0, 2.5, 3.0]");
}

void USBPDController::handlePDOProfiles(WebServerClass &server) {
  if (!isPDBoardConnected()) {
    server.send(503, "application/json",
                "{\"success\":false,\"message\":\"PD board not connected\"}");
    return;
  }

  String response = getAllPDOProfiles();
  server.send(200, "application/json", response);
}

void USBPDController::handleSetPDConfig(WebServerClass &server) {
  String postBody = server.arg("plain");
  Serial.println("Received config: " + postBody);

  // Parse JSON
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, postBody);

  if (error) {
    server.send(400, "application/json",
                "{\"success\":false,\"message\":\"Invalid JSON\"}");
    return;
  }

  float voltage = doc["voltage"];
  float current = doc["current"];

  // Validate values
  if (voltage < 5.0 || voltage > 20.0 || current < 0.5 || current > 3.0) {
    server.send(400, "application/json",
                "{\"success\":false,\"message\":\"Invalid values\"}");
    return;
  }

  // Check if PD board is connected
  if (!isPDBoardConnected()) {
    server.send(503, "application/json",
                "{\"success\":false,\"message\":\"PD board not connected\"}");
    return;
  }

  // Apply configuration
  bool success = setPDConfig(voltage, current);

  if (success) {
    String response =
        "{\"success\":true,\"voltage\":" + String(currentVoltage) +
        ",\"current\":" + String(currentCurrent) + "}";
    server.send(200, "application/json", response);
  } else {
    server.send(
        500, "application/json",
        "{\"success\":false,\"message\":\"Failed to set configuration\"}");
  }
}
void USBPDController::handleSetup(WebServerClass &server) {
  String response =
      "Starting WiFi configuration portal. Please connect to the WiFi "
      "network named \"" +
      String(wifiManager.getAPName()) +
      "\" and navigate to http://192.168.4.1/wifi";

  // Send response before starting config portal
  server.send(200, "text/plain", response);

  // Delay to ensure the response is sent
  delay(500);

  // Stop the server first to ensure clean transition
  server.stop();

  // Start the config portal
  wifiManager.startConfigPortal();
}