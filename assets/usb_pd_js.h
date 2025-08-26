#ifndef USB_PD_JS_H
#define USB_PD_JS_H

#include <Arduino.h>

const char USB_PD_JS[] PROGMEM = R"rawliteral(
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
  
  fetch('pd-status')
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
  fetch('available-voltages')
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
      // Update button state after loading options
      updateApplyButtonState();
    });
    
  // Load available currents
  fetch('available-currents')
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
      // Update button state after loading options
      updateApplyButtonState();
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

// Function to update apply button state
function updateApplyButtonState() {
  const voltage = document.getElementById('voltageSelect').value;
  const current = document.getElementById('currentSelect').value;
  const applyBtn = document.getElementById('applyBtn');
  
  if (voltage && current) {
    applyBtn.disabled = false;
    applyBtn.style.opacity = '1';
  } else {
    applyBtn.disabled = true;
    applyBtn.style.opacity = '0.5';
  }
}

// Add event listeners to dropdowns to update button state
document.getElementById('voltageSelect').addEventListener('change', updateApplyButtonState);
document.getElementById('currentSelect').addEventListener('change', updateApplyButtonState);

// Apply button click handler
document.getElementById('applyBtn').addEventListener('click', function() {
  const voltage = document.getElementById('voltageSelect').value;
  const current = document.getElementById('currentSelect').value;
  
  // This should not happen due to button being disabled, but just in case
  if (!voltage || !current) {
    return;
  }
  
  // Disable form while applying
  setFormEnabled(false);
  document.getElementById('statusMessage').className = 'status-message info';
  document.getElementById('statusMessage').innerText = 'Applying settings...';
  document.getElementById('statusMessage').classList.remove('hidden');
  
  fetch('set-pd-config', {
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
      
      // Show success notification
      if (typeof TickerTape !== 'undefined' && TickerTape.showNotification) {
        TickerTape.showNotification('USB PD settings applied successfully', 'success', 3000);
      }
      
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
      
      // Show error notification
      if (typeof TickerTape !== 'undefined' && TickerTape.showNotification) {
        TickerTape.showNotification('Error: ' + data.message, 'error', 3000);
      }
      
      // If board not connected, hide form
      if (data.message === 'PD board not connected') {
        document.getElementById('configSection').classList.add('hidden');
      }
      
      // Refresh the current status
      setTimeout(fetchCurrentConfig, 3000);
    }
    setFormEnabled(true);
    updateApplyButtonState(); // Re-check button state after re-enabling
  })
  .catch(error => {
    console.error('Error:', error);
    document.getElementById('statusMessage').className = 'status-message error';
    document.getElementById('statusMessage').innerText = 'Failed to apply configuration';
    document.getElementById('retryContainer').classList.remove('hidden');
    setFormEnabled(true);
    updateApplyButtonState(); // Re-check button state after re-enabling
    
    // Show error notification
    if (typeof TickerTape !== 'undefined' && TickerTape.showNotification) {
      TickerTape.showNotification('Failed to apply configuration', 'error', 3000);
    }
    
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
  document.getElementById('cancelBtn').disabled = !enabled;
  
  // Don't override the apply button's disabled state logic
  if (enabled) {
    updateApplyButtonState();
  } else {
    document.getElementById('applyBtn').disabled = true;
    document.getElementById('applyBtn').style.opacity = '0.5';
  }
}

// Load PDO profiles
function loadPDOProfiles() {
  fetch('pdo-profiles')
    .then(response => response.json())
    .then(data => {
      const container = document.getElementById('pdoProfiles');
      
      if (data.error) {
        container.innerHTML = '<div>Error: ' + data.error + '</div>';
        return;
      }
      
      // Check if pdos array exists and is not empty
      if (!data.pdos || !Array.isArray(data.pdos) || data.pdos.length === 0) {
        container.innerHTML = '<div>No PDO profiles available. Device may be disconnected.</div>';
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
)rawliteral";

#endif // USB_PD_JS_H