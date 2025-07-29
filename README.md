# USB-C Power Delivery Controller

A web-based interface for controlling USB-C Power Delivery (PD) settings through an ESP8266 or ESP32-S3 microcontroller and the SparkFun STUSB4500 breakout board. This library allows you to configure voltage and current output for connected USB-C devices over a user-friendly web interface.

## Features

- **Web Interface**: Intuitive UI for setting and monitoring USB-C PD profiles
- **Dynamic PDO Configuration**: Configure Power Data Objects (PDOs) for different voltage/current requirements
- **Profile Management**: View and select from available power profiles
- **Real-time Monitoring**: Check current voltage, current, and power output
- **REST API**: Control USB-C PD settings programmatically via API endpoints
- **I2C Communication**: Works with the SparkFun STUSB4500 breakout board over I2C

## Hardware Requirements

- ESP8266-based board (NodeMCU, Wemos D1, etc.) or ESP32-S3 board
- [SparkFun USB-C Power Delivery Board (STUSB4500)](https://www.sparkfun.com/products/15801)
- USB-C PD compliant power supply
- USB-C device to power

## Wiring

### ESP8266 Wiring

Connect the SparkFun STUSB4500 board to your ESP8266:

| ESP8266     | STUSB4500 |
|-------------|-----------|
| 3.3V        | 3.3V      |
| GND         | GND       |
| D1 (GPIO 5) | SCL       |
| D2 (GPIO 4) | SDA       |

### ESP32-S3 Wiring

Connect the SparkFun STUSB4500 board to your ESP32-S3:

| ESP32-S3 | STUSB4500 |
|----------|-----------|
| 3.3V     | 3.3V      |
| GND      | GND       |
| GPIO 5   | SCL       |
| GPIO 4   | SDA       |

**Note:** The default I2C pins on ESP32-S3 are GPIO 9 (SCL) and GPIO 8 (SDA), but you can use any pins by calling `Wire.begin(SDA_PIN, SCL_PIN)` before initializing the USB PD controller.

## Installation

1. Create a new PlatformIO project for your ESP8266 device.
2. Add this library to your project:
   - Clone this repository into your project's `lib` folder: 
     ```
     git clone https://github.com/yourusername/usb_pd_controller.git lib/usb_pd_controller
     ```
   - Or use git submodules if your project is a git repository:
     ```
     git submodule add https://github.com/yourusername/usb_pd_controller.git lib/usb_pd_controller
     ```

3. Make sure to add the SparkFun STUSB4500 Arduino Library to your `platformio.ini`:
   ```
   lib_deps = 
     https://github.com/sparkfun/SparkFun_STUSB4500_Arduino_Library.git
     bblanchon/ArduinoJson@^6.20.0
   ```

## Usage

### Basic Usage

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <usb_pd_controller.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize USB PD controller with default I2C address (0x28)
  usbPDController.begin();
  
  // If you're using WiFi, set up a web server
  #if defined(ESP8266)
    ESP8266WebServer server(80);
  #else
    WebServer server(80);  // For ESP32-S3
  #endif
  
  // Add USB PD controller web handlers to your server
  usbPDController.setupWebHandlers(server);
  
  // Start the web server
  server.begin();
  Serial.println("Web server started. You can now access the USB PD control interface.");
}

void loop() {
  // Handle USB PD controller operations
  usbPDController.handle();
}
```

### Integration with Web Server

This library can be integrated with any ESP8266WebServer or WebServer (ESP32) instance:

#### ESP8266 Example

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <usb_pd_controller.h>

// WiFi credentials
const char* ssid = "YourWiFiNetwork";
const char* password = "YourWiFiPassword";

// Create web server
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin();
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());
  
  // Initialize USB PD controller
  usbPDController.begin();
  
  // Add USB PD controller web handlers to the server
  usbPDController.setupWebHandlers(server);
  
  // Start the web server
  server.begin();
  Serial.println("Web server started with USB PD control interface");
}

void loop() {
  // Handle HTTP requests
  server.handleClient();
  
  // Handle USB PD controller operations
  usbPDController.handle();
}
```

#### ESP32-S3 Example

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <usb_pd_controller.h>
#include <wifi_ap.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C - optionally specify pins for ESP32-S3
  // Default ESP32-S3 I2C pins are GPIO9 (SCL) and GPIO8 (SDA)
  Wire.begin(8, 9);  
  
  // Initialize the WiFi manager and USB PD controller after WiFi connects
  wifiManager.onSetupComplete([]() {
    // Initialize USB PD controller
    usbPDController.begin();
    
    // Get reference to the web server
    WebServer &server = wifiManager.getWebServer();
    
    // Setup USB PD controller web handlers
    usbPDController.setupWebHandlers(server);
  });
  
  // Start WiFi setup with automatic portal if needed
  wifiManager.begin("USB-PD-Setup", true);
}

void loop() {
  // Handle WiFi operations
  wifiManager.handle();
  
  // Handle USB PD controller operations
  usbPDController.handle();
}
```

## Web Interface

The library provides a responsive web interface for controlling USB-C PD settings:

- **Main Interface**: `http://[device-ip]/` - USB-C PD control interface
- **API Endpoints**:
  - `GET /pd-status` - Returns current PD status
  - `GET /available-voltages` - Returns available voltage options
  - `GET /available-currents` - Returns available current options
  - `GET /pdo-profiles` - Returns all PDO profiles
  - `POST /set-pd-config` - Sets new voltage and current configuration

## Supported Voltage and Current Ranges

- **Voltages**: 5V, 9V, 12V, 15V, 20V (standard USB-C PD voltages)
- **Currents**: 0.5A to 3.0A (depending on power supply capabilities)

## How It Works

The STUSB4500 chip can negotiate with USB-C PD power supplies to request specific voltage and current levels. This library provides an interface to configure:

1. **PDO Profiles**: The STUSB4500 supports three Power Data Object (PDO) profiles:
   - PDO1: Fixed at 5V (default profile)
   - PDO2: Configurable up to 12V
   - PDO3: Configurable up to 20V

2. When you set a new configuration through the web interface or API, the library:
   - Selects the appropriate PDO based on the requested voltage
   - Updates the voltage and current values for that PDO
   - Makes that PDO the highest priority
   - Writes the settings to the STUSB4500 chip
   - Triggers a soft reset to apply the changes immediately

## Troubleshooting

- **Board Not Detected**: Make sure the I2C connections are correct and the STUSB4500 board is properly powered.
- **Power Not Changing**: Some power supplies may not support all voltage/current combinations. Try different values.
- **Web Interface Not Loading**: Ensure your WiFi connection is stable and the ESP8266 is properly connected to your network.

## Dependencies

- Wire library (for I2C communication)
- SparkFun STUSB4500 Arduino Library
- ESP8266WebServer library (for ESP8266 boards)
- WebServer library (for ESP32-S3 boards)
- ArduinoJson library (for API responses)
- WiFi library (for wireless connectivity)
- DNSServer library (for captive portal in setup mode)

## Installation

1. Install the required libraries through the Arduino Library Manager:
   - ArduinoJson (by Benoit Blanchon)
   - SparkFun STUSB4500 Arduino Library (by SparkFun)
   
2. Create a new project with the appropriate board selected:
   - For ESP8266: Select your ESP8266 board type (NodeMCU, Wemos D1, etc.)
   - For ESP32-S3: Select "ESP32S3 Dev Module" or your specific ESP32-S3 board
   
3. Add the USB PD Controller and WiFi AP modules to your project:
   - Copy the `lib/usb_pd_controller` and `lib/wifi_ap` folders to your project's library folder
   
4. Use the example code as a starting point for your project

## License

MIT License

Copyright (c) 2023-2024 Your Name

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.