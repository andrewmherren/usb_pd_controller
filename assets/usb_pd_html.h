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
  <!-- Optional app-specific theme CSS is loaded after default styles -->
  <link rel="icon" href="/assets/favicon.svg" type="image/svg+xml">
  <link rel="icon" href="/assets/favicon.ico" sizes="any">
  <style>
    .info-message {
      background: #fff3cd;
      border: 1px solid #ffeaa7;
      color: #856404;
      padding: 12px;
      border-radius: 4px;
      text-align: center;
    }
  </style>
  <script src="/assets/web-platform-utils.js"></script>
  <script src="assets/usb-pd-controller.js"></script>
</head>
<body>
  <div class="container">
    {{NAV_MENU}}
    <h1>USB-C Power Delivery Control</h1>
    
    <div class="status-grid">
      <div class="status-card">
        <h3>Current Status</h3>
        <p>Voltage: <span id="currentVoltage" class="info">Loading...</span>V</p>
        <p>Current: <span id="currentCurrent" class="info">Loading...</span>A</p>
        <p>Power: <span id="currentPower" class="info">Loading...</span>W</p>
        <div id="statusMessage" class="status-message hidden"></div>
        <div id="retryContainer" class="button-group hidden">
          <button id="retryBtn" class="btn btn-warning">Retry Connection</button>
        </div>
      </div>
    </div>
    
    <div class="status-card">
      <h3>PDO Profiles 
        <button id="refreshPDOBtn" class="btn btn-secondary">Refresh</button>
      </h3>
      <div id="pdoProfiles" class="pdo-container">
        <div>Loading PDO profiles...</div>
      </div>
    </div>
    
    <div id="configSection" class="status-card config-section">
      <h3>Set New Configuration</h3>
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
        <button id="applyBtn" class="btn btn-primary">Apply Configuration</button>
        <button id="cancelBtn" class="btn btn-secondary">Cancel</button>
      </div>
    </div>
    <div class="footer">
      <p>USB-C Power Delivery Controller</p>
      <p>Unified web interface via Web Router</p>
    </div>
  </div>
</body>
</html>)rawliteral";

#endif // USB_PD_HTML_H