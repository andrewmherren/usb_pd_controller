# USB-C Power Delivery Controller

A web-based interface for controlling USB-C Power Delivery (PD) settings through an ESP32-S3 microcontroller and the SparkFun STUSB4500 breakout board. Implements the `IWebModule` interface for seamless integration with web routing systems.

## Features

- **Web Module Interface**: Implements `IWebModule` interface for integration with web routing systems
- **Web Interface**: Intuitive UI for setting and monitoring USB-C PD profiles
- **Dynamic PDO Configuration**: Configure Power Data Objects (PDOs) for different voltage/current requirements
- **Profile Management**: View and select from available power profiles
- **Real-time Monitoring**: Check current voltage, current, and power output
- **REST API**: Control USB-C PD settings programmatically via API endpoints
- **I2C Communication**: Works with the SparkFun STUSB4500 breakout board over I2C
- **Global Instance**: Provides `extern` global instance for easy access across modules

## Hardware Requirements

- ESP32-S3 board
- [SparkFun USB-C Power Delivery Board (STUSB4500)](https://www.sparkfun.com/products/15801)
- USB-C PD compliant power supply
- USB-C device to power

## Wiring

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

1. Create a new PlatformIO project for your ESP32 device.
2. Add this library to your project:
   - Clone this repository into your project's `lib` folder: 
     ```
     git clone https://github.com/andrewmherren/usb_pd_controller.git lib/usb_pd_controller
     ```
   - Or use git submodules if your project is a git repository:
     ```
     git submodule add https://github.com/andrewmherren/usb_pd_controller.git lib/usb_pd_controller
     ```

3. Make sure to add the required dependencies to your `platformio.ini`:
   ```
   lib_deps = 
     https://github.com/sparkfun/SparkFun_STUSB4500_Arduino_Library.git
     bblanchon/ArduinoJson@^6.20.0
     https://github.com/andrewmherren/web_module_interface.git
   ```

## Usage

### IWebModule Interface

The USB PD Controller implements the `IWebModule` interface and provides these routes:

```cpp
// Routes provided by getHttpRoutes() / getHttpsRoutes():
// GET  /              - Main USB PD control page  
// GET  /pd-status     - Get current PD status (JSON)
// GET  /available-voltages - Get available voltage options (JSON)
// GET  /available-currents - Get available current options (JSON)
// GET  /pdo-profiles  - Get PDO profiles information (JSON)
// POST /set-pd-config - Set new voltage and current configuration (JSON)
```

### Basic Usage with Web Router

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <web_router.h>
#include <usb_pd_controller.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize USB PD controller with default I2C address (0x28)
  usbPDController.begin();
  
  // Register USB PD controller with web router
  webRouter.registerModule("/usb_pd", &usbPDController);
  
  // Start the web router
  webRouter.begin(80, 443);
  
  DEBUG_PRINTLN("Web router started with USB PD controller at /usb_pd/");
}

void loop() {
  // Handle web router
  webRouter.handle();
  
  // Handle USB PD controller operations
  usbPDController.handle();
}
```

### Integration with WiFi Manager

This library can be integrated with WiFi Manager and Web Router:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <wifi_ap.h>
#include <web_router.h>
#include <usb_pd_controller.h>

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C - optionally specify pins for ESP32-S3
  Wire.begin(4, 5);  // SDA=4, SCL=5
  
  // Initialize WiFi Manager
  wifiManager.begin("TickerTape", true);
  
  // Only proceed with USB PD Controller when WiFi is connected
  if (wifiManager.getConnectionState() == WIFI_CONNECTED) {
    // Initialize USB PD controller
    usbPDController.begin();
    
    // Register USB PD Controller module with web router
    webRouter.registerModule("/usb_pd", &usbPDController);
    
    // Register WiFi Manager module
    webRouter.registerModule("/wifi", &wifiManager);
    
    // Start web router
    webRouter.begin(80, 443);
    
    DEBUG_PRINTLN("USB PD Controller available at " + 
                   webRouter.getBaseUrl() + "/usb_pd/");
  } else {
    DEBUG_PRINTLN("Running in WiFi setup mode. Connect to AP: " + 
                   String(wifiManager.getAPName()));
  }
}

void loop() {
  // Always handle WiFi Manager
  wifiManager.handle();
  
  // Only handle web router and USB PD when WiFi is connected
  if (wifiManager.getConnectionState() == WIFI_CONNECTED) {
    webRouter.handle();
    usbPDController.handle();
  }
}
```

## Web Interface

The library provides a responsive web interface for controlling USB-C PD settings:

- **Main Interface**: 
  - When registered with web router: `http://[device-ip]/usb_pd/`
  - Via mDNS: `http://[hostname].local/usb_pd/`

- **API Endpoints** (all relative to base path):
  - `GET /pd-status` - Returns current PD status (JSON)
  - `GET /available-voltages` - Returns available voltage options (JSON)
  - `GET /available-currents` - Returns available current options (JSON)
  - `GET /pdo-profiles` - Returns all PDO profiles (JSON)
  - `POST /set-pd-config` - Sets new voltage and current configuration (JSON)

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
- **Web Interface Not Loading**: Ensure your WiFi connection is stable and the ESP32 is properly connected to your network.

## Dependencies

- **web_module_interface** - Abstract interface for web module integration
- Wire library (for I2C communication)
- SparkFun STUSB4500 Arduino Library
- ArduinoJson library (for API responses)

## Architecture

This module follows modern architecture patterns:

- **IWebModule Implementation**: Implements standard interface for web router integration
- **Global Instance**: Provides global `usbPDController` instance accessible from any module
- **Module Independence**: Functions as a standalone module with clear dependencies

## Installation

1. Install the required libraries through PlatformIO:
   ```ini
   lib_deps = 
     https://github.com/sparkfun/SparkFun_STUSB4500_Arduino_Library.git
     bblanchon/ArduinoJson@^6.20.0
     https://github.com/andrewmherren/web_module_interface.git
   ```
   
2. Create a new project with the appropriate board selected
   
3. Add the USB PD Controller module to your project
   
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