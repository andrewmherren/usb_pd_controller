#include "../include/usb_pd_core.h"

bool USBPDCore::readConfig(float &voltageOut, float &currentOut,
                           int &activePdoOut) {
  chip.read();
  int pdo = chip.getPdoNumber();
  float v = chip.getVoltage(pdo);
  float c = chip.getCurrent(pdo);
  if (v <= 0 || c <= 0) {
    return false;
  }
  cachedPdo = pdo;
  cachedVoltage = v;
  cachedCurrent = c;
  voltageOut = v;
  currentOut = c;
  activePdoOut = pdo;
  return true;
}

bool USBPDCore::setConfig(float voltage, float current) {
  // Read current to ensure chip state
  chip.read();

  if (voltage == 5.0f) {
    chip.setCurrent(1, current);
    chip.setPdoNumber(1);
  } else if (voltage <= 12.0f) {
    chip.setVoltage(2, voltage);
    chip.setCurrent(2, current);
    chip.setCurrent(1, current); // fallback PDO1
    chip.setPdoNumber(2);
  } else {
    chip.setVoltage(3, voltage);
    chip.setCurrent(3, current);
    chip.setVoltage(2, 12.0f); // middle fallback
    chip.setCurrent(2, current);
    chip.setCurrent(1, current); // final fallback
    chip.setPdoNumber(3);
  }

  chip.write();
  chip.softReset();

  // Small settle assumed by caller; read back
  float v, c;
  int p;
  if (!readConfig(v, c, p))
    return false;
  return true;
}

String USBPDCore::buildPdoProfilesJson() const {
  String json = "{\"pdos\":[";
  for (int i = 1; i <= 3; ++i) {
    if (i > 1)
      json += ",";
    float v = chip.getVoltage(i);
    float c = chip.getCurrent(i);
    bool active = (chip.getPdoNumber() == i);
    json += "{";
    json += "\"number\":" + String(i) + ",";
    json += "\"voltage\":" + String(v) + ",";
    json += "\"current\":" + String(c) + ",";
    json += "\"power\":" + String(v * c) + ",";
    json += "\"active\":" + String(active ? "true" : "false");
    if (i == 1) {
      json += ",\"fixed\":true";
    }
    json += "}";
  }
  json += "],\"activePDO\":" + String(chip.getPdoNumber()) + "}";
  return json;
}
