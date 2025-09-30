#include "usb_pd_controller.h"
#include "assets/usb_pd_html.h"
#include "assets/usb_pd_js.h"
#include <web_platform.h>

// Create global instance of USBPDController
USBPDController usbPDController;

// USBPDController implementation
USBPDController::USBPDController()
    : currentVoltage(0.0), currentCurrent(0.0), pdBoardConnected(false),
      lastCheckTime(0), i2cAddress(0x28) {}

void USBPDController::begin() {
  Serial.println("USB PD Controller module initialized");
  initializeHardware(); // Use default I2C address
}

void USBPDController::initializeHardware(uint8_t i2cAddress) {
  // Set the I2C address
  this->i2cAddress = i2cAddress;

  DEBUG_PRINTLN("Initializing USB PD Controller hardware...");
  DEBUG_PRINT("Using I2C address: 0x");
  DEBUG_PRINTLN(String(i2cAddress, HEX));

  // Check if PD board is connected
  pdBoardConnected = isPDBoardConnected();

  // Initialize USB-PD controller if board is connected
  if (pdBoardConnected) {
    pdBoardConnected = pdController.begin();
    if (pdBoardConnected) {
      DEBUG_PRINTLN("STUSB4500 initialized successfully");
      readPDConfig();
    } else {
      DEBUG_PRINTLN("Failed to initialize STUSB4500");
    }
  } else {
    DEBUG_PRINTLN("STUSB4500 not detected on I2C bus");
  }

  DEBUG_PRINTLN("USB PD Controller hardware initialized");
}

void USBPDController::handle() {
  // Periodically check if PD board connection status has changed
  if (millis() - lastCheckTime >
      30000) { // Check every 30 seconds to reduce I2C spam
    lastCheckTime = millis();

    bool connected = isPDBoardConnected();
    if (connected != pdBoardConnected) {
      if (connected) {
        DEBUG_PRINTLN("PD board connected");
        pdBoardConnected = pdController.begin();
        if (pdBoardConnected) {
          readPDConfig();
        }
      } else {
        DEBUG_PRINTLN("PD board disconnected");
        pdBoardConnected = false;
      }
    }
  }
}

std::vector<RouteVariant> USBPDController::getHttpRoutes() {
  return {// Main page route - local access only for security
          WebRoute("/", WebModule::WM_GET,
                   [this](WebRequest &req, WebResponse &res) {
                     mainPageHandler(req, res);
                   },
                   {AuthType::LOCAL_ONLY}),

          // JavaScript assets - local access only
          WebRoute("/assets/usb-pd-controller.js", WebModule::WM_GET,
                   [](WebRequest &req, WebResponse &res) {
                     res.setProgmemContent(USB_PD_JS, "application/javascript");
                     res.setHeader("Cache-Control", "public, max-age=3600");
                   },
                   {AuthType::LOCAL_ONLY}),

          // API endpoints - authentication required for control, local access
          // for monitoring
          ApiRoute(
              "/api/status", WebModule::WM_GET,
              [this](WebRequest &req, WebResponse &res) {
                pdStatusHandler(req, res);
              },
              {AuthType::LOCAL_ONLY, AuthType::SESSION, AuthType::PAGE_TOKEN},
              API_DOC("Get Power Delivery status",
                      "Returns current PD board connection status and "
                      "voltage/current readings",
                      "getPDStatus", {"maker", "power"})),

          ApiRoute(
              "/api/voltages", WebModule::WM_GET,
              [this](WebRequest &req, WebResponse &res) {
                availableVoltagesHandler(req, res);
              },
              {AuthType::LOCAL_ONLY, AuthType::SESSION, AuthType::PAGE_TOKEN},
              API_DOC("Get available voltages",
                      "Returns list of supported voltage levels",
                      "getAvailableVoltages", {"maker", "power"})),

          ApiRoute(
              "/api/currents", WebModule::WM_GET,
              [this](WebRequest &req, WebResponse &res) {
                availableCurrentsHandler(req, res);
              },
              {AuthType::LOCAL_ONLY, AuthType::SESSION, AuthType::PAGE_TOKEN},
              API_DOC("Get available currents",
                      "Returns list of supported current levels",
                      "getAvailableCurrents", {"maker", "power"})),

          ApiRoute(
              "/api/profiles", WebModule::WM_GET,
              [this](WebRequest &req, WebResponse &res) {
                pdoProfilesHandler(req, res);
              },
              {AuthType::LOCAL_ONLY, AuthType::SESSION, AuthType::PAGE_TOKEN},
              API_DOC("Get PDO profiles",
                      "Returns Power Delivery Object profiles with voltage, "
                      "current and power specifications",
                      "getPDOProfiles", {"maker", "power"})),

          ApiRoute(
              "/api/configure", WebModule::WM_POST,
              [this](WebRequest &req, WebResponse &res) {
                setPDConfigHandler(req, res);
              },
              {AuthType::SESSION,
               AuthType::PAGE_TOKEN}, // Require authentication for control
              API_DOC("Set Power Delivery configuration",
                      "Updates the USB-C PD voltage and current settings",
                      "setPDConfig", {"maker", "power"})
                  .withRequestBody(R"({
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "type": "object",
                "description": "PD configuration request",
                "required": ["voltage", "current"],
                "properties": {
                  "voltage": {
                    "type": "number",
                    "minimum": 5.0,
                    "maximum": 20.0,
                    "description": "Target voltage in volts"
                  },
                  "current": {
                    "type": "number",
                    "minimum": 0.5,
                    "maximum": 3.0,
                    "description": "Target current in amperes"
                  }
                }
              }
            }
          }
        })")
                  .withRequestExample(R"({
          "voltage": 12.0,
          "current": 2.0
        })")
                  .withResponseExample(R"({
          "success": true,
          "voltage": 12.0,
          "current": 2.0
        })"))};
}

std::vector<RouteVariant> USBPDController::getHttpsRoutes() {
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
    DEBUG_PRINTLN("Cannot set PD config: board not connected");
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
    DEBUG_PRINTLN("PD configuration updated successfully");
    return true;
  } else {
    DEBUG_PRINTLN("Failed to read back PD configuration");
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

// Route handler implementations
void USBPDController::mainPageHandler(WebRequest &req, WebResponse &res) {
  // Use PROGMEM content for memory efficiency
  res.setProgmemContent(USB_PD_HTML, "text/html");
}

void USBPDController::pdStatusHandler(WebRequest &req, WebResponse &res) {
  JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
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

    json["success"] = pdBoardConnected && currentVoltage > 0;
    json["connected"] = connected;
    if (pdBoardConnected && currentVoltage > 0) {
      json["voltage"] = currentVoltage;
      json["current"] = currentCurrent;
    } else {
      json["message"] = connected ? "Board initialized but values not read"
                                  : "PD board not connected";
    }
  });
}

void USBPDController::availableVoltagesHandler(WebRequest &req,
                                               WebResponse &res) {
  JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
    JsonArray voltages = json.createNestedArray("voltages");
    voltages.add(5.0);
    voltages.add(9.0);
    voltages.add(12.0);
    voltages.add(15.0);
    voltages.add(20.0);
  });
}

void USBPDController::availableCurrentsHandler(WebRequest &req,
                                               WebResponse &res) {
  JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
    JsonArray currents = json.createNestedArray("currents");
    currents.add(0.5);
    currents.add(1.0);
    currents.add(1.5);
    currents.add(2.0);
    currents.add(2.5);
    currents.add(3.0);
  });
}

void USBPDController::pdoProfilesHandler(WebRequest &req, WebResponse &res) {
  if (!isPDBoardConnected()) {
    res.setStatus(503); // Service unavailable
    JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "PD board not connected";
      json.createNestedArray("pdos"); // Empty array
    });
    return;
  }

  JsonResponseBuilder::createResponse<512>(res, [&](JsonObject &json) {
    JsonArray pdos = json.createNestedArray("pdos");

    for (int i = 1; i <= 3; i++) {
      JsonObject pdo = pdos.createNestedObject();
      float voltage = pdController.getVoltage(i);
      float current = pdController.getCurrent(i);
      bool isActive = (pdController.getPdoNumber() == i);

      pdo["number"] = i;
      pdo["voltage"] = voltage;
      pdo["current"] = current;
      pdo["power"] = voltage * current;
      pdo["active"] = isActive;
      if (i == 1) {
        pdo["fixed"] = true; // PDO1 is always fixed at 5V
      }
    }

    json["activePDO"] = pdController.getPdoNumber();
  });
}

void USBPDController::setPDConfigHandler(WebRequest &req, WebResponse &res) {
  // Parse JSON from request body
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, req.getBody());

  if (error) {
    res.setStatus(400);
    JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "Invalid JSON";
    });
    return;
  }

  float voltage = doc["voltage"];
  float current = doc["current"];

  // Validate values
  if (voltage < 5.0 || voltage > 20.0 || current < 0.5 || current > 3.0) {
    res.setStatus(400);
    JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "Invalid values - voltage must be 5.0-20.0V, current "
                      "must be 0.5-3.0A";
    });
    return;
  }

  // Check if PD board is connected
  if (!isPDBoardConnected()) {
    res.setStatus(503);
    JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "PD board not connected";
    });
    return;
  }

  // Apply configuration
  bool success = setPDConfig(voltage, current);

  if (success) {
    JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
      json["success"] = true;
      json["voltage"] = currentVoltage;
      json["current"] = currentCurrent;
    });
  } else {
    res.setStatus(500);
    JsonResponseBuilder::createResponse(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "Failed to set configuration";
    });
  }
}