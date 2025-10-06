# [Web Platform](https://www.github.com/andrewmherren/web_platform) - USB-C Power Delivery Controller

A comprehensive web module that provides USB-C Power Delivery control with real-time monitoring and configuration through a responsive web interface. Built specifically for the WebPlatform ecosystem on ESP32 devices.

## ✨ Features

- 🔧 **Flexible I2C Configuration**: Configurable SDA/SCL pins and I2C address through JSON config
- 📱 **Responsive Web Interface**: Complete USB-C PD control dashboard with real-time updates
- 🔌 **Multi-Board Support**: Extensible architecture supporting different PD controller boards
- ⚡ **Real-Time Monitoring**: Live voltage, current, and power readings with connection status
- 📊 **PDO Profile Management**: View and configure Power Delivery Object profiles
- 🛡️ **Security-First Design**: Authentication-protected routes with session and token support
- 🚀 **RESTful API**: Complete API for programmatic control with OpenAPI 3.0 documentation
- 📋 **Maker-Friendly**: Tagged endpoints for inclusion in Maker API specifications

## Quick Start

### Hardware Requirements

- **ESP32 development board** (tested on ESP32-S3)
- **[SparkFun USB-C Power Delivery Board (STUSB4500)](https://www.sparkfun.com/products/15801)**
- **USB-C PD compliant power supply**
- **Jumper wires** for I2C connection

### Default Wiring (ESP32-S3)

| ESP32-S3 | STUSB4500 | Notes |
|----------|-----------|--------|
| 3.3V     | 3.3V      | Power |
| GND      | GND       | Ground |
| GPIO 4   | SDA       | I2C Data (configurable) |
| GPIO 5   | SCL       | I2C Clock (configurable) |

### Build Configuration

Add the required dependencies to your `platformio.ini`:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
lib_deps = 
  https://github.com/andrewmherren/web_platform.git
  https://github.com/andrewmherren/usb_pd_controller.git
  https://github.com/sparkfun/SparkFun_STUSB4500_Arduino_Library.git

# Optional: Enable OpenAPI documentation for USB PD API endpoints
build_flags = -DWEB_PLATFORM_OPENAPI=1
```

### Installation

```cpp
#include <web_platform.h>
#include <usb_pd_controller.h>

void setup() {
  Serial.begin(115200);
  
  // Set up navigation with USB PD control link
  std::vector<NavigationItem> navItems = {
    NavigationItem("Home", "/"),
    NavigationItem("USB PD Control", "/usb_pd/"),
    Authenticated(NavigationItem("Account", "/account")),
    Authenticated(NavigationItem("Logout", "/logout"))
  };
  webPlatform.setNavigationMenu(navItems);
  
  // Configure USB PD Controller with I2C settings
  StaticJsonDocument<128> usbPdConfig;
  usbPdConfig["SDA"] = 4;           // GPIO 4 for I2C data
  usbPdConfig["SCL"] = 5;           // GPIO 5 for I2C clock  
  usbPdConfig["board"] = "sparkfun"; // Board type
  usbPdConfig["i2cAddress"] = 0x28;  // Optional: custom I2C address
  
  // Register the module with configuration
  webPlatform.registerModule("/usb_pd", &usbPDController, usbPdConfig.as<JsonVariant>());
  
  // Initialize WebPlatform
  webPlatform.begin("MyDevice");
}

void loop() {
  webPlatform.handle();
  delay(10); // Allow ESP32 system tasks to run
}
```

### Access the Interface

1. Connect to your device's web interface
2. Navigate to `/usb_pd/` or use the navigation menu
3. Log in with your WebPlatform credentials (required for control operations)
4. Configure USB-C PD voltage and current settings

## Configuration Options

### I2C Pin Configuration

The module supports flexible I2C pin assignment through JSON configuration:

```cpp
// Default configuration (ESP32-S3 compatible)
StaticJsonDocument<128> config;
config["SDA"] = 4;
config["SCL"] = 5;
webPlatform.registerModule("/usb_pd", &usbPDController, config.as<JsonVariant>());

// Alternative pins for different ESP32 boards
StaticJsonDocument<128> altConfig;
altConfig["SDA"] = 21;  // Different GPIO for SDA
altConfig["SCL"] = 22;  // Different GPIO for SCL
webPlatform.registerModule("/usb_pd", &usbPDController, altConfig.as<JsonVariant>());

// No configuration (uses defaults: SDA=4, SCL=5, board="sparkfun", i2cAddress=0x28)
webPlatform.registerModule("/usb_pd", &usbPDController);
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `SDA` | int | 4 | GPIO pin for I2C data line |
| `SCL` | int | 5 | GPIO pin for I2C clock line |
| `board` | string | "sparkfun" | Board type identifier |
| `i2cAddress` | int | 0x28 | I2C address of the PD controller |

### Future Board Support

The architecture supports multiple board types for future expansion:

```cpp
// Current supported board
config["board"] = "sparkfun";  // SparkFun STUSB4500

// Future board support (when available)
// config["board"] = "custom";
```

## Power Delivery Capabilities

### Supported Voltage Levels
- **5V** (USB standard, always available)
- **9V** (Quick Charge compatible)
- **12V** (Laptop charging)
- **15V** (High-power devices)
- **20V** (Maximum USB-C PD specification)

### Supported Current Levels
- **0.5A** (Low power devices)
- **1.0A** (Phone charging)
- **1.5A** (Tablet charging)
- **2.0A** (Fast charging)
- **2.5A** (High-power charging)
- **3.0A** (Maximum for most PD supplies)

### Power Delivery Object (PDO) Management

The STUSB4500 supports three configurable PDO profiles:

1. **PDO 1**: Fixed at 5V (USB standard, non-configurable)
2. **PDO 2**: Configurable voltage up to 12V
3. **PDO 3**: Configurable voltage up to 20V

The module automatically selects the appropriate PDO based on requested voltage and configures it with the desired current limit.

## API Endpoints

All API endpoints support both session-based (web interface) and token-based (API) authentication:

### Status and Monitoring

```bash
# Get current PD status and readings
GET /usb_pd/api/status
# Response: {"success": true, "connected": true, "voltage": 12.0, "current": 2.0}

# Get available voltage options
GET /usb_pd/api/voltages  
# Response: {"voltages": [5.0, 9.0, 12.0, 15.0, 20.0]}

# Get available current options
GET /usb_pd/api/currents
# Response: {"currents": [0.5, 1.0, 1.5, 2.0, 2.5, 3.0]}

# Get all PDO profiles
GET /usb_pd/api/profiles
# Response: {"pdos": [...], "activePDO": 2}
```

### Control Operations

```bash
# Set new voltage and current configuration
POST /usb_pd/api/configure
Content-Type: application/json
Authorization: Bearer YOUR_TOKEN

{
  "voltage": 12.0,
  "current": 2.0
}

# Response: {"success": true, "voltage": 12.0, "current": 2.0}
```

## OpenAPI 3.0 Integration

When OpenAPI documentation is enabled, the USB PD Controller provides comprehensive API documentation:

```cpp
// All endpoints are documented with maker-friendly tags
ApiRoute("/api/status", WebModule::WM_GET, statusHandler, {AuthType::LOCAL_ONLY},
         API_DOC("Get Power Delivery status",
                 "Returns current PD board connection status and voltage/current readings",
                 "getPDStatus", {"maker", "power"}));
```

### Using with Maker API

The USB PD Controller integrates seamlessly with the [Maker API module](https://github.com/andrewmherren/maker_api) for interactive testing:

```cpp
// Register both modules for complete API testing experience  
webPlatform.registerModule("/api-explorer", &makerAPI);
webPlatform.registerModule("/usb_pd", &usbPDController, usbPdConfig);
```

Access the API explorer at `/api-explorer/` to interactively test USB PD endpoints.

## Security and Authentication

### Route-Level Security

- **Monitoring Endpoints**: Local network access only for security
- **Control Endpoints**: Require authentication (session or API token)
- **Configuration Changes**: Protected with CSRF tokens when using web interface
- **API Access**: Secure token-based authentication for programmatic control

### Authentication Types

```cpp
// Monitoring - local network only
{AuthType::LOCAL_ONLY, AuthType::SESSION, AuthType::PAGE_TOKEN}

// Control operations - authentication required
{AuthType::SESSION, AuthType::PAGE_TOKEN}
```

## Error Handling

The module provides comprehensive error handling:

```json
// Board not connected
{"success": false, "error": "PD board not connected"}

// Invalid configuration
{"success": false, "error": "Invalid values - voltage must be 5.0-20.0V, current must be 0.5-3.0A"}

// Configuration failure
{"success": false, "error": "Failed to set configuration"}
```

## Troubleshooting

### Common Issues

**Board not detected**: 
- Check I2C wiring and connections
- Verify power supply to STUSB4500 board
- Confirm I2C pins in configuration match physical wiring

**Configuration not applying**:
- Ensure power supply supports requested voltage/current combination
- Check that PD-capable device is connected to output
- Some power supplies require negotiation time after configuration changes

**Web interface not loading**:
- Verify ESP32 is connected to WiFi network
- Check authentication - login may be required
- Ensure WebPlatform is properly initialized

**API calls failing**:
- Verify authentication token is valid
- Check Content-Type header for POST requests
- Ensure request body is valid JSON format

### Debug Output

Enable debug output to see detailed I2C and configuration information:

```cpp
// Enable debug output before webPlatform.begin()
Serial.setDebugOutput(true);
```

Debug output shows:
- I2C initialization with configured pins
- Board detection and connection status  
- Configuration parsing and validation
- PDO profile selection and updates

## Integration Examples

### Complete Project Example

```cpp
#include <web_platform.h>
#include <usb_pd_controller.h>

void setup() {
  Serial.begin(115200);
  
  // Device navigation
  std::vector<NavigationItem> navItems = {
    NavigationItem("Dashboard", "/"),
    NavigationItem("USB PD Control", "/usb_pd/"),
    NavigationItem("Settings", "/settings/"),
    Authenticated(NavigationItem("Account", "/account")),
    Authenticated(NavigationItem("Logout", "/logout")),
    Unauthenticated(NavigationItem("Login", "/login"))
  };
  webPlatform.setNavigationMenu(navItems);
  
  // Configure USB PD module for ESP32-S3
  StaticJsonDocument<128> pdConfig;
  pdConfig["SDA"] = 4;
  pdConfig["SCL"] = 5;
  pdConfig["board"] = "sparkfun";
  
  webPlatform.registerModule("/usb_pd", &usbPDController, pdConfig.as<JsonVariant>());
  webPlatform.begin("PowerHub", "1.0.0");
  
  // Add custom dashboard route
  if (webPlatform.isConnected()) {
    webPlatform.registerWebRoute("/", [](WebRequest& req, WebResponse& res) {
      String html = R"(
        <div class="container">
          <h1>Power Hub Dashboard</h1>
          <div class="status-grid">
            <div class="status-card">
              <h3>USB-C PD Control</h3>
              <p>Configure voltage and current output</p>
              <a href="/usb_pd/" class="btn btn-primary">Open Control Panel</a>
            </div>
          </div>
        </div>
      )";
      res.setContent(html, "text/html");
    }, {AuthType::LOCAL_ONLY});
  }
}

void loop() {
  webPlatform.handle();
  delay(10); // Allow ESP32 system tasks to run
}
```

### API Integration Example

```cpp
// Programmatically set PD configuration
void setPowerProfile(float voltage, float current) {
  if (webPlatform.isConnected()) {
    // Use internal API or external HTTP calls
    usbPDController.setPDConfig(voltage, current);
    
    Serial.printf("Set power profile: %.1fV @ %.1fA\n", voltage, current);
  }
}

// Check current power status
void checkPowerStatus() {
  if (usbPDController.isPDBoardConnected()) {
    usbPDController.readPDConfig();
    Serial.printf("Current output: %.1fV @ %.1fA\n", 
                  usbPDController.getCurrentVoltage(), 
                  usbPDController.getCurrentCurrent());
  } else {
    Serial.println("PD board not connected");
  }
}

void loop() {
  webPlatform.handle();
  
  // Example of periodic status check
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) { // Check every 30 seconds
    checkPowerStatus();
    lastCheck = millis();
  }
  
  delay(10); // Allow ESP32 system tasks to run
}
```

## WebPlatform Module Ecosystem

The USB PD Controller is part of the growing WebPlatform module ecosystem:

- **[Web Platform](https://github.com/andrewmherren/web_platform)**: Core framework with authentication, routing, and web services
- **[Maker API](https://github.com/andrewmherren/maker_api)**: Interactive API documentation and testing interface
- **USB PD Controller**: This module - USB-C Power Delivery control and monitoring

## Memory Efficiency

The module is designed for optimal ESP32 memory usage:

- **PROGMEM Assets**: Web interface assets stored in flash memory
- **Efficient JSON**: Minimal JSON document sizes for API responses
- **Connection Caching**: I2C connection status cached to reduce bus traffic
- **Optional Features**: OpenAPI documentation can be disabled to save memory

## Hardware Compatibility

**Tested Platforms:**
- ESP32-S3 (primary development platform)
- ESP32 (basic compatibility)

**Supported PD Controllers:**
- SparkFun STUSB4500 breakout board (current)
- Future boards through extensible architecture

## Contributing

Contributions are welcome! Areas of interest:

- **Additional Board Support**: Support for other USB-C PD controller boards
- **Enhanced Monitoring**: Additional power metrics and logging
- **Safety Features**: Overcurrent/overvoltage protection and alerts
- **UI Improvements**: Enhanced web interface with better visualizations

## License

This module is part of the WebPlatform ecosystem and follows the same MIT licensing terms.

**Made with ⚡ for the ESP32 maker community**