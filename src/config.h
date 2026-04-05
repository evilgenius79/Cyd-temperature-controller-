#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// Pin Assignments for ESP32-2432S028 (CYD) - Guition board
// ============================================================
//
// GPIO access points on this board:
//   JST connector (bottom-right): GND, IO22, IO27, 3.3V
//   RGB LED pads (back of board):  IO4 (Red), IO16 (Green), IO17 (Blue)
//
// The RGB LED must be desoldered or its traces cut to use
// GPIO 4/16/17 cleanly for MOSFET control.

// DS18B20 Temperature Sensors (OneWire bus)
// Both sensors share one data pin with a 4.7k pull-up to 3.3V
// Accessible on the JST connector
#define ONE_WIRE_BUS 27

// Peltier module PWM control via MOSFET gate pins
// Accessible by soldering to RGB LED pads (Green & Blue)
#define PELTIER_1_PIN 16
#define PELTIER_2_PIN 17

// Fan PWM control via MOSFET gate pins
// Fan 1: JST connector   Fan 2: RGB LED pad (Red)
#define FAN_1_PIN 22
#define FAN_2_PIN 4

// PWM configuration
#define PWM_FREQ 25000       // 25kHz - good for MOSFETs and fan control
#define PWM_RESOLUTION 8     // 8-bit: 0-255
#define PELTIER_1_CH 0
#define PELTIER_2_CH 1
#define FAN_1_CH 2
#define FAN_2_CH 3

// Temperature defaults (Celsius)
#define DEFAULT_TARGET_TEMP 25.0f
#define DEFAULT_HIGH_LIMIT 40.0f
#define DEFAULT_LOW_LIMIT 10.0f
#define TEMP_READ_INTERVAL 1000  // ms between sensor reads

// Display
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Touch calibration (adjust for your specific CYD unit)
#define TOUCH_X_MIN 300
#define TOUCH_X_MAX 3900
#define TOUCH_Y_MIN 300
#define TOUCH_Y_MAX 3900

#endif
