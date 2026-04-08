/**
 * @file gps_test.ino
 * @brief GPS Test for ESP32 Dev Module + NEO-M8N (Quan-Sheng V2.0)
 *
 * WIRING (5 or 6 wire cable for ESP32):
 *
 *   Wire Color  →  ESP32 Dev Module
 *   ──────────────────────────────────
 *   RED         →  5V or 3.3V
 *   BLACK/BROWN →  GND
 *   GREEN       →  GPIO 16 (Serial2 RX)   ← GPS TX
 *   YELLOW      →  not connected           (GPS RX — we don't need it)
 *   WHITE       →  GPIO 22 (SCL)           ← compass I2C
 *   ORANGE      →  GPIO 21 (SDA)           ← compass I2C
 *
 *   If GREEN doesn't work, try YELLOW on GPIO 16 instead.
 *
 * This sketch:
 *   1. Opens Serial2 at 230400 baud (adjust GPS_BAUD if needed)
 *   2. Feeds every byte into BOTH a UBX parser and an NMEA parser
 *   3. Prints parsed GPS data every second
 *   4. Also runs an I2C scan to find the onboard compass
 *
 * Serial commands:
 *   'd' — toggle raw data dump (see every byte coming from GPS)
 *   'n' — toggle NMEA sentence display
 *   's' — I2C scan
 */

#include <Wire.h>

#define GPS_BAUD 230400

// ESP32 Dev Module pin definitions
const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;
const int I2C_SDA = 21;
const int I2C_SCL = 22;

bool raw_dump  = false;
bool nmea_dump = true;

// --- GPS data ---
struct {
  float   lat, lon, alt_m, speed_kn, course_deg, hdop;
  uint8_t fix, sats;
  uint8_t hour, min, sec, day, month;
  uint16_t year;
  bool    has_fix, has_data;
  uint32_t last_update;
  char    protocol[5];  // "UBX" or "NMEA"
} gps;

// ===================== UBX PARSER =====================
#define UBX_MAX 100
struct {
  uint8_t state, cls, id;
  uint16_t len, idx;
  uint8_t payload[UBX_MAX];
  uint8_t ck_a, ck_b;
} ubx;

void ubx_handle() {
  if (ubx.cls == 0x01 && ubx.id == 0x07 && ubx.len >= 84) {
    uint8_t *p = ubx.payload;
    gps.year  = *(uint16_t*)&p[4];
    gps.month = p[6]; gps.day = p[7];
    gps.hour  = p[8]; gps.min = p[9]; gps.sec = p[10];
    gps.fix   = p[20];
    gps.has_fix = (gps.fix >= 2);
    gps.sats  = p[23];
    gps.lon   = *(int32_t*)&p[24] / 1e7f;
    gps.lat   = *(int32_t*)&p[28] / 1e7f;
    gps.alt_m = *(int32_t*)&p[36] / 1000.0f;
    gps.speed_kn  = (*(int32_t*)&p[60] / 1000.0f) * 1.94384f;
    gps.course_deg = *(int32_t*)&p[64] / 1e5f;
    gps.hdop  = *(uint16_t*)&p[76] / 100.0f;
    gps.has_data = true;
    gps.last_update = millis();
    strcpy(gps.protocol, "UBX");
  }
}

bool ubx_feed(uint8_t c) {
  switch (ubx.state) {
    case 0: if (c == 0xB5) ubx.state = 1; break;
    case 1: ubx.state = (c == 0x62) ? 2 : 0; break;
    case 2: ubx.cls = c; ubx.ck_a = c; ubx.ck_b = c; ubx.state = 3; break;
    case 3: ubx.id = c; ubx.ck_a += c; ubx.ck_b += ubx.ck_a; ubx.state = 4; break;
    case 4: ubx.len = c; ubx.ck_a += c; ubx.ck_b += ubx.ck_a; ubx.state = 5; break;
    case 5:
      ubx.len |= ((uint16_t)c << 8);
      ubx.ck_a += c; ubx.ck_b += ubx.ck_a;
      ubx.idx = 0;
      if (ubx.len > UBX_MAX) { ubx.state = 0; break; }
      ubx.state = (ubx.len > 0) ? 6 : 7;
      break;
    case 6:
      ubx.payload[ubx.idx++] = c;
      ubx.ck_a += c; ubx.ck_b += ubx.ck_a;
      if (ubx.idx >= ubx.len) ubx.state = 7;
      break;
    case 7: ubx.state = (c == ubx.ck_a) ? 8 : 0; break;
    case 8: ubx.state = 0; if (c == ubx.ck_b) return true; break;
    default: ubx.state = 0; break;
  }
  return false;
}

// ===================== NMEA PARSER =====================
#define NMEA_BUF 128
char nmea_buf[NMEA_BUF];
uint8_t nmea_idx = 0;

#define MAX_F 20
char *fld[MAX_F];

uint8_t split(char *s) {
  uint8_t c = 0; fld[c++] = s;
  while (*s && c < MAX_F) { if (*s == ',' || *s == '*') { *s = '\0'; fld[c++] = s+1; } s++; }
  return c;
}

float nmea2dec(const char *v, const char *d) {
  if (!v[0]) return 0;
  float r = atof(v);
  int deg = (int)(r / 100);
  return (deg + (r - deg*100) / 60.0f) * ((d[0]=='S'||d[0]=='W') ? -1 : 1);
}

void nmea_parse(char *s) {
  // checksum verify
  if (s[0] != '$') return;
  uint8_t cs = 0;
  char *p = s+1;
  while (*p && *p != '*') cs ^= *p++;
  if (*p != '*' || cs != (uint8_t)strtol(p+1, NULL, 16)) return;

  if (nmea_dump) { Serial.print("[NMEA] "); Serial.println(s); }

  char *t = s + 3;
  uint8_t n = split(s);

  if (strncmp(t, "GGA", 3) == 0 && n >= 15) {
    if (strlen(fld[1]) >= 6) {
      gps.hour = (fld[1][0]-'0')*10+(fld[1][1]-'0');
      gps.min  = (fld[1][2]-'0')*10+(fld[1][3]-'0');
      gps.sec  = (fld[1][4]-'0')*10+(fld[1][5]-'0');
    }
    if (strlen(fld[2]) > 0) {
      gps.lat = nmea2dec(fld[2], fld[3]);
      gps.lon = nmea2dec(fld[4], fld[5]);
    }
    gps.fix  = atoi(fld[6]);
    gps.has_fix = (gps.fix > 0);
    gps.sats = atoi(fld[7]);
    if (strlen(fld[8]) > 0) gps.hdop = atof(fld[8]);
    if (strlen(fld[9]) > 0) gps.alt_m = atof(fld[9]);
    gps.has_data = true;
    gps.last_update = millis();
    strcpy(gps.protocol, "NMEA");
  }
  else if (strncmp(t, "RMC", 3) == 0 && n >= 12) {
    if (fld[2][0] == 'A') {
      if (strlen(fld[3]) > 0) {
        gps.lat = nmea2dec(fld[3], fld[4]);
        gps.lon = nmea2dec(fld[5], fld[6]);
      }
      if (strlen(fld[7]) > 0) gps.speed_kn = atof(fld[7]);
      if (strlen(fld[8]) > 0) gps.course_deg = atof(fld[8]);
      gps.has_fix = true;
    }
    if (strlen(fld[9]) >= 6) {
      gps.day   = (fld[9][0]-'0')*10+(fld[9][1]-'0');
      gps.month = (fld[9][2]-'0')*10+(fld[9][3]-'0');
      gps.year  = 2000+(fld[9][4]-'0')*10+(fld[9][5]-'0');
    }
    gps.has_data = true;
    gps.last_update = millis();
    strcpy(gps.protocol, "NMEA");
  }
}

// ===================== I2C SCAN =====================
void i2c_scan() {
  Serial.println("\n=== I2C Scan ===");
  int n = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print("  0x"); if (a < 16) Serial.print("0");
      Serial.print(a, HEX); Serial.print("  ");
      if (a == 0x0D) Serial.print("QMC5883L (compass)");
      else if (a == 0x1E) Serial.print("HMC5883L (compass)");
      else Serial.print("?");
      Serial.println();
      n++;
    }
  }
  Serial.print("Total: "); Serial.print(n); Serial.println(" device(s)\n");
}

// ===================== MAIN =====================
uint32_t last_print = 0;
uint32_t byte_count = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n============================================");
  Serial.println("  GPS Test — ESP32 Dev Module + NEO-M8N");
  Serial.println("  Auto-detects UBX binary or NMEA ASCII");
  Serial.println("============================================");
  Serial.println("  'd' = toggle raw byte dump");
  Serial.println("  'n' = toggle NMEA sentence display");
  Serial.println("  's' = I2C scan (compass)");
  Serial.println("============================================\n");

  memset(&gps, 0, sizeof(gps));
  memset(&ubx, 0, sizeof(ubx));

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  i2c_scan();

  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.print("GPS serial open at "); Serial.print(GPS_BAUD); Serial.println(" baud. Waiting for GPS data...\n");
}

void loop() {
  // Commands
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'd' || cmd == 'D') {
      raw_dump = !raw_dump;
      Serial.print("Raw dump: "); Serial.println(raw_dump ? "ON" : "OFF");
    }
    else if (cmd == 'n' || cmd == 'N') {
      nmea_dump = !nmea_dump;
      Serial.print("NMEA display: "); Serial.println(nmea_dump ? "ON" : "OFF");
    }
    else if (cmd == 's' || cmd == 'S') i2c_scan();
  }

  // Read GPS
  while (Serial2.available()) {
    uint8_t c = Serial2.read();
    byte_count++;

    if (raw_dump) {
      Serial.print(c, HEX); Serial.print(' ');
      if (byte_count % 32 == 0) Serial.println();
    }

    // UBX
    if (ubx_feed(c)) {
      ubx_handle();
      if (gps.has_data && !gps.protocol[0]) strcpy(gps.protocol, "UBX");
    }

    // NMEA
    if (c == '$') { nmea_idx = 0; nmea_buf[nmea_idx++] = c; }
    else if (c == '\n' || c == '\r') {
      if (nmea_idx > 5) { nmea_buf[nmea_idx] = '\0'; nmea_parse(nmea_buf); }
      nmea_idx = 0;
    }
    else if (nmea_idx > 0 && nmea_idx < NMEA_BUF - 1) nmea_buf[nmea_idx++] = c;
  }

  // Print status every second
  if (millis() - last_print >= 1000) {
    Serial.println("--------------------------------------------");
    Serial.print("Bytes received: "); Serial.println(byte_count);

    if (byte_count == 0) {
      Serial.println("  NO DATA — check wiring!");
      Serial.println("  Try swapping GREEN and YELLOW on pin 16");
    }
    else if (!gps.has_data) {
      Serial.println("  Receiving bytes but no valid GPS packets yet");
      Serial.println("  (waiting for fix — can take 30-90s outdoors)");
    }
    else {
      Serial.print("Protocol: "); Serial.println(gps.protocol);
      Serial.print("Fix:  "); Serial.print(gps.has_fix ? "YES" : "NO");
      Serial.print("  type="); Serial.print(gps.fix);
      Serial.print("  sats="); Serial.print(gps.sats);
      Serial.print("  HDOP="); Serial.println(gps.hdop, 1);

      if (gps.has_fix) {
        Serial.print("Lat:  "); Serial.print(gps.lat, 7); Serial.println(" deg");
        Serial.print("Lon:  "); Serial.print(gps.lon, 7); Serial.println(" deg");
        Serial.print("Alt:  "); Serial.print(gps.alt_m, 1); Serial.println(" m MSL");
        Serial.print("Speed:"); Serial.print(gps.speed_kn, 1); Serial.println(" kn");
        Serial.print("Crs:  "); Serial.print(gps.course_deg, 1); Serial.println(" deg");
      }

      char tbuf[32];
      snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d UTC  %02d/%02d/%04d",
               gps.hour, gps.min, gps.sec, gps.day, gps.month, gps.year);
      Serial.print("Time: "); Serial.println(tbuf);
      Serial.print("Age:  "); Serial.print(millis() - gps.last_update); Serial.println(" ms");
    }

    last_print = millis();
    byte_count = 0;  // reset per-second counter
  }

  delay(1);
}
