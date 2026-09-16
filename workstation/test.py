from dotenv import load_dotenv
load_dotenv()

import os
from src.pitahaya_classifier import PitahayaClassifier

def main():

    base_dir = os.path.dirname(os.path.abspath(__file__))
    absolute_path = os.path.join(base_dir, 'dataset', 'dragon_fruit_7.jpg')

    classifier = PitahayaClassifier()
    class_name = classifier.test(absolute_path)

if __name__ == "__main__":
    main()