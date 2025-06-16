#include "ESP32_OV5640_AF_Manual.h"

OV5640_Manual::OV5640_Manual() : OV5640() {
    current_focus_position = OV5640_FOCUS_DEFAULT;
    near_limit = OV5640_FOCUS_MIN;
    far_limit = OV5640_FOCUS_MAX;
    vcm_powered = false;
}

bool OV5640_Manual::initManualFocus() {
    // Initialize VCM control
    
    // Set VCM clock divider for 20kHz operation
    // Assuming 24MHz input clock: divider = 24000000 / 20000 = 1200
    if (!writeVCMRegister(OV5640_REG_VCM_CONTROL_3, 0x04)) return false; // High byte
    if (!writeVCMRegister(OV5640_REG_VCM_CONTROL_2, 0xB0)) return false; // Low byte
    
    // Set VCM current control (1.0x current)
    if (!writeVCMRegister(OV5640_REG_VCM_CONTROL_4, 0x04)) return false;
    
    // Enable VCM power
    if (!enableVCMPower(true)) return false;
    
    // Set initial focus position
    if (!setManualFocus(OV5640_FOCUS_DEFAULT, OV5640_SLEW_SINGLE_200US)) return false;
    
    delay(100); // Allow time for initial positioning
    
    return true;
}

bool OV5640_Manual::setManualFocus(uint16_t position) {
    return setManualFocus(position, OV5640_SLEW_SINGLE_200US);
}

bool OV5640_Manual::setManualFocus(uint16_t position, uint8_t slew_rate) {
    if (position > OV5640_FOCUS_MAX) {
        position = OV5640_FOCUS_MAX;
    }
    
    if (!vcm_powered) {
        if (!enableVCMPower(true)) return false;
    }
    
    // Split 10-bit position into high and low parts
    uint8_t position_high = (position >> 4) & 0x3F;  // Bits [9:4] in lower 6 bits
    uint8_t position_low = (position & 0x0F) << 4;   // Bits [3:0] in upper 4 bits
    
    // Set VCM control register 1 (high bits + power control)
    uint8_t vcm_ctrl1 = position_high;  // Bit[7] = 0 (power on), Bit[5:0] = position[9:4]
    if (!writeVCMRegister(OV5640_REG_VCM_CONTROL_1, vcm_ctrl1)) return false;
    
    // Set VCM control register 0 (low bits + slew rate)
    uint8_t vcm_ctrl0 = position_low | (slew_rate & 0x0F);
    if (!writeVCMRegister(OV5640_REG_VCM_CONTROL_0, vcm_ctrl0)) return false;
    
    current_focus_position = position;
    
    // Wait for focus movement to complete
    return waitForFocusComplete();
}

uint16_t OV5640_Manual::getManualFocus() {
    if (!vcm_powered) return 0;
    
    uint8_t high = readVCMRegister(OV5640_REG_VCM_CONTROL_1);
    uint8_t low = readVCMRegister(OV5640_REG_VCM_CONTROL_0);
    
    uint16_t position = ((high & 0x3F) << 4) | ((low >> 4) & 0x0F);
    current_focus_position = position;
    
    return position;
}

bool OV5640_Manual::releaseManualFocus() {
    // Power down VCM
    return enableVCMPower(false);
}

bool OV5640_Manual::enableVCMPower(bool enable) {
    uint8_t vcm_ctrl1 = readVCMRegister(OV5640_REG_VCM_CONTROL_1);
    
    if (enable) {
        vcm_ctrl1 &= ~0x80;  // Clear bit 7 (power on)
        vcm_powered = true;
    } else {
        vcm_ctrl1 |= 0x80;   // Set bit 7 (power down)
        vcm_powered = false;
    }
    
    return writeVCMRegister(OV5640_REG_VCM_CONTROL_1, vcm_ctrl1);
}

bool OV5640_Manual::setFocusNear() {
    return setManualFocus(near_limit);
}

bool OV5640_Manual::setFocusFar() {
    return setManualFocus(far_limit);
}

bool OV5640_Manual::setFocusMid() {
    uint16_t mid_position = (near_limit + far_limit) / 2;
    return setManualFocus(mid_position);
}

bool OV5640_Manual::focusStep(int8_t steps) {
    int32_t new_position = current_focus_position + (steps * 10);
    
    if (new_position < near_limit) new_position = near_limit;
    if (new_position > far_limit) new_position = far_limit;
    
    return setManualFocus((uint16_t)new_position);
}

bool OV5640_Manual::calibrateNearLimit() {
    // Move to minimum position and test image sharpness
    uint16_t best_position = OV5640_FOCUS_MIN;
    uint32_t best_sharpness = 0;
    
    for (uint16_t pos = OV5640_FOCUS_MIN; pos < OV5640_FOCUS_MIN + 200; pos += 10) {
        setManualFocus(pos);
        delay(100);
        
        uint32_t sharpness = calculateSharpness();
        if (sharpness > best_sharpness) {
            best_sharpness = sharpness;
            best_position = pos;
        }
    }
    
    near_limit = best_position;
    return setManualFocus(near_limit);
}

bool OV5640_Manual::calibrateFarLimit() {
    // Move to maximum position and test image sharpness
    uint16_t best_position = OV5640_FOCUS_MAX;
    uint32_t best_sharpness = 0;
    
    for (uint16_t pos = OV5640_FOCUS_MAX - 200; pos <= OV5640_FOCUS_MAX; pos += 10) {
        setManualFocus(pos);
        delay(100);
        
        uint32_t sharpness = calculateSharpness();
        if (sharpness > best_sharpness) {
            best_sharpness = sharpness;
            best_position = pos;
        }
    }
    
    far_limit = best_position;
    return setManualFocus(far_limit);
}

uint16_t OV5640_Manual::findOptimalFocus(uint8_t window_size) {
    uint16_t best_position = current_focus_position;
    uint32_t best_sharpness = 0;
    
    uint16_t start_pos = (current_focus_position > window_size * 10) ? 
                        current_focus_position - window_size * 10 : near_limit;
    uint16_t end_pos = (current_focus_position < far_limit - window_size * 10) ? 
                      current_focus_position + window_size * 10 : far_limit;
    
    for (uint16_t pos = start_pos; pos <= end_pos; pos += 5) {
        setManualFocus(pos);
        delay(50);
        
        uint32_t sharpness = calculateSharpness();
        if (sharpness > best_sharpness) {
            best_sharpness = sharpness;
            best_position = pos;
        }
    }
    
    setManualFocus(best_position);
    return best_position;
}

// Private helper functions
bool OV5640_Manual::writeVCMRegister(uint16_t reg, uint8_t value) {
    return wrSensorReg16_8(reg, value) == 0;
}

uint8_t OV5640_Manual::readVCMRegister(uint16_t reg) {
    uint8_t value = 0;
    rdSensorReg16_8(reg, &value);
    return value;
}

bool OV5640_Manual::waitForFocusComplete(uint32_t timeout_ms) {
    uint32_t start_time = millis();
    
    // For manual focus, we just wait a reasonable time based on slew rate
    // In a real implementation, you might monitor focus status registers
    delay(100);  // Basic settling time
    
    return (millis() - start_time) < timeout_ms;
}

uint32_t OV5640_Manual::calculateSharpness() {
    // Simplified sharpness calculation
    // In a real implementation, you would analyze image data
    // This is a placeholder that returns a random value for demonstration
    return random(1000, 10000);
}
