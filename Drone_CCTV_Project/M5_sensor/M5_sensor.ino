#include <M5Unified.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"

// --- Hardware Pins ---
const int trigPin = 17; 
const int echoPin = 18;
const int SERVO_PIN = 8;  

// --- Objects ---
WiFiClientSecure espClient;
PubSubClient client(espClient);
Servo myServo;

// --- Variables ---
unsigned long lastUpdate = 0;
int currentAngle = 0;
bool moveForward = true;
bool manualAlert = false;
int currentDistance = 0;

// --- ตัวแปรสำหรับระบบเปิด/ปิด (System ON/OFF) ---
bool systemOn = true;

// --- Variables สำหรับระบบเสียงเตือน ---
int dangerCount = 0;                 
bool isAlarmActive = false;          
unsigned long alarmTriggerTime = 0;

// --- Variables สำหรับเสียงเตือน ---
int beepCount = 0;
bool isBeeping = false;
unsigned long lastBeepTime = 0;
int beepType = 1; // 💡 1 = เสียงสั้น (Sensor ตรวจเจอ), 2 = เสียงยาว (กดจาก Web)

// ==========================================
// 🚀 ฟังก์ชันรับคำสั่งจากเว็บ (MQTT Callback)
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    // ตรวจสอบว่ามีคำสั่งจากหน้าเว็บส่งมาหรือไม่
    if (String(topic) == "drone/config") {
        if (message.indexOf("\"status\": \"ARMED\"") != -1) {
            systemOn = true;
            Serial.println("Received: SYSTEM ON");
        } 
        else if (message.indexOf("\"status\": \"DISARMED\"") != -1) {
            systemOn = false;
            Serial.println("Received: SYSTEM OFF");
        }
        // 🚨 รับคำสั่งเปิดเสียงไซเรนจากหน้าเว็บ
        else if (message.indexOf("\"command\": \"ALARM_ON\"") != -1) {
            isAlarmActive = true;
            alarmTriggerTime = millis();
            
            // เริ่มเสียงเตือน
            beepCount = 0;
            isBeeping = true;
            beepType = 2; // 💡 บังคับให้เป็นเสียงยาว (ไซเรนจากหน้าเว็บ)
            
            Serial.println("🚨 Received: MANUAL ALARM TRIGGERED!");
        }
    }
}

// ฟังก์ชันสำหรับอ่านค่าระยะทางจาก Ultrasonic Sensor
int getDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long duration = pulseIn(echoPin, HIGH, 30000); 
    if (duration == 0) return 999;
    int dist = duration * 0.034 / 2;
    return dist;
}

// ฟังก์ชันวาดปุ่มและเช็คการสัมผัส
bool isButtonPressed(int touchX, int touchY, int btnY) {
    return (touchX >= 40 && touchX <= 280 && touchY >= btnY && touchY <= (btnY + 45));
}

void drawModernButton(int y, String label, bool state) {
    M5.Display.drawRoundRect(40, y, 240, 45, 10, WHITE);
    if (state) {
        M5.Display.fillRoundRect(55, y + 10, 40, 25, 12, GREEN);
        M5.Display.fillCircle(83, y + 22, 9, WHITE);
    } else {
        M5.Display.fillRoundRect(55, y + 10, 40, 25, 12, DARKGREY);
        M5.Display.fillCircle(67, y + 22, 9, WHITE);
    }
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(110, y + 15);
    M5.Display.print(label);
}

void connectToAWS() {
    while (!client.connected()) {
        M5.Display.clear();
        M5.Display.setCursor(10, 10);
        M5.Display.println("Connecting AWS...");
        if (client.connect("m5_radar_station")) {
            M5.Display.println("AWS SUCCESS!");
            // เมื่อต่อ AWS สำเร็จ ให้เริ่ม "ฟัง" คำสั่งจากเว็บ
            client.subscribe("drone/config");
        } else {
            M5.Display.printf("Fail rc=%d\n", client.state());
            delay(5000);
        }
    }
}

// ==========================================
// 🚀 SETUP FUNCTION
// ==========================================
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.begin();
    M5.Display.setTextSize(2);
    
    M5.Speaker.setVolume(150); 

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    ESP32PWM::allocateTimer(0);
    myServo.setPeriodHertz(50);
    myServo.attach(SERVO_PIN, 500, 2400);

    espClient.setCACert(AWS_CERT_CA);
    espClient.setCertificate(AWS_CERT_CRT);
    espClient.setPrivateKey(AWS_CERT_PRIVATE);
    client.setServer(mqtt_server, 8883);
    
    // ตั้งค่าฟังก์ชัน Callback สำหรับรับคำสั่ง MQTT
    client.setCallback(callback);

    WiFi.begin(ssid, password);
}

// ==========================================
// 🚀 MAIN LOOP
// ==========================================
void loop() {
    M5.update();
    if (WiFi.status() != WL_CONNECTED || !client.connected()) {
        connectToAWS();
        return;
    }
    client.loop();

    // ----------------------------------------------------
    // ตรวจจับการกดปุ่ม (จิ้มหน้าจอ หรือ กดปุ่มที่เครื่อง)
    // ----------------------------------------------------
    auto touch = M5.Touch.getDetail();
    bool stateChanged = false; 

    if (touch.wasPressed()) {
        if (isButtonPressed(touch.x, touch.y, 60)) {
            manualAlert = !manualAlert;
        }
        if (isButtonPressed(touch.x, touch.y, 115)) {
            systemOn = !systemOn;
            stateChanged = true;
        }
    }
    
    if (M5.BtnA.wasPressed()) {
        systemOn = !systemOn;
        stateChanged = true;
    }

    // ถ้าผู้ใช้กดปุ่มเปิด/ปิดจาก M5 ให้ส่งสถานะไปบอกหน้าเว็บด้วย
    if (stateChanged) {
        if (systemOn) {
            client.publish("drone/config", "{\"status\": \"ARMED\"}");
        } else {
            client.publish("drone/config", "{\"status\": \"DISARMED\"}");
        }
    }
    // ----------------------------------------------------

    if (millis() - lastUpdate > 300) {
        lastUpdate = millis();
        bool isAlert = false;

        if (systemOn) {
            myServo.write(currentAngle);
            currentDistance = getDistance();

            if (currentDistance > 0 && currentDistance < 20) {
                dangerCount++;
            } else {
                dangerCount = 0;
            }

            if (dangerCount >= 2 && !isAlarmActive) {
                isAlarmActive = true;
                alarmTriggerTime = millis();
                
                // เริ่มเสียงเตือน
                beepCount = 0;
                isBeeping = true;
                beepType = 1; // 💡 บังคับให้เป็นเสียงสั้นเตือนภัย (Sensor จับได้)
            }

            if (isAlarmActive && (millis() - alarmTriggerTime >= 1000)) {
                isAlarmActive = false;
                dangerCount = 0;
            }

            isAlert = M5.BtnB.isPressed() ||
                      manualAlert || (currentDistance > 0 && currentDistance < 20);

            if (moveForward) { 
                currentAngle += 20;
                if (currentAngle >= 180) moveForward = false; 
            } else { 
                currentAngle -= 20;
                if (currentAngle <= 0) moveForward = true; 
            }
        } else {
            currentDistance = 999;
            if (!isAlarmActive) { 
                dangerCount = 0;
            } else {
                if (millis() - alarmTriggerTime >= 2000) {
                    isAlarmActive = false;
                }
            }
        }

        // --- ส่วนวาดหน้าจอ (UI Drawing) ---
        M5.Display.startWrite();
        M5.Display.clear();

        // 🚨 Header สีแดงกะพริบตอนมีคนกดปุ่ม SOUND ALARM
        if (isAlarmActive) {
            M5.Display.fillRoundRect(0, 0, 320, 40, 0, RED);
        } else if (!systemOn) {
            M5.Display.fillRoundRect(0, 0, 320, 40, 0, DARKGREY);
        } else {
            M5.Display.fillRoundRect(0, 0, 320, 40, 0, BLUE);
        }
        
        M5.Display.setTextColor(WHITE);
        M5.Display.setCursor(10, 10);
        if (isAlarmActive) {
            M5.Display.println("!!! ALARM TRIGGERED !!!");
        } else {
            M5.Display.println(systemOn ? "TACTICAL RADAR ON" : "SYSTEM PAUSED");
        }

        drawModernButton(60, "DETECTION", isAlert && systemOn);
        drawModernButton(115, "RADAR SYSTEM", systemOn);
        drawModernButton(170, "AWS CLOUD", client.connected());

        M5.Display.setCursor(10, 220);
        M5.Display.setTextSize(1);
        
        if (!systemOn) {
            M5.Display.setTextColor(LIGHTGREY);
            M5.Display.printf("SYSTEM OFF | PRESS BTN TO START");
        } else if (currentDistance == 999) {
            M5.Display.setTextColor(GREEN);
            M5.Display.printf("ANGLE: %d | SAFE: OUT OF RANGE", currentAngle);
        } else if (currentDistance < 20) {
            M5.Display.setTextColor(RED);
            M5.Display.printf("ANGLE: %d | DANGER ZONE: %d CM", currentAngle, currentDistance);
        } else {
            M5.Display.setTextColor(GREEN);
            M5.Display.printf("ANGLE: %d | SAFE ZONE: %d CM", currentAngle, currentDistance);
        }

        M5.Display.fillRect(230, 215, 85, 22, WHITE);
        M5.Display.setTextColor(BLUE);
        M5.Display.setCursor(235, 220);
        M5.Display.printf("SECURE");

        M5.Display.endWrite();

        // --- ส่งข้อมูลขึ้น AWS ---
        if (systemOn) {
            int check = (currentDistance * 3) % 100;
            String payload = "{\"alert\":" + String(isAlert ? "true" : "false") + 
                             ",\"dist\":" + String(currentDistance) + 
                             ",\"angle\":" + String(currentAngle) + 
                             ",\"hash\":" + String(check) + "}";
            client.publish("drone/alert", payload.c_str());
        }
    }
    
    // ==========================================
    // 🔊 ระบบควบคุมจังหวะเสียง (Non-blocking)
    // ==========================================
    if (isBeeping) {
        // 💡 แยกจังหวะเสียงตามประเภท: เสียงยาว (Web) ใช้ 600ms, เสียงสั้น (Sensor) ใช้ 150ms
        int interval = (beepType == 2) ? 600 : 150;     
        // 💡 แยกความยาวเสียง: เสียงยาวลาก 500ms, เสียงสั้น 100ms
        int toneDuration = (beepType == 2) ? 500 : 100; 

        if (millis() - lastBeepTime >= interval) { 
            lastBeepTime = millis();
            if (beepCount % 2 == 0) {
                M5.Speaker.tone(2500, toneDuration); // เล่นเสียงตามระยะเวลาที่กำหนด
            } 
            
            beepCount++;
            // ดังครบ 3 ครั้ง (นับทั้งเปิด/ปิดรวม 6 จังหวะ)
            if (beepCount >= 6) {
                isBeeping = false;
                beepCount = 0;
            }
        }
    }
}