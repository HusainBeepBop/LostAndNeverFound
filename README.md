# LostAndNeverFound

A collection of utility projects for embedded systems and GPS modules.

## GPS Baud Rate Scanner

This Arduino sketch scans common baud rates to determine the correct communication speed for a GPS module. It checks for valid UBX (u-blox binary) or NMEA (text) protocol data.

### Supported Boards
- Teensy 4.0 (uses Serial4 on pin 16)
- ESP32 (uses Serial2 on pins 16 RX, 17 TX)

### Hardware Requirements
- Compatible microcontroller (Teensy 4.0 or ESP32)
- GPS module connected to the specified serial pins
- USB connection for serial monitor output

### Usage
1. Connect your GPS module to the appropriate pins:
   - Teensy 4.0: GPS TX to pin 16
   - ESP32: GPS TX to pin 16 (RX2)
2. Flash the `gps_baud_scan.ino` sketch to your board
3. Open the serial monitor at 115200 baud
4. The sketch will automatically scan baud rates and report findings

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
- Teensy 4.0 (uses Serial4 on pin 16, I2C on pins 18 SDA / 19 SCL)
- ESP32 (uses Serial2 on pins 16 RX / 17 TX, I2C on pins 21 SDA / 22 SCL)

### Hardware Requirements
- Compatible microcontroller (Teensy 4.0 or ESP32)
- GPS module (tested with NEO-M8N Quan-Sheng V2.0)
- Optional: I2C IMU/compass module (MPU6050, QMC5883L, or HMC5883L)
- USB connection for serial monitor output

### Wiring
Connect the GPS module using the 5-6 wire cable as follows:

**Teensy 4.0:**
- RED → 5V (VIN)
- BLACK/BROWN → GND
- GREEN → Pin 16 (Serial4 RX) ← GPS TX
- YELLOW → Not connected (GPS RX - not needed)
- WHITE → Pin 19 (SCL0) ← Compass/MPU6050 I2C SCL
- ORANGE → Pin 18 (SDA0) ← Compass/MPU6050 I2C SDA

**ESP32:**
- RED → 5V or 3.3V
- BLACK/BROWN → GND
- GREEN → Pin 16 (Serial2 RX) ← GPS TX
- YELLOW → Not connected (GPS RX - not needed)
- WHITE → Pin 22 (SCL) ← Compass/MPU6050 I2C SCL
- ORANGE → Pin 21 (SDA) ← Compass/MPU6050 I2C SDA

### Usage
1. Connect your GPS module according to the wiring above
2. Flash the `gps_test.ino` sketch to your board
3. Open the serial monitor at 115200 baud
4. The sketch will automatically start parsing GPS data and display status every second

### Serial Commands
- `d` - Toggle raw data dump (shows every byte received from GPS)
- `n` - Toggle NMEA sentence display
- `s` - Run I2C scan to detect compass and other I2C devices

### GPS Data Displayed
- Protocol detected (UBX or NMEA)
- Fix status and type
- Number of satellites
- HDOP (Horizontal Dilution of Precision)
- Latitude and Longitude
- Altitude (MSL)
- Speed (knots)
- Course (degrees)
- UTC Time and Date
- Data age (milliseconds since last update)

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
- Teensy (e.g., Teensy 4.0 using pins 18 SDA / 19 SCL)
- ESP32 / ESP8266 (using their default hardware I2C pins, typically 21 SDA / 22 SCL for ESP32)

### Usage
1. Connect the MPU6050 to the appropriate I2C pins for your microcontroller (VCC to 3.3V/5V, GND to GND, SDA to SDA, SCL to SCL).
2. Flash the `mpu6050_test.ino` sketch.
3. Open the serial monitor at 115200 baud.
4. It will display raw accelerometer/gyroscope readings and formatted temperature in Celsius.

## MPU6050 Motion Detector

This Arduino sketch uses an MPU6050 IMU to robustly detect if it is "Stationary" or "Moving" (such as being carried in a bag or pocket). 

### Details
- **Dynamic Acceleration**: Uses a Low-Pass Filter on the accelerometer to map and subtract the baseline pull of gravity. This isolates purely dynamic "jolts" and ignores orientation changes (e.g., slowly turning the sensor over).
- **Absolute Rotation**: Uses raw Gyroscope magnitude to detect physical spinning or swinging motions.
- **Walking-Optimized Debounce**: Uses a full 1-second debounce window to prevent cyclical walking movements from accidentally triggering a "Stationary" reading mid-step.
- **Easy Sensitivity Control**: Includes a simple 1-10 `SENSITIVITY` scale for quickly dialing in the detection threshold.
- Turns the microcontroller's `LED_BUILTIN` **ON** when stationary, and **OFF** when moving.