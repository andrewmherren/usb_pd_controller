#ifndef USB_PD_HTML_H
#define USB_PD_HTML_H

#include <Arduino.h>

const char USB_PD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>USB-C Power Delivery Control</title>
  <link rel="stylesheet" href="/assets/style.css" type="text/css">
  <link rel="stylesheet" href="/assets/tickertape-theme.css" type="text/css">
  <link rel="icon" href="/favicon.svg" type="image/svg+xml">
  <link rel="icon" href="/favicon.ico" sizes="any">
</head>
<body>
  <div class="container">
    <h1>⚡ USB-C Power Delivery Control</h1>
    
    <div class="status-grid">
      <div class="status-card">
        <h3>Current Status</h3>
        <p>Voltage: <span id="currentVoltage" class="info">Loading...</span>V</p>
        <p>Current: <span id="currentCurrent" class="info">Loading...</span>A</p>
        <p>Power: <span id="currentPower" class="info">Loading...</span>W</p>
        <div id="statusMessage" class="status-message hidden"></div>
        <div id="retryContainer" class="button-group hidden">
          <button id="retryBtn" class="btn btn-warning">🔄 Retry Connection</button>
        </div>
      </div>
    </div>
    
    <div class="status-card">
      <h3>PDO Profiles 
        <button id="refreshPDOBtn" class="btn btn-secondary refresh-button">🔄 Refresh</button>
      </h3>
      <div id="pdoProfiles" class="pdo-container">
        <div>Loading PDO profiles...</div>
      </div>
    </div>
    
    <div id="configSection" class="status-card config-section">
      <h3>⚙️ Set New Configuration</h3>
      <div class="form-group">
        <label for="voltageSelect">Voltage:</label>
        <select id="voltageSelect" class="form-control">
          <option value="">Select voltage...</option>
        </select>
      </div>
      
      <div class="form-group">
        <label for="currentSelect">Current:</label>
        <select id="currentSelect" class="form-control">
          <option value="">Select current...</option>
        </select>
      </div>
      
      <div class="button-group">
        <button id="applyBtn" class="btn btn-primary">✅ Apply Configuration</button>
        <button id="cancelBtn" class="btn btn-secondary">❌ Cancel</button>
      </div>
    </div>
    
    <!-- Navigation menu will be auto-injected here -->
    
    <div class="footer">
      <p>USB-C Power Delivery Controller</p>
      <p>Unified web interface via Web Router</p>
    </div>
  </div>

  <!-- Load TickerTape utilities -->
  <script src="/assets/tickertape-utils.js"></script>
  <!-- Load USB PD Controller JavaScript -->
  <script src="/assets/usb-pd-controller.js"></script>
</body>
</html>)rawliteral";

#endif // USB_PD_HTML_H