OV5640 Autofocus Register Documentation
Key Registers for Manual Focus Control
Command Registers
0x3022 - AF Command Register
Main command register for autofocus control:

0x00: No operation
0x03: Single autofocus trigger
0x04: Continuous autofocus mode
0x08: Release focus (manual mode)

0x3023 - AF Trigger Register
Secondary control register:

0x00: Normal operation
0x01: Trigger/confirm command

Status Registers
0x3029 - AF Status Register
Reports autofocus status:

Bit 4 (0x10): Focus achieved
Bit 5 (0x20): Focusing in progress
Bit 6 (0x40): Focus failed
Value 0x70: Firmware running correctly

Focus Position Registers
0x3602-0x3603 - Focus Position (16-bit)
Manual focus position control:

0x3602: High byte of focus position
0x3603: Low byte of focus position
Range: 0x0000 (infinity) to 0x01FF (macro)

Usage Examples
Set Manual Focus to Mid-Range
cpp// Release autofocus
sensor->set_reg(sensor, 0x3022, 0xff, 0x08);
sensor->set_reg(sensor, 0x3023, 0xff, 0x01);

// Set focus position to 0x0080
sensor->set_reg(sensor, 0x3602, 0xff, 0x00);  // High byte
sensor->set_reg(sensor, 0x3603, 0xff, 0x80);  // Low byte

// Trigger manual focus
sensor->set_reg(sensor, 0x3022, 0xff, 0x03);
sensor->set_reg(sensor, 0x3023, 0xff, 0x01);
Read Current Focus Position
cppuint8_t high = sensor->get_reg(sensor, 0x3602, 0xff);
uint8_t low = sensor->get_reg(sensor, 0x3603, 0xff);
uint16_t focus_value = (high << 8) | low;
Check Focus Status
cppuint8_t status = sensor->get_reg(sensor, 0x3029, 0xff);
if (status & 0x10) {
    // Focus achieved
} else if (status & 0x20) {
    // Focusing in progress
} else if (status & 0x40) {
    // Focus failed
}
Focus Value Calibration
Different camera modules may have different focus ranges. Common ranges:
Module TypeMin (Infinity)Max (Macro)Standard0x00000x01FFSome modules0x00100x01F0Limited range0x00200x0180
Always calibrate your specific module to determine its actual range.
Timing Considerations

After setting focus position, allow 10-50ms for mechanical movement
Single autofocus can take 100-500ms to complete
Check status register to confirm operation completion

Troubleshooting
Focus Commands Not Working

Ensure AF firmware is loaded (check 0x3029 == 0x70)
Verify AF-VCC is powered (2.8V or 3.3V)
Try releasing focus before setting new position

Focus Position Not Changing

Some positions may be outside module's physical range
Motor may be stuck - try full range sweep
Check current draw on AF-VCC (should spike during movement)

Inconsistent Focus

Allow settling time after movement
Temperature changes can affect focus
Some modules have backlash - approach from same direction
