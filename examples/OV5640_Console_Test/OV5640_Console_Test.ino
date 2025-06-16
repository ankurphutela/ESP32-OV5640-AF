/*
  Simple Focus Test for OV5640
  Quick test to verify manual focus is working
*/

#include "esp_camera.h"
#include "ESP32_OV5640_AF.h"

// Camera pins - ADJUST FOR YOUR BOARD!
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

OV5640 ov5640 = OV5640();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nOV5640 Manual Focus Test");
  
  // Camera config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;
  
  // Init camera
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed!");
    return;
  }
  Serial.println("Camera initialized");
  
  // Start autofocus
  sensor_t* sensor = esp_camera_sensor_get();
  ov5640.start(sensor);
  
  if (ov5640.focusInit() == 0) {
    Serial.println("Focus init successful!");
  } else {
    Serial.println("Focus init failed!");
    return;
  }
  
  // Test sequence
  Serial.println("\nStarting focus test sequence...\n");
  
  // Test 1: Read current focus
  uint16_t currentFocus = ov5640.getCurrentFocusValue();
  Serial.printf("Current focus value: 0x%04X (%d)\n\n", currentFocus, currentFocus);
  
  // Test 2: Set different focus values
  Serial.println("Testing manual focus at different values:");
  
  uint16_t testValues[] = {0x0000, 0x0040, 0x0080, 0x0100, 0x0180, 0x01FF};
  const char* descriptions[] = {"Infinity", "Far", "Medium", "Near", "Close", "Macro"};
  
  for (int i = 0; i < 6; i++) {
    Serial.printf("Setting focus to %s (0x%04X)... ", descriptions[i], testValues[i]);
    
    if (ov5640.setManualFocus(testValues[i]) == 0) {
      Serial.print("OK");
      delay(500);  // Wait for focus to settle
      
      // Verify the focus was set
      uint16_t readback = ov5640.getCurrentFocusValue();
      Serial.printf(" - Readback: 0x%04X\n", readback);
    } else {
      Serial.println("FAILED");
    }
  }
  
  // Test 3: Single autofocus
  Serial.println("\nTesting single autofocus...");
  if (ov5640.triggerSingleAutoFocus() == 0) {
    uint16_t afValue = ov5640.getCurrentFocusValue();
    Serial.printf("Autofocus successful! Focus value: 0x%04X (%d)\n", afValue, afValue);
  } else {
    Serial.println("Autofocus failed!");
  }
  
  Serial.println("\nFocus test complete!");
  Serial.println("You can now enter focus values (0-511) via serial monitor");
}

void loop() {
  if (Serial.available()) {
    int focusValue = Serial.parseInt();
    if (focusValue >= 0 && focusValue <= 511) {
      Serial.printf("Setting focus to: %d (0x%04X)\n", focusValue, focusValue);
      
      if (ov5640.setManualFocus(focusValue) == 0) {
        delay(100);
        uint16_t actual = ov5640.getCurrentFocusValue();
        Serial.printf("Focus set. Actual value: %d (0x%04X)\n", actual, actual);
      } else {
        Serial.println("Failed to set focus!");
      }
    }
  }
}
