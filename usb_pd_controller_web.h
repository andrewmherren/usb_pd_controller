#ifndef USB_PD_CONTROLLER_WEB_H
#define USB_PD_CONTROLLER_WEB_H

// Embedded HTML content for USB PD device control interface
const char USB_PD_CONTROLLER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>USB-C Power Delivery Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .container { max-width: 600px; margin: 0 auto; }
    h1 { color: #0066cc; }
    .status-card {
      background-color: #f5f5f5;
      border-radius: 8px;
      padding: 15px;
      margin-bottom: 20px;
    }
    .form-group {
      margin-bottom: 15px;
    }
    label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
    }
    .form-control {
      width: 100%;
      padding: 8px;
      border: 1px solid #ddd;
      border-radius: 4px;
      box-sizing: border-box;
    }
    .button-group {
      display: flex;
      gap: 10px;
      margin-top: 20px;
    }
    .btn {
      padding: 10px 15px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-weight: bold;
    }
    .btn-primary {
      background-color: #0066cc;
      color: white;
    }
    .btn-primary:hover {
      background-color: #0055aa;
    }
    .btn-secondary {
      background-color: #f0f0f0;
      color: #333;
    }
    .btn-secondary:hover {
      background-color: #ddd;
    }
    .btn-warning {
      background-color: #ff9900;
      color: white;
    }
    .btn-warning:hover {
      background-color: #ff8800;
    }
    .footer {
      margin-top: 30px;
      font-size: 0.8em;
      color: #666;
      text-align: center;
    }
    .status-message {
      padding: 10px;
      border-radius: 4px;
      margin-top: 10px;
      font-weight: bold;
    }
    .error {
      background-color: #ffeeee;
      color: #cc0000;
    }
    .success {
      background-color: #eeffee;
      color: #008800;
    }
    .info {
      background-color: #eeeeff;
      color: #0000cc;
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
      border: 2px solid #ddd;
      border-radius: 8px;
      padding: 15px;
      background-color: #f9f9f9;
    }
    .pdo-card.active {
      border-color: #0066cc;
      background-color: #e6f3ff;
    }
    .pdo-card.fixed {
      border-color: #ff9900;
      background-color: #fff3e6;
    }
    .pdo-header {
      font-weight: bold;
      margin-bottom: 10px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .pdo-badge {
      background-color: #0066cc;
      color: white;
      padding: 2px 8px;
      border-radius: 12px;
      font-size: 0.8em;
    }
    .pdo-badge.fixed {
      background-color: #ff9900;
    }
    .pdo-details {
      font-size: 0.9em;
      line-height: 1.4;
    }
    .refresh-button {
      margin-left: 10px;
      padding: 5px 10px;
      font-size: 0.8em;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>USB-C Power Delivery Control</h1>
    
    <div class="status-card">
      <h2>Current Status</h2>
      <p>Active Voltage: <span id="currentVoltage">Loading...</span>V</p>
      <p>Active Current: <span id="currentCurrent">Loading...</span>A</p>
      <p>Active Power: <span id="currentPower">Loading...</span>W</p>
      <div id="statusMessage" class="status-message hidden"></div>
      <div id="retryContainer" class="button-group hidden">
        <button id="retryBtn" class="btn btn-warning">Retry Connection</button>
      </div>
    </div>
    
    <div class="status-card">
      <h2>PDO Profiles 
        <button id="refreshPDOBtn" class="btn btn-secondary refresh-button">Refresh</button>
      </h2>
      <div id="pdoProfiles" class="pdo-container">
        <div>Loading PDO profiles...</div>
      </div>
    </div>
    
    <div id="configSection" class="config-section">
      <h2>Set New Configuration</h2>
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
        <button id="applyBtn" class="btn btn-primary">Apply</button>
        <button id="cancelBtn" class="btn btn-secondary">Cancel</button>
      </div>
    </div>
    
    <div class="footer">
      <p>USB-C PD Control</p>
      <p><a href="/wifi">WiFi Setup</a></p>
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
        const badgeClass = pdo.fixed ? 'pdo-badge fixed' : 'pdo-badge';
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