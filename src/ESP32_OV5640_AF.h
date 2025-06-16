#ifndef ESP32_OV5640_AF_MANUAL_H
#define ESP32_OV5640_AF_MANUAL_H

#include "ESP32_OV5640_AF.h"

// VCM Control Registers
#define OV5640_REG_VCM_CONTROL_1    0x3603
#define OV5640_REG_VCM_CONTROL_0    0x3602
#define OV5640_REG_VCM_CONTROL_2    0x3604
#define OV5640_REG_VCM_CONTROL_3    0x3605
#define OV5640_REG_VCM_CONTROL_4    0x3606

// Focus positions
#define OV5640_FOCUS_MIN            0      // Nearest focus
#define OV5640_FOCUS_MAX            1023   // Farthest focus
#define OV5640_FOCUS_DEFAULT        512    // Mid-range focus

// Slew rate modes for focus movement speed
#define OV5640_SLEW_DIRECT          0x00   // Direct jump mode
#define OV5640_SLEW_SINGLE_50US     0x01   // 50µs steps
#define OV5640_SLEW_SINGLE_100US    0x02   // 100µs steps
#define OV5640_SLEW_SINGLE_200US    0x03   // 200µs steps
#define OV5640_SLEW_SINGLE_400US    0x04   // 400µs steps
#define OV5640_SLEW_SINGLE_800US    0x05   // 800µs steps
#define OV5640_SLEW_SINGLE_1600US   0x06   // 1600µs steps
#define OV5640_SLEW_SINGLE_3200US   0x07   // 3200µs steps

class OV5640_Manual : public OV5640 {
public:
    OV5640_Manual();
    
    // Manual focus functions
    bool initManualFocus();
    bool setManualFocus(uint16_t position);
    bool setManualFocus(uint16_t position, uint8_t slew_rate);
    uint16_t getManualFocus();
    bool releaseManualFocus();
    bool enableVCMPower(bool enable);
    
    // Focus convenience functions
    bool setFocusNear();
    bool setFocusFar();
    bool setFocusMid();
    bool focusStep(int8_t steps);
    
    // Focus calibration helpers
    bool calibrateNearLimit();
    bool calibrateFarLimit();
    uint16_t findOptimalFocus(uint8_t window_size = 5);
    
private:
    uint16_t current_focus_position;
    uint16_t near_limit;
    uint16_t far_limit;
    bool vcm_powered;
    
    bool writeVCMRegister(uint16_t reg, uint8_t value);
    uint8_t readVCMRegister(uint16_t reg);
    bool waitForFocusComplete(uint32_t timeout_ms = 1000);
    uint32_t calculateSharpness();
};

#endif
