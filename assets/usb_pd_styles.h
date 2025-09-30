#ifndef USB_PD_STYLES_H
#define USB_PD_STYLES_H

#include <Arduino.h>

// USB-PD Controller specific styles
const char USB_PD_STYLES_CSS[] PROGMEM = R"css(
/* Special USB-PD card types */
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

/* Refresh button modifier */
.refresh-button {
  margin-left: 10px;
  padding: 8px 16px;
  font-size: 0.8em;
}

/* Fuel Gauge Indicators - Useful for power monitoring */
.gauge-container {
  margin: 15px 0;
}

.gauge {
  position: relative;
  width: 120px;
  height: 60px;
  margin: 0 auto 10px;
  overflow: hidden;
}

.gauge .gauge-svg {
  width: 120px;
  height: 60px;
  position: absolute;
  top: 0;
  left: 0;
}

.gauge .gauge-arc {
  stroke: #4CAF50;
  fill: none;
  stroke-width: 12;
  stroke-linecap: round;
  transform-origin: center;
}

.gauge-label {
  position: absolute;
  top: 35px;
  left: 50%;
  transform: translateX(-50%);
  text-align: center;
  font-weight: bold;
  font-size: 14px;
  color: #fff;
}

.gauge-text {
  text-align: center;
  font-size: 12px;
  color: rgba(255, 255, 255, 0.8);
  margin-top: 5px;
}

/* Status value styling with gauges */
.status-value {
  color: rgba(255, 255, 255, 0.9);
  font-weight: 500;
  padding: 5px 0;
}

.status-value.with-gauge {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.gauge-inline {
  width: 60px;
  height: 30px;
  position: relative;
  margin-left: 15px;
}

.gauge-svg {
  width: 100%;
  height: 100%;
  position: absolute;
  top: 0;
  left: 0;
}

.gauge-bg {
  stroke: rgba(255, 255, 255, 0.2);
  fill: none;
  stroke-width: 15;
  stroke-linecap: round;
}

.gauge-arc {
  stroke: #4CAF50;
  fill: none;
  stroke-width: 15;
  stroke-linecap: round;
  transform-origin: center;
}

.gauge-inline .gauge-label {
  top: 18px;
  font-size: 10px;
}
)css";

#endif // USB_PD_STYLES_H