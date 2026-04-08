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

## Tracker Main (Luggage Tracker Firmware)

This is the primary firmware for the ESP32 luggage tracker. It integrates MPU6050 motion detection, GPS fix acquisition, SPIFFS offline storage, and an automated Wi-Fi upload system that syncs data with Firebase Realtime Database. The system is designed to intelligently balance power consumption with tracking frequency based on motion.

### Key Features
- **Motion-Aware GPS Polling**: Use MPU6050 to gate GPS activation (stationary = 5min interval, moving = 30s)
- **Offline Storage**: Up to 500 location records saved to SPIFFS when offline
- **Firebase Sync**: Automatic POST to Firebase Realtime Database when WiFi is available
- **LED Status Indicators**: 14+ different LED blink patterns for real-time status feedback
- **Secure WiFi**: Uses TLS/SSL for secure uploads
- **Low-Power Design**: Optional GPS power control via GPIO 4 transistor

### Supported Boards
- **ESP32 Dev Module** (e.g., ESP32-WROOM-32D)

### Hardware Requirements
- **ESP32 Dev Module** 
- **NEO-M8N GPS Module** (Quan-Sheng V2.0 tested)
- **MPU6050 IMU** (6-axis accelerometer + gyroscope)
- **Optional:** Transistor/MOSFET (2N7000, etc.) for GPS power control on GPIO 4

### Wiring Diagram

**GPS Module (Serial2):**
- GPS TX (GREEN) → GPIO 16 (RX2)
- GPS GND (BLACK) → GND
- GPS VCC (RED) → 5V or 3.3V
- GPS RX (YELLOW) → Not connected (optional)

**GPS Power Control (Optional but Recommended):**
- GPIO 4 → Gate of N-channel MOSFET 
- MOSFET source → GND
- MOSFET drain → GPS GND (or 5V pull-down via resistor)
- When GPIO 4 = HIGH: GPS powered ON
- When GPIO 4 = LOW: GPS powered OFF

**MPU6050 (I2C):**
- SCL → GPIO 22
- SDA → GPIO 21
- VCC → 3.3V
- GND → GND
- AD0 → GND (I2C address 0x68)

**LED (Built-in):**
- GPIO 2 (already on board)

### Installation & Dependencies

1. **Install ArduinoJson Library:**
   - Open Arduino IDE → Sketch → Include Library → Manage Libraries
   - Search for "ArduinoJson" by Benoit Blanchon
   - Install version 6.x or 7.x

2. **Install ESP32 Board Support:**
   - Preferences → Additional Boards Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Install ESP32 board package via Boards Manager

3. **Select Board:**
   - Board: **ESP32 Dev Module**
   - Baud rate: **115200**

### Configuration

Edit the `CONFIG` section at the top of `tracker_main.ino`:

```cpp
// ===== CONFIG =====
const bool SAVE_BATTERY_MODE = false;           // true = motion-gated GPS, false = GPS every 30s always
const char* WIFI_SSID       = "YourSSID";       // Your WiFi network
const char* WIFI_PASS       = "YourPassword";   // Your WiFi password
const char* FIREBASE_HOST   = "your-db.firebaseio.com";  // Firebase Realtime DB URL
const char* FIREBASE_AUTH   = "YourDatabaseSecret";      // Firebase database secret
const char* DEVICE_ID       = "MASTER";         // Unique device identifier
const int GPS_POWER_PIN     = 4;                // GPIO for GPS power control (optional)
const int MPU_ADDR          = 0x68;             // MPU6050 I2C address
const int SENSITIVITY       = 5;                // Motion sensitivity (1-10, 5=medium)
const int MAX_RECORDS       = 500;              // Max records in SPIFFS before trimming
```

### Firebase Setup

1. **Create a Firebase Project:**
   - Go to [Firebase Console](https://console.firebase.google.com/)
   - Create a new project
   - Enable Realtime Database in **Build** → **Realtime Database**
   - Start in **Test Mode** for development

2. **Get Database Credentials:**
   - Database URL: From **Realtime Database** page, copy the URL (without `https://` and `/`)
   - Database Secret: Go to **Project Settings** → **Service Accounts** → **Database Secrets** → click the icon to copy the secret

3. **Example Firebase JSON Structure:**
```json
{
  "records": [
    {
      "ts": "2024-01-15T12:34:56Z",
      "device_id": "MASTER",
      "fix": true,
      "moving": false,
      "sats": 8,
      "lat": 37.7749,
      "lon": -122.4194,
      "alt": 15.3,
      "spd": 0.5,
      "sent": true
    }
  ]
}
```

### LED Blink Language

The built-in LED (GPIO 2) communicates system status via different blink patterns:

| Pattern | Meaning |
|---------|---------|
| 3 fast blinks | Boot successful |
| 2 medium blinks | SPIFFS mount successful |
| LED solid ON (2s) | SPIFFS mount **FAILED** |
| Slow pulse (2s cycle) | GPS searching for fix |
| 5 rapid blinks | GPS fix acquired! |
| 2 long slow blinks | GPS failed (no fix after 60s) |
| Rapid double-blinks | WiFi connecting... |
| 5 fast blinks + 1 long ON | WiFi upload successful |
| 3 slow blinks | WiFi failed (no connection) |
| 1 short blink | Record saved to SPIFFS |
| 1 tiny blip every 5s | Idle heartbeat (system alive) |

### Operation Modes

**Battery Mode OFF (`SAVE_BATTERY_MODE = false`):**
- GPS acquisition every 30 seconds (constantly)
- MPU6050 idle (skipped entirely)
- WiFi upload cycle every 30 seconds
- Best for active tracking, higher power draw

**Battery Mode ON (`SAVE_BATTERY_MODE = true`):**
- MPU6050 motion detection **enabled**
- When **MOVING**: GPS every 30 seconds + upload every 30s
- When **STATIONARY**: GPS every 5 minutes + put GPS to sleep (GPIO 4 LOW)
- Best for battery life when item is stationary in a bag/luggage

### Serial Monitor Output

Example of a typical tracking cycle:

```
========================================
  LostAndNeverFound - Tracker Firmware
========================================
  Device ID    : MASTER
  Battery Mode : OFF (GPS every 30s)
  Firebase     : your-db.firebaseio.com
  WiFi SSID    : Google Pixel 8
========================================

[INIT ] Bluetooth stopped, WiFi off
[INIT ] GPS power pin (GPIO 4) -> LOW
[INIT ] Mounting SPIFFS...
[INIT ] SPIFFS OK - 24 KB used / 1024 KB total
[INIT ] Ready! First GPS fix in ~30s

[LOOP ] Next GPS in 25s | moving=no | uptime=10s
[LOOP ] ---- GPS cycle starting ----
[GPS ] Powering GPS on...
[GPS ] Searching for fix... (timeout 60s)
[GPS ] Searching... 5s | sats=0 | fix=no
[GPS ] Searching... 10s | sats=5 | fix=no
[GPS ] Searching... 15s | sats=8 | fix=YES
[GPS ] Fix acquired! lat=37.774900 lon=-122.419400 | sats=8 | alt=15.3m | spd=0.5kn
[STORE] Record saved (FIX) -> 2024-01-15T12:34:56Z | sats=8 | moving=no
[WIFI ] 1 unsent record(s). Connecting to "Google Pixel 8"...
[WIFI ] Connected! IP: 192.168.1.100
[WIFI ] POSTing to: https://your-db.firebaseio.com/history.json?auth=...
[WIFI ] Sending 1 records (256 bytes)...
[WIFI ] Upload OK! 1 records sent (HTTP 200)
[WIFI ] WiFi off
[LOOP ] ---- GPS cycle done ----

[LOOP ] Next GPS in 30s | moving=no | uptime=45s
```

### Troubleshooting

**"MPU6050 not found! Check SDA/SCL wiring"**
- Verify GPIO 21 (SDA) and GPIO 22 (SCL) connections
- Check for loose wires on the MPU6050 module
- Try running `mpu6050_test.ino` to isolate the issue

**"SPIFFS mount failed!"**
- LED stays solid ON for 2 seconds
- Flash the device with SPIFFS formatting enabled
- In Arduino IDE: Tools → Erase All Flash Before Sketch Upload

**"GPS not acquiring fix"**
- Ensure GPS antenna is outdoors with clear sky view (30-90 seconds typical)
- Check GPS TX (GREEN) is connected to GPIO 16
- Run `gps_test.ino` to verify GPS communication separately
- Check GPS baud rate (230400 is standard)

**"WiFi connecting but not uploading"**
- Verify WIFI_SSID and WIFI_PASS are correct
- Check Firebase credentials (URL and database secret)
- Ensure Firebase Realtime Database is in **Test Mode** (or update security rules)
- Verify internet connection on the WiFi network

**"Records accumulating, not uploading"**
- Check WiFi SSID/password
- Verify Firebase credentials
- Wait 60+ seconds for first GPS fix (SPIFFS won't sync without at least one record)
- Check serial monitor for HTTP error codes

### Data Format (JSON Lines)

Records stored in `/history.jsonl` (one JSON object per line):

```json
{"ts":"2024-01-15T12:34:56Z","fix":true,"moving":false,"sats":8,"sent":false,"lat":"37.7749","lon":"-122.4194","alt":15.3,"spd":0.5}
{"ts":"2024-01-15T12:35:26Z","fix":true,"moving":false,"sats":8,"sent":true,"lat":"37.7749","lon":"-122.4194","alt":15.2,"spd":0.0}
{"ts":"2024-01-15T12:36:00Z","fix":false,"moving":false,"sats":0,"sent":false,"lat":null,"lon":null,"alt":null,"spd":null}
```

- **ts**: ISO 8601 timestamp (from GPS) or "unknown@{millis}"
- **fix**: Boolean - did GPS acquire a fix?
- **moving**: Boolean - is the device moving (MPU6050 state)?
- **sats**: Number of satellites locked
- **sent**: Has this record been uploaded to Firebase?
- **lat/lon**: Decimal degrees (null if no fix)
- **alt**: Altitude in meters (null if no fix)
- **spd**: Speed in knots (null if no fix)