import cv2
from typing import List, Optional

# Constants for capture properties
CAPTURE_PROPERTIES = {
    'POS_MSEC': 0,
    'POS_FRAMES': 1,
    'POS_AVI_RATIO': 2,
    'FRAME_WIDTH': 3,
    'FRAME_HEIGHT': 4,
    'FPS': 5,
    'FOURCC': 6,
    'FRAME_COUNT': 7,
    'FORMAT': 8,
    'MODE': 9,
    'BRIGHTNESS': 10,
    'CONTRAST': 11,
    'SATURATION': 12,
    'HUE': 13,
    'GAIN': 14,
    'EXPOSURE': 15,
    'CONVERT_RGB': 16,
    'WHITE_BALANCE_BLUE_U': 17,
    'RECTIFICATION': 18,
    'MONOCROME': 19,
    'SHARPNESS': 20,
    'AUTO_EXPOSURE': 21,
    'GAMMA': 22,
    'TEMPERATURE': 23,
    'TRIGGER': 24,
    'TRIGGER_DELAY': 25,
    'WHITE_BALANCE_RED_V': 26,
    'ZOOM': 27,
    'FOCUS': 28,
    'GUID': 29,
    'ISO_SPEED': 30,
    'MAX_DC1394': 31,
    'BACKLIGHT': 32,
    'PAN': 33,
    'TILT': 34,
    'ROLL': 35,
    'IRIS': 36,
    'SETTINGS': 37
}

def get_property(cap: cv2.VideoCapture, property_name: str) -> Optional[float]:
    """Get the value of a specified property from the video capture object."""
    property_key = property_name.upper()
    if property_key in CAPTURE_PROPERTIES:
        return cap.get(CAPTURE_PROPERTIES[property_key])
    else:
        raise ValueError(f"Property '{property_name}' is not valid.")

def set_property(cap: cv2.VideoCapture, property_name: str, value: float) -> bool:
    """Set the value of a specified property on the video capture object."""
    property_key = property_name.upper()
    if property_key in CAPTURE_PROPERTIES:
        return cap.set(CAPTURE_PROPERTIES[property_key], value)
    else:
        raise ValueError(f"Property '{property_name}' is not valid.")

def get_valid_properties(cap: cv2.VideoCapture) -> List[str]:
    """Return a list of valid properties that can be accessed from the video capture object."""
    return [key for key in CAPTURE_PROPERTIES.keys() if cap.get(CAPTURE_PROPERTIES[key]) != 0]

# Example usage
if __name__ == "__main__":
    cap = cv2.VideoCapture(0)
    ret, frame = cap.read()
    if ret:
        cv2.imshow('frame', frame)
        valid_properties = get_valid_properties(cap)
        print(valid_properties)
    else:
        print("Failed to capture frame.")
    cap.release()
    cv2.destroyAllWindows()

# P-munchy
