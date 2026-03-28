from flask import Flask, Response, request, jsonify, send_file, render_template
from flask_cors import CORS
import cv2
import numpy as np  
import time
import requests 
import csv
import sqlite3
import os
from datetime import datetime

app = Flask(__name__)
CORS(app)

# ==========================================
# 🛑 ตั้งค่าระบบ Server
# ==========================================
TELEGRAM_BOT_TOKEN = "8645894418:AAHwF-FotcNBYuuvjycf3qJjRk8J5JvkFr0" 
TELEGRAM_CHAT_ID = "5471802385"
SAVE_FOLDER = "drone_picture"
os.makedirs(SAVE_FOLDER, exist_ok=True)

INCIDENT_IMG_PATH = os.path.join(SAVE_FOLDER, "latest_incident.jpg") 
latest_captures = ["", "", "", ""]
latest_incident_info = {
    "time": "NO DATA", "dist": "-", "angle": "-", 
    "battery": "-", "altitude": "-", "pitch": "-", "roll": "-", "yaw": "-"
}

# 💡 ตัวแปรสถานะระบบ
mission_status_text = "STANDBY"
is_patrolling = False
pending_command = "NONE"
latest_frame_bytes = None
latest_telemetry = {"battery": "-", "altitude": "0.0m", "pitch": "0°", "roll": "0°", "yaw": "0°"}

# ==========================================
# 🗄️ Database & Logging (CSV + SQLite)
# ==========================================
def init_db():
    conn = sqlite3.connect('drone_data.db')
    cursor = conn.cursor()
    cursor.execute('''CREATE TABLE IF NOT EXISTS drone_logs 
                      (id INTEGER PRIMARY KEY AUTOINCREMENT, 
                       timestamp TEXT, event_type TEXT, image_path TEXT)''')
    conn.commit()
    conn.close()

init_db()

def log_data(event_type, image_path):
    current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    file_exists = os.path.isfile('drone_log.csv')
    with open('drone_log.csv', mode='a', newline='', encoding='utf-8') as file:
        writer = csv.writer(file)
        if not file_exists:
            writer.writerow(['Timestamp', 'Event Type', 'Image Path'])
        writer.writerow([current_time, event_type, image_path])

    try:
        conn = sqlite3.connect('drone_data.db')
        cursor = conn.cursor()
        cursor.execute("INSERT INTO drone_logs (timestamp, event_type, image_path) VALUES (?, ?, ?)",
                       (current_time, event_type, image_path))
        conn.commit()
        conn.close()
    except Exception as e:
        print(f"⚠️ Database Error: {e}")

# ==========================================
# 🛡️ ฟังก์ชันจัดการรายงาน (Report Generation)
# ==========================================
def process_and_send_telegram():
    global mission_status_text
    mission_status_text = "GENERATING REPORT..."
    try:
        imgs = []
        for p in latest_captures:
            if p != "" and os.path.exists(p):
                img = cv2.imread(p)
                img = cv2.resize(img, (320, 240))
                imgs.append(img)
            else:
                imgs.append(np.zeros((240, 320, 3), dtype=np.uint8))

        top_row = np.hstack((imgs[0], imgs[1]))
        bottom_row = np.hstack((imgs[2], imgs[3]))
        combined_img = np.vstack((top_row, bottom_row))
        
        cv2.putText(combined_img, "INCIDENT REPORT 360", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 255), 2)
        cv2.imwrite(INCIDENT_IMG_PATH, combined_img)

        if TELEGRAM_BOT_TOKEN != "" and TELEGRAM_CHAT_ID != "":
            mission_status_text = "SENDING TO TELEGRAM..."
            url = f"https://api.telegram.org/bot{TELEGRAM_BOT_TOKEN}/sendPhoto"
            caption_text = (
                f"🚨 **TACTICAL ALERT** 🚨\nตรวจพบความผิดปกติในรัศมีเฝ้าระวัง!\n\n"
                f"⏱ **เวลา:** {latest_incident_info['time']}\n"
                f"🔋 **แบตโดรน:** {latest_incident_info['battery']}%\n"
                f"📏 **ระยะตรวจจับ:** {latest_incident_info['dist']} CM"
            )
            payload = {"chat_id": TELEGRAM_CHAT_ID, "caption": caption_text, "parse_mode": "Markdown"}
            with open(INCIDENT_IMG_PATH, 'rb') as photo:
                requests.post(url, data=payload, files={"photo": photo}, timeout=15)
                
    except Exception as e:
        print(f"❌ ระบบ Report ผิดพลาด: {e}")
    finally:
        mission_status_text = "STANDBY"

# ==========================================
# 🔌 API สำหรับ Worker (โดรน Tello)
# ==========================================
@app.route('/api/get_command', methods=['GET'])
def api_get_command():
    global pending_command
    cmd = pending_command
    pending_command = "NONE"
    return jsonify({"command": cmd})

@app.route('/api/telemetry', methods=['POST'])
def api_telemetry():
    global latest_telemetry
    latest_telemetry = request.json
    return "OK"

@app.route('/api/status', methods=['POST'])
def api_status():
    global mission_status_text, is_patrolling
    data = request.json
    mission_status_text = data.get("status", mission_status_text)
    is_patrolling = data.get("is_patrolling", is_patrolling)
    return "OK"

@app.route('/api/video_frame', methods=['POST'])
def api_video_frame():
    global latest_frame_bytes
    latest_frame_bytes = request.data
    return "OK"

@app.route('/api/upload_capture', methods=['POST'])
def api_upload_capture():
    global latest_captures
    index = int(request.form['index'])
    file = request.files['image']
    
    ts = int(time.time())
    filepath = os.path.join(SAVE_FOLDER, f"intruder_angle{index+1}_{ts}.jpg")
    file.save(filepath)
    latest_captures[index] = filepath
    log_data(f"Intruder Capture Angle {index+1}", filepath)
    return "OK"

@app.route('/api/finish_mission', methods=['POST'])
def api_finish_mission():
    global is_patrolling
    process_and_send_telegram()
    is_patrolling = False
    return "OK"

# ==========================================
# 🌐 Web API Endpoints สำหรับ Frontend
# ==========================================
@app.route('/')
def home():
    return render_template('index.html')

# 💡 Route สำหรับดึงไฟล์โลโก้ PNG
@app.route('/drone-cctv-logo.png')
def get_logo():
    if os.path.exists('drone-cctv-logo.png'):
        return send_file('drone-cctv-logo.png', mimetype='image/png')
    return jsonify({"error": "Logo file not found"}), 404

@app.route('/trigger_alert', methods=['POST'])
def trigger_alert():
    global pending_command, latest_incident_info
    req_data = request.get_json() or {}
    
    # ⏱ บันทึกเวลาและข้อมูล Telemetry ณ เสี้ยววินาทีที่เกิดเหตุ
    latest_incident_info["time"] = datetime.now().strftime('%H:%M:%S')
    latest_incident_info["dist"] = req_data.get("dist", "MANUAL")
    latest_incident_info["angle"] = req_data.get("angle", "MANUAL")
    
    latest_incident_info['battery'] = latest_telemetry.get('battery', '-')
    latest_incident_info['altitude'] = latest_telemetry.get('altitude', '-')
    latest_incident_info['pitch'] = latest_telemetry.get('pitch', '-')
    latest_incident_info['roll'] = latest_telemetry.get('roll', '-')
    latest_incident_info['yaw'] = latest_telemetry.get('yaw', '-')
    
    pending_command = "MISSION"
    return jsonify({"status": "success"})

@app.route('/drone_land', methods=['POST'])
def drone_land():
    global pending_command
    pending_command = "LAND"
    return jsonify({"status": "success"})

@app.route('/telemetry')
def get_telemetry():
    data = latest_telemetry.copy()
    data["mission_status"] = mission_status_text
    return jsonify(data)

@app.route('/latest_image/<int:index>')
def get_latest_image(index):
    if 0 <= index < 4 and latest_captures[index] != "" and os.path.exists(latest_captures[index]):
        return send_file(latest_captures[index], mimetype='image/jpeg')
    return jsonify({"error": "No image"}), 404

@app.route('/incident_report')
def get_incident_report():
    if os.path.exists(INCIDENT_IMG_PATH):
        return send_file(INCIDENT_IMG_PATH, mimetype='image/jpeg')
    return jsonify({"error": "No report"}), 404

@app.route('/incident_data')
def get_incident_data():
    return jsonify(latest_incident_info)

# ==========================================
# 🚀 รัน Server
# ==========================================
if __name__ == '__main__':
    print("🚀 Web Server Online (Port 5000)... Waiting for Worker")
    app.run(host='0.0.0.0', port=5000, debug=False)