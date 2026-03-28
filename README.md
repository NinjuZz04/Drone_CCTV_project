# 🚁 Drone CCTV Project - Tactical Surveillance System

A comprehensive intelligent drone surveillance system combining DJI Tello drone control, ultrasonic radar detection, mobile command interface, and cloud-based monitoring. This project creates a unified platform for tactical perimeter surveillance with real-time alerts and automated threat response.

---

## 📋 Table of Contents

1. [Project Overview](#-project-overview)
2. [System Architecture](#-system-architecture)
3. [Hardware Components](#-hardware-components)
4. [Software Stack](#-software-stack)
5. [Installation & Setup](#-installation--setup)
6. [Project Structure](#-project-structure)
7. [Component Details](#-component-details)
8. [How It Works](#-how-it-works)
9. [API Documentation](#-api-documentation)
10. [Configuration](#-configuration)
11. [Deployment](#-deployment)
12. [Troubleshooting](#-troubleshooting)

---

## 🎯 Project Overview

### Purpose
This system is designed to provide **tactical perimeter surveillance** by integrating:
- An autonomous drone for aerial reconnaissance
- An ultrasonic radar for ground-level threat detection
- A multi-interface command center (web & mobile)
- Automated incident reporting via Telegram

### Key Features
✅ **Real-time Video Streaming** from the drone  
✅ **360° Coverage Photography** when threats are detected  
✅ **Ultrasonic Radar Scanning** with servo-controlled sweep  
✅ **PIN-Protected Mobile Command Interface** (M5Stack remote)  
✅ **Web-Based Tactical Dashboard** for monitoring  
✅ **Automated Incident Reports** sent to Telegram  
✅ **Bi-directional MQTT Communication** for all components  
✅ **Database Logging** (CSV + SQLite) for audit trails  
✅ **AWS Cloud Integration** for secure IoT communication  

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    CLOUD (AWS IoT)                              │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ MQTT Broker (SSL/TLS Encrypted)                          │  │
│  │ Topics:                                                  │  │
│  │  • drone/config   (Command & Status)                     │  │
│  │  • drone/alert    (Radar Telemetry)                      │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                      ▲        ▲        ▲
                      │        │        │
         ┌────────────┘        │        └──────────────┐
         │                     │                       │
    ┌────▼────────┐   ┌────────▼────────┐   ┌─────────▼────────┐
    │ M5STACK     │   │ M5STACK         │   │ LAPTOP/SERVER    │
    │ REMOTE      │   │ SENSOR STATION  │   │ (Flask Backend)  │
    │ (Control)   │   │ (Radar)         │   │                  │
    │             │   │                 │   │ • Video Stream   │
    │ • PIN Auth  │   │ • Ultrasonic    │   │ • Web Dashboard  │
    │ • 4 Buttons │   │ • Servo Control │   │ • Worker Process │
    │ • Status    │   │ • Alert Beeps   │   │ • Telegram Bot   │
    └─────────────┘   └─────────────────┘   └──────────────────┘
         3.3V               3.3V                    WiFi
         WiFi              WiFi                   (5GHz)
                                │
                                │ (WiFi Direct)
                          ┌─────▼──────┐
                          │ DJI TELLO  │
                          │   DRONE    │
                          │            │
                          │ • Camera   │
                          │ • Flight   │
                          │ • IMU      │
                          │ • Battery  │
                          └────────────┘
```

---

## 🔧 Hardware Components

### 1. **DJI Tello Drone**
- **Purpose**: Aerial surveillance platform
- **Connection**: Direct WiFi (192.168.10.1)
- **Capabilities**:
  - Video streaming at 720p
  - 13-minute flight time
  - Pitch, roll, yaw sensor data
  - Battery monitoring
  - Time-of-Flight distance sensor

### 2. **M5Stack Remote (Command Interface)**
- **Device**: M5Stack ESP32 with 2-inch TFT display (320×240)
- **Connectivity**: WiFi + AWS IoT (MQTT over SSL)
- **Input**: Capacitive touchscreen
- **Features**:
  - PIN-protected access (default: 1234)
  - Three UI pages (Status, PIN Entry, Command)
  - Real-time system status display
  - Audio feedback (speaker)

**Button Layout (Command Page)**:
```
┌─────────────────────────────────┐
│ RADAR ON/OFF  │ TRIGGER ALARM   │
├─────────────────────────────────┤
│ DRONE TAKEOFF │ DRONE LAND      │
├─────────────────────────────────┤
│    LOCK & EXIT                  │
└─────────────────────────────────┘
```

### 3. **M5Stack Sensor Station (Radar)**
- **Device**: M5Stack ESP32 with same display
- **Sensors**:
  - Ultrasonic Range Sensor (HC-SR04)
    - Trigger Pin: GPIO 17
    - Echo Pin: GPIO 18
    - Detection Range: 2cm - 400cm
  - Servo Motor (MG90S or equivalent)
    - Control Pin: GPIO 8
    - Sweep Range: 0° - 180°
    - Update Interval: 300ms
- **Output**: Audio alarm (2500Hz beep)

**Ultrasonic Logic**:
- Scans 180° horizontally
- Sweeps at 20° increments every 300ms
- Triggers alarm if object < 20cm detected twice
- Differentiates between sensor and manual alerts

### 4. **Server/Laptop**
- **OS**: Linux/Windows/macOS
- **Role**: Central command center
- **Resources**: Minimum 2GB RAM, WiFi capable
- **Outputs**: Telegram notifications, CSV logs, SQLite database

---

## 💻 Software Stack

### Backend
- **Framework**: Flask (Python 3.8+)
- **Real-time Communication**: WebSocket (MJPEG video stream)
- **Video Processing**: OpenCV (cv2)
- **Drone API**: djitellopy library
- **Cloud**: AWS IoT Core (MQTT)
- **Notifications**: Telegram Bot API
- **Database**: SQLite3 + CSV logging

### Firmware
- **Microcontroller**: Arduino IDE / PlatformIO
- **Libraries**:
  - M5Unified (device abstraction)
  - WiFiClientSecure (SSL/TLS)
  - PubSubClient (MQTT)
  - ESP32Servo (servo control)

### Frontend
- **Technology**: Vanilla HTML5/CSS3/JavaScript
- **Styling**: Custom tactical theme with monospace fonts
- **Protocol**: MQTT (Paho JavaScript library)
- **Authentication**: Local storage + PIN verification

### Communication Protocols
- **Drone ↔ Laptop**: WiFi Direct (TCP/UDP)
- **All IoT ↔ Cloud**: AWS IoT MQTT (SSL/TLS 8883)
- **Web Browser ↔ Server**: HTTP/WebSocket (Flask)

---

## 📦 Installation & Setup

### Prerequisites
- Python 3.8+
- pip package manager
- Arduino IDE or PlatformIO
- WiFi network with 5GHz band (for server)
- AWS IoT Core account with certificates
- Telegram Bot API token

### Step 1: Server Setup (Laptop/Raspberry Pi)

```bash
# Clone or download the project
cd Drone_CCTV_Project

# Install Python dependencies
pip install Flask Flask-CORS opencv-python numpy requests djitellopy

# Create necessary folders
mkdir drone_picture

# Run the server
python web_server.py
```

**Server will start at**: `http://0.0.0.0:5000`

### Step 2: Drone (Tello) Setup

```bash
# Place the drone in a WiFi-enabled area
# The drone creates its own WiFi network: "TELLO-XXXXXX"

# Connect your laptop to the drone's WiFi
# Run the drone backend:
python drone/drone_backendbeta.py
```

### Step 3: M5Stack Remote (Command Interface)

**Hardware Setup**:
1. Connect M5Stack to computer via USB
2. Install Arduino IDE or PlatformIO
3. Install M5Stack board support

**Configuration**:
1. Edit `M5_remote/secrets.h`:
```cpp
#define ssid "YOUR_WIFI_SSID"
#define password "YOUR_WIFI_PASSWORD"
#define mqtt_server "your-iot-endpoint.amazonaws.com"
#define AWS_CERT_CA "-----BEGIN CERTIFICATE-----\n..."
#define AWS_CERT_CRT "-----BEGIN CERTIFICATE-----\n..."
#define AWS_CERT_PRIVATE "-----BEGIN RSA PRIVATE KEY-----\n..."
```

2. Upload firmware:
   - Tools → Board → M5Stack
   - Tools → Upload Speed → 1500000
   - Sketch → Upload

### Step 4: M5Stack Sensor (Radar Station)

**Hardware Wiring**:
```
Ultrasonic Sensor:
  VCC → 5V
  GND → GND
  TRIG (GPIO 17) → Trigger Pin
  ECHO (GPIO 18) → Echo Pin

Servo Motor:
  VCC → 5V
  GND → GND
  PWM (GPIO 8) → Signal Pin
```

**Configuration**:
1. Edit `M5_sensor/secrets.h` (same as above)
2. Verify ultrasonic pins match your wiring
3. Upload firmware (same process as remote)

### Step 5: Telegram Bot Setup

1. Create a bot via **@BotFather** on Telegram
2. Get your **Chat ID** by sending a message to **@userinfobot**
3. Update in `web_server.py`:
```python
TELEGRAM_BOT_TOKEN = "YOUR_BOT_TOKEN"
TELEGRAM_CHAT_ID = "YOUR_CHAT_ID"
```

---

## 📁 Project Structure

```
Drone_CCTV_Project/
│
├── drone/
│   └── drone_backendbeta.py          # Tello drone control & video streaming
│       ├── 🎯 Functions:
│       │   ├── generate_frames()     # Live video feed generator
│       │   ├── guard_dog_mission()   # 4-angle photo capture mission
│       │   └── process_and_send_telegram()  # Report generation
│       ├── 🔌 Routes:
│       │   ├── /video_feed           # MJPEG stream
│       │   ├── /trigger_alert [POST] # Start mission
│       │   ├── /drone_land [POST]    # Emergency stop
│       │   ├── /telemetry            # Real-time data
│       │   └── /incident_report      # Retrieve composite image
│       └── 🗄️ Data: drone_data.db, drone_log.csv
│
├── m5stack_remote/
│   ├── M5_remote.ino                 # Command interface firmware
│   │   ├── 🖼️ Pages:
│   │   │   ├── PAGE_STATUS   (System armed status display)
│   │   │   ├── PAGE_PIN      (4-digit PIN entry keypad)
│   │   │   └── PAGE_COMMAND  (2×2 action buttons)
│   │   ├── 🎨 Features:
│   │   │   ├── HUD bracket decorations
│   │   │   ├── Animated status rings
│   │   │   ├── Tactical button styling
│   │   │   └── Distance visualization bars
│   │   ├── 📡 MQTT Topics:
│   │   │   ├── drone/config  (Publish: ARMED/DISARMED)
│   │   │   └── drone/config  (Publish: ALARM_ON, TAKEOFF, LAND)
│   │   └── 🔐 Security:
│   │       ├── PIN validation (default: 1234)
│   │       └── AWS IoT certificates (SSL/TLS)
│   └── secrets.h                     # WiFi & AWS credentials
│
├── m5stack_sensor/
│   ├── M5_sensor.ino                 # Ultrasonic radar station
│   │   ├── 📡 Sensors:
│   │   │   ├── getDistance()         # HC-SR04 ultrasonic
│   │   │   ├── myServo.write()       # Servo sweep control
│   │   │   └── MQTT callback         # Command receiver
│   │   ├── 🚨 Logic:
│   │   │   ├── Dual-confirm detection (2 consecutive < 20cm)
│   │   │   ├── Manual alarm trigger
│   │   │   ├── 300ms scan interval
│   │   │   └── 20° sweep increments
│   │   ├── 🔊 Audio:
│   │   │   ├── Sensor alert: 150ms beep
│   │   │   └── Manual alarm: 600ms beep (3× cycle)
│   │   ├── 📊 Display:
│   │   │   ├── Current angle
│   │   │   ├── Distance reading
│   │   │   ├── System status
│   │   │   └── AWS connection indicator
│   │   └── 📤 MQTT Publish:
│   │       └── drone/alert (JSON: distance, angle, alert flag)
│   └── secrets.h                     # WiFi & AWS credentials
│
├── server/
│   ├── web_server.py                 # Flask backend (alternative route)
│   │   ├── 🔌 Worker API Routes:
│   │   │   ├── /api/get_command      # Poll for pending commands
│   │   │   ├── /api/telemetry        # Receive drone data
│   │   │   ├── /api/upload_capture   # Receive captured images
│   │   │   └── /api/finish_mission   # Signal mission complete
│   │   ├── 🌐 Public Routes:
│   │   │   ├── /                     # Main dashboard
│   │   │   ├── /telemetry            # JSON telemetry
│   │   │   ├── /trigger_alert [POST] # Start mission
│   │   │   └── /incident_report      # Composite image
│   │   └── 🗄️ Same database as drone backend
│   └── templates/
│       ├── GUI.html                  # Tactical web dashboard
│       │   ├── 🔑 Auth:
│       │   │   ├── Login screen with password
│       │   │   └── localStorage persistence
│       │   ├── 📊 Dashboard Panels:
│       │   │   ├── Live video stream (MJPEG)
│       │   │   ├── Telemetry readout (Battery, altitude, angles)
│       │   │   ├── Image gallery (4 angle captures)
│       │   │   ├── Radar signal indicator
│       │   │   ├── Incident report viewer
│       │   │   └── System log viewer
│       │   ├── 🎮 Controls:
│       │   │   ├── Trigger Alert button
│       │   │   ├── Emergency Land button
│       │   │   ├── Manual Radar toggle
│       │   │   └── Audio alert button
│       │   └── 🎨 Theme: Dark tactical UI with monospace font
│       └── drone-cctv-logo.png       # Project branding
│
└── assets/
    └── drone-cctv-logo.png           # PNG logo file
```

---

## 🔍 Component Details

### 1. Drone Backend (`drone/drone_backendbeta.py`)

**Configuration Variables**:
```python
SIMULATION_MODE = False              # Set True for testing without drone
TELEGRAM_BOT_TOKEN = "XXX"          # Your bot token
TELEGRAM_CHAT_ID = "XXX"            # Your chat ID
SAVE_FOLDER = "drone_picture"       # Image storage location
```

**Key Functions**:

**`generate_frames()`**
- Continuously reads frames from Tello drone
- Resizes to 640×480
- Adds status text overlay
- Encodes as MJPEG for HTTP streaming
- Skips dark frames (camera warmup protection)

**`guard_dog_mission()`**
- Mission sequence when alert triggered
- Captures 4 photos with 1.5s delay between each
- Retry mechanism (5 attempts per angle)
- Saves to `drone_picture/intruder_angleN_TIMESTAMP.jpg`
- Logs to both CSV and SQLite
- Triggers Telegram report at end

**`process_and_send_telegram(image_paths)`**
- Combines 4 images into 2×2 grid (640×480 total)
- Adds "INCIDENT REPORT 360" label
- Posts to Telegram Bot API with metadata
- Includes: time, battery %, distance, angle

**Database Schema** (SQLite):
```
drone_logs:
  id (INTEGER PRIMARY KEY)
  timestamp (TEXT) - YYYY-MM-DD HH:MM:SS
  event_type (TEXT) - e.g., "Test Camera 1", "MISSION_START"
  image_path (TEXT) - Full path to saved image
```

**API Routes**:
- `GET /` - Renders index.html
- `GET /video_feed` - MJPEG stream (multipart/x-mixed-replace)
- `POST /trigger_alert` - Start guard_dog_mission()
  - Input: `{dist, angle}` (optional)
  - Response: `{status: "success"}`
- `POST /drone_land` - Emergency stop
- `GET /latest_image/<index>` - Get angle N capture
- `GET /telemetry` - Real-time drone stats
- `GET /incident_report` - Composite 4-angle image
- `GET /incident_data` - Metadata (time, battery, distance)

---

### 2. M5Stack Remote (`m5stack_remote/M5_remote.ino`)

**State Machine**:
```
PAGE_STATUS (default)
  └─ [Touch center area]
     └─> PAGE_PIN
         └─ [Enter CORRECT_PIN = "1234"]
            └─> PAGE_COMMAND
                ├─ [RADAR button] → Publish ARMED/DISARMED
                ├─ [ALARM button] → Publish ALARM_ON
                ├─ [TAKEOFF button] → Publish TAKEOFF
                ├─ [LAND button] → Publish LAND
                └─ [LOCK & EXIT] → Back to PAGE_STATUS
```

**Display Elements**:

**PAGE_STATUS**:
- Central HUD with animated rings
  - **Armed** (Red rings): Crosshair + "ARMED" text
  - **Disarmed** (Grey rings): "SAFE" text
  - **Alert** (Pulsing red): Triple ring + "ALERT!!!"
- Distance visualization bar (bottom)
- Center area clickable to unlock

**PAGE_PIN**:
- 3×4 numpad (1-9, DEL, 0, OK)
- Displays entered dots: `●●●●`
- Pin message feedback: "ENTER PIN" / "WRONG PIN!" / "CORRECT PIN ACCEPTED"
- Back arrow (top-left) to cancel

**PAGE_COMMAND**:
- 2×2 button grid:
  - Top-left: RADAR (Cyan) - Toggles ARMED/DISARMED
  - Top-right: TRIGGER (Orange) - Manual alarm
  - Bottom-left: TAKEOFF (Dark Cyan)
  - Bottom-right: LAND (Red)
- Lock & Exit button (bottom)
- System status indicator (green/red)

**MQTT Configuration**:
```cpp
mqtt_server = "your-iot-endpoint.amazonaws.com"
port = 8883 (SSL/TLS)
client_id = "m5_remote"
topic_subscribe = "drone/config"
topic_publish = "drone/config"
```

**Color Palette** (16-bit 565 format):
```
C_BG = 0x0000 (Black)
C_NAVY = 0x000F (Dark blue)
C_CYAN = 0x07FF (Bright cyan)
C_GREEN = 0x07E0 (Bright green)
C_RED = 0xF800 (Bright red)
C_ORANGE = 0xFD20 (Orange)
C_WHITE = 0xFFFF
C_GREY = 0x7BEF (Mid grey)
```

---

### 3. M5Stack Sensor (`m5stack_sensor/M5_sensor.ino`)

**Hardware Configuration**:
```cpp
const int trigPin = 17;   // Ultrasonic trigger
const int echoPin = 18;   // Ultrasonic echo
const int SERVO_PIN = 8;  // Servo PWM
```

**Distance Calculation**:
```
duration = pulseIn(echoPin, HIGH, 30000us)
distance_cm = duration * 0.034 / 2
             = (microseconds * 343m/s) / 2
```

**Servo Sweep Pattern**:
- Starts at 0°
- Increments 20° every 300ms
- Reaches 180°, reverses
- Cycle period: ~5.4 seconds

**Threat Detection Logic**:
```
if (distance < 20cm) {
  dangerCount++
}

if (dangerCount >= 2) {
  ALARM ACTIVATED
  Sound: 2500Hz beep (100ms)
  Interval: 150ms (3× total)
}

Alarm auto-resets after 1000ms if threat clears
```

**Dual-Alert System**:
- **Beep Type 1** (Sensor-triggered): 150ms intervals, 100ms duration
- **Beep Type 2** (Manual alarm from web): 600ms intervals, 500ms duration

**MQTT Messages**:

*Subscribe* (`drone/config`):
```json
{"status": "ARMED"}     // Enable radar
{"status": "DISARMED"}  // Disable radar
{"command": "ALARM_ON"} // Manual alarm trigger (5 beeps)
```

*Publish* (`drone/alert`):
```json
{
  "alert": true,        // Threat detected
  "dist": 15,           // Distance in cm
  "angle": 120,         // Current servo angle
  "hash": 45            // Checksum for verification
}
```

**Display Layout**:
```
┌─ TACTICAL RADAR ON ─────────────────┐
├─────────────────────────────────────┤
│ DETECTION: OFF/ON ♦               │  Status indicator
│ RADAR SYSTEM: ON/OFF ♦            │  Toggle button
│ AWS CLOUD: CONNECTED ♦            │  Connection status
├─────────────────────────────────────┤
│ ANGLE: 45 | SAFE ZONE: 150 CM     │  Real-time metrics
│                    [ SECURE ]       │
└─────────────────────────────────────┘
```

---

### 4. Web Dashboard (`server/templates/GUI.html`)

**Authentication**:
- Prompt for password on page load
- Validates against hardcoded password
- Stores in localStorage as `drone_auth`
- Auto-login on refresh if token exists

**Layout** (2-column):

**Left Panel (350px)**:
- Live video stream (MJPEG from `/video_feed`)
- Real-time telemetry (battery, altitude, angles)
- Current incident data

**Right Panel (Responsive)**:
- Video feed live stream
- Control buttons (Trigger, Land)
- Radar status indicator
- 4-angle capture gallery
- System log viewer
- Mission status

**Key JavaScript Functions**:
```javascript
updateTelemetry()        // Poll /telemetry every 500ms
displayVideoFeed()       // Fetch /video_feed stream
triggerAlert()           // POST /trigger_alert
emergencyLand()          // POST /drone_land
getLatestImage(index)    // Fetch /latest_image/<index>
getIncidentReport()      // Fetch /incident_report
```

**Styling**:
- Dark tactical theme (#050a0f background)
- Monospace font: "Share Tech Mono"
- Border color: #1a3c54 (dark cyan)
- Text color: #70b8d9 (light cyan)
- Alert: #ff3b3b (bright red)
- Safe: #00ffaa (bright green)

---

## ⚙️ How It Works

### 1. System Startup Sequence

```
[Startup]
  │
  ├─► Server (Flask) starts on localhost:5000
  │
  ├─► M5Stack Remote: Connects to WiFi → MQTT → Waits for PIN input
  │
  ├─► M5Stack Sensor: Connects to WiFi → MQTT → Subscribes to drone/config
  │
  └─► Drone: Connects directly to server WiFi → Video stream ready
```

### 2. Normal Operation (Standby)

```
[Standby Loop - Every 500ms]
  │
  ├─► Radar scans 180°, publishes distance/angle to MQTT
  │   └─ If distance < 20cm: dangerCount++
  │
  ├─► Web dashboard: Polls /telemetry, displays live video
  │
  └─► M5Stack Remote: Displays system status (ARMED/DISARMED)
```

### 3. Threat Detection Workflow

```
[Radar detects object < 20cm twice consecutively]
  │
  ├─► M5Stack Sensor: Activates alarm beep (150ms × 3)
  │
  ├─► Publishes to drone/alert: {alert: true, dist: 15, angle: 120}
  │
  └─► Waits for human decision...
      │
      ├─ [Human presses TRIGGER on Remote OR Web]
      │  │
      │  ├─► Drone mission starts (guard_dog_mission)
      │  │   ├─ Captures 4 angles: 0°, 90°, 180°, 270°
      │  │   └─ Each with 1.5s delay, 5 retry attempts
      │  │
      │  ├─► Combines images into 2×2 grid
      │  │
      │  ├─► Sends to Telegram Bot API with metadata:
      │  │   • Time: 14:23:45
      │  │   • Battery: 72%
      │  │   • Distance: 15cm
      │  │   • Angle: 120°
      │  │
      │  └─► Database logs incident
      │
      └─ [False alarm resets after 1000ms with no new detections]
```

### 4. Emergency Landing

```
[User presses LAND on Remote/Web]
  │
  ├─► M5Stack Remote sends: {command: "LAND"} to drone/config
  │
  ├─► Drone backend receives via Flask
  │
  ├─► Executes: tello.land()
  │
  ├─► Sets mission_status = "EMERGENCY LANDING..."
  │
  └─► Resets to STANDBY after 2 seconds
```

---

## 🔌 API Documentation

### Flask Backend Endpoints

#### **GET** `/`
Returns the main HTML dashboard.

#### **GET** `/video_feed`
Streams live video from drone.
- **Content-Type**: `multipart/x-mixed-replace; boundary=frame`
- **Format**: MJPEG (Motion JPEG)
- **Resolution**: 640×480
- **Updates**: Real-time

#### **POST** `/trigger_alert`
Initiates the guard_dog_mission.
- **Request Body**:
  ```json
  {
    "dist": "15",    // (optional) Distance in cm
    "angle": "120"   // (optional) Angle in degrees
  }
  ```
- **Response**: `{status: "success"}`
- **Side Effects**:
  - Captures 4 angles
  - Sends Telegram report
  - Logs to database

#### **POST** `/drone_land`
Emergency landing sequence.
- **Response**: `{status: "success"}`

#### **GET** `/telemetry`
Real-time drone telemetry.
- **Response**:
  ```json
  {
    "battery": 72,
    "altitude": "1.2m",
    "pitch": "-5°",
    "roll": "2°",
    "yaw": "45°",
    "mission_status": "STANDBY"
  }
  ```

#### **GET** `/latest_image/<index>`
Retrieve captured image by angle.
- **Parameters**: `index` = 0, 1, 2, or 3
- **Response**: JPEG image
- **Status**: 404 if not available

#### **GET** `/incident_report`
Composite 2×2 image of all 4 angles.
- **Response**: JPEG image (640×480)
- **Label**: "INCIDENT REPORT 360"

#### **GET** `/incident_data`
Metadata about the incident.
- **Response**:
  ```json
  {
    "time": "14:23:45",
    "dist": "15",
    "angle": "120",
    "battery": "72",
    "altitude": "1.2m",
    "pitch": "-5°",
    "roll": "2°",
    "yaw": "45°"
  }
  ```

### MQTT Topics

#### **Topic**: `drone/config`
Bi-directional configuration and commands.

*From Remote/Web to Sensor*:
```json
{"status": "ARMED"}              // Start radar
{"status": "DISARMED"}           // Stop radar
{"command": "ALARM_ON"}          // Trigger manual alarm
{"command": "TAKEOFF"}           // Drone takeoff
{"command": "LAND"}              // Drone land
```

#### **Topic**: `drone/alert`
From sensor to server, published every 300ms.
```json
{
  "alert": true,     // Threat detected
  "dist": 18,        // Distance cm
  "angle": 45,       // Servo angle degrees
  "hash": 54         // Distance × 3 % 100 (verify checksum)
}
```

---

## ⚙️ Configuration

### System Settings

**`drone_backendbeta.py` (or `web_server.py`)**:
```python
# Simulation mode (test without hardware)
SIMULATION_MODE = False

# Telegram notifications
TELEGRAM_BOT_TOKEN = "8645894418:AAHwF-FotcNBYuuvjycf3qJjRk8J5JvkFr0"
TELEGRAM_CHAT_ID = "5471802385"

# Image storage
SAVE_FOLDER = "drone_picture"
```

**`m5stack_remote/secrets.h`**:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* mqtt_server = "XXXXXX.iot.region.amazonaws.com";

// AWS IoT certificates (download from AWS console)
const char* AWS_CERT_CA = "-----BEGIN CERTIFICATE-----\n...";
const char* AWS_CERT_CRT = "-----BEGIN CERTIFICATE-----\n...";
const char* AWS_CERT_PRIVATE = "-----BEGIN RSA PRIVATE KEY-----\n...";
```

**`m5stack_remote.ino` - PIN Code**:
```cpp
const String CORRECT_PIN = "1234";  // Change this!
```

**`m5stack_sensor/secrets.h`**: Same as remote

### AWS IoT Configuration

1. **Create Thing**:
   - AWS IoT Console → Things → Create Thing → Name: "drone-cctv"

2. **Create Policy**:
   ```json
   {
     "Version": "2012-10-17",
     "Statement": [
       {
         "Effect": "Allow",
         "Action": "iot:*",
         "Resource": "*"
       }
     ]
   }
   ```

3. **Create Certificates**:
   - Attach to Thing
   - Download: CA cert, device cert, private key
   - Paste into `secrets.h`

4. **MQTT Broker Settings**:
   - Port: 8883 (SSL/TLS)
   - Protocol: MQTT v3.1.1
   - Keep-Alive: 60 seconds

---

## 🚀 Deployment

### Local Network Deployment

**Network Requirements**:
- Server WiFi: 5GHz capable, 2+ Mbps bandwidth
- Drone WiFi: 2.4GHz direct connection to server
- M5Stack devices: 2.4GHz to same network as server

**IP Addressing**:
```
Server:          192.168.x.x (auto-assigned)
Drone Tello:     192.168.10.1 (direct WiFi)
M5Stack Remote:  192.168.x.x (auto-assigned)
M5Stack Sensor:  192.168.x.x (auto-assigned)
```

**Starting Services**:

```bash
# Terminal 1: Start Flask server
cd Drone_CCTV_Project
python web_server.py

# Terminal 2: Start drone backend (if not using web_server.py)
python drone/drone_backendbeta.py

# Terminal 3: Power on M5Stack devices
# (They auto-connect via firmware)
```

### Cloud Deployment (AWS)

**Option 1: Lightweight (Raspberry Pi)**
- Deploy Flask on Raspberry Pi 4
- Use AWS IoT for MQTT only
- Local video storage

**Option 2: Full Cloud (Lambda + S3)**
- API Gateway → Lambda → RDS
- S3 for image storage
- CloudWatch for logging

**Option 3: Docker Container**
```dockerfile
FROM python:3.9-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY server/ .
CMD ["python", "web_server.py"]
```

---

## 🔧 Troubleshooting

### Common Issues

#### **1. Drone Won't Connect**
```
Error: "Cannot connect to Tello at 192.168.10.1"
```
**Solutions**:
- Ensure drone is powered on
- Check WiFi: Device sees "TELLO-XXXXXX" network
- Connect laptop to drone WiFi first
- Restart drone (power off/on)
- Check battery level (minimum 50%)

#### **2. M5Stack Not Connecting to MQTT**
```
Error: "rc=3" or "rc=4" in AWS connection
```
**Solutions**:
- Verify AWS certificate paths in `secrets.h`
- Check MQTT broker endpoint is correct
- Ensure WiFi credentials are accurate
- Verify M5Stack is on same network as broker
- Check AWS IoT policy allows connect action

#### **3. Video Stream Lags or Stops**
```
Issue: MJPEG stream freezes frequently
```
**Solutions**:
- Reduce video resolution (currently 640×480)
- Increase WiFi signal strength
- Reduce drone distance from server
- Check server CPU usage
- Restart `generate_frames()` thread

#### **4. Ultrasonic Sensor Returns 999**
```
Issue: "SAFE: OUT OF RANGE" always shows
```
**Solutions**:
- Check trigger/echo pin wiring (GPIO 17/18)
- Verify sensor power (5V)
- Test with multimeter: TRIG pulse should see voltage
- Check ultrasonic sensor isn't damaged
- Clean sensor lens

#### **5. Telegram Notifications Not Arriving**
```
Issue: Incident reports don't post to Telegram
```
**Solutions**:
- Verify bot token format (doesn't include "Bot")
- Check chat ID is numeric (from @userinfobot)
- Test token: `curl https://api.telegram.org/botXXX/getMe`
- Ensure image composite is valid JPEG
- Check internet connection on server

#### **6. Database Errors**
```
Error: "database is locked" or SQLite errors
```
**Solutions**:
- Ensure only one instance of Flask running
- Check file permissions on `drone_data.db`
- Manually delete corrupt DB: `rm drone_data.db`
- Backend will recreate on next start
- Check disk space (min 1GB recommended)

#### **7. PIN Entry Not Working**
```
Issue: Keypad inputs not registering
```
**Solutions**:
- Calibrate M5Stack touch: Menu → Settings → Touch Calib
- Check if in correct page (PAGE_PIN should show keypad)
- Verify PIN is exactly "1234" (or your configured PIN)
- Restart M5Stack firmware upload
- Test with different coordinates

---

## 📊 Monitoring & Logging

### CSV Log Format
```
Timestamp,Event Type,Image Path
2024-01-15 14:23:45,Test Camera 1,drone_picture/intruder_angle1_1705329825.jpg
2024-01-15 14:24:12,MISSION_START,drone_picture/intruder_angle2_1705329852.jpg
```

### SQLite Queries
```sql
-- Recent incidents
SELECT * FROM drone_logs WHERE event_type LIKE '%Test Camera%' ORDER BY timestamp DESC LIMIT 10;

-- Count missions
SELECT COUNT(*) FROM drone_logs WHERE event_type = 'MISSION_START';

-- Export to CSV
SELECT * FROM drone_logs;
```

### Database Maintenance
```bash
# Backup database
cp drone_data.db drone_data.db.backup

# Vacuum (optimize)
sqlite3 drone_data.db VACUUM;

# View table structure
sqlite3 drone_data.db ".schema drone_logs"
```

---

## 🔐 Security Considerations

### Current Protections
✅ PIN protection on M5Stack remote (hardcoded PIN)  
✅ AWS IoT SSL/TLS encryption for MQTT  
✅ No public internet exposure (local network only)  

### Recommended Improvements
- [ ] Change default PIN ("1234")
- [ ] Implement rate limiting on API endpoints
- [ ] Add password authentication to web dashboard
- [ ] Enable AWS IoT policy enforcement
- [ ] Use VPN for remote access
- [ ] Encrypt image storage on server
- [ ] Implement JWT tokens for API auth
- [ ] Monitor AWS IoT connection logs

---

## 📝 File Checksums (Reference)

```
drone/drone_backendbeta.py     ~   Python
m5stack_remote/M5_remote.ino   ~   Arduino
m5stack_sensor/M5_sensor.ino   ~   Arduino
server/web_server.py           ~   Python
templates/GUI.html             ~   HTML/CSS/JS
```

---

## 🎓 Learning Resources

- **DJI Tello SDK**: https://github.com/davisking/tello-drone
- **M5Stack Docs**: https://docs.m5stack.com
- **AWS IoT**: https://docs.aws.amazon.com/iot/
- **OpenCV**: https://docs.opencv.org/
- **Flask**: https://flask.palletsprojects.com/
- **MQTT Protocol**: https://mqtt.org/

---

## 📄 License & Attribution

This project combines open-source libraries and custom firmware. Please respect:
- DJI Tello SDK (Apache 2.0)
- Flask (BSD)
- OpenCV (Apache 2.0)
- M5Stack libraries (MIT)
- PubSubClient (MIT)

---

## 🤝 Support & Contribution

For issues, questions, or improvements:

1. **Check troubleshooting** section above
2. **Review GitHub issues** in djitellopy repo
3. **Consult AWS IoT documentation**
4. **Test with SIMULATION_MODE=True** for isolated debugging

---

**Last Updated**: March 2024  
**Project Status**: Active & Maintained  
**Tested On**: Python 3.8+, Arduino 1.8.19+, M5Stack CoreS3
