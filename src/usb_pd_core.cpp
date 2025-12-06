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
  // Refactored to minimize synthetic branch inflation from many String +/+= operations.
  // Using a fixed-size stack buffer and snprintf reduces exception/alloc branches counted by gcov.
  // Max length estimation (worst case):
  //   Per PDO object ~70 chars * 3 + wrapper ~30 < 250. Use 256 for safety.
  char buf[256];
  int pos = 0;
  auto append = [&](int written) {
    if (written < 0 || written >= (int)sizeof(buf) - pos) {
      // Truncate safely if overflow would occur; ensure valid JSON termination later.
      pos = (int)sizeof(buf) - 1;
    } else {
      pos += written;
    }
  };

  append(snprintf(buf + pos, sizeof(buf) - pos, "{\"pdos\":["));
  int activePdo = chip.getPdoNumber();
  for (int i = 1; i <= 3; ++i) {
    if (i > 1) {
      append(snprintf(buf + pos, sizeof(buf) - pos, ","));
    }
    float v = chip.getVoltage(i);
    float c = chip.getCurrent(i);
    float pwr = v * c;
    bool active = (activePdo == i);
    append(snprintf(buf + pos, sizeof(buf) - pos,
                   "{\"number\":%d,\"voltage\":%.3g,\"current\":%.3g,\"power\":%.3g,\"active\":%s%s}",
                   i, v, c, pwr, active ? "true" : "false", i == 1 ? ",\"fixed\":true" : ""));
  }
  append(snprintf(buf + pos, sizeof(buf) - pos, "],\"activePDO\":%d}", activePdo));
  // Ensure null termination
  buf[sizeof(buf) - 1] = '\0';
  return String(buf);
}
