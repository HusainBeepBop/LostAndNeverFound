# LostAndNeverFound

A collection of utility projects for embedded systems and GPS modules, optimized for **ESP32 Dev Module**.

## GPS Baud Rate Scanner

This Arduino sketch scans common baud rates to determine the correct communication speed for a GPS module. It checks for valid UBX (u-blox binary) or NMEA (text) protocol data.

### Supported Boards
- ESP32 Dev Module (uses Serial2 on GPIO 16 RX, GPIO 17 TX)

### Hardware Requirements
- **ESP32 Dev Module** (e.g., ESP32-WROOM-32D)
- GPS module (tested with NEO-M8N Quan-Sheng V2.0)
- USB connection for serial monitor output

### Installation & Setup
1. **Board Setup in Arduino IDE:**
   - Install ESP32 board package: Go to Preferences → Additional Boards Manager URLs and add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Install the ESP32 board via Boards Manager
   - Select Board: ESP32 Dev Module
   - Set baud rate to 115200

2. **Connect your GPS module to ESP32:**
   - GPS TX (GREEN wire) → GPIO 16 (RX2)
   - GPS RX (YELLOW wire) → Not connected (optional)
   - GPS GND (BLACK wire) → GND
   - GPS 5V (RED wire) → 5V or VIN
   - Compass SCL (WHITE wire) → GPIO 22 (SCL)
   - Compass SDA (ORANGE wire) → GPIO 21 (SDA)

3. Flash the `gps_baud_scan.ino` sketch to your board
4. Open the serial monitor at 115200 baud
5. The sketch will automatically scan baud rates and report findings

### Baud Rates Tested
- 4800
- 9600
- 19200
- 38400
- 57600
- 115200
- 230400

### Output Explanation
For each baud rate, the sketch reports:
- Total bytes received
- Number of UBX sync patterns (0xB5 0x62)
- Number of NMEA sentences ($)
- Sample hex data

It tests both normal and inverted polarity. Look for "<<<< FOUND!" to identify the correct baud rate.

### Example Output
```
Trying 9600 ... 1245 bytes, 0 UBX, 12 NMEA  <<<< FOUND! [NMEA]
    hex: 24 47 50 47 47 41 2C ...
```

### Notes
- The scan takes about 2 minutes to complete (1.5s per baud rate x 2 polarities x 7 rates)
- Ensure your GPS module is powered and transmitting data
- For u-blox modules, it may use UBX protocol at higher baud rates

## GPS Test

This Arduino sketch provides a comprehensive GPS testing utility that parses both UBX binary and NMEA ASCII protocols. It displays real-time GPS data including position, speed, time, and satellite information.

### Supported Boards
- ESP32 Dev Module (uses Serial2 on GPIO 16 RX / GPIO 17 TX, I2C on GPIO 21 SDA / GPIO 22 SCL)

### Hardware Requirements
- **ESP32 Dev Module** (e.g., ESP32-WROOM-32D)
- GPS module (tested with NEO-M8N Quan-Sheng V2.0)
- Optional: I2C IMU/compass module (MPU6050, QMC5883L, or HMC5883L)
- USB connection for serial monitor output

### Wiring
Connect the GPS module using the 5-6 wire cable as follows:

**ESP32 Dev Module:**
- RED → 5V or VIN (if GPS module is 5V-tolerant) OR 3.3V
- BLACK/BROWN → GND
- GREEN → GPIO 16 (Serial2 RX) ← GPS TX
- YELLOW → Not connected (GPS RX - not needed)
- WHITE → GPIO 22 (SCL) ← Compass/MPU6050 I2C SCL
- ORANGE → GPIO 21 (SDA) ← Compass/MPU6050 I2C SDA

### Usage
1. **Board Setup in Arduino IDE:**
   - Install ESP32 board package if not already done
   - Select Board: **ESP32 Dev Module**
   - Set baud rate to **115200**

2. Connect your GPS module according to the wiring above
3. Flash the `gps_test.ino` sketch to your board
4. Open the serial monitor at **115200 baud**
5. The sketch will automatically start parsing GPS data and display status every second

### Serial Commands
- `d` - Toggle raw data dump (shows every byte received from GPS)
- `n` - Toggle NMEA sentence display
- `s` - Run I2C scan to detect compass and other I2C devices

### GPS Data Displayed
- Protocol detected (UBX or NMEA)
- Fix status and satellite count
- HDOP (horizontal dilution of precision)
- Latitude and longitude (if fixed)
- Altitude, speed, and heading
- UTC timestamp
- Data age (ms since last update)

### Example Output
```
--------------------------------------------
Bytes received: 1250
Protocol: NMEA
Fix:  YES  type=1  sats=8  HDOP=1.2
Lat:  37.7749000 deg
Lon:  -122.4194000 deg
Alt:  15.3 m MSL
Speed: 0.0 kn
Crs:  0.0 deg
Time: 12:34:56 UTC  01/15/2024
Age:  250 ms
```

### Notes
- GPS baud rate is set to 230400 (adjust `GPS_BAUD` define if needed)
- Auto-detects UBX binary or NMEA ASCII protocols
- I2C scan helps verify compass connection
- GPS fix may take 30-90 seconds outdoors
- If no data is received, try swapping the GREEN and YELLOW wires on the RX pin

## MPU6050 Test

This Arduino sketch provides a simple, dependency-free utility to read accelerometer, gyroscope, and temperature data from an MPU6050 IMU sensor via the I2C protocol.

### Supported Boards
- ESP32 Dev Module (using I2C on GPIO 21 SDA / GPIO 22 SCL)

### Hardware Requirements
- **ESP32 Dev Module** (e.g., ESP32-WROOM-32D)
- **MPU6050 IMU sensor module** (6-DOF accelerometer + gyroscope)
- USB connection for serial monitor output

### Wiring (MPU6050 to ESP32)
- GND → GND
- VCC → 3.3V
- SDA → GPIO 21 (I2C SDA)
- SCL → GPIO 22 (I2C SCL)
- INT → Not connected (optional)
- AD0 → GND (to use I2C address 0x68)

### Usage
1. **Board Setup in Arduino IDE:**
   - Install ESP32 board package if not already done
   - Select Board: **ESP32 Dev Module**
   - Set baud rate to **115200**

2. Connect the MPU6050 to the I2C pins as shown in the wiring section above
3. Flash the `mpu6050_test.ino` sketch
4. Open the serial monitor at 115200 baud
5. It will display raw accelerometer/gyroscope readings and formatted temperature in Celsius

### Output Format
```
Accel [X: 150  Y: -450  Z: 16200]  Temp: 29.5 C  Gyro [X: 5  Y: -2  Z: 1]
```

## MPU6050 Motion Detector

This Arduino sketch uses an MPU6050 IMU to robustly detect if it is "Stationary" or "Moving" (such as being carried in a bag or pocket).

### Supported Boards
- ESP32 Dev Module (using I2C on GPIO 21 SDA / GPIO 22 SCL, LED on GPIO 2)

### Hardware Requirements
- **ESP32 Dev Module** (e.g., ESP32-WROOM-32D)
- **MPU6050 IMU sensor module** (6-DOF accelerometer + gyroscope)
- USB connection for serial monitor output

### Wiring (MPU6050 to ESP32)
- GND → GND
- VCC → 3.3V
- SDA → GPIO 21 (I2C SDA)
- SCL → GPIO 22 (I2C SCL)
- INT → Not connected (optional)
- AD0 → GND (to use I2C address 0x68)

### LED Indicator
- **GPIO 2 (Built-in LED)**: ON when stationary, OFF when moving

### How It Works
- **Dynamic Acceleration**: Uses a Low-Pass Filter on the accelerometer to map and subtract the baseline pull of gravity. This isolates purely dynamic "jolts" and ignores orientation changes (e.g., slowly turning the sensor over).
- **Absolute Rotation**: Uses raw Gyroscope magnitude to detect physical spinning or swinging motions.
- **Walking-Optimized Debounce**: Uses a full 1-second debounce window to prevent cyclical walking movements from accidentally triggering a "Stationary" reading mid-step.
- **Easy Sensitivity Control**: Includes a simple 1-10 `SENSITIVITY` scale for quickly dialing in the detection threshold.

### Configuration
Edit these constants in the sketch to adjust behavior:
```cpp
const int SENSITIVITY = 5;           // 1-10 scale (5 = medium)
const bool DEBUG_MODE = false;       // Set to true to see raw values
const int STATIONARY_DEBOUNCE = 20;  // Cycles of no motion before "Stationary"
```

### Usage
1. **Board Setup in Arduino IDE:**
   - Install ESP32 board package if not already done
   - Select Board: **ESP32 Dev Module**
   - Set baud rate to **115200**

2. Connect the MPU6050 to the I2C pins as shown in the wiring section above
3. Flash the `motion_detector.ino` sketch
4. Open the serial monitor at 115200 baud
5. Watch the output: "Moving" or "Stationary"
6. The built-in LED (GPIO 2) will light up when truly stationary

### Output Example
```
Moving
Moving
Stationary
Stationary
```

## Tracker Main (Luggage Tracker)

This is the primary firmware for the ESP32 luggage tracker. It integrates MPU6050 motion detection, GPS fix acquisition, SPIFFS offline storage, and an automated Wi-Fi upload system that syncs data with Firebase Realtime Database.

### Supported Boards
- ESP32 Dev Module 

### Hardware Requirements
- **ESP32 Dev Module** 
- **MPU6050 IMU**
- **NEO-M8N GPS Module**

### Setup Instructions

1. **Install Dependencies:**
   - In the Arduino Library Manager, install **ArduinoJson** (version 6 or 7).

2. **Wiring:**
   - **GPS Module:**
     - GPS TX → GPIO 16
     - GPS GND → GND
     - GPS VCC → GPIO 4 (Use this pin as power control for the GPS. Default HIGH when active, LOW when sleeping)
   - **MPU6050 (I2C):**
     - SCL → GPIO 22
     - SDA → GPIO 21

3. **Firebase Setup:**
   - Go to the [Firebase Console](https://console.firebase.google.com/) and create a new project.
   - On the left sidebar, click on **Build** -> **Realtime Database** and click **Create Database**.
   - Start in **Test Mode** (or update rules to be read/write restricted based on requests).
   - Once created, note your database URL format (e.g., `your-project-id-default-rtdb.firebaseio.com`).
   - Go to **Project Settings** (gear icon) -> **Service accounts** -> **Database secrets**. Note down the "Secret" token.

4. **Code Configuration & Preferences:**
   - Open `tracker_main/tracker_main.ino`
   - **`SAVE_BATTERY_MODE`**: Set to `true` to smartly use the MPU6050 to heavily throttle GPS pings while completely stationary. Set to `false` to disable motion logic entirely and aggressively force GPS fetches and server updates every 30 seconds unconditionally.
   - Update `WIFI_SSID` and `WIFI_PASS` with your mobile hotspot or Wi-Fi.
   - Update `FIREBASE_HOST` with your Firebase Realtime DB URL (exclude `https://` and trailing slash).
   - Update `FIREBASE_AUTH` with your Database Secret.
   - Set `DEVICE_ID` to uniquely identify this tracker snippet.

### How it Works
- **Storage:** If you are offline, it aggressively saves GPS fixes (up to a rolling 500 limit) to the internal SPIFFS memory.
- **Motion Profiles:** If `SAVE_BATTERY_MODE` is enabled, the MPU6050 acts as a switch: when moving, the tracker updates every 30 seconds; when perfectly stationary, it updates remotely (every 5 minutes) and forcefully kills GPS power. If `SAVE_BATTERY_MODE` is set to `false`, it disregards motion entirely and always tracks/uploads every 30 seconds.
- **Uploads:** Once in range of a configured Wi-Fi channel, it connects securely (`WiFiClientSecure`), batches all unsent JSON rows, fires a `POST` directly to the Firebase Realtime DB, and marks local records as "sent". The ESP32's built-in LED (GPIO 2) prominently lights up for 1 second upon every successful Wi-Fi upload array to give a clear verification cue.