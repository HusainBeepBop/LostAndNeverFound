/**
 * GPS Baud Rate Scanner — ESP32 Dev Module
 * Tries every common baud rate on Serial2 (GPIO 16 RX, GPIO 17 TX)
 * and reports which one has valid UBX or NMEA data.
 *
 * Just flash and watch the serial monitor at 115200.
 */

// ESP32 Dev Module pin definitions
const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;

const uint32_t bauds[] = { 4800, 9600, 19200, 38400, 57600, 115200, 230400 };
const uint8_t NUM_BAUDS = sizeof(bauds) / sizeof(bauds[0]);

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n==========================================");
  Serial.println("  GPS Baud Rate Scanner — ESP32 Dev Module (Serial2)");
  Serial.println("==========================================\n");

  for (uint8_t i = 0; i < NUM_BAUDS; i++) {
    uint32_t baud = bauds[i];
    Serial.print("Trying "); Serial.print(baud); Serial.print(" ... ");

    // Try normal polarity
    Serial2.begin(baud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    delay(100);
    while (Serial2.available()) Serial2.read(); // flush
    delay(1500); // collect 1.5 seconds of data

    uint32_t total = 0;
    uint32_t ubx_syncs = 0;
    uint32_t nmea_dollars = 0;
    bool prev_was_b5 = false;
    uint8_t sample[32];
    uint8_t sample_len = 0;

    while (Serial2.available()) {
      uint8_t c = Serial2.read();
      if (sample_len < 32) sample[sample_len++] = c;
      total++;
      if (c == 0xB5) prev_was_b5 = true;
      else { if (prev_was_b5 && c == 0x62) ubx_syncs++; prev_was_b5 = false; }
      if (c == '$') nmea_dollars++;
    }

    Serial2.end();

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
    Serial2.begin(baud, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN, true);
    delay(100);
    while (Serial2.available()) Serial2.read();
    delay(1500);

    total = 0; ubx_syncs = 0; nmea_dollars = 0;
    prev_was_b5 = false; sample_len = 0;

    while (Serial2.available()) {
      uint8_t c = Serial2.read();
      if (sample_len < 32) sample[sample_len++] = c;
      total++;
      if (c == 0xB5) prev_was_b5 = true;
      else { if (prev_was_b5 && c == 0x62) ubx_syncs++; prev_was_b5 = false; }
      if (c == '$') nmea_dollars++;
    }

    Serial2.end();

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
