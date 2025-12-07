import gi
import sys
import time

# Versión de GStreamer
gi.require_version('Gst', '1.0')
from gi.repository import Gst

import cv2
import numpy as np
from ultralytics import YOLO
import socket
import json

# ==========================================
# CONFIGURACIÓN DE RED RW612
# ==========================================
RW612_IP = "172.20.10.7"      # Destino (La tarjeta)
RW612_PORT = 6000             # Puerto UDP

MI_MAC_WIFI_IP = "172.20.10.5" # MAC EN EL HOTSPOT (Origen)

print(f"=== INICIANDO SISTEMA ===")
print(f"Origen (Mac): {MI_MAC_WIFI_IP}")
print(f"Destino (RW612): {RW612_IP}:{RW612_PORT}")

# Socket UDP y AMARRARLO al Wi-Fi
try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    # Esta línea fuerza a que los datos salgan por el Hotspot
    sock.bind((MI_MAC_WIFI_IP, 0)) 
    
    sock.settimeout(0.1) 
except Exception as e:
    print(f"[ERROR CRÍTICO] No se pudo usar la IP {MI_MAC_WIFI_IP}.")
    print("Verifica que sigues conectado al Hotspot 'YadinRdz'.")
    sys.exit(1)

# ==========================================
# LÓGICA DE CONTROL
# ==========================================

def calcular_control(error_x, error_y):
    # Ganancias 
    K_slide = 0.5
    K_yaw   = 0.4
    K_pitch = 0.4

    slide = int(K_slide * error_x)
    yaw   = int(K_yaw   * error_x)
    pitch = int(-K_pitch * error_y)

    # Limitar a +/- 255
    slide = max(min(slide, 255), -255)
    yaw   = max(min(yaw, 255), -255)
    pitch = max(min(pitch, 255), -255)

    return slide, yaw, pitch

def enviar_comandos(slide, yaw, pitch):
    paquete = f"SLIDE:{slide};YAW:{yaw};PITCH:{pitch};"
    
    # Print de depuración
    print(f"[MAC -> RW612] Enviando: {paquete}")

    try:
        sock.sendto(paquete.encode(), (RW612_IP, RW612_PORT))
    except Exception as e:
        print(f"[RED ERROR] Fallo envío: {e}")

# ==========================================
# GSTREAMER PIPELINE
# ==========================================
Gst.init(None)

PIPELINE_STR = """
udpsrc port=5000 caps="application/x-rtp, media=video, clock-rate=90000, encoding-name=H264, payload=96" !
rtph264depay ! avdec_h264 !
videoconvert ! video/x-raw, format=BGR !
appsink name=sink emit-signals=true sync=false max-buffers=1 drop=true
"""

print("Cargando modelo YOLO...")
model = YOLO("yolov8n.pt")
print("Modelo cargado.")

try:
    pipeline = Gst.parse_launch(PIPELINE_STR)
    appsink = pipeline.get_by_name("sink")
    pipeline.set_state(Gst.State.PLAYING)
    print("Esperando video de la Pico Pi...")
except Exception as e:
    print(f"[ERROR GSTREAMER] {e}")
    sys.exit(1)

# ==========================================
# CAPTURA DE FRAMES
# ==========================================
def grab_frame():
    try:
        sample = appsink.emit("pull-sample")
        if sample is None:
            return None
        buffer = sample.get_buffer()
        caps = sample.get_caps()
        h = caps.get_structure(0).get_value("height")
        w = caps.get_structure(0).get_value("width")
        ok, map_info = buffer.map(Gst.MapFlags.READ)
        if not ok: return None
        frame = np.ndarray((h, w, 3), dtype=np.uint8, buffer=map_info.data)
        buffer.unmap(map_info)
        return frame
    except:
        return None

# ==========================================
# BUCLE PRINCIPAL
# ==========================================
print("\n=== LISTO. SISTEMA CORRIENDO ===")

try:
    while True:
        frame = grab_frame()
        
        if frame is None:
            time.sleep(0.01)
            continue

        # Inferencia
        results = model(frame, verbose=False)

        person_boxes = []
        for result in results:
            for box in result.boxes:
                if int(box.cls[0]) == 0: 
                    person_boxes.append(box)

        annotated = frame.copy()
        h, w, _ = frame.shape
        cx_img = w // 2
        cy_img = h // 2

        cv2.rectangle(annotated, (cx_img - 20, cy_img - 20), (cx_img + 20, cy_img + 20), (255, 255, 255), 2)

        if len(person_boxes) > 0:
            best = max(person_boxes, key=lambda b: float(b.conf[0]))
            x1, y1, x2, y2 = best.xyxy[0].cpu().numpy().astype(int)
            cx_obj = (x1 + x2) // 2
            cy_obj = (y1 + y2) // 2

            cv2.rectangle(annotated, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv2.circle(annotated, (cx_obj, cy_obj), 6, (0, 255, 0), -1)

            error_x = cx_obj - cx_img
            error_y = cy_obj - cy_img

            # ENVIAR DATOS
            slide_cmd, yaw_cmd, pitch_cmd = calcular_control(error_x, error_y)
            enviar_comandos(slide_cmd, yaw_cmd, pitch_cmd)

            cv2.putText(annotated, f"CMD: S{slide_cmd} Y{yaw_cmd} P{pitch_cmd}", (10, 30), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        
        cv2.imshow("Mac YOLO -> RW612 Control", annotated)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

except KeyboardInterrupt:
    print("Stop.")
finally:
    pipeline.set_state(Gst.State.NULL)
    cv2.destroyAllWindows()
    sock.close()
