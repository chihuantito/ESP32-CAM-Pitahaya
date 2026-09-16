import threading
import cv2
import os
import numpy as np
import paho.mqtt.client as mqtt

class MQTTReceiver:

    def __init__(self):
        self.latest_frame = None
        self.lock = threading.Lock()
        self.client = self._setup_client()

    def _setup_client(self):
        try:
            client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        except AttributeError:
            client = mqtt.Client()
        
        client.on_connect = self._on_connect
        client.on_message = self._on_message
        return client

    def _on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            print("[MQTT] Successfully connected to the broker.")
            client.subscribe(os.getenv('MOSQUITTO_TOPIC_CAMERA'))
        else:
            print(f"[MQTT] Connection error. Code: {rc}")

    def _on_message(self, client, userdata, msg):
        try:
            np_bytes = np.frombuffer(msg.payload, np.uint8)
            frame = cv2.imdecode(np_bytes, cv2.IMREAD_COLOR)
            if frame is not None:
                with self.lock:
                    self.latest_frame = frame
        except Exception as e:
            print(f"[MQTT] Error decoding frame: {e}")

    def start(self):
        self.client.connect(os.getenv('MOSQUITTO_IP'), int(os.getenv('MOSQUITTO_PORT')), 60)
        thread = threading.Thread(target=self.client.loop_forever, daemon=True)
        thread.start()

    def get_frame(self):
        with self.lock:
            return self.latest_frame.copy() if self.latest_frame is not None else None