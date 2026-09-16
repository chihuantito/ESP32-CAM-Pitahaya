# 🐉 Sistema de Clasificación de Pitahaya en Tiempo Real (ESP32-CAM + YOLO26 + MQTT)

> Sistema IoT de visión por computadora diseñado para clasificar el estado de conservación de la pitahaya (*dragon fruit*) en tiempo real. Integra un microcontrolador **ESP32-CAM** para la captura y transmisión inalámbrica, un broker **MQTT**, y un modelo de inferencia **Ultralytics YOLO26**.

---

## 🏷️ Clases del Modelo (`YOLO26n-cls`)

El modelo de clasificación fue optimizado para identificar tres estados principales:

| Clase (`class_name`) | Descripción |
| --- | --- |
| **`Good fruit`** | Pitahaya en óptimo estado de conservación. |
| **`Bad fruit`** | Pitahaya en mal estado (dañada, enferma o podrida). |
| **`Without fruit`** | Ausencia de fruta / Fondo o superficie vacía. |

---

## 🛠️ Arquitectura del Sistema

```text
+---------------+   Wi-Fi / MQTT   +-------------------+   Bytes JPEG   +-------------------------------------+
|  ESP32-CAM    | --------------> | Broker Mosquitto  | -------------> | Cliente Python                      |
| (QVGA @ 5fps) | <-------------- |   (MQTT Broker)   |                | (OpenCV + Inferencia YOLO26)       |
+---------------+   `camara/led`   +-------------------+                +-------------------------------------+

```

### Componentes del Flujo

1. **ESP32-CAM (Firmware C / ESP-IDF)**
* Captura fotogramas a resolución **QVGA (320x240)** con una tasa de transferencia de ~5 FPS.
* Publica el *buffer* de imagen en formato JPEG directamente en el tópico `camara/frame`.
* Se suscribe al tópico `camara/led` para recibir comandos de control sobre el LED Flash (encendido/apagado).


2. **Broker MQTT (Mosquitto)**
* Actúa como intermediario ligero distribuyendo la transmisión binaria de fotogramas y los comandos de control con baja latencia.


3. **Cliente Python (Recepción e Inferencia)**
* **`MQTTReceiver`**: Procesa la recepción continua de imágenes usando subprocesos (*multithreading*) y decodifica el flujo en OpenCV de forma segura mediante un cerrojo de sincronización (`threading.Lock`).
* **`PitahayaClassifier`**: Ejecuta la inferencia frame por frame mediante el modelo **YOLO26**, calculando la latencia en milisegundos y seleccionando la clase con el nivel de confianza más alto.
* **`main.py`**: Renderiza la transmisión en vivo, superpone las métricas (FPS, latencia y predicción) y gestiona la interfaz gráfica.