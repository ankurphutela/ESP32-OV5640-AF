/*
  ESP32_OV5640_AF.cpp - Library for OV5640 Auto Focus (ESP32 Camera)
  Modified version with Manual Focus support
  Original Created by Eric Nam, December 23, 2021.
  Released into the public domain.
*/

#include "Arduino.h"
#include "ESP32_OV5640_AF.h"
#include "OV5640_AF_firmware.h"

OV5640::OV5640()
{
  sensor = NULL;
  current_mode = AF_MODE_RELEASED;
  current_focus_value = 0x0080;  // Default mid-range
  min_focus_value = 0x0000;      // Infinity
  max_focus_value = 0x01FF;      // Macro
}

void OV5640::start(sensor_t* _sensor)
{
  sensor = _sensor;
}

byte OV5640::sendCommand()
{
  if (!sensor) return -1;
  
  byte rdVal;
  sensor->set_reg(sensor, 0x3022, 0xff, 0);
  sensor->set_reg(sensor, 0x3023, 0xff, 0);
  rdVal = sensor->get_reg(sensor, 0x3023, 0xff);
  
  if (rdVal != 0) {
    return -1;
  } else {
    return 0;
  }
}

byte OV5640::getFWStatus()
{
  if (!sensor) return -1;
  
  byte rdVal;
  sensor->set_reg(sensor, 0x3029, 0xff, 0x7f);
  rdVal = sensor->get_reg(sensor, 0x3029, 0xff);
  
  if (rdVal != 0x70) {
    return -1;
  } else {
    return 0;
  }
}

byte OV5640::focusInit()
{
  if (!sensor) return -1;
  
  // Load AF firmware
  for (uint16_t i = 0; i < sizeof(OV5640_AF_firmware); i++) {
    sensor->set_reg(sensor, 0x8000 + i, 0xff, OV5640_AF_firmware[i]);
  }
  
  // Enable AF
  sensor->set_reg(sensor, 0x3000, 0x20, 0x20);
  sensor->set_reg(sensor, 0x3022, 0xff, 0x00);
  
  // Wait for firmware initialization
  delay(10);
  
  return getFWStatus();
}

byte OV5640::autoFocusMode()
{
  if (!sensor) return -1;
  
  // Set continuous auto focus mode
  sensor->set_reg(sensor, 0x3022, 0xff, 0x04);
  sensor->set_reg(sensor, 0x3023, 0xff, 0x00);
  sensor->set_reg(sensor, 0x3024, 0xff, 0x00);
  sensor->set_reg(sensor, 0x3025, 0xff, 0x00);
  sensor->set_reg(sensor, 0x3026, 0xff, 0x00);
  sensor->set_reg(sensor, 0x3027, 0xff, 0x00);
  sensor->set_reg(sensor, 0x3028, 0xff, 0x00);
  sensor->set_reg(sensor, 0x3029, 0xff, 0x7f);
  sensor->set_reg(sensor, 0x302a, 0xff, 0x00);
  
  current_mode = AF_MODE_AUTO_CONTINUOUS;
  return sendCommand();
}

byte OV5640::infinityFocusMode()
{
  return setManualFocus(min_focus_value);
}

// New manual focus implementation
byte OV5640::setFocusMode(focus_mode_t mode)
{
  if (!sensor) return -1;
  
  current_mode = mode;
  
  switch (mode) {
    case AF_MODE_AUTO_CONTINUOUS:
      return autoFocusMode();
      
    case AF_MODE_AUTO_SINGLE:
      return triggerSingleAutoFocus();
      
    case AF_MODE_MANUAL:
      return releaseFocus();
      
    case AF_MODE_RELEASED:
      return releaseFocus();
      
    default:
      return -1;
  }
}

byte OV5640::setManualFocus(uint16_t focus_value)
{
  if (!sensor) return -1;
  
  // Constrain focus value to valid range
  if (focus_value > max_focus_value) {
    focus_value = max_focus_value;
  }
  
  // Release autofocus first
  releaseFocus();
  
  // Set focus position registers
  sensor->set_reg(sensor, 0x3602, 0xff, (focus_value >> 8) & 0xff);
  sensor->set_reg(sensor, 0x3603, 0xff, focus_value & 0xff);
  
  // Trigger manual focus
  sensor->set_reg(sensor, 0x3022, 0xff, 0x03);
  sensor->set_reg(sensor, 0x3023, 0xff, 0x01);
  
  // Store current value
  current_focus_value = focus_value;
  current_mode = AF_MODE_MANUAL;
  
  delay(10);  // Small delay for focus adjustment
  
  return sendCommand();
}

byte OV5640::triggerSingleAutoFocus()
{
  if (!sensor) return -1;
  
  // Release any current focus
  releaseFocus();
  
  // Trigger single AF
  sensor->set_reg(sensor, 0x3022, 0xff, 0x03);
  sensor->set_reg(sensor, 0x3023, 0xff, 0x01);
  
  current_mode = AF_MODE_AUTO_SINGLE;
  
  // Wait for focus to complete (max 500ms)
  for (int i = 0; i < 50; i++) {
    if (getFocusStatus() == AF_STATUS_FOCUSED) {
      // Read back the focus value
      current_focus_value = getCurrentFocusValue();
      return 0;
    }
    delay(10);
  }
  
  return -1;  // Focus timeout
}

byte OV5640::releaseFocus()
{
  if (!sensor) return -1;
  
  // Release focus command
  sensor->set_reg(sensor, 0x3022, 0xff, 0x08);
  sensor->set_reg(sensor, 0x3023, 0xff, 0x01);
  
  current_mode = AF_MODE_RELEASED;
  
  return sendCommand();
}

uint16_t OV5640::getCurrentFocusValue()
{
  if (!sensor) return 0;
  
  uint8_t high = sensor->get_reg(sensor, 0x3602, 0xff);
  uint8_t low = sensor->get_reg(sensor, 0x3603, 0xff);
  
  return (high << 8) | low;
}

focus_status_t OV5640::getFocusStatus()
{
  if (!sensor) return AF_STATUS_IDLE;
  
  uint8_t status = sensor->get_reg(sensor, 0x3029, 0xff);
  
  // Decode status bits
  if (status & 0x10) {
    return AF_STATUS_FOCUSED;
  } else if (status & 0x20) {
    return AF_STATUS_FOCUSING;
  } else if (status & 0x40) {
    return AF_STATUS_NOT_FOCUSED;
  }
  
  return AF_STATUS_IDLE;
}

byte OV5640::calibrateFocusRange()
{
  if (!sensor) return -1;
  
  // Find minimum focus (infinity)
  setManualFocus(0x0000);
  delay(100);
  min_focus_value = getCurrentFocusValue();
  
  // Find maximum focus (macro)
  setManualFocus(0x01FF);
  delay(100);
  max_focus_value = getCurrentFocusValue();
  
  // Return to middle focus
  setManualFocus((min_focus_value + max_focus_value) / 2);
  
  return 0;
}

uint16_t OV5640::getMinFocusValue()
{
  return min_focus_value;
}

uint16_t OV5640::getMaxFocusValue()
{
  return max_focus_value;
}

uint16_t OV5640::mapDistanceToFocus(uint16_t distance_mm)
{
  // Map distance in mm to focus value
  // This is approximate and may need calibration
  // Close distance = higher focus value
  // Far distance = lower focus value
  
  if (distance_mm < 100) {
    // Macro range
    return max_focus_value;
  } else if (distance_mm > 10000) {
    // Infinity
    return min_focus_value;
  } else {
    // Linear interpolation (inverse relationship)
    // You may want to use a logarithmic scale for better results
    uint32_t range = max_focus_value - min_focus_value;
    uint32_t mapped = range - (range * (distance_mm - 100) / 9900);
    return min_focus_value + mapped;
  }
}

// Internal helper functions
byte OV5640::writeReg16(uint16_t reg, uint16_t value)
{
  if (!sensor) return -1;
  
  sensor->set_reg(sensor, reg, 0xff, (value >> 8) & 0xff);
  sensor->set_reg(sensor, reg + 1, 0xff, value & 0xff);
  
  return 0;
}

uint16_t OV5640::readReg16(uint16_t reg)
{
  if (!sensor) return 0;
  
  uint8_t high = sensor->get_reg(sensor, reg, 0xff);
  uint8_t low = sensor->get_reg(sensor, reg + 1, 0xff);
  
  return (high << 8) | low;
}
