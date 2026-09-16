# 🐉 Clasificación de Pitahaya

> Sistema IoT de visión por computadora diseñado para clasificar el estado de conservación de la pitahaya (*dragon fruit*) en tiempo real. Integra un **ESP32-CAM** para la captura y transmisión inalámbrica, un broker **MQTT**, y un modelo de inferencia **Ultralytics YOLO26**.

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
| (QVGA @ 5fps) | <-------------- |   (MQTT Broker)   |                | (OpenCV + Inferencia YOLO26)        |
+---------------+   `camara/led`   +-------------------+                +-------------------------------------+

```

### Componentes del Flujo

1. **ESP32-CAM (Firmware C / ESP-IDF)**
* Captura fotogramas a resolución **QVGA (320x240)** a ~5 FPS.
* Publica el *buffer* de imagen en formato JPEG directamente en el tópico `camara/frame`.
* Se suscribe al tópico `camara/led` para recibir comandos de control sobre el LED Flash (encendido/apagado).


2. **Broker MQTT (Mosquitto)**
* Actúa como intermediario ligero, distribuyendo la transmisión de fotogramas y comandos de control con baja latencia.


3. **Cliente Python (Recepción, Inferencia y Visualización)**
* **Recepción y Decodificación Concurrente:** Consume el flujo continuo de imágenes (bytes JPEG) desde el broker MQTT.
* **Análisis por IA (YOLO26):** Ejecuta la inferencia en tiempo real sobre cada fotograma recibido para determinar el estado de la fruta y la latencia.
* **Visualización:** Genera una interfaz gráfica que muestra el video de la cámara con un panel superpuesto (*overlay*) con las métricas y la predicción obtenida.