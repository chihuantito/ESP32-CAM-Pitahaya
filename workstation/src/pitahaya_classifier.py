import os
import time
from ultralytics import YOLO

class PitahayaClassifier:
    def __init__(self):
        self.model = YOLO(os.getenv('MODEL_PATH'))

    def predict(self, frame):
        start_time = time.perf_counter()
        
        # YOLO classification inference
        results = self.model(frame, 
                             imgsz=int(os.getenv('MODEL_IMG_SIZE')), 
                             verbose=True)
        
        # Time measurement in milliseconds
        latency_ms = (time.perf_counter() - start_time) * 1000
        
        # Extract main class and confidence
        probs = results[0].probs
        top1_idx = probs.top1
        class_name = results[0].names[top1_idx]
        confidence = float(probs.top1conf.item())

        # Print log with prediction details
        print(f"Log -> Class: {class_name} | Confidence: {confidence:.4f} | Latency: {latency_ms:.2f} ms")

        # Return only the predicted class name
        return class_name
        
    def test(self, image_path: str):
        """
        Tests the model classification using an image file path.
        """
        if not os.path.exists(image_path):
            raise FileNotFoundError(f"Error: Image not found at path '{image_path}'")
            
        print(f"Starting test for: {image_path}")
        
        # Ultralytics YOLO natively accepts string paths as input
        return self.predict(image_path)