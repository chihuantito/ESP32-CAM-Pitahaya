import time
import cv2
from dotenv import load_dotenv
from src.mqtt_receiver import MQTTReceiver
from src.pitahaya_classifier import PitahayaClassifier

load_dotenv()


def main():
    mqtt_receiver = MQTTReceiver()
    mqtt_receiver.start()

    classifier = PitahayaClassifier()

    fps_counter = 0
    start_time = time.time()
    fps_display = 0

    print("[MAIN] System ready. Press 'q' to exit.")

    while True:
        frame = mqtt_receiver.get_frame()

        if frame is not None:
            # Classify the current frame
            class_name = classifier.predict(frame)

            # Format prediction string
            text = f"Prediction: {class_name}"

            # Black outline for high contrast
            cv2.putText(
                frame,
                text,
                (15, 40),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 0, 0),
                4,
                cv2.LINE_AA,
            )
            # Main bright green text
            cv2.putText(
                frame,
                text,
                (15, 40),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.7,
                (0, 255, 0),
                2,
                cv2.LINE_AA,
            )

            # App FPS calculation
            fps_counter += 1
            if (time.time() - start_time) >= 1.0:
                fps_display = fps_counter
                fps_counter = 0
                start_time = time.time()

            cv2.imshow("ESP32-CAM Stream", frame)

        # 200 ms delay matching the ESP32-CAM rate (~5 FPS)
        if cv2.waitKey(100) & 0xFF == ord("q"):
            break

    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()