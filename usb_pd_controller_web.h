#ifndef USB_PD_CONTROLLER_WEB_H
#define USB_PD_CONTROLLER_WEB_H

// Embedded HTML content for USB PD device control interface
const char USB_PD_CONTROLLER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>USB-C Power Delivery Control</title>
  <style>
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      margin: 0;
      padding: 20px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      color: white;
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
      background: rgba(255, 255, 255, 0.1);
      padding: 30px;
      border-radius: 15px;
      backdrop-filter: blur(10px);
      box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
    }
    h1 {
      text-align: center;
      margin-bottom: 30px;
      font-size: 2.5em;
      text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);
    }
    .status-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
      gap: 20px;
      margin-bottom: 30px;
    }
    .status-card {
      background: rgba(255, 255, 255, 0.15);
      padding: 20px;
      border-radius: 10px;
      border: 1px solid rgba(255, 255, 255, 0.2);
    }
    .status-card h3 {
      margin: 0 0 10px 0;
      color: #fff;
      font-size: 1.2em;
    }
    .status-card p {
      margin: 5px 0;
      opacity: 0.9;
    }
    .success { color: #4CAF50; font-weight: bold; }
    .info { color: #2196F3; font-weight: bold; }
    .warning { color: #FF9800; font-weight: bold; }
    .error { color: #f44336; font-weight: bold; }
    
    .form-group {
      margin-bottom: 15px;
    }
    label {
      display: block;
      margin-bottom: 8px;
      font-weight: bold;
      color: #fff;
    }
    .form-control {
      width: 100%;
      padding: 12px;
      border: 1px solid rgba(255, 255, 255, 0.3);
      border-radius: 8px;
      box-sizing: border-box;
      background: rgba(255, 255, 255, 0.1);
      color: white;
      backdrop-filter: blur(5px);
    }
    .form-control:focus {
      outline: none;
      border-color: rgba(255, 255, 255, 0.5);
      background: rgba(255, 255, 255, 0.15);
    }
    .form-control option {
      background: #764ba2;
      color: white;
    }
    .nav-links {
      display: flex;
      justify-content: center;
      gap: 20px;
      margin-top: 30px;
      flex-wrap: wrap;
    }
    .nav-links a {
      background: rgba(255, 255, 255, 0.2);
      color: white;
      text-decoration: none;
      padding: 12px 24px;
      border-radius: 25px;
      border: 1px solid rgba(255, 255, 255, 0.3);
      transition: all 0.3s ease;
    }
    .nav-links a:hover {
      background: rgba(255, 255, 255, 0.3);
      transform: translateY(-2px);
    }
    .button-group {
      display: flex;
      gap: 15px;
      margin-top: 20px;
      justify-content: center;
      flex-wrap: wrap;
    }
    .btn {
      padding: 12px 24px;
      border: none;
      border-radius: 25px;
      cursor: pointer;
      font-weight: bold;
      transition: all 0.3s ease;
      border: 1px solid rgba(255, 255, 255, 0.3);
    }
    .btn-primary {
      background: rgba(76, 175, 80, 0.8);
      color: white;
    }
    .btn-primary:hover {
      background: rgba(76, 175, 80, 1);
      transform: translateY(-2px);
    }
    .btn-secondary {
      background: rgba(255, 255, 255, 0.2);
      color: white;
    }
    .btn-secondary:hover {
      background: rgba(255, 255, 255, 0.3);
      transform: translateY(-2px);
    }
    .btn-warning {
      background: rgba(255, 152, 0, 0.8);
      color: white;
    }
    .btn-warning:hover {
      background: rgba(255, 152, 0, 1);
      transform: translateY(-2px);
    }
    .footer {
      text-align: center;
      margin-top: 40px;
      opacity: 0.7;
      font-size: 0.9em;
    }
    .status-message {
      padding: 15px;
      border-radius: 10px;
      margin-top: 15px;
      font-weight: bold;
      background: rgba(255, 255, 255, 0.1);
      border: 1px solid rgba(255, 255, 255, 0.2);
    }
    .status-message.error {
      background: rgba(244, 67, 54, 0.2);
      border-color: rgba(244, 67, 54, 0.5);
      color: #ffcdd2;
    }
    .status-message.success {
      background: rgba(76, 175, 80, 0.2);
      border-color: rgba(76, 175, 80, 0.5);
      color: #c8e6c9;
    }
    .status-message.info {
      background: rgba(33, 150, 243, 0.2);
      border-color: rgba(33, 150, 243, 0.5);
      color: #bbdefb;
    }
    .hidden {
      display: none;
    }
    .pdo-container {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 15px;
      margin-bottom: 20px;
    }
    .pdo-card {
      border: 2px solid rgba(255, 255, 255, 0.2);
      border-radius: 10px;
      padding: 15px;
      background: rgba(255, 255, 255, 0.1);
      backdrop-filter: blur(5px);
    }
    .pdo-card.active {
      border-color: rgba(76, 175, 80, 0.8);
      background: rgba(76, 175, 80, 0.2);
    }
    .pdo-card.fixed {
      border-color: rgba(255, 152, 0, 0.8);
      background: rgba(255, 152, 0, 0.2);
    }
    .pdo-header {
      font-weight: bold;
      margin-bottom: 10px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      color: #fff;
    }
    .pdo-badge {
      background: rgba(33, 150, 243, 0.8);
      color: white;
      padding: 4px 12px;
      border-radius: 15px;
      font-size: 0.75em;
      font-weight: bold;
    }
    .pdo-badge.fixed {
      background: rgba(255, 152, 0, 0.8);
    }
    .pdo-badge.active {
      background: rgba(76, 175, 80, 0.8);
    }
    .pdo-details {
      font-size: 0.9em;
      line-height: 1.4;
      color: rgba(255, 255, 255, 0.9);
    }
    .refresh-button {
      margin-left: 10px;
      padding: 8px 16px;
      font-size: 0.8em;
    }
  </style>
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
    
    <div class="nav-links">
      <a href="/">🏠 Home</a>
      <a href="/wifi">📡 WiFi Settings</a>
      <a href="/web_router">🚀 Web Router Status</a>
    </div>
    
    <div class="footer">
      <p>USB-C Power Delivery Controller</p>
      <p>Unified web interface via Web Router</p>
    </div>
  </div>

  <script>
// Get current configuration on page load
window.onload = function() {
  // Initialize with the assumption that device is not connected yet
  document.getElementById('currentVoltage').innerText = 'Loading...';
  document.getElementById('currentCurrent').innerText = 'Loading...';
  document.getElementById('currentPower').innerText = 'Loading...';
  document.getElementById('statusMessage').className = 'status-message info';
  document.getElementById('statusMessage').innerText = 'Checking device status...';
  document.getElementById('statusMessage').classList.remove('hidden');
  document.getElementById('configSection').classList.add('hidden');
  
  fetchCurrentConfig();
  loadAvailableOptions();
  loadPDOProfiles();
};

function fetchCurrentConfig() {
  // Show loading state
  document.getElementById('currentVoltage').innerText = 'Loading...';
  document.getElementById('currentCurrent').innerText = 'Loading...';
  document.getElementById('currentPower').innerText = 'Loading...';
  document.getElementById('statusMessage').className = 'status-message info';
  document.getElementById('statusMessage').innerText = 'Checking device status...';
  document.getElementById('statusMessage').classList.remove('hidden');
  document.getElementById('retryContainer').classList.add('hidden');
  
  fetch('/pd-status')
    .then(response => response.json())
    .then(data => {
      if (data.success) {
        // Success case - PD board connected and values read
        document.getElementById('currentVoltage').innerText = data.voltage;
        document.getElementById('currentCurrent').innerText = data.current;
        document.getElementById('currentPower').innerText = (data.voltage * data.current).toFixed(2);
        
        // Pre-select the current values in dropdowns
        selectOptionByValue('voltageSelect', data.voltage);
        selectOptionByValue('currentSelect', data.current);
        
        // Show success status and enable form
        document.getElementById('statusMessage').className = 'status-message success';
        document.getElementById('statusMessage').innerText = 'Device connected and ready';
        document.getElementById('configSection').classList.remove('hidden');
        document.getElementById('retryContainer').classList.add('hidden');
        setFormEnabled(true);
        
      } else {
        // Error case - PD board not connected or values not read
        document.getElementById('currentVoltage').innerText = 'N/A';
        document.getElementById('currentCurrent').innerText = 'N/A';
        document.getElementById('currentPower').innerText = 'N/A';
        
        // Show error status and disable form
        document.getElementById('statusMessage').className = 'status-message error';
        document.getElementById('statusMessage').innerText = data.message;
        document.getElementById('retryContainer').classList.remove('hidden');
        
        if (data.connected) {
          // Board is connected but values not read - still show form
          document.getElementById('configSection').classList.remove('hidden');
          setFormEnabled(false);
        } else {
          // Board not connected - hide form completely
          document.getElementById('configSection').classList.add('hidden');
        }
      }
    }).catch(error => {
      console.error('Error fetching status:', error);
      document.getElementById('currentVoltage').innerText = 'Error';
      document.getElementById('currentCurrent').innerText = 'Error';
      document.getElementById('currentPower').innerText = 'Error';
      
      // Display error message and show retry button
      document.getElementById('statusMessage').className = 'status-message error';
      document.getElementById('statusMessage').innerText = 'Failed to communicate with device';
      document.getElementById('configSection').classList.add('hidden');
      document.getElementById('retryContainer').classList.remove('hidden');
    });
}

function loadAvailableOptions() {
  // Load available voltages
  fetch('/available-voltages')
    .then(response => response.json())
    .then(voltages => {
      const voltageSelect = document.getElementById('voltageSelect');
      voltageSelect.innerHTML = '<option value="">Select voltage...</option>';
      voltages.forEach(voltage => {
        const option = document.createElement('option');
        option.value = voltage;
        option.text = voltage + ' V';
        voltageSelect.appendChild(option);
      });
    });
    
  // Load available currents
  fetch('/available-currents')
    .then(response => response.json())
    .then(currents => {
      const currentSelect = document.getElementById('currentSelect');
      currentSelect.innerHTML = '<option value="">Select current...</option>';
      currents.forEach(current => {
        const option = document.createElement('option');
        option.value = current;
        option.text = current + ' A';
        currentSelect.appendChild(option);
      });
    });
}

function selectOptionByValue(selectId, value) {
  const select = document.getElementById(selectId);
  for (let i = 0; i < select.options.length; i++) {
    if (Math.abs(parseFloat(select.options[i].value) - value) < 0.01) {
      select.selectedIndex = i;
      break;
    }
  }
}

// Apply button click handler
document.getElementById('applyBtn').addEventListener('click', function() {
  const voltage = document.getElementById('voltageSelect').value;
  const current = document.getElementById('currentSelect').value;
  
  if (!voltage || !current) {
    alert('Please select both voltage and current');
    return;
  }
  
  // Disable form while applying
  setFormEnabled(false);
  document.getElementById('statusMessage').className = 'status-message info';
  document.getElementById('statusMessage').innerText = 'Applying settings...';
  document.getElementById('statusMessage').classList.remove('hidden');
  
  fetch('/set-pd-config', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({
      voltage: parseFloat(voltage),
      current: parseFloat(current)
    })
  })
  .then(response => response.json())
  .then(data => {
    if (data.success) {
      // Update displayed values
      document.getElementById('currentVoltage').innerText = data.voltage;
      document.getElementById('currentCurrent').innerText = data.current;
      document.getElementById('currentPower').innerText = (data.voltage * data.current).toFixed(2);
      
      // Display success message
      document.getElementById('statusMessage').className = 'status-message success';
      document.getElementById('statusMessage').innerText = 'Settings applied successfully';
      
      // Refresh PDO profiles to show the new active PDO
      setTimeout(loadPDOProfiles, 1000);
      
      // Hide success message after 3 seconds
      setTimeout(function() {
        document.getElementById('statusMessage').classList.add('hidden');
      }, 3000);
    } else {
      // Display error message
      document.getElementById('statusMessage').className = 'status-message error';
      document.getElementById('statusMessage').innerText = 'Error: ' + data.message;
      document.getElementById('retryContainer').classList.remove('hidden');
      
      // If board not connected, hide form
      if (data.message === 'PD board not connected') {
        document.getElementById('configSection').classList.add('hidden');
      }
      
      // Refresh the current status
      setTimeout(fetchCurrentConfig, 3000);
    }
    setFormEnabled(true);
  })
  .catch(error => {
    console.error('Error:', error);
    document.getElementById('statusMessage').className = 'status-message error';
    document.getElementById('statusMessage').innerText = 'Failed to apply configuration';
    document.getElementById('retryContainer').classList.remove('hidden');
    setFormEnabled(true);
    
    // Refresh the current status after a delay
    setTimeout(fetchCurrentConfig, 3000);
  });
});

// Cancel button click handler
document.getElementById('cancelBtn').addEventListener('click', function() {
  fetchCurrentConfig(); // Reset dropdowns to current values
});

// Retry button click handler
document.getElementById('retryBtn').addEventListener('click', function() {
  fetchCurrentConfig(); // Try to reconnect and get status
  loadPDOProfiles();
});

function setFormEnabled(enabled) {
  document.getElementById('voltageSelect').disabled = !enabled;
  document.getElementById('currentSelect').disabled = !enabled;
  document.getElementById('applyBtn').disabled = !enabled;
  document.getElementById('cancelBtn').disabled = !enabled;
}

// Load PDO profiles
function loadPDOProfiles() {
  fetch('/pdo-profiles')
    .then(response => response.json())
    .then(data => {
      const container = document.getElementById('pdoProfiles');
      
      if (data.error) {
        container.innerHTML = '<div>Error: ' + data.error + '</div>';
        return;
      }
      
      let html = '';
      data.pdos.forEach(pdo => {
        const cardClass = pdo.active ? 'pdo-card active' : (pdo.fixed ? 'pdo-card fixed' : 'pdo-card');
        const badgeClass = pdo.active ? 'pdo-badge active' : (pdo.fixed ? 'pdo-badge fixed' : 'pdo-badge');
        const badgeText = pdo.active ? 'ACTIVE' : (pdo.fixed ? 'FIXED' : 'AVAILABLE');
        
        html += `
          <div class="${cardClass}">
            <div class="pdo-header">
              PDO${pdo.number}
              <span class="${badgeClass}">${badgeText}</span>
            </div>
            <div class="pdo-details">
              <div><strong>${pdo.voltage}V</strong> @ <strong>${pdo.current}A</strong></div>
              <div>Max Power: <strong>${pdo.power.toFixed(1)}W</strong></div>
              ${pdo.fixed ? '<div><em>Fixed 5V profile</em></div>' : ''}
            </div>
          </div>
        `;
      });
      
      container.innerHTML = html;
    })
    .catch(error => {
      console.error('Error loading PDO profiles:', error);
      document.getElementById('pdoProfiles').innerHTML = '<div>Failed to load PDO profiles</div>';
    });
}

// Add event listener for refresh button
document.getElementById('refreshPDOBtn').addEventListener('click', function() {
  loadPDOProfiles();
});
  </script>
</body>
</html>
)rawliteral";

#endif // USB_PD_CONTROLLER_WEB_H