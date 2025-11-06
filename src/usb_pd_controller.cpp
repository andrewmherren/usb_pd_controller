#include "usb_pd_controller.h"
#include "../assets/usb_pd_html.h"
#include "../assets/usb_pd_js.h"

#if defined(ARDUINO) || defined(ESP_PLATFORM)
#include "chip/stusb4500_chip.h"

// Create global instance of USBPDController with real STUSB4500 adapter
// Only available on Arduino/ESP32 platforms; native tests create their own
// instances
static STUSB4500Chip g_stusb4500Adapter;
USBPDController usbPDController(g_stusb4500Adapter);
#endif

// USBPDController implementation
USBPDController::USBPDController(IUsbPdChip &chip)
    : pdController(chip), core(pdController) {}

void USBPDController::begin() {
  // Use debug macro to avoid direct Serial dependency in native tests
  DEBUG_PRINTLN("USB PD Controller module initialized");
  initializeHardware();
}

void USBPDController::begin(const JsonVariant &config) {
  parseConfig(config);
  begin(); // Call the parameterless version
}

void USBPDController::initializeHardware() {
  DEBUG_PRINTLN("Initializing USB PD Controller hardware...");
  DEBUG_PRINT("Using I2C address: 0x");
  DEBUG_PRINTLN(String(i2cAddress, HEX));
  DEBUG_PRINTF("I2C pins: SDA=%d, SCL=%d\n", sdaPin, sclPin);
  DEBUG_PRINTF("Board type: %s\n", boardType.c_str());

// Initialize I2C with configured pins
#if defined(ARDUINO) || defined(ESP_PLATFORM)
  Wire.begin(sdaPin, sclPin);
#else
  Wire.begin(); // ArduinoFake doesn't support 2-param version
#endif

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
  // Check if it's time to check PD board status (every 30 seconds to reduce I2C
  // spam)
  if (millis() - lastCheckTime <= 30000) {
    return;
  }

  lastCheckTime = millis();
  bool connected = isPDBoardConnected();

  // No change in connection status
  if (connected == pdBoardConnected) {
    return;
  }

  // Handle disconnection
  if (!connected) {
    DEBUG_PRINTLN("PD board disconnected");
    pdBoardConnected = false;
    return;
  }

  // Handle connection
  DEBUG_PRINTLN("PD board connected");
  pdBoardConnected = pdController.begin();
  if (pdBoardConnected) {
    readPDConfig();
  }
}

std::vector<RouteVariant> USBPDController::getHttpRoutes() {
  return {// Main page route - local access only for security
          WebRoute("/", WebModule::WM_GET,
                   [this](WebRequestCore &req, WebResponseCore &res) {
                     mainPageHandler(req, res);
                   },
                   {AuthType::NONE}),

          // JavaScript assets - local access only
          WebRoute("/assets/usb-pd-controller.js", WebModule::WM_GET,
                   [](WebRequestCore &req, WebResponseCore &res) {
                     res.setProgmemContent(USB_PD_JS, "application/javascript");
                     res.setHeader("Cache-Control", "public, max-age=3600");
                   },
                   {AuthType::NONE}),

          // API endpoints - authentication required for control, local access
          // for monitoring
          ApiRoute(
              "/api/status", WebModule::WM_GET,
              [this](WebRequestCore &req, WebResponseCore &res) {
                pdStatusHandler(req, res);
              },
              {AuthType::SESSION, AuthType::PAGE_TOKEN, AuthType::TOKEN},
              API_DOC("Get Power Delivery status",
                      "Returns current PD board connection status and "
                      "voltage/current readings",
                      "getPDStatus", {"power delivery"})),

          ApiRoute(
              "/api/voltages", WebModule::WM_GET,
              [this](WebRequestCore &req, WebResponseCore &res) {
                availableVoltagesHandler(req, res);
              },
              {AuthType::SESSION, AuthType::PAGE_TOKEN, AuthType::TOKEN},
              API_DOC("Get available voltages",
                      "Returns list of supported voltage levels",
                      "getAvailableVoltages", {"power delivery"})),

          ApiRoute(
              "/api/currents", WebModule::WM_GET,
              [this](WebRequestCore &req, WebResponseCore &res) {
                availableCurrentsHandler(req, res);
              },
              {AuthType::SESSION, AuthType::PAGE_TOKEN, AuthType::TOKEN},
              API_DOC("Get available currents",
                      "Returns list of supported current levels",
                      "getAvailableCurrents", {"power delivery"})),

          ApiRoute(
              "/api/profiles", WebModule::WM_GET,
              [this](WebRequestCore &req, WebResponseCore &res) {
                pdoProfilesHandler(req, res);
              },
              {AuthType::SESSION, AuthType::PAGE_TOKEN, AuthType::TOKEN},
              API_DOC("Get PDO profiles",
                      "Returns Power Delivery Object profiles with voltage, "
                      "current and power specifications",
                      "getPDOProfiles", {"power delivery"})),

          ApiRoute(
              "/api/configure", WebModule::WM_POST,
              [this](WebRequestCore &req, WebResponseCore &res) {
                setPDConfigHandler(req, res);
              },
              {AuthType::SESSION, AuthType::PAGE_TOKEN,
               AuthType::TOKEN}, // Require authentication for control
              API_DOC("Set Power Delivery configuration",
                      "Updates the USB-C PD voltage and current settings",
                      "setPDConfig", {"power delivery"})
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
  // Rely solely on the chip's probe, which performs the necessary I2C check
  return pdController.probe(i2cAddress);
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
  float v, c;
  int p;
  if (!core.readConfig(v, c, p)) {
    return false;
  }
  currentVoltage = v;
  currentCurrent = c;

  return true;
}

bool USBPDController::setPDConfig(float voltage, float current) {
  if (!pdBoardConnected) {
    DEBUG_PRINTLN("Cannot set PD config: board not connected");
    return false;
  }

  bool ok = core.setConfig(voltage, current);
  // Add a small delay to allow negotiation
  delay(100);
  if (ok) {
    currentVoltage = core.currentVoltage();
    currentCurrent = core.currentCurrent();
    DEBUG_PRINTLN("PD configuration updated successfully");
  } else {
    DEBUG_PRINTLN("Failed to read back PD configuration");
  }
  return ok;
}

String USBPDController::getAllPDOProfiles() {
  if (!pdBoardConnected) {
    return R"({\"error\":\"PD board not connected\"})";
  }

  return core.buildPdoProfilesJson();
}

// Route handler implementations
void USBPDController::mainPageHandler(WebRequestCore &req,
                                      WebResponseCore &res) {
  // Use PROGMEM content for memory efficiency
  res.setProgmemContent(USB_PD_HTML, "text/html");
}

void USBPDController::pdStatusHandler(WebRequestCore &req,
                                      WebResponseCore &res) {
  respondJson(res, [&](JsonObject &json) {
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

void USBPDController::availableVoltagesHandler(WebRequestCore &req,
                                               WebResponseCore &res) {
  respondJson(res, [&](JsonObject &json) {
    JsonArray voltages = json["voltages"].to<JsonArray>();
    voltages.add(5.0);
    voltages.add(9.0);
    voltages.add(12.0);
    voltages.add(15.0);
    voltages.add(20.0);
  });
}

void USBPDController::availableCurrentsHandler(WebRequestCore &req,
                                               WebResponseCore &res) {
  respondJson(res, [&](JsonObject &json) {
    JsonArray currents = json["currents"].to<JsonArray>();
    currents.add(0.5);
    currents.add(1.0);
    currents.add(1.33);
    currents.add(1.5);
    currents.add(1.67);
    currents.add(2.0);
    currents.add(2.25);
    currents.add(2.5);
    currents.add(3.0);
  });
}

void USBPDController::pdoProfilesHandler(WebRequestCore &req,
                                         WebResponseCore &res) {
  if (!isPDBoardConnected()) {
    res.setStatus(503); // Service unavailable
    respondJson(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "PD board not connected";
      json["pdos"].to<JsonArray>(); // Empty array
    });
    return;
  }

  // Return all PDO profiles with active PDO indicator
  respondJson(res, [&](JsonObject &json) {
    JsonArray pdos = json.createNestedArray("pdos");

    // Build PDO profiles directly from chip data
    for (int i = 1; i <= 3; ++i) {
      JsonObject pdo = pdos.createNestedObject();
      float v = pdController.getVoltage(i);
      float c = pdController.getCurrent(i);
      bool active = (pdController.getPdoNumber() == i);

      pdo["number"] = i;
      pdo["voltage"] = v;
      pdo["current"] = c;
      pdo["power"] = v * c;
      pdo["active"] = active;

      if (i == 1) {
        pdo["fixed"] = true;
      }
    }

    json["activePDO"] = pdController.getPdoNumber();
  });
}
void USBPDController::setPDConfigHandler(WebRequestCore &req,
                                         WebResponseCore &res) {
  // Parse JSON from request body
  DynamicJsonDocument doc(256);
  DeserializationError error = deserializeJson(doc, req.getBody());

  if (error) {
    res.setStatus(400);
    respondJson(res, [&](JsonObject &json) {
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
    respondJson(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "Invalid values - voltage must be 5.0-20.0V, current "
                      "must be 0.5-3.0A";
    });
    return;
  }

  // Check if PD board is connected
  if (!isPDBoardConnected()) {
    res.setStatus(503);
    respondJson(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "PD board not connected";
    });
    return;
  }

  // Apply configuration
  bool success = setPDConfig(voltage, current);

  if (success) {
    respondJson(res, [&](JsonObject &json) {
      json["success"] = true;
      json["voltage"] = currentVoltage;
      json["current"] = currentCurrent;
    });
  } else {
    res.setStatus(500);
    respondJson(res, [&](JsonObject &json) {
      json["success"] = false;
      json["error"] = "Failed to set configuration";
    });
  }
}

void USBPDController::parseConfig(const JsonVariant &config) {
  if (config.isNull()) {
    DEBUG_PRINTLN("USB PD Controller: Using default configuration");
    return;
  }

  // Parse I2C pin configuration
  if (config.containsKey("SDA")) {
    sdaPin = config["SDA"].as<int>();
    DEBUG_PRINTF("USB PD Controller: Configured SDA pin: %d\n", sdaPin);
  }

  if (config.containsKey("SCL")) {
    sclPin = config["SCL"].as<int>();
    DEBUG_PRINTF("USB PD Controller: Configured SCL pin: %d\n", sclPin);
  }

  // Parse board type
  if (config.containsKey("board")) {
    boardType = config["board"].as<const char *>();
    DEBUG_PRINTF("USB PD Controller: Configured board type: %s\n",
                 boardType.c_str());

    // Validate board type
    if (boardType != "sparkfun") {
      DEBUG_PRINTF("USB PD Controller: WARNING - Unsupported board type "
                   "'%s', using 'sparkfun'\n",
                   boardType.c_str());
      boardType = "sparkfun";
    }
  }

  // Parse I2C address if provided
  if (config.containsKey("i2cAddress")) {
    i2cAddress = config["i2cAddress"].as<uint8_t>();
    DEBUG_PRINTF("USB PD Controller: Configured I2C address: 0x%02X\n",
                 i2cAddress);
  }
}