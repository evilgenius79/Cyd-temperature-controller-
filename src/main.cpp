// ============================================================
// CYD Peltier Cooling Controller
// ESP32-2432S028 + DS18B20 + Peltier + Fans
// ============================================================

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

// ---- Hardware objects ----
TFT_eSPI tft = TFT_eSPI();
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ---- State ----
float temp1 = 0.0f, temp2 = 0.0f;           // current readings
float temp1_min = 999.0f, temp1_max = -999.0f;
float temp2_min = 999.0f, temp2_max = -999.0f;
float highLimit = DEFAULT_HIGH_LIMIT;
float lowLimit = DEFAULT_LOW_LIMIT;

uint8_t peltier1_pwm = 0;   // 0-255
uint8_t peltier2_pwm = 0;
uint8_t fan1_pwm = 128;     // fans default 50%
uint8_t fan2_pwm = 128;

bool autoMode = true;        // auto vs manual peltier control
float targetTemp = DEFAULT_TARGET_TEMP;

unsigned long lastTempRead = 0;
unsigned long lastDisplayUpdate = 0;
bool alarmActive = false;

// ---- UI state ----
enum Screen { SCREEN_MAIN, SCREEN_SETTINGS };
Screen currentScreen = SCREEN_MAIN;

// Button regions (x, y, w, h)
struct Button {
    int16_t x, y, w, h;
    const char* label;
};

// Main screen buttons
Button btnPelt1Up   = {230, 40, 40, 30, "+"};
Button btnPelt1Down = {275, 40, 40, 30, "-"};
Button btnPelt2Up   = {230, 75, 40, 30, "+"};
Button btnPelt2Down = {275, 75, 40, 30, "-"};
Button btnFan1Up    = {230, 110, 40, 30, "+"};
Button btnFan1Down  = {275, 110, 40, 30, "-"};
Button btnFan2Up    = {230, 145, 40, 30, "+"};
Button btnFan2Down  = {275, 145, 40, 30, "-"};
Button btnAutoMode  = {10, 180, 90, 30, "AUTO"};
Button btnSettings  = {110, 180, 90, 30, "SETTINGS"};
Button btnResetMinMax = {210, 180, 100, 30, "RST Hi/Lo"};

// Settings screen buttons
Button btnHighUp    = {230, 40, 40, 30, "+"};
Button btnHighDown  = {275, 40, 40, 30, "-"};
Button btnLowUp     = {230, 80, 40, 30, "+"};
Button btnLowDown   = {275, 80, 40, 30, "-"};
Button btnTargetUp  = {230, 120, 40, 30, "+"};
Button btnTargetDown= {275, 120, 40, 30, "-"};
Button btnBack      = {110, 180, 100, 30, "BACK"};

// ---- Color scheme ----
#define CLR_BG        TFT_BLACK
#define CLR_TEXT      TFT_WHITE
#define CLR_LABEL     TFT_CYAN
#define CLR_VALUE     TFT_GREEN
#define CLR_WARN      TFT_YELLOW
#define CLR_ALARM     TFT_RED
#define CLR_BTN       0x4208   // dark gray
#define CLR_BTN_TEXT  TFT_WHITE
#define CLR_HEADER    0x000F   // dark blue
#define CLR_AUTO_ON   TFT_GREEN
#define CLR_AUTO_OFF  TFT_DARKGREY

// ---- Forward declarations ----
void readTemperatures();
void updateOutputs();
void drawMainScreen();
void drawSettingsScreen();
void drawButton(Button& btn, uint16_t bg = CLR_BTN);
bool touchInButton(int tx, int ty, Button& btn);
void handleMainTouch(int tx, int ty);
void handleSettingsTouch(int tx, int ty);
uint16_t tempColor(float t);
void drawBar(int x, int y, int w, int h, uint8_t val, uint16_t color);

// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("CYD Cooling Controller starting...");

    // Init display
    tft.init();
    tft.setRotation(1);  // landscape
    tft.fillScreen(CLR_BG);

    // Init touch
    // Touch is handled by TFT_eSPI's built-in touch driver

    // Init temperature sensors
    sensors.begin();
    int deviceCount = sensors.getDeviceCount();
    Serial.printf("Found %d DS18B20 sensor(s)\n", deviceCount);

    // Init PWM channels
    ledcSetup(PELTIER_1_CH, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(PELTIER_2_CH, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(FAN_1_CH, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(FAN_2_CH, PWM_FREQ, PWM_RESOLUTION);

    ledcAttachPin(PELTIER_1_PIN, PELTIER_1_CH);
    ledcAttachPin(PELTIER_2_PIN, PELTIER_2_CH);
    ledcAttachPin(FAN_1_PIN, FAN_1_CH);
    ledcAttachPin(FAN_2_PIN, FAN_2_CH);

    // Start fans at default
    ledcWrite(FAN_1_CH, fan1_pwm);
    ledcWrite(FAN_2_CH, fan2_pwm);

    // Initial sensor read
    sensors.requestTemperatures();
    delay(750);
    readTemperatures();

    // Draw initial screen
    drawMainScreen();
}

// ============================================================
void loop() {
    unsigned long now = millis();

    // Read sensors periodically
    if (now - lastTempRead >= TEMP_READ_INTERVAL) {
        lastTempRead = now;
        sensors.requestTemperatures();
        readTemperatures();
        updateOutputs();
    }

    // Refresh display at ~4 Hz
    if (now - lastDisplayUpdate >= 250) {
        lastDisplayUpdate = now;
        if (currentScreen == SCREEN_MAIN)
            drawMainScreen();
        else
            drawSettingsScreen();
    }

    // Handle touch
    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty)) {
        // Map touch coords to screen coords
        int sx = tx;
        int sy = ty;

        if (currentScreen == SCREEN_MAIN)
            handleMainTouch(sx, sy);
        else
            handleSettingsTouch(sx, sy);

        // Simple debounce
        delay(200);
    }
}

// ============================================================
void readTemperatures() {
    float t1 = sensors.getTempCByIndex(0);
    float t2 = sensors.getTempCByIndex(1);

    // Validate readings (-127 means disconnected)
    if (t1 > -100.0f) {
        temp1 = t1;
        if (t1 < temp1_min) temp1_min = t1;
        if (t1 > temp1_max) temp1_max = t1;
    }
    if (t2 > -100.0f) {
        temp2 = t2;
        if (t2 < temp2_min) temp2_min = t2;
        if (t2 > temp2_max) temp2_max = t2;
    }

    // Check alarm
    alarmActive = (temp1 > highLimit || temp2 > highLimit ||
                   temp1 < lowLimit  || temp2 < lowLimit);

    Serial.printf("T1=%.1f T2=%.1f P1=%d P2=%d F1=%d F2=%d\n",
                  temp1, temp2, peltier1_pwm, peltier2_pwm, fan1_pwm, fan2_pwm);
}

// ============================================================
void updateOutputs() {
    if (autoMode) {
        // Simple proportional control: scale peltier power based on
        // how far above target temp we are
        float avg = (temp1 + temp2) / 2.0f;
        float error = avg - targetTemp;

        if (error > 0) {
            // Above target - cool harder
            int pwm = (int)(error * 25.0f);  // 25 PWM units per degree
            if (pwm > 255) pwm = 255;
            peltier1_pwm = pwm;
            peltier2_pwm = pwm;
        } else {
            // At or below target - peltiers off
            peltier1_pwm = 0;
            peltier2_pwm = 0;
        }

        // Fans ramp with peltier load (min 30% when peltiers are on)
        if (peltier1_pwm > 0 || peltier2_pwm > 0) {
            int fanPwm = 76 + ((int)peltier1_pwm * 179 / 255);  // 30%-100%
            fan1_pwm = fanPwm;
            fan2_pwm = fanPwm;
        } else {
            fan1_pwm = 0;
            fan2_pwm = 0;
        }
    }

    ledcWrite(PELTIER_1_CH, peltier1_pwm);
    ledcWrite(PELTIER_2_CH, peltier2_pwm);
    ledcWrite(FAN_1_CH, fan1_pwm);
    ledcWrite(FAN_2_CH, fan2_pwm);
}

// ============================================================
uint16_t tempColor(float t) {
    if (t >= highLimit) return CLR_ALARM;
    if (t <= lowLimit) return CLR_ALARM;
    if (t >= highLimit - 3.0f) return CLR_WARN;
    if (t <= lowLimit + 3.0f) return CLR_WARN;
    return CLR_VALUE;
}

// ============================================================
void drawBar(int x, int y, int w, int h, uint8_t val, uint16_t color) {
    int fillW = (int)((float)val / 255.0f * w);
    tft.fillRect(x, y, fillW, h, color);
    tft.fillRect(x + fillW, y, w - fillW, h, 0x2104);
    tft.drawRect(x, y, w, h, CLR_TEXT);
}

// ============================================================
void drawButton(Button& btn, uint16_t bg) {
    tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 4, bg);
    tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 4, CLR_TEXT);
    tft.setTextColor(CLR_BTN_TEXT, bg);
    tft.setTextSize(1);
    int textW = strlen(btn.label) * 6;
    tft.setCursor(btn.x + (btn.w - textW) / 2, btn.y + (btn.h - 8) / 2);
    tft.print(btn.label);
}

// ============================================================
bool touchInButton(int tx, int ty, Button& btn) {
    return (tx >= btn.x && tx <= btn.x + btn.w &&
            ty >= btn.y && ty <= btn.y + btn.h);
}

// ============================================================
void drawMainScreen() {
    // Header bar
    tft.fillRect(0, 0, SCREEN_WIDTH, 20, CLR_HEADER);
    tft.setTextColor(CLR_TEXT, CLR_HEADER);
    tft.setTextSize(1);
    tft.setCursor(4, 6);
    tft.print("CYD COOLING CONTROLLER");

    if (alarmActive) {
        tft.setTextColor(CLR_ALARM, CLR_HEADER);
        tft.setCursor(240, 6);
        tft.print("!! ALARM !!");
    } else {
        tft.setCursor(240, 6);
        tft.setTextColor(CLR_VALUE, CLR_HEADER);
        tft.print("  NORMAL   ");
    }

    // Mode indicator
    tft.setCursor(4, 22);
    tft.setTextColor(autoMode ? CLR_AUTO_ON : CLR_AUTO_OFF, CLR_BG);
    tft.setTextSize(1);
    tft.printf("Mode: %s  Target: %.1fC  ", autoMode ? "AUTO" : "MANUAL", targetTemp);

    // ---- Temperature readings ----
    int y = 38;
    tft.setTextSize(1);

    // Sensor 1 - Inlet
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.setCursor(4, y);
    tft.print("Inlet : ");
    tft.setTextColor(tempColor(temp1), CLR_BG);
    tft.setTextSize(2);
    tft.setCursor(55, y - 3);
    tft.printf("%5.1f", temp1);
    tft.setTextSize(1);
    tft.setCursor(120, y);
    tft.printf("C  Lo:%5.1f Hi:%5.1f", temp1_min, temp1_max);

    y += 30;

    // Sensor 2 - Outlet
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.setCursor(4, y);
    tft.setTextSize(1);
    tft.print("Outlet: ");
    tft.setTextColor(tempColor(temp2), CLR_BG);
    tft.setTextSize(2);
    tft.setCursor(55, y - 3);
    tft.printf("%5.1f", temp2);
    tft.setTextSize(1);
    tft.setCursor(120, y);
    tft.printf("C  Lo:%5.1f Hi:%5.1f", temp2_min, temp2_max);

    y += 30;

    // Delta T
    float deltaT = temp2 - temp1;
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.setCursor(4, y);
    tft.setTextSize(1);
    tft.printf("Delta T: ");
    tft.setTextColor(CLR_VALUE, CLR_BG);
    tft.printf("%+.1f C   ", deltaT);

    // ---- Output controls ----
    y += 18;
    int barX = 75, barW = 140, barH = 12;

    // Peltier 1
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.setCursor(4, y + 2);
    tft.printf("Pelt 1: %3d%%", (int)(peltier1_pwm * 100 / 255));
    drawBar(barX, y, barW, barH, peltier1_pwm, TFT_BLUE);
    drawButton(btnPelt1Up);
    drawButton(btnPelt1Down);

    y += 20;

    // Peltier 2
    tft.setCursor(4, y + 2);
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.printf("Pelt 2: %3d%%", (int)(peltier2_pwm * 100 / 255));
    drawBar(barX, y, barW, barH, peltier2_pwm, TFT_BLUE);
    drawButton(btnPelt2Up);
    drawButton(btnPelt2Down);

    y += 20;

    // Fan 1
    tft.setCursor(4, y + 2);
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.printf("Fan  1: %3d%%", (int)(fan1_pwm * 100 / 255));
    drawBar(barX, y, barW, barH, fan1_pwm, TFT_CYAN);
    drawButton(btnFan1Up);
    drawButton(btnFan1Down);

    y += 20;

    // Fan 2
    tft.setCursor(4, y + 2);
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.printf("Fan  2: %3d%%", (int)(fan2_pwm * 100 / 255));
    drawBar(barX, y, barW, barH, fan2_pwm, TFT_CYAN);
    drawButton(btnFan2Up);
    drawButton(btnFan2Down);

    // ---- Bottom buttons ----
    drawButton(btnAutoMode, autoMode ? TFT_DARKGREEN : CLR_BTN);
    drawButton(btnSettings);
    drawButton(btnResetMinMax);

    // Limits bar at very bottom
    tft.fillRect(0, 215, SCREEN_WIDTH, 25, CLR_HEADER);
    tft.setTextColor(CLR_TEXT, CLR_HEADER);
    tft.setCursor(4, 220);
    tft.printf("Limits  Low: %.0fC  High: %.0fC", lowLimit, highLimit);
}

// ============================================================
void drawSettingsScreen() {
    tft.fillRect(0, 0, SCREEN_WIDTH, 20, 0x4000);
    tft.setTextColor(CLR_TEXT, 0x4000);
    tft.setTextSize(1);
    tft.setCursor(4, 6);
    tft.print("SETTINGS");

    int y = 45;
    tft.setTextSize(1);

    // High limit
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.setCursor(10, y);
    tft.printf("High Alarm Limit: ");
    tft.setTextColor(CLR_ALARM, CLR_BG);
    tft.setTextSize(2);
    tft.setCursor(130, y - 4);
    tft.printf("%5.1f C ", highLimit);
    drawButton(btnHighUp);
    drawButton(btnHighDown);

    y += 40;

    // Low limit
    tft.setTextSize(1);
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.setCursor(10, y);
    tft.printf("Low Alarm Limit:  ");
    tft.setTextColor(TFT_CYAN, CLR_BG);
    tft.setTextSize(2);
    tft.setCursor(130, y - 4);
    tft.printf("%5.1f C ", lowLimit);
    drawButton(btnLowUp);
    drawButton(btnLowDown);

    y += 40;

    // Target temp
    tft.setTextSize(1);
    tft.setTextColor(CLR_LABEL, CLR_BG);
    tft.setCursor(10, y);
    tft.printf("Target Temp:      ");
    tft.setTextColor(CLR_VALUE, CLR_BG);
    tft.setTextSize(2);
    tft.setCursor(130, y - 4);
    tft.printf("%5.1f C ", targetTemp);
    drawButton(btnTargetUp);
    drawButton(btnTargetDown);

    drawButton(btnBack);
}

// ============================================================
void handleMainTouch(int tx, int ty) {
    // Peltier/Fan adjustments only in manual mode
    if (!autoMode) {
        if (touchInButton(tx, ty, btnPelt1Up))   { peltier1_pwm = min(255, peltier1_pwm + 25); }
        if (touchInButton(tx, ty, btnPelt1Down))  { peltier1_pwm = max(0, peltier1_pwm - 25); }
        if (touchInButton(tx, ty, btnPelt2Up))    { peltier2_pwm = min(255, peltier2_pwm + 25); }
        if (touchInButton(tx, ty, btnPelt2Down))   { peltier2_pwm = max(0, peltier2_pwm - 25); }
        if (touchInButton(tx, ty, btnFan1Up))     { fan1_pwm = min(255, fan1_pwm + 25); }
        if (touchInButton(tx, ty, btnFan1Down))   { fan1_pwm = max(0, fan1_pwm - 25); }
        if (touchInButton(tx, ty, btnFan2Up))     { fan2_pwm = min(255, fan2_pwm + 25); }
        if (touchInButton(tx, ty, btnFan2Down))   { fan2_pwm = max(0, fan2_pwm - 25); }
        updateOutputs();
    }

    if (touchInButton(tx, ty, btnAutoMode)) {
        autoMode = !autoMode;
    }

    if (touchInButton(tx, ty, btnSettings)) {
        currentScreen = SCREEN_SETTINGS;
        tft.fillScreen(CLR_BG);
    }

    if (touchInButton(tx, ty, btnResetMinMax)) {
        temp1_min = temp1; temp1_max = temp1;
        temp2_min = temp2; temp2_max = temp2;
    }
}

// ============================================================
void handleSettingsTouch(int tx, int ty) {
    if (touchInButton(tx, ty, btnHighUp))    { highLimit += 1.0f; }
    if (touchInButton(tx, ty, btnHighDown))  { highLimit -= 1.0f; }
    if (touchInButton(tx, ty, btnLowUp))     { lowLimit += 1.0f; }
    if (touchInButton(tx, ty, btnLowDown))   { lowLimit -= 1.0f; }
    if (touchInButton(tx, ty, btnTargetUp))  { targetTemp += 0.5f; }
    if (touchInButton(tx, ty, btnTargetDown)){ targetTemp -= 0.5f; }

    if (touchInButton(tx, ty, btnBack)) {
        currentScreen = SCREEN_MAIN;
        tft.fillScreen(CLR_BG);
    }
}
