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

async function fetchCurrentConfig() {
  // Show loading state
  document.getElementById('currentVoltage').innerText = 'Loading...';
  document.getElementById('currentCurrent').innerText = 'Loading...';
  document.getElementById('currentPower').innerText = 'Loading...';
  document.getElementById('statusMessage').className = 'status-message info';
  document.getElementById('statusMessage').innerText = 'Checking device status...';
  document.getElementById('statusMessage').classList.remove('hidden');
  document.getElementById('retryContainer').classList.add('hidden');
  
  try {
    const data = await AuthUtils.fetchJSON('api/status');
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
  } catch (error) {
    console.error('Error fetching status:', error);
    document.getElementById('currentVoltage').innerText = 'Error';
    document.getElementById('currentCurrent').innerText = 'Error';
    document.getElementById('currentPower').innerText = 'Error';
    
    // Display error message and show retry button
    document.getElementById('statusMessage').className = 'status-message error';
    document.getElementById('statusMessage').innerText = 'Failed to communicate with device: ' + error.message;
    document.getElementById('configSection').classList.add('hidden');
    document.getElementById('retryContainer').classList.remove('hidden');
  }
}

async function loadAvailableOptions() {
  try {
    // Load voltage options
    const voltageResponse = await AuthUtils.fetchJSON('api/voltages');
    const voltageSelect = document.getElementById('voltageSelect');
    voltageSelect.innerHTML = '<option value="">Select voltage...</option>';
    
    if (voltageResponse.voltages && Array.isArray(voltageResponse.voltages)) {
      voltageResponse.voltages.forEach(voltage => {
        const option = document.createElement('option');
        option.value = voltage;
        option.text = voltage + ' V';
        voltageSelect.appendChild(option);
      });
    }
    
    // Add event listener for voltage changes to filter currents  
    voltageSelect.addEventListener('change', function() {
      updateCurrentOptions();
    });
    
    updateApplyButtonState();
  } catch (error) {
    console.error('Error loading voltages:', error);
  }
  
  try {
    // Load current options
    const currentResponse = await AuthUtils.fetchJSON('api/currents');
    const currentSelect = document.getElementById('currentSelect');
    currentSelect.innerHTML = '<option value="">Select current...</option>';
    
    if (currentResponse.currents && Array.isArray(currentResponse.currents)) {
      currentResponse.currents.forEach(current => {
        const option = document.createElement('option');
        option.value = current;
        option.text = current + ' A';
        currentSelect.appendChild(option);
      });
    }
    
    // Add event listener for current changes to filter voltages
    if (currentSelect) {
      currentSelect.addEventListener('change', function() {
        updateVoltageOptions();
      });
    }
    
    updateApplyButtonState();
  } catch (error) {
    console.error('Error loading currents:', error);
  }
}

function updateCurrentOptions() {
  const voltageSelect = document.getElementById('voltageSelect');
  const currentSelect = document.getElementById('currentSelect');
  const selectedVoltage = parseFloat(voltageSelect.value);
  
  // Save current selection
  const currentValue = currentSelect.value;
  
  if (!selectedVoltage || isNaN(selectedVoltage)) {
    // If no voltage selected, show all currents
    currentSelect.innerHTML = '<option value="">Select current...</option>';
    const allCurrents = [0.5, 1.0, 1.33, 1.5, 1.67, 2.0, 2.25, 2.5, 3.0];
    allCurrents.forEach(current => {
      const option = document.createElement('option');
      option.value = current;
      option.text = current + ' A';
      currentSelect.appendChild(option);
    });
    
    // Restore selection if it was valid
    if (currentValue) {
      currentSelect.value = currentValue;
    }
    
    updateApplyButtonState();
    return;
  }
  
  // Clear and repopulate based on voltage
  currentSelect.innerHTML = '<option value="">Select current...</option>';
  
  // Define realistic current limits based on common PD profiles
  let maxCurrent = 3.0; // Default max
  
  if (selectedVoltage === 5.0) {
    maxCurrent = 3.0;  // USB-C can do 5V@3A
  } else if (selectedVoltage === 9.0) {
    maxCurrent = 2.25; // Common 9V profile is 2.25A (20W)
  } else if (selectedVoltage === 12.0) {
    maxCurrent = 1.67; // Common 12V profile is 1.67A (20W)
  } else if (selectedVoltage === 15.0) {
    maxCurrent = 1.33; // 15V@1.33A = 20W
  } else if (selectedVoltage === 20.0) {
    maxCurrent = 1.0;  // 20V@1A = 20W
  }
  
  // Standard current options
  const allCurrents = [0.5, 1.0, 1.33, 1.5, 1.67, 2.0, 2.25, 2.5, 3.0];
  
  allCurrents.forEach(current => {
    if (current <= maxCurrent + 0.01) { // Small tolerance
      const option = document.createElement('option');
      option.value = current;
      option.text = current + ' A';
      currentSelect.appendChild(option);
    }
  });
  
  // Restore selection if still valid
  if (currentValue && parseFloat(currentValue) <= maxCurrent + 0.01) {
    currentSelect.value = currentValue;
  }
  
  updateApplyButtonState();
}

function updateVoltageOptions() {
  const voltageSelect = document.getElementById('voltageSelect');
  const currentSelect = document.getElementById('currentSelect');
  const selectedCurrent = parseFloat(currentSelect.value);
  
  // Save current voltage selection
  const voltageValue = voltageSelect.value;
  
  if (!selectedCurrent || isNaN(selectedCurrent)) {
    // If no current selected, show all voltages
    voltageSelect.innerHTML = '<option value="">Select voltage...</option>';
    const allVoltages = [5.0, 9.0, 12.0, 15.0, 20.0];
    allVoltages.forEach(voltage => {
      const option = document.createElement('option');
      option.value = voltage;
      option.text = voltage + ' V';
      voltageSelect.appendChild(option);
    });
    
    // Restore selection if it was valid
    if (voltageValue) {
      voltageSelect.value = voltageValue;
    }
    
    updateApplyButtonState();
    return;
  }
  
  // Clear and repopulate based on current
  voltageSelect.innerHTML = '<option value="">Select voltage...</option>';
  
  // Define which voltages can support the selected current
  const voltageCurrentLimits = {
    5.0: 3.0,   // 5V can do up to 3A
    9.0: 2.25,  // 9V can do up to 2.25A
    12.0: 1.67, // 12V can do up to 1.67A
    15.0: 1.33, // 15V can do up to 1.33A
    20.0: 1.0   // 20V can do up to 1A
  };
  
  Object.entries(voltageCurrentLimits).forEach(([voltage, maxCurrent]) => {
    if (selectedCurrent <= maxCurrent + 0.01) { // Small tolerance
      const option = document.createElement('option');
      option.value = parseFloat(voltage);
      option.text = voltage + ' V';
      voltageSelect.appendChild(option);
    }
  });
  
  // Restore selection if still valid
  if (voltageValue && voltageCurrentLimits[voltageValue] && selectedCurrent <= voltageCurrentLimits[voltageValue] + 0.01) {
    voltageSelect.value = voltageValue;
  }
  
  updateApplyButtonState();
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
document.addEventListener('DOMContentLoaded', function() {
  const voltageSelect = document.getElementById('voltageSelect');
  const currentSelect = document.getElementById('currentSelect');
  
  if (voltageSelect) voltageSelect.addEventListener('change', updateApplyButtonState);
  if (currentSelect) currentSelect.addEventListener('change', updateApplyButtonState);
});

// Apply button click handler
document.addEventListener('DOMContentLoaded', function() {
  const applyBtn = document.getElementById('applyBtn');
  if (applyBtn) {
    applyBtn.addEventListener('click', async function() {
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
      
      try {
        const data = await AuthUtils.fetchJSON('api/configure', {
          method: 'POST',
          body: JSON.stringify({
            voltage: parseFloat(voltage),
            current: parseFloat(current)
          })
        });
        
        if (data.success) {
          // Update displayed values
          document.getElementById('currentVoltage').innerText = data.voltage;
          document.getElementById('currentCurrent').innerText = data.current;
          document.getElementById('currentPower').innerText = (data.voltage * data.current).toFixed(2);
          
          // Display success message
          document.getElementById('statusMessage').className = 'status-message success';
          document.getElementById('statusMessage').innerText = 'Settings applied successfully';
          
          // Show success notification using UIUtils
          UIUtils.showAlert('Success', 'USB PD settings applied successfully', 'success');
          
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
          
          // Show error notification using UIUtils
          UIUtils.showAlert('Error', data.message, 'error');
          
          // If board not connected, hide form
          if (data.message === 'PD board not connected') {
            document.getElementById('configSection').classList.add('hidden');
          }
          
          // Refresh the current status
          setTimeout(fetchCurrentConfig, 3000);
        }
        setFormEnabled(true);
        updateApplyButtonState(); // Re-check button state after re-enabling
      } catch (error) {
        console.error('Error:', error);
        document.getElementById('statusMessage').className = 'status-message error';
        document.getElementById('statusMessage').innerText = 'Failed to apply configuration: ' + error.message;
        document.getElementById('retryContainer').classList.remove('hidden');
        setFormEnabled(true);
        updateApplyButtonState(); // Re-check button state after re-enabling
        
        // Show error notification using UIUtils
        UIUtils.showAlert('Error', 'Failed to apply configuration', 'error');
        
        // Refresh the current status after a delay
        setTimeout(fetchCurrentConfig, 3000);
      }
    });
  }
  
  // Cancel button click handler
  const cancelBtn = document.getElementById('cancelBtn');
  if (cancelBtn) {
    cancelBtn.addEventListener('click', function() {
      fetchCurrentConfig(); // Reset dropdowns to current values
    });
  }
  
  // Retry button click handler
  const retryBtn = document.getElementById('retryBtn');
  if (retryBtn) {
    retryBtn.addEventListener('click', function() {
      fetchCurrentConfig(); // Try to reconnect and get status
      loadPDOProfiles();
    });
  }
  
  // PDO refresh button click handler
  const refreshPDOBtn = document.getElementById('refreshPDOBtn');
  if (refreshPDOBtn) {
    refreshPDOBtn.addEventListener('click', function() {
      loadPDOProfiles();
    });
  }
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
async function loadPDOProfiles() {
  try {
    const data = await AuthUtils.fetchJSON('api/profiles');
    const container = document.getElementById('pdoProfiles');
    
    if (data.error) {
      container.innerHTML = '<div class="info-message">' + data.error + '</div>';
      return;
    }
    
    // Check if pdos array exists and is not empty
    if (!data.pdos || !Array.isArray(data.pdos) || data.pdos.length === 0) {
      container.innerHTML = '<div class="info-message">No PDO profiles available. Device may be disconnected.</div>';
      return;
    }
    
    let html = '';
    
    data.pdos.forEach(pdo => {
      const cardClass = pdo.active ? 'pdo-card active' : (pdo.fixed ? 'pdo-card fixed' : 'pdo-card');
      const badgeClass = pdo.active ? 'pdo-badge active' : (pdo.fixed ? 'pdo-badge fixed' : 'pdo-badge');
      const badgeText = pdo.active ? 'ACTIVE' : (pdo.fixed ? 'FIXED' : 'CONFIGURED');
      
      html += `
        <div class="${cardClass}">
          <div class="pdo-header">
            PDO${pdo.number}
            <span class="${badgeClass}">${badgeText}</span>
          </div>
          <div class="pdo-details">
            <div><strong>${pdo.voltage}V</strong> @ <strong>${pdo.current}A</strong></div>
            <div>Max Power: <strong>${pdo.power.toFixed(1)}W</strong></div>
            ${pdo.fixed ? '<div><em>Fixed 5V USB-C standard</em></div>' : ''}
          </div>
        </div>
      `;
    });
    
    container.innerHTML = html;
  } catch (error) {
    console.error('Error loading PDO profiles:', error);
    document.getElementById('pdoProfiles').innerHTML = 
      '<div class="error-message">Failed to load PDO profiles: ' + error.message + '</div>';
  }
}
)rawliteral";

#endif // USB_PD_JS_H