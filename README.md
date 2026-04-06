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