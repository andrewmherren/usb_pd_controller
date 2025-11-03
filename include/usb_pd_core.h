#ifndef USB_PD_CORE_H
#define USB_PD_CORE_H

#include <interface/string_compat.h>
#include <usb_pd_chip.h>

// Core, Arduino-free logic for configuring a USB-PD chip.
// This can be tested in native builds with a fake IUsbPdChip.
class USBPDCore {
public:
  explicit USBPDCore(IUsbPdChip &chip) : chip(chip) {}

  // Reads current configuration from the chip
  bool readConfig(float &voltageOut, float &currentOut, int &activePdoOut);

  // Set target voltage/current with fallback PDO strategy, commits to device
  bool setConfig(float voltage, float current);

  // Build a compact JSON string describing all 3 PDOs and active PDO
  String buildPdoProfilesJson() const;

  // Update cached readings (must call readConfig first or setConfig success)
  float currentVoltage() const { return cachedVoltage; }
  float currentCurrent() const { return cachedCurrent; }
  int activePdo() const { return cachedPdo; }

private:
  IUsbPdChip &chip;
  float cachedVoltage = 0.0f;
  float cachedCurrent = 0.0f;
  int cachedPdo = 0;
};

#endif // USB_PD_CORE_H
