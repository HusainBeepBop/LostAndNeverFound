# LostAndNeverFound

**LostAndNeverFound** is a localized "Hot or Cold" proximity tracking system built on Bluetooth Low Energy (BLE) technology. Pivoting away from GPS and Wi-Fi synchronization, this project utilizes an ESP32 as a dedicated BLE Beacon and a modern Python-based Desktop Application to visually track the distance to your lost items based on BLE signal strength (RSSI).

## 🚀 Development Stack & Architecture

The project is split into two primary components:

*   **ESP32 Firmware (Transmitter):** Written in C++ using the Arduino IDE. Configures an ESP32 chip to broadcast high-frequency BLE scanning packets resembling an iBeacon.
*   **Python Desktop Tracker (Receiver):** A dynamic interface written in Python 3. It utilizes the **[Flet](https://flet.dev/)** framework for sleek, responsive UI design (utilizing Flutter under the hood) and the **[Bleak](https://bleak.readthedocs.io/)** library for asynchronous bluetooth hardware scanning.

---

## 1. Project Structure: Development vs. Demonstration

The project is organized into two distinct pairs of files to separate raw debugging from the polished, multi-tag tracking system.

*   **Development Pair:** `ble_test.ino` & `python_tracker_ble.py`
    *(Use these files for raw signal testing, calibration, and single-tag debugging).*
*   **Demonstration Pair (RECOMMENDED):** `ble_beacon.ino` & `python_tracker_beacon.py`
    *(Use these files to run the polished tracking interface. This pair supports simultaneously tracking multiple unique tags and filters out unrecognized devices).*

---

## 2. Running the Demonstration

To experience the full visual "Hot or Cold" tracking system, you should utilize the **Demonstration Pair**.

### Step A: Configure and Flash the Firmware (`ble_beacon.ino`)
1.  Open `ble_beacon/ble_beacon.ino` in your Arduino IDE.
2.  Ensure you have the latest **ESP32 Board Definitions (Core 3.x or higher)** installed.
3.  **Rename your Tag:** Look for the `#define TAG_NAME` macro near the top of the file. Change it to a unique identifier for your physical tag (e.g., `#define TAG_NAME "Aastha-Wallet"`).
4.  Select your board (**ESP32 Dev Module**) and flash the sketch. The tag will boot and immediately begin broadcasting wirelessly. 
5.  *(Optional)* Repeat this step for additional ESP32 modules, giving each a unique `TAG_NAME`.

### Step B: Configure the Tracker App (`python_tracker_beacon.py`)
1.  Open `python_tracker_beacon.py` in your code editor.
2.  **Register your Tags:** Locate the `KNOWN_TAGS` python dictionary near the top. Ensure the active dictionary keys exactly match the `TAG_NAME` values you flashed onto your ESP32 boards. The dictionary values dictate what is shown on the UI:
    ```python
    KNOWN_TAGS = {
        "Aastha-Bag":       "Main Bag",
        "Aastha-Backpack":  "Backpack",
        "Aastha-Case":      "Laptop Case",
        "Aastha-Wallet":    "Wallet",
    }
    ```

### Step C: Run and Demonstrate
1.  Ensure your computer's Bluetooth hardware is turned on.
2.  If this is your first time, install the required dependencies (ideally in a `venv`):
    ```bash
    pip install -r requirements.txt
    ```
3.  Launch the tracker application:
    ```bash
    python python_tracker_beacon.py
    ```
4.  **Demonstrate:** The Flet desktop UI will seamlessly render. Move your ESP32 module closer or further away from your computer to watch the user interface fluidly transition gradients based on the live RSSI strength! Use the pill buttons at the bottom of the app to switch tracking context between different registered tags.

---

## 📦 Legacy Components (GPS / Firebase Stack)

*Note: The project originally explored a high-latency, long-range cellular architecture utilizing GPS boards (NEO-M8N) and accelerometer motion-gating (MPU6050) broadcasting to Firebase. These experimental repositories remain in the ecosystem for reference but are no longer the active development direction.*

*   **`tracker_main`**: Original MCU luggage tracker with SPIFFS backup and Firebase REST pushes.
*   **`motion_detector`**: Low-pass filtering implementations for the MPU6050.
*   **`gps_test` & `gps_baud_scan`**: Utilities for parsing raw NMEA and UBX GPS patterns.