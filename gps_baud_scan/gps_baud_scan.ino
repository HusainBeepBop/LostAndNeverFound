/**
 * GPS Baud Rate Scanner — Teensy 4.0 or ESP32
 * Tries every common baud rate on Serial4 (Teensy pin 16) or Serial2 (ESP32 pins 16 RX, 17 TX)
 * and reports which one has valid UBX or NMEA data.
 *
 * Just flash and watch the serial monitor at 115200.
 */

#ifdef ESP32
HardwareSerial &gpsSerial = Serial2;
const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;
#else // Teensy
HardwareSerial &gpsSerial = Serial4;
#endif

const uint32_t bauds[] = { 4800, 9600, 19200, 38400, 57600, 115200, 230400 };
const uint8_t NUM_BAUDS = sizeof(bauds) / sizeof(bauds[0]);

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n==========================================");
  Serial.println("  GPS Baud Rate Scanner — Serial4 (Teensy) or Serial2 (ESP32)");
  Serial.println("==========================================\n");

  for (uint8_t i = 0; i < NUM_BAUDS; i++) {
    uint32_t baud = bauds[i];
    Serial.print("Trying "); Serial.print(baud); Serial.print(" ... ");

    // Try normal polarity
    gpsSerial.begin(baud);
    delay(100);
    while (gpsSerial.available()) gpsSerial.read(); // flush
    delay(1500); // collect 1.5 seconds of data

    uint32_t total = 0;
    uint32_t ubx_syncs = 0;
    uint32_t nmea_dollars = 0;
    bool prev_was_b5 = false;
    uint8_t sample[32];
    uint8_t sample_len = 0;

    while (gpsSerial.available()) {
      uint8_t c = gpsSerial.read();
      if (sample_len < 32) sample[sample_len++] = c;
      total++;
      if (c == 0xB5) prev_was_b5 = true;
      else { if (prev_was_b5 && c == 0x62) ubx_syncs++; prev_was_b5 = false; }
      if (c == '$') nmea_dollars++;
    }

    gpsSerial.end();

    Serial.print(total); Serial.print(" bytes, ");
    Serial.print(ubx_syncs); Serial.print(" UBX, ");
    Serial.print(nmea_dollars); Serial.print(" NMEA");

    if (ubx_syncs > 0 || nmea_dollars > 0) {
      Serial.print("  <<<< FOUND! ");
      if (ubx_syncs > 0) Serial.print("[UBX] ");
      if (nmea_dollars > 0) Serial.print("[NMEA] ");
    }
    else if (total > 20) Serial.print("  (garbled)");
    else if (total == 0) Serial.print("  (no data)");

    Serial.println();

    // Print sample hex
    if (total > 0) {
      Serial.print("    hex: ");
      for (uint8_t j = 0; j < sample_len; j++) {
        if (sample[j] < 0x10) Serial.print('0');
        Serial.print(sample[j], HEX);
        Serial.print(' ');
      }
      Serial.println();
    }

    // Try inverted polarity
    #ifdef ESP32
    gpsSerial.begin(baud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN, true);
    #else
    gpsSerial.begin(baud, SERIAL_8N1_RXINV);
    #endif
    delay(100);
    while (gpsSerial.available()) gpsSerial.read();
    delay(1500);

    total = 0; ubx_syncs = 0; nmea_dollars = 0;
    prev_was_b5 = false; sample_len = 0;

    while (gpsSerial.available()) {
      uint8_t c = gpsSerial.read();
      if (sample_len < 32) sample[sample_len++] = c;
      total++;
      if (c == 0xB5) prev_was_b5 = true;
      else { if (prev_was_b5 && c == 0x62) ubx_syncs++; prev_was_b5 = false; }
      if (c == '$') nmea_dollars++;
    }

    gpsSerial.end();

    if (total > 0) {
      Serial.print("  inv:  "); Serial.print(total); Serial.print(" bytes, ");
      Serial.print(ubx_syncs); Serial.print(" UBX, ");
      Serial.print(nmea_dollars); Serial.print(" NMEA");
      if (ubx_syncs > 0 || nmea_dollars > 0) {
        Serial.print("  <<<< FOUND INVERTED! ");
        if (ubx_syncs > 0) Serial.print("[UBX] ");
        if (nmea_dollars > 0) Serial.print("[NMEA] ");
      }
      else if (total > 20) Serial.print("  (garbled)");
      Serial.println();
      if (total > 0) {
        Serial.print("    hex: ");
        for (uint8_t j = 0; j < sample_len; j++) {
          if (sample[j] < 0x10) Serial.print('0');
          Serial.print(sample[j], HEX);
          Serial.print(' ');
        }
        Serial.println();
      }
    }

    Serial.println();
  }

  Serial.println("==========================================");
  Serial.println("  SCAN COMPLETE");
  Serial.println("  Look for <<<< FOUND! above");
  Serial.println("==========================================");
}

void loop() {}
