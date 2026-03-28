from flask import Flask, Response, request, jsonify, send_file, render_template
from flask_cors import CORS
import cv2
import numpy as np  
import time
import requests 
import threading
import csv
import sqlite3
import os

app = Flask(__name__)
CORS(app)

# ==========================================
# 🛑 ตั้งค่าระบบ
# ==========================================
SIMULATION_MODE = False  
TELEGRAM_BOT_TOKEN = "8645894418:AAHwF-FotcNBYuuvjycf3qJjRk8J5JvkFr0" 
TELEGRAM_CHAT_ID = "5471802385"

# 📁 สร้างโฟลเดอร์เก็บรูปภาพ (ถ้ายังไม่มี ระบบจะสร้างให้เอง)
SAVE_FOLDER = "drone_picture"
os.makedirs(SAVE_FOLDER, exist_ok=True)

latest_captures = ["", "", "", ""]
INCIDENT_IMG_PATH = os.path.join(SAVE_FOLDER, "latest_incident.jpg") 

latest_incident_info = {
    "time": "NO DATA", "dist": "-", "angle": "-", 
    "battery": "-", "altitude": "-", "pitch": "-", "roll": "-", "yaw": "-"
}

mission_status_text = "STANDBY"

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
    current_time = time.strftime('%Y-%m-%d %H:%M:%S')
    
    # --- 1. Data logging - Text-based (CSV) ---
    file_exists = os.path.isfile('drone_log.csv')
    with open('drone_log.csv', mode='a', newline='', encoding='utf-8') as file:
        writer = csv.writer(file)
        if not file_exists:
            writer.writerow(['Timestamp', 'Event Type', 'Image Path'])
        writer.writerow([current_time, event_type, image_path])

    # --- 2. Data logging - Database (SQLite) ---
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
# 🚁 เชื่อมต่อโดรน Tello (Direct Wi-Fi)
# ==========================================
is_patrolling = False
abort_mission = False

if not SIMULATION_MODE:
    from djitellopy import Tello
    # 💡 ใช้ IP มาตรฐานสำหรับต่อโดรนตรงๆ
    tello = Tello(host='192.168.10.1')
    tello.connect()
    
    try:
        print("🔄 กำลังรีเซ็ตระบบวิดีโอ...")
        tello.streamoff()
    except:
        pass
    time.sleep(1)
    
    tello.streamon()
    print("⏳ กำลังอุ่นเครื่องรับภาพวิดีโอ (รอ 3 วินาที)...")
    tello.get_frame_read() 
    time.sleep(3) 
    print("✅ โดรนเชื่อมต่อสำเร็จและกล้องพร้อมทำงานเต็มรูปแบบ!")

# ==========================================
# 🚀 ระบบรวมภาพและส่ง Telegram
# ==========================================
def process_and_send_telegram(image_paths):
    global mission_status_text
    mission_status_text = "GENERATING REPORT..."
    try:
        imgs = []
        for p in image_paths:
            if os.path.exists(p):
                img = cv2.imread(p)
                img = cv2.resize(img, (320, 240))
                imgs.append(img)
            else:
                imgs.append(np.zeros((240, 320, 3), dtype=np.uint8))
        
        while len(imgs) < 4:
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
                f"🚨 **TACTICAL ALERT** 🚨\n"
                f"โดรนปฏิบัติภารกิจทดสอบกล้อง 4 มุม เรียบร้อย!\n\n"
                f"⏱ **เวลา:** {latest_incident_info['time']}\n"
                f"🔋 **แบตโดรน:** {latest_incident_info['battery']}%\n"
            )
            payload = {"chat_id": TELEGRAM_CHAT_ID, "caption": caption_text, "parse_mode": "Markdown"}
            with open(INCIDENT_IMG_PATH, 'rb') as photo:
                requests.post(url, data=payload, files={"photo": photo})
                
    except Exception as e:
        print(f"❌ ส่ง Telegram พลาด: {e}")

# ==========================================
# 📸 ภารกิจถ่ายภาพ 4 มุม
# ==========================================
def guard_dog_mission():
    global is_patrolling, latest_captures, abort_mission, mission_status_text
    is_patrolling = True
    abort_mission = False
    saved_images = []
    
    for i in range(4): latest_captures[i] = ""
    
    try:
        mission_status_text = "TESTING CAMERA..."
        
        for i in range(4):
            if abort_mission: break 
            
            mission_status_text = f"CAPTURING PHOTO {i+1}/4..."
            time.sleep(1.5) 
            
            ts = int(time.time())
            # 💡 บันทึกรูปลงในโฟลเดอร์ drone_picture
            filepath = os.path.join(SAVE_FOLDER, f"intruder_angle{i+1}_{ts}.jpg")
            
            if not SIMULATION_MODE:
                try:
                    frame = None
                    for attempt in range(5):
                        frame_read = tello.get_frame_read()
                        if frame_read is not None and getattr(frame_read, 'frame', None) is not None:
                            temp_frame = frame_read.frame
                            if np.mean(temp_frame) > 10:
                                frame = temp_frame
                                break
                            else:
                                print(f"⏳ กล้องเพิ่งตื่น ภาพยังมืดอยู่ ขอรับใหม่... ({attempt+1}/5)")
                                time.sleep(1)
                        else:
                            print(f"⏳ รอภาพมุมที่ {i+1} ({attempt+1}/5)...")
                            time.sleep(1)

                    if frame is not None:
                        cv2.imwrite(filepath, frame)
                        saved_images.append(filepath)
                        latest_captures[i] = filepath
                        log_data(f"Test Camera {i+1}", filepath)
                        print(f"✅ ถ่ายรูปที่ {i+1} สำเร็จ! (เซฟที่: {filepath})")
                    else:
                        print(f"⚠️ ยอมแพ้! ดึงภาพมุมที่ {i+1} ไม่ได้")
                        
                except Exception as e:
                    print(f"⚠️ Error แคปรูป: {str(e)}")

        if not abort_mission and len(saved_images) > 0:
            mission_status_text = "SENDING TO TELEGRAM..."
            process_and_send_telegram(saved_images)
            
    except Exception as e:
        print(f"❌ ภารกิจล้มเหลว: {e}")
    finally:
        mission_status_text = "STANDBY"
        is_patrolling = False
        abort_mission = False

# ==========================================
# 🎥 เปิดสตรีมวิดีโอสด
# ==========================================
def generate_frames():
    while True:
        if SIMULATION_MODE:
            time.sleep(0.1)
        else:
            try:
                frame_read = tello.get_frame_read()
                if frame_read is not None and getattr(frame_read, 'frame', None) is not None:
                    frame = frame_read.frame
                    
                    if np.mean(frame) < 5:
                        time.sleep(0.1)
                        continue

                    frame = cv2.resize(frame, (640, 480))
                    cv2.putText(frame, "FLIGHT MODE: DISABLED", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
                    if is_patrolling:
                        cv2.putText(frame, f"STATUS: {mission_status_text}", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
                        
                    ret, buffer = cv2.imencode('.jpg', frame)
                    yield (b'--frame\r\n' b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
                else:
                    time.sleep(0.1)
            except:
                time.sleep(0.1)

# ==========================================
# 🌐 Web API Endpoints 
# ==========================================
@app.route('/')
def home():
    return render_template('index.html')

@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/trigger_alert', methods=['POST'])
def trigger_alert():
    global is_patrolling, latest_incident_info
    if is_patrolling: return {"status": "ignored"}
    
    req_data = request.get_json() or {}
    latest_incident_info["dist"] = req_data.get("dist", "MANUAL")
    latest_incident_info["angle"] = req_data.get("angle", "MANUAL")
    latest_incident_info["time"] = time.strftime('%H:%M:%S')
    
    if not SIMULATION_MODE:
        try: 
            latest_incident_info["battery"] = tello.get_battery()
            latest_incident_info["altitude"] = f"{tello.get_distance_tof() / 100.0}m"
            latest_incident_info["pitch"] = f"{tello.get_pitch()}°"
            latest_incident_info["roll"] = f"{tello.get_roll()}°"
            latest_incident_info["yaw"] = f"{tello.get_yaw()}°"
        except: pass

    threading.Thread(target=guard_dog_mission).start()
    return {"status": "success"}

@app.route('/drone_land', methods=['POST'])
def drone_land():
    global is_patrolling, abort_mission, mission_status_text
    abort_mission = True 
    mission_status_text = "EMERGENCY LANDING..."
    if not SIMULATION_MODE:
        try: tello.send_command_without_return("stop"); tello.land()
        except: pass
    
    def reset_status():
        global is_patrolling, mission_status_text
        time.sleep(2)
        mission_status_text = "STANDBY"
        is_patrolling = False
    threading.Thread(target=reset_status).start()
    return {"status": "success"}

@app.route('/latest_image/<int:index>')
def get_latest_image(index):
    if 0 <= index < 4 and latest_captures[index] != "" and os.path.exists(latest_captures[index]):
        return send_file(latest_captures[index], mimetype='image/jpeg')
    return jsonify({"error": "No image"}), 404

@app.route('/telemetry')
def get_telemetry():
    if SIMULATION_MODE:
        return jsonify({"battery": 100, "altitude": "0.0m", "pitch": "0°", "roll": "0°", "yaw": "0°", "mission_status": mission_status_text})
    else:
        try: 
            return jsonify({
                "battery": tello.get_battery(), "altitude": f"{tello.get_distance_tof() / 100.0}m", 
                "pitch": f"{tello.get_pitch()}°", "roll": f"{tello.get_roll()}°", "yaw": f"{tello.get_yaw()}°",
                "mission_status": mission_status_text
            })
        except: return jsonify({"error": "Failed"}), 500

@app.route('/incident_report')
def get_incident_report():
    if os.path.exists(INCIDENT_IMG_PATH):
        return send_file(INCIDENT_IMG_PATH, mimetype='image/jpeg')
    return jsonify({"error": "No report"}), 404

@app.route('/incident_data')
def get_incident_data():
    return jsonify(latest_incident_info)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False)