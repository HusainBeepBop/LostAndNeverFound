# LostAndNeverFound

**LostAndNeverFound** is a localized "Hot or Cold" proximity tracking system built on Bluetooth Low Energy (BLE) technology. Pivoting away from GPS and Wi-Fi synchronization, this project utilizes an ESP32 as a dedicated BLE Beacon and a modern Python-based Desktop Application to visually track the distance to your lost items based on BLE signal strength (RSSI).

## 🚀 Development Stack & Architecture

The project is split into two primary components:

*   **ESP32 Firmware (Transmitter):** Written in C++ using the Arduino IDE. Configures an ESP32 chip to broadcast high-frequency BLE scanning packets resembling an iBeacon.
*   **Python Desktop Tracker (Receiver):** A dynamic interface written in Python 3. It utilizes the **[Flet](https://flet.dev/)** framework for sleek, responsive UI design (utilizing Flutter under the hood) and the **[Bleak](https://bleak.readthedocs.io/)** library for asynchronous bluetooth hardware scanning.

---

## 1. ESP32 BLE Beacon (`/ble_beacon`)

The firmware converts a standard ESP32 module into a localized tracking beacon. It broadcasts a unique Service UUID while utilizing standard iBeacon data payloads to ensure clean visibility.

### Hardware Requirements
*   **ESP32 Dev Module** (tested with ESP32-WROOM-32D)
*   Micro-USB or USB-C cable for power/programming.

### Installation & Flashing
1.  Open the `ble_beacon/ble_beacon.ino` sketch in your Arduino IDE.
2.  Ensure you have the latest **ESP32 Board Definitions (Core 3.x or higher)** installed via the Boards Manager. (Note: earlier versions rely on `std::string`, whereas the modern core integrates Arduino `String` into the BLE Libraries).
3.  Select **ESP32 Dev Module** and set your COM Port.
4.  Upload the sketch.
5.  Upon successful boot, the built-in LED (GPIO 2) will flash once. The beacon will immediately begin anonymously blasting its UUID (`4fafc201-1fb5-459e-8fcc-c5c9c331914b`) into the surrounding area.

---

## 2. Python BLE Desktop Tracker

The software component (`python_tracker_ble.py`) is an elegant desktop utility that parses raw BLE RSSI (Received Signal Strength Indicator) values from the surrounding environment and maps them into a fluid "Hot or Cold" graphical interface.

### Key Features
*   **Real-time Triangulation:** Captures dBm strength to approximate distance.
*   **Dynamic Gradient Animations:** The Flet-powered UI dynamically changes background colors based on signal strength:
    *   🔵 **Freezing** (<-80 dBm): Distant signal.
    *   🟠 **Getting Warmer** (-80 to -60 dBm): Approaching the item.
    *   🔴 **Burning Hot!** (>-60 dBm): Right on top of the item.
*   **Async Asynchronous Backend:** Flet's UI thread and Bleak's device scanner work concurrently via Python `asyncio`.
*   **Lost Device Watchdog:** Automatically reverts to a continuous scanning state if the beacon signal vanishes for more than 3 seconds.

### Setup & Usage

1.  Make sure Python 3.8+ is installed on your machine.
2.  Install all required dependencies using the included `requirements.txt` file (ideally in a `venv`):
    ```bash
    pip install -r requirements.txt
    ```
    *Core dependencies include `flet`, `bleak`, and Windows/BLE extensions where applicable.*
3.  Ensure your OS's Bluetooth hardware is activated.
4.  Run the interface:
    ```bash
    python python_tracker_ble.py
    ```

---

## 📦 Legacy Components (GPS / Firebase Stack)

*Note: The project originally explored a high-latency, long-range cellular architecture utilizing GPS boards (NEO-M8N) and accelerometer motion-gating (MPU6050) broadcasting to Firebase. These experimental repositories remain in the ecosystem for reference but are no longer the active development direction.*

*   **`tracker_main`**: Original MCU luggage tracker with SPIFFS backup and Firebase REST pushes.
*   **`motion_detector`**: Low-pass filtering implementations for the MPU6050.
*   **`gps_test` & `gps_baud_scan`**: Utilities for parsing raw NMEA and UBX GPS patterns.