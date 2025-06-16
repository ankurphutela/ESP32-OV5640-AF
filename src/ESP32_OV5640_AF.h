/*
  ESP32_OV5640_AF.h - Library for OV5640 Auto Focus (ESP32 Camera)
  Modified version with Manual Focus support
  Original Created by Eric Nam, December 23, 2021.
  Released into the public domain.
*/

#ifndef ESP32_OV5640_AF_h
#define ESP32_OV5640_AF_h

#include "Arduino.h"
#include "sensor.h"

// Focus modes
typedef enum {
    AF_MODE_AUTO_CONTINUOUS = 0,
    AF_MODE_AUTO_SINGLE = 1,
    AF_MODE_MANUAL = 2,
    AF_MODE_RELEASED = 3
} focus_mode_t;

// Focus status
typedef enum {
    AF_STATUS_IDLE = 0,
    AF_STATUS_FOCUSING = 1,
    AF_STATUS_FOCUSED = 2,
    AF_STATUS_NOT_FOCUSED = 3
} focus_status_t;

class OV5640
{
  public:
    OV5640();
    
    // Original functions
    void start(sensor_t* _sensor);
    byte sendCommand();
    byte getFWStatus();
    byte focusInit();
    byte autoFocusMode();
    byte infinityFocusMode();
    
    // New manual focus functions
    byte setFocusMode(focus_mode_t mode);
    byte setManualFocus(uint16_t focus_value);
    byte triggerSingleAutoFocus();
    byte releaseFocus();
    uint16_t getCurrentFocusValue();
    focus_status_t getFocusStatus();
    
    // Focus range functions
    byte calibrateFocusRange();
    uint16_t getMinFocusValue();
    uint16_t getMaxFocusValue();
    uint16_t mapDistanceToFocus(uint16_t distance_mm);
    
  private:
    sensor_t* sensor;
    focus_mode_t current_mode;
    uint16_t current_focus_value;
    uint16_t min_focus_value;
    uint16_t max_focus_value;
    
    // Internal functions
    byte writeReg16(uint16_t reg, uint16_t value);
    uint16_t readReg16(uint16_t reg);
};

#endif
