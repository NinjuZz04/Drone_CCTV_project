#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

// --- Objects ---
WiFiClientSecure espClient;
PubSubClient client(espClient);

// --- State Machine ---
enum PageState { PAGE_STATUS, PAGE_PIN, PAGE_COMMAND };
PageState currentPage = PAGE_STATUS;
bool needRedraw = true;

// --- PIN System ---
const String CORRECT_PIN = "1234";
String enteredPin = "";
String pinMessage = "ENTER PIN";

// --- System Variables ---
bool systemArmed = true;
int lastDist = 0;
String statusMsg = "SYSTEM NORMAL";
unsigned long lastStatusUpdate = 0;

// ==========================================
// 🎨 Custom Color Palette (16-bit 565 format)
// ==========================================
// Dark backgrounds
#define C_BG         0x0000   // Pure black
#define C_PANEL      0x0841   // Dark blue-grey panel  RGB≈(8,16,8)
#define C_NAVY       0x000F   // Deep navy blue
#define C_DARKNAVY   0x0010   // Near-black navy      RGB≈(0,0,128)

// Accent colors
#define C_CYAN       0x07FF   // Bright cyan  (= CYAN)
#define C_DCYAN      0x03EF   // Dark cyan    (= DARKCYAN)
#define C_GREEN      0x07E0   // Bright green (= GREEN)
#define C_DKGREEN    0x03E0   // Dark green   (= DARKGREEN)

// Alert / status colors
#define C_RED        0xF800   // Bright red
#define C_DARKRED    0x3000   // Dark red     RGB≈(48,0,0)
#define C_DEEPRED    0x6000   // Medium red   RGB≈(96,0,0)
#define C_ORANGE     0xFD20   // Orange       (= ORANGE)
#define C_YELLOW     0xFFE0   // Yellow       (= YELLOW)
#define C_MAROON     0x7800   // Maroon       (= MAROON)

// Neutral
#define C_WHITE      0xFFFF
#define C_LGREY      0xC618   // Light grey
#define C_GREY       0x7BEF   // Mid grey     (= DARKGREY)
#define C_DKGREY     0x39C7   // Dark grey
#define C_XDKGREY    0x2104   // Extra dark grey / shadow

// ==========================================
// 🔧 Core Drawing Helpers
// ==========================================

// Draws 4 corner bracket decorations around a rectangle
void drawHUD_Brackets(int x, int y, int w, int h, uint16_t color, int len = 12) {
    // Top-left
    M5.Display.drawFastHLine(x,     y,     len, color);
    M5.Display.drawFastVLine(x,     y,     len, color);
    // Top-right
    M5.Display.drawFastHLine(x+w-len, y,   len, color);
    M5.Display.drawFastVLine(x+w,   y,     len, color);
    // Bottom-left
    M5.Display.drawFastHLine(x,     y+h,   len, color);
    M5.Display.drawFastVLine(x,     y+h-len, len, color);
    // Bottom-right
    M5.Display.drawFastHLine(x+w-len, y+h, len, color);
    M5.Display.drawFastVLine(x+w,   y+h-len, len, color);
}

// Draws a subtle dot-grid background
void drawDotGrid(uint16_t dotColor = 0x0841) {
    for (int gx = 0; gx < 320; gx += 32) {
        for (int gy = 40; gy < 240; gy += 20) {
            M5.Display.drawPixel(gx, gy, dotColor);
        }
    }
}

// Draws a thin horizontal divider with optional tick marks
void drawDivider(int y, uint16_t color, bool ticks = false) {
    M5.Display.drawFastHLine(0, y, 320, color);
    if (ticks) {
        for (int tx = 0; tx < 320; tx += 16) {
            M5.Display.drawFastVLine(tx, y-1, 3, color);
        }
    }
}

// Fills background and draws divider line
void drawPageHeader(const char* title, uint16_t bgColor, uint16_t accentColor) {
    M5.Display.fillRect(0, 0, 320, 36, bgColor);
    M5.Display.drawFastHLine(0, 36, 320, accentColor);
    // Accent corner ticks
    M5.Display.drawFastVLine(0,   0, 36, accentColor);
    M5.Display.drawFastVLine(319, 0, 36, accentColor);
    M5.Display.setTextColor(C_WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString(title, 160, 18);
}

// Draws a styled action button with shadow + accent border
void drawTacticalBtn(int x, int y, int w, int h,
                     const char* label, const char* sublabel,
                     uint16_t fillColor, uint16_t borderColor,
                     uint16_t textColor = C_WHITE) {
    // Drop shadow
    M5.Display.fillRoundRect(x+2, y+2, w, h, 7, C_XDKGREY);
    // Main fill
    M5.Display.fillRoundRect(x, y, w, h, 7, fillColor);
    // Top-edge highlight for 3D feel
    M5.Display.drawFastHLine(x+8, y+1, w-16, 0x4208);
    // Accent border
    M5.Display.drawRoundRect(x,   y,   w, h, 7, borderColor);
    M5.Display.drawRoundRect(x+1, y+1, w-2, h-2, 6, borderColor == C_WHITE ? 0x4208 : fillColor);

    if (sublabel != nullptr && strlen(sublabel) > 0) {
        // Two-line button: sublabel small at top, label large in center
        M5.Display.setTextColor(0xC638); // muted text for sublabel
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(1);
        M5.Display.drawString(sublabel, x + w/2, y + h/2 - 10);
        M5.Display.setTextColor(textColor);
        M5.Display.setTextSize(2);
        M5.Display.drawString(label, x + w/2, y + h/2 + 8);
    } else {
        M5.Display.setTextColor(textColor);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(2);
        M5.Display.drawString(label, x + w/2, y + h/2);
    }
}

// Draws a horizontal progress/distance bar
void drawDistBar(int x, int y, int w, int h, int val, int maxVal) {
    // Track
    M5.Display.fillRoundRect(x, y, w, h, 3, C_XDKGREY);
    M5.Display.drawRoundRect(x, y, w, h, 3, C_DKGREY);
    // Fill
    int fillW = constrain(map(val, 0, maxVal, 0, w - 4), 0, w - 4);
    if (fillW > 0) {
        uint16_t barColor = (val < 20) ? C_RED : (val < 50 ? C_ORANGE : C_GREEN);
        M5.Display.fillRoundRect(x+2, y+2, fillW, h-4, 2, barColor);
        // Bright tip
        M5.Display.drawFastVLine(x+1+fillW, y+2, h-4, C_WHITE);
    }
    // Tick marks at 25%, 50%, 75%
    for (int tick = 1; tick <= 3; tick++) {
        int tx = x + (w * tick / 4);
        M5.Display.drawFastVLine(tx, y + h - 4, 3, C_DKGREY);
    }
}

// Draws the animated armed-status ring indicator
void drawStatusRing(int cx, int cy, bool armed, bool alert) {
    if (alert) {
        // Triple pulsing rings — danger state
        M5.Display.fillCircle(cx, cy, 36, C_DARKRED);
        M5.Display.drawCircle(cx, cy, 38, C_RED);
        M5.Display.drawCircle(cx, cy, 42, C_DEEPRED);
        M5.Display.drawCircle(cx, cy, 46, C_DARKRED);
        // Inner icon
        M5.Display.setTextColor(C_WHITE);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(1);
        M5.Display.drawString("ALERT", cx, cy - 7);
        M5.Display.setTextColor(C_RED);
        M5.Display.setTextSize(2);
        M5.Display.drawString("!!!", cx, cy + 8);
    } else if (armed) {
        // Armed — red rings
        M5.Display.fillCircle(cx, cy, 36, C_MAROON);
        M5.Display.drawCircle(cx, cy, 38, C_RED);
        M5.Display.drawCircle(cx, cy, 42, C_DEEPRED);
        M5.Display.drawCircle(cx, cy, 46, C_DARKRED);
        // Crosshair lines
        M5.Display.drawFastHLine(cx-34, cy, 68, C_DEEPRED);
        M5.Display.drawFastVLine(cx, cy-34, 68, C_DEEPRED);
        // Inner icon
        M5.Display.setTextColor(0xFF8C); // orange-red
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(1);
        M5.Display.drawString("STATUS", cx, cy - 8);
        M5.Display.setTextColor(C_WHITE);
        M5.Display.setTextSize(2);
        M5.Display.drawString("ARMED", cx, cy + 7);
    } else {
        // Disarmed — grey rings
        M5.Display.fillCircle(cx, cy, 36, C_PANEL);
        M5.Display.drawCircle(cx, cy, 38, C_DKGREY);
        M5.Display.drawCircle(cx, cy, 42, C_XDKGREY);
        M5.Display.setTextColor(C_GREY);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(1);
        M5.Display.drawString("STATUS", cx, cy - 8);
        M5.Display.setTextColor(C_LGREY);
        M5.Display.setTextSize(2);
        M5.Display.drawString("SAFE", cx, cy + 7);
    }
}

// ==========================================
// 🚀 MQTT Callback
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
    String msg = "";
    for (int i = 0; i < length; i++) msg += (char)payload[i];

    if (String(topic) == "drone/config") {
        if (msg.indexOf("\"status\": \"ARMED\"")    != -1) systemArmed = true;
        if (msg.indexOf("\"status\": \"DISARMED\"") != -1) systemArmed = false;
        if (currentPage == PAGE_STATUS || currentPage == PAGE_COMMAND) needRedraw = true;
    }

    if (String(topic) == "drone/alert") {
        if (msg.indexOf("\"dist\":") != -1) {
            int distStart = msg.indexOf("\"dist\":") + 7;
            int distEnd   = msg.indexOf(",", distStart);
            lastDist = msg.substring(distStart, distEnd).toInt();

            statusMsg = (lastDist > 0 && lastDist < 20 && systemArmed)
                        ? "!!! INTRUDER !!!" : "SYSTEM NORMAL";

            if (currentPage == PAGE_STATUS && millis() - lastStatusUpdate > 1000) {
                needRedraw = true;
                lastStatusUpdate = millis();
            }
        }
    }
}

void connectToAWS() {
    while (!client.connected()) {
        M5.Display.clear(C_BG);
        // Header
        M5.Display.fillRect(0, 0, 320, 36, C_DARKNAVY);
        M5.Display.drawFastHLine(0, 36, 320, C_CYAN);
        M5.Display.setTextColor(C_CYAN);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(2);
        M5.Display.drawString("SENTINEL SYSTEM", 160, 18);

        // Connection status box
        drawHUD_Brackets(40, 80, 240, 80, C_CYAN);
        M5.Display.setTextColor(C_LGREY);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextSize(1);
        M5.Display.drawString("CONNECTING TO AWS IOT", 160, 105);
        M5.Display.setTextColor(C_YELLOW);
        M5.Display.setTextSize(2);
        M5.Display.drawString("PLEASE WAIT...", 160, 128);

        if (client.connect("m5_remote_client")) {
            client.subscribe("drone/config");
            client.subscribe("drone/alert");
            needRedraw = true;
        } else {
            delay(3000);
        }
    }
}

// ==========================================
// 🖥️  PAGE: STATUS
// ==========================================
void drawStatusPage() {
    bool isAlert = (statusMsg == "!!! INTRUDER !!!");

    M5.Display.clear(C_BG);
    drawDotGrid();

    // ---- Header bar ----
    uint16_t hdrBg     = systemArmed ? C_DEEPRED  : C_PANEL;
    uint16_t hdrAccent = systemArmed ? C_RED       : C_DKGREY;
    M5.Display.fillRect(0, 0, 320, 36, hdrBg);
    M5.Display.drawFastHLine(0, 36, 320, hdrAccent);
    M5.Display.drawFastVLine(0,   0, 36, hdrAccent);
    M5.Display.drawFastVLine(319, 0, 36, hdrAccent);

    // Left: system name
    M5.Display.setTextColor(C_LGREY);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    M5.Display.drawString("SENTINEL v1.0", 8, 18);

    // Right: armed status badge
    uint16_t badgeColor = systemArmed ? C_RED : C_DKGREY;
    uint16_t badgeText  = systemArmed ? C_WHITE : C_GREY;
    M5.Display.fillRoundRect(198, 7, 116, 22, 4, badgeColor);
    M5.Display.setTextColor(badgeText);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(1);
    M5.Display.drawString(systemArmed ? "ARMED" : "DISARMED", 256, 18);

    // ---- Status ring (center) ----
    drawStatusRing(160, 100, systemArmed, isAlert);

    // ---- Divider ----
    drawDivider(155, C_PANEL, true);

    // ---- Radar distance section ----
    M5.Display.setTextColor(C_CYAN);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    M5.Display.drawString("RADAR DISTANCE", 10, 166);

    // Distance value badge (right)
    uint16_t distColor = (lastDist > 0 && lastDist < 20) ? C_RED : C_DKGREEN;
    M5.Display.fillRoundRect(240, 158, 72, 18, 3, distColor);
    M5.Display.setTextColor(C_WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(1);
    M5.Display.drawString(String(lastDist) + " cm", 276, 167);

    drawDistBar(10, 178, 300, 10, lastDist, 200);

    // ---- Status message label ----
    uint16_t msgColor = isAlert ? C_RED : (systemArmed ? C_CYAN : C_GREY);
    M5.Display.setTextColor(msgColor);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(isAlert ? 2 : 1);
    M5.Display.drawString(isAlert ? "INTRUDER DETECTED" : statusMsg, 160, 198);

    // ---- Unlock button  (touch area: x=40,y=170,w=240,h=50 → centre y≈195) ----
    // Mapped to y=205–238 to avoid overlap with status text
    uint16_t btnBorder = systemArmed ? C_CYAN : C_DKGREY;
    uint16_t btnText   = systemArmed ? C_CYAN : C_GREY;
    M5.Display.fillRoundRect(40, 205, 240, 33, 8, C_DARKNAVY);
    M5.Display.drawRoundRect(40, 205, 240, 33, 8, btnBorder);
    // Side accent ticks
    M5.Display.drawFastHLine(40,  213, 10, btnBorder);
    M5.Display.drawFastHLine(270, 213, 10, btnBorder);
    M5.Display.setTextColor(btnText);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString(">  UNLOCK COMMANDS  <", 160, 221);
}

// NOTE: touch area for unlock button mapped in handleStatusTouch (y>170 && y<220)
// Drawing is shifted to y=205 but the full 170–240 zone still triggers the event.

// ==========================================
// 🔑  PAGE: PIN
// ==========================================
void drawPinPage() {
    M5.Display.clear(C_BG);
    drawDotGrid();

    // ---- Header ----
    M5.Display.fillRect(0, 0, 320, 36, C_DARKNAVY);
    M5.Display.drawFastHLine(0, 36, 320, C_CYAN);
    M5.Display.drawFastVLine(0,   0, 36, C_CYAN);
    M5.Display.drawFastVLine(319, 0, 36, C_CYAN);

    // Back button (touch: x<80, y<30)
    M5.Display.setTextColor(C_DKGREY);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    M5.Display.drawString("< BACK", 6, 18);

    M5.Display.setTextColor(C_CYAN);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString("ACCESS CONTROL", 170, 18);

    // ---- PIN message ----
    bool isError = (pinMessage == "WRONG PIN!");
    M5.Display.setTextColor(isError ? C_RED : C_YELLOW);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(isError ? 2 : 1);
    M5.Display.drawString(pinMessage, 160, 50);

    // ---- PIN dots (4 circles centred on screen) ----
    // Spacing: 4 dots × 30px gap, start at 160 - 45 = 115
    const int dotY       = 70;
    const int dotR       = 10;
    const int dotSpacing = 30;
    const int dotStart   = 160 - (3 * dotSpacing / 2); // = 115
    for (int i = 0; i < 4; i++) {
        int dx = dotStart + i * dotSpacing;
        if (i < (int)enteredPin.length()) {
            M5.Display.fillCircle(dx, dotY, dotR, C_CYAN);
            M5.Display.drawCircle(dx, dotY, dotR + 3, C_DCYAN);
        } else {
            M5.Display.drawCircle(dx, dotY, dotR, C_DKGREY);
            M5.Display.drawPixel(dx, dotY, C_GREY); // centre dot
        }
    }

    // Thin divider below dots
    M5.Display.drawFastHLine(20, 84, 280, C_PANEL);

    // ---- Numpad  (touch areas: xs[]={40,120,200}, ys[]={80,120,160,200}, 70×35) ----
    const int xs[]  = {40, 120, 200};
    const int ys[]  = {87, 124, 161, 198};
    const int btnW  = 70, btnH = 33;

    const char* numLabels[4][3] = {
        {"1","2","3"},
        {"4","5","6"},
        {"7","8","9"},
        {"DEL","0","OK"}
    };
    // Border color per button
    uint16_t numBorder[4][3] = {
        {C_DKGREY, C_DKGREY, C_DKGREY},
        {C_DKGREY, C_DKGREY, C_DKGREY},
        {C_DKGREY, C_DKGREY, C_DKGREY},
        {C_RED,    C_DKGREY, C_GREEN }
    };
    uint16_t numFill[4][3] = {
        {C_PANEL, C_PANEL, C_PANEL},
        {C_PANEL, C_PANEL, C_PANEL},
        {C_PANEL, C_PANEL, C_PANEL},
        {C_DARKRED, C_PANEL, C_DKGREEN}
    };
    uint16_t numText[4][3] = {
        {C_WHITE, C_WHITE, C_WHITE},
        {C_WHITE, C_WHITE, C_WHITE},
        {C_WHITE, C_WHITE, C_WHITE},
        {C_RED,   C_WHITE, C_GREEN}
    };

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            int bx = xs[col], by = ys[row];
            // Shadow
            M5.Display.fillRoundRect(bx+2, by+2, btnW, btnH, 5, C_XDKGREY);
            // Fill
            M5.Display.fillRoundRect(bx, by, btnW, btnH, 5, numFill[row][col]);
            // Border
            M5.Display.drawRoundRect(bx, by, btnW, btnH, 5, numBorder[row][col]);
            // Label
            M5.Display.setTextColor(numText[row][col]);
            M5.Display.setTextDatum(middle_center);
            M5.Display.setTextSize(2);
            M5.Display.drawString(numLabels[row][col], bx + btnW/2, by + btnH/2);
        }
    }
}

// ==========================================
// ⚡  PAGE: COMMAND CENTER
// ==========================================
void drawCommandPage() {
    M5.Display.clear(C_BG);
    drawDotGrid();

    // ---- Header  ----
    M5.Display.fillRect(0, 0, 36, 36, C_DKGREEN);
    M5.Display.fillRect(36, 0, 284, 36, 0x01A0); // dark teal
    M5.Display.fillRect(284, 0, 36, 36, C_DKGREEN);
    M5.Display.drawFastHLine(0, 36, 320, C_GREEN);
    M5.Display.drawFastVLine(0,   0, 36, C_GREEN);
    M5.Display.drawFastVLine(319, 0, 36, C_GREEN);

    M5.Display.setTextColor(C_WHITE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString("COMMAND CENTER", 160, 18);

    // Unlocked sub-label
    M5.Display.setTextColor(C_GREEN);
    M5.Display.setTextSize(1);
    // (inside the 36px header — small text at same y works with size 1)
    M5.Display.setTextDatum(middle_right);
    M5.Display.drawString("UNLOCKED", 312, 29);

    // ---- 2×2 Command button grid ----
    // Touch areas: x=10/165, y=45/115, w=145, h=60

    // --- RADAR ON/OFF  (top-left) ---
    bool on = systemArmed;
    uint16_t radFill   = on ? C_DARKNAVY : C_PANEL;
    uint16_t radBorder = on ? C_CYAN     : C_DKGREY;
    uint16_t radText   = on ? C_CYAN     : C_GREY;
    M5.Display.fillRoundRect(12, 47, 141, 56, 8, radFill);
    M5.Display.drawRoundRect(12, 47, 141, 56, 8, radBorder);
    // Side tick marks
    M5.Display.drawFastVLine(12,  55, 10, radBorder);
    M5.Display.drawFastVLine(152, 55, 10, radBorder);
    M5.Display.setTextColor(0x8C71);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(1);
    M5.Display.drawString("RADAR", 82, 65);
    M5.Display.setTextColor(radText);
    M5.Display.setTextSize(2);
    M5.Display.drawString(on ? "ACTIVE" : "STANDBY", 82, 85);

    // --- ALARM  (top-right) ---
    M5.Display.fillRoundRect(167, 47, 141, 56, 8, 0x2000); // very dark red-orange
    M5.Display.drawRoundRect(167, 47, 141, 56, 8, C_ORANGE);
    M5.Display.drawFastVLine(167, 55, 10, C_ORANGE);
    M5.Display.drawFastVLine(307, 55, 10, C_ORANGE);
    M5.Display.setTextColor(0x8C71);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(1);
    M5.Display.drawString("TRIGGER", 237, 65);
    M5.Display.setTextColor(C_ORANGE);
    M5.Display.setTextSize(2);
    M5.Display.drawString("ALARM", 237, 85);

    // --- TAKEOFF  (bottom-left) ---
    M5.Display.fillRoundRect(12, 117, 141, 56, 8, 0x000A); // dark blue
    M5.Display.drawRoundRect(12, 117, 141, 56, 8, C_DCYAN);
    M5.Display.drawFastVLine(12,  125, 10, C_DCYAN);
    M5.Display.drawFastVLine(152, 125, 10, C_DCYAN);
    M5.Display.setTextColor(0x8C71);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(1);
    M5.Display.drawString("DRONE", 82, 135);
    M5.Display.setTextColor(C_DCYAN);
    M5.Display.setTextSize(2);
    M5.Display.drawString("TAKEOFF", 82, 155);

    // --- LAND  (bottom-right) ---
    M5.Display.fillRoundRect(167, 117, 141, 56, 8, C_DARKRED);
    M5.Display.drawRoundRect(167, 117, 141, 56, 8, C_RED);
    M5.Display.drawFastVLine(167, 125, 10, C_RED);
    M5.Display.drawFastVLine(307, 125, 10, C_RED);
    M5.Display.setTextColor(0x8C71);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(1);
    M5.Display.drawString("DRONE", 237, 135);
    M5.Display.setTextColor(C_RED);
    M5.Display.setTextSize(2);
    M5.Display.drawString("LAND", 237, 155);

    // ---- Divider ----
    drawDivider(181, C_PANEL);

    // ---- Lock & Exit button  (touch area: y=185–230) ----
    M5.Display.fillRoundRect(32, 185, 256, 42, 8, C_PANEL);
    M5.Display.drawRoundRect(32, 185, 256, 42, 8, C_DKGREY);
    // Corner accent brackets
    drawHUD_Brackets(32, 185, 256, 42, C_GREY, 8);
    M5.Display.setTextColor(C_GREY);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString("LOCK  &  EXIT", 160, 206);
}

// ==========================================
// 👆 Touch Handlers  (logic unchanged)
// ==========================================
void handleStatusTouch(int x, int y) {
    if (x > 40 && x < 280 && y > 170 && y < 240) {
        currentPage = PAGE_PIN;
        enteredPin  = "";
        pinMessage  = "ENTER PIN";
        needRedraw  = true;
    }
}

void handlePinTouch(int x, int y) {
    if (x < 80 && y < 30) {
        currentPage = PAGE_STATUS;
        needRedraw  = true;
        return;
    }

    int xs[] = {40, 120, 200};
    int ys[] = {80, 120, 160, 200};
    String key = "";

    if (y > ys[0] && y < ys[0]+35) {
        if (x > xs[0] && x < xs[0]+70) key = "1";
        if (x > xs[1] && x < xs[1]+70) key = "2";
        if (x > xs[2] && x < xs[2]+70) key = "3";
    }
    if (y > ys[1] && y < ys[1]+35) {
        if (x > xs[0] && x < xs[0]+70) key = "4";
        if (x > xs[1] && x < xs[1]+70) key = "5";
        if (x > xs[2] && x < xs[2]+70) key = "6";
    }
    if (y > ys[2] && y < ys[2]+35) {
        if (x > xs[0] && x < xs[0]+70) key = "7";
        if (x > xs[1] && x < xs[1]+70) key = "8";
        if (x > xs[2] && x < xs[2]+70) key = "9";
    }
    if (y > ys[3] && y < ys[3]+35) {
        if (x > xs[0] && x < xs[0]+70) key = "DEL";
        if (x > xs[1] && x < xs[1]+70) key = "0";
        if (x > xs[2] && x < xs[2]+70) key = "OK";
    }

    if (key != "") {
        if (key == "DEL") {
            if (enteredPin.length() > 0) enteredPin.remove(enteredPin.length() - 1);
        } else if (key == "OK") {
            if (enteredPin == CORRECT_PIN) {
                currentPage = PAGE_COMMAND;
                M5.Speaker.tone(2000, 100);
            } else {
                pinMessage = "WRONG PIN!";
                enteredPin = "";
                M5.Speaker.tone(500, 300);
            }
        } else {
            if (enteredPin.length() < 4) enteredPin += key;
        }
        needRedraw = true;
    }
}

void handleCommandTouch(int x, int y) {
    if (y >= 45 && y <= 105) {
        if (x >= 10 && x <= 155) {  // RADAR ON/OFF
            systemArmed = !systemArmed;
            client.publish("drone/config",
                systemArmed ? "{\"status\": \"ARMED\"}" : "{\"status\": \"DISARMED\"}");
        }
        if (x >= 165 && x <= 310) {  // ALARM
            client.publish("drone/config", "{\"command\": \"ALARM_ON\"}");
        }
    }
    if (y >= 115 && y <= 175) {
        if (x >= 10 && x <= 155) {  // TAKEOFF
            client.publish("drone/config", "{\"command\": \"TAKEOFF\"}");
        }
        if (x >= 165 && x <= 310) {  // LAND
            client.publish("drone/config", "{\"command\": \"LAND\"}");
        }
    }
    if (y >= 185 && y <= 230) {  // LOCK & EXIT
        currentPage = PAGE_STATUS;
    }
    needRedraw = true;
}

// ==========================================
// ⚙️  Setup & Loop
// ==========================================
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.begin();
    M5.Speaker.setVolume(100);

    espClient.setCACert(AWS_CERT_CA);
    espClient.setCertificate(AWS_CERT_CRT);
    espClient.setPrivateKey(AWS_CERT_PRIVATE);
    client.setServer(mqtt_server, 8883);
    client.setCallback(callback);

    WiFi.begin(ssid, password);
}

void loop() {
    M5.update();

    if (WiFi.status() != WL_CONNECTED || !client.connected()) {
        connectToAWS();
        return;
    }

    client.loop();

    auto touch = M5.Touch.getDetail();
    if (touch.wasPressed()) {
        if      (currentPage == PAGE_STATUS)  handleStatusTouch(touch.x, touch.y);
        else if (currentPage == PAGE_PIN)     handlePinTouch(touch.x, touch.y);
        else if (currentPage == PAGE_COMMAND) handleCommandTouch(touch.x, touch.y);
    }

    if (needRedraw) {
        M5.Display.startWrite();
        if      (currentPage == PAGE_STATUS)  drawStatusPage();
        else if (currentPage == PAGE_PIN)     drawPinPage();
        else if (currentPage == PAGE_COMMAND) drawCommandPage();
        M5.Display.endWrite();
        needRedraw = false;
    }
}
