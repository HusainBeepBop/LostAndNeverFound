// LostAndNeverFound - ESP32 Luggage Tracker
// TODO: Replace delay-based idle with ESP32 light sleep in v2 for better battery life
// Requirements: ArduinoJson library (install via Arduino Library Manager)
//
// ============================================================
//  LED BLINK LANGUAGE (GPIO 2 built-in LED)
// ============================================================
//  BOOT OK         : 3 fast blinks
//  SPIFFS OK       : 2 medium blinks
//  SPIFFS FAIL     : LED stays ON solid for 2s
//  GPS SEARCHING   : slow single pulse every 2s (non-blocking)
//  GPS FIX OK      : 5 rapid blinks
//  GPS NO FIX      : 2 long slow blinks
//  WIFI CONNECTING : rapid double-blink repeated
//  WIFI UPLOAD OK  : 5 fast blinks + 1 long ON
//  WIFI FAIL/NOCON : 3 slow blinks
//  SAVED TO SPIFFS : 1 short blink
//  IDLE HEARTBEAT  : 1 tiny blip every 5s
// ============================================================

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ===== CONFIG =====
const bool SAVE_BATTERY_MODE = true; // false = GPS every 30s regardless of motion

const char* WIFI_SSID       = "Google Pixel 8";
const char* WIFI_PASS       = "easyentry";
const char* FIREBASE_HOST   = "lostandneverfound-4bf46-default-rtdb.firebaseio.com";
const char* FIREBASE_AUTH   = "13EhYvgUxOAGsPXTgTWk4O77xB2QzfuzvhS3K4Bp";
const char* DEVICE_ID       = "MASTER";

const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;
#define GPS_BAUD      230400
#define GPS_POWER_PIN 4   // Optional transistor: HIGH=GPS on, LOW=GPS off
#define LED_PIN       2   // Built-in LED

const int  MPU_ADDR            = 0x68;
const int  I2C_SDA             = 21;
const int  I2C_SCL             = 22;
const int  SENSITIVITY         = 5;
const long ACCEL_THRESHOLD     = 500 * SENSITIVITY;
const long GYRO_THRESHOLD      = 400 * SENSITIVITY;
const bool DEBUG_MODE          = false;
const int  STATIONARY_DEBOUNCE = 20;
const int  MAX_RECORDS         = 500;

// ===== GLOBALS =====
bool     is_moving            = false;
int      stationary_counter   = STATIONARY_DEBOUNCE;
uint32_t last_mpu_poll        = 0;
uint32_t last_gps_fix_attempt = 0;
uint32_t last_heartbeat       = 0;
uint32_t last_countdown_print = 0;

uint32_t current_loop_delay   = 2000;
uint32_t current_gps_interval = 300000;

// ===== LED HELPERS =====
// Blocking blink — safe to call outside the GPS search loop
void led_blink(int count, int on_ms, int off_ms) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(on_ms);
    digitalWrite(LED_PIN, LOW);
    if (i < count - 1) delay(off_ms);
  }
}

// Double-blip once — call repeatedly while WiFi is connecting
void led_double_blip() {
  digitalWrite(LED_PIN, HIGH); delay(60);
  digitalWrite(LED_PIN, LOW);  delay(60);
  digitalWrite(LED_PIN, HIGH); delay(60);
  digitalWrite(LED_PIN, LOW);
}

// ===== MPU6050 =====
int16_t ax, ay, az, gx, gy, gz;
float   grav_x = 0, grav_y = 0, grav_z = 0;

void mpu_init() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println("[MPU ] ERROR: MPU6050 not found! Check SDA/SCL wiring.");
  } else {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission(true);

    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 14, true);
    if (Wire.available() == 14) {
      grav_x = Wire.read() << 8 | Wire.read();
      grav_y = Wire.read() << 8 | Wire.read();
      grav_z = Wire.read() << 8 | Wire.read();
    }
    Serial.println("[MPU ] OK - MPU6050 initialized");
  }
}

void mpu_read_and_process() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  if (Wire.available() == 14) {
    ax = Wire.read() << 8 | Wire.read();
    ay = Wire.read() << 8 | Wire.read();
    az = Wire.read() << 8 | Wire.read();
    Wire.read(); Wire.read();
    gx = Wire.read() << 8 | Wire.read();
    gy = Wire.read() << 8 | Wire.read();
    gz = Wire.read() << 8 | Wire.read();
  }

  grav_x = (grav_x * 0.9) + (ax * 0.1);
  grav_y = (grav_y * 0.9) + (ay * 0.1);
  grav_z = (grav_z * 0.9) + (az * 0.1);

  long dyn_x     = abs(ax - (long)grav_x);
  long dyn_y     = abs(ay - (long)grav_y);
  long dyn_z     = abs(az - (long)grav_z);
  long total_dyn = dyn_x + dyn_y + dyn_z;
  long total_gyro= abs(gx) + abs(gy) + abs(gz);

  if (DEBUG_MODE) {
    static uint32_t last_dbg = 0;
    if (millis() - last_dbg > 2000) {
      Serial.printf("[MPU ] Dyn=%ld  Gyro=%ld  (thr: accel=%ld gyro=%ld)\n",
                    total_dyn, total_gyro, ACCEL_THRESHOLD, GYRO_THRESHOLD);
      last_dbg = millis();
    }
  }

  bool moving_now = (total_dyn > ACCEL_THRESHOLD) || (total_gyro > GYRO_THRESHOLD);
  bool was_moving = is_moving;

  if (moving_now) {
    stationary_counter = 0;
    is_moving = true;
  } else {
    stationary_counter++;
    if (stationary_counter >= STATIONARY_DEBOUNCE) {
      is_moving = false;
      stationary_counter = STATIONARY_DEBOUNCE;
    } else {
      is_moving = true;
    }
  }

  if (was_moving != is_moving) {
    if (is_moving) {
      Serial.println("[MPU ] State -> MOVING  (GPS interval: 30s)");
      current_loop_delay   = 500;
      current_gps_interval = 30000;
    } else {
      Serial.println("[MPU ] State -> STATIONARY");
      current_loop_delay   = 2000;
      current_gps_interval = SAVE_BATTERY_MODE ? 300000 : 30000;
    }
  }
}

// ===== GPS =====
struct {
  float    lat, lon, alt_m, speed_kn, hdop;
  uint8_t  fix, sats;
  uint8_t  hour, min, sec, day, month;
  uint16_t year;
  bool     has_fix, has_data;
  uint32_t last_update;
} gps;

#define UBX_MAX 100
struct {
  uint8_t  state, cls, id;
  uint16_t len, idx;
  uint8_t  payload[UBX_MAX];
  uint8_t  ck_a, ck_b;
} ubx;

void ubx_handle() {
  if (ubx.cls == 0x01 && ubx.id == 0x07 && ubx.len >= 84) {
    uint8_t *p = ubx.payload;
    gps.year     = *(uint16_t*)&p[4];
    gps.month    = p[6];  gps.day  = p[7];
    gps.hour     = p[8];  gps.min  = p[9];  gps.sec = p[10];
    gps.fix      = p[20];
    gps.has_fix  = (gps.fix >= 2);
    gps.sats     = p[23];
    gps.lon      = *(int32_t*)&p[24] / 1e7f;
    gps.lat      = *(int32_t*)&p[28] / 1e7f;
    gps.alt_m    = *(int32_t*)&p[36] / 1000.0f;
    gps.speed_kn = (*(int32_t*)&p[60] / 1000.0f) * 1.94384f;
    gps.hdop     = *(uint16_t*)&p[76] / 100.0f;
    gps.has_data = true;
    gps.last_update = millis();
  }
}

bool ubx_feed(uint8_t c) {
  switch (ubx.state) {
    case 0: if (c == 0xB5) ubx.state = 1; break;
    case 1: ubx.state = (c == 0x62) ? 2 : 0; break;
    case 2: ubx.cls = c; ubx.ck_a = c; ubx.ck_b = c; ubx.state = 3; break;
    case 3: ubx.id  = c; ubx.ck_a += c; ubx.ck_b += ubx.ck_a; ubx.state = 4; break;
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

#define NMEA_BUF 128
char    nmea_buf[NMEA_BUF];
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
  return (deg + (r - deg*100) / 60.0f) * ((d[0]=='S' || d[0]=='W') ? -1 : 1);
}

void nmea_parse(char *s) {
  if (s[0] != '$') return;
  uint8_t cs = 0; char *p = s+1;
  while (*p && *p != '*') cs ^= *p++;
  if (*p != '*' || cs != (uint8_t)strtol(p+1, NULL, 16)) return;

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
    gps.fix      = atoi(fld[6]);
    gps.has_fix  = (gps.fix > 0);
    gps.sats     = atoi(fld[7]);
    if (strlen(fld[8]) > 0) gps.hdop  = atof(fld[8]);
    if (strlen(fld[9]) > 0) gps.alt_m = atof(fld[9]);
    gps.has_data = true;
    gps.last_update = millis();
  } else if (strncmp(t, "RMC", 3) == 0 && n >= 12) {
    if (fld[2][0] == 'A') {
      if (strlen(fld[3]) > 0) {
        gps.lat = nmea2dec(fld[3], fld[4]);
        gps.lon = nmea2dec(fld[5], fld[6]);
      }
      if (strlen(fld[7]) > 0) gps.speed_kn = atof(fld[7]);
      gps.has_fix = true;
    }
    if (strlen(fld[9]) >= 6) {
      gps.day   = (fld[9][0]-'0')*10+(fld[9][1]-'0');
      gps.month = (fld[9][2]-'0')*10+(fld[9][3]-'0');
      gps.year  = 2000+(fld[9][4]-'0')*10+(fld[9][5]-'0');
    }
    gps.has_data = true;
    gps.last_update = millis();
  }
}

bool attempt_gps_fix() {
  Serial.println("[GPS ] Powering GPS on...");
  digitalWrite(GPS_POWER_PIN, HIGH);
  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  uint32_t start_m      = millis();
  uint32_t last_status  = start_m;
  uint32_t last_led_t   = start_m;
  bool     led_state    = false;

  gps.has_data = false;
  gps.has_fix  = false;

  Serial.println("[GPS ] Searching for fix... (timeout 60s)");

  while (millis() - start_m < 60000) {
    // Feed bytes into both parsers
    while (Serial2.available()) {
      uint8_t c = Serial2.read();
      if (ubx_feed(c)) { ubx_handle(); }
      if (c == '$') { nmea_idx = 0; nmea_buf[nmea_idx++] = c; }
      else if (c == '\n' || c == '\r') {
        if (nmea_idx > 5) { nmea_buf[nmea_idx] = '\0'; nmea_parse(nmea_buf); }
        nmea_idx = 0;
      } else if (nmea_idx > 0 && nmea_idx < NMEA_BUF - 1) {
        nmea_buf[nmea_idx++] = c;
      }
    }

    // Non-blocking LED: short pulse every 2s = "searching"
    uint32_t now = millis();
    if (!led_state && now - last_led_t >= 2000) {
      digitalWrite(LED_PIN, HIGH);
      led_state = true;
      last_led_t = now;
    } else if (led_state && now - last_led_t >= 100) {
      digitalWrite(LED_PIN, LOW);
      led_state = false;
    }

    // Status print every 5s
    if (millis() - last_status >= 5000) {
      uint32_t elapsed = (millis() - start_m) / 1000;
      Serial.printf("[GPS ] Searching... %lus | sats=%d | fix=%s\n",
                    elapsed, gps.sats, gps.has_fix ? "YES" : "no");
      last_status = millis();
    }

    // Fix acquired
    if (gps.has_fix && gps.sats >= 4) {
      digitalWrite(LED_PIN, LOW);
      Serial.printf("[GPS ] Fix acquired! lat=%.6f lon=%.6f | sats=%d | alt=%.1fm | spd=%.1fkn\n",
                    gps.lat, gps.lon, gps.sats, gps.alt_m, gps.speed_kn);
      led_blink(5, 80, 80); // 5 rapid blinks = GPS success
      return true;
    }

    delay(10);
  }

  digitalWrite(LED_PIN, LOW);
  Serial.println("[GPS ] No fix after 60s - moving on");
  led_blink(2, 600, 300); // 2 long slow blinks = GPS failed
  return false;
}

// ===== STORAGE =====
void maintain_record_limit() {
  File file = SPIFFS.open("/history.jsonl", FILE_READ);
  if (!file) return;
  int lines = 0;
  while (file.available()) { if (file.read() == '\n') lines++; }
  file.close();

  if (lines > MAX_RECORDS) {
    Serial.printf("[STORE] Trimming history (%d records > %d limit)\n", lines, MAX_RECORDS);
    File rfile = SPIFFS.open("/history.jsonl", FILE_READ);
    File wfile = SPIFFS.open("/history_temp.jsonl", FILE_WRITE);
    if (rfile && wfile) {
      bool first_skipped = false;
      while (rfile.available()) {
        String line = rfile.readStringUntil('\n');
        if (!first_skipped) { first_skipped = true; continue; }
        wfile.println(line);
      }
    }
    if (rfile) rfile.close();
    if (wfile) wfile.close();
    SPIFFS.remove("/history.jsonl");
    SPIFFS.rename("/history_temp.jsonl", "/history.jsonl");
  }
}

void save_record(bool fix_ok) {
  // Build timestamp — use GPS time if we have it, otherwise mark as unknown
  char ts[32];
  if (gps.year > 2000) {
    snprintf(ts, sizeof(ts), "%04d-%02d-%02dT%02d:%02d:%02dZ",
             gps.year, gps.month, gps.day, gps.hour, gps.min, gps.sec);
  } else {
    snprintf(ts, sizeof(ts), "unknown@%lums", millis());
  }

  StaticJsonDocument<256> doc;
  doc["ts"]     = ts;
  doc["fix"]    = fix_ok;           // NEW: tells the app whether GPS locked
  doc["moving"] = is_moving;
  doc["sats"]   = gps.sats;
  doc["sent"]   = false;

  if (fix_ok) {
    doc["lat"] = serialized(String(gps.lat, 6));
    doc["lon"] = serialized(String(gps.lon, 6));
    doc["alt"] = gps.alt_m;
    doc["spd"] = gps.speed_kn;
  } else {
    // Explicit nulls so the app knows these fields are intentionally absent
    doc["lat"] = nullptr;
    doc["lon"] = nullptr;
    doc["alt"] = nullptr;
    doc["spd"] = nullptr;
  }

  String line;
  serializeJson(doc, line);

  File file = SPIFFS.open("/history.jsonl", FILE_APPEND);
  if (!file) file = SPIFFS.open("/history.jsonl", FILE_WRITE);

  if (file) {
    file.println(line);
    file.close();
    if (fix_ok) {
      Serial.printf("[STORE] Record saved (FIX)    -> %s | sats=%d | moving=%s\n",
                    ts, gps.sats, is_moving ? "yes" : "no");
    } else {
      Serial.printf("[STORE] Record saved (NO FIX) -> %s | moving=%s\n",
                    ts, is_moving ? "yes" : "no");
    }
    led_blink(1, 120, 0); // 1 short blink = saved to flash
  } else {
    Serial.println("[STORE] ERROR: Could not open history.jsonl for writing!");
  }
  maintain_record_limit();
}

void mark_records_as_sent() {
  File rfile = SPIFFS.open("/history.jsonl", FILE_READ);
  File wfile = SPIFFS.open("/history_temp.jsonl", FILE_WRITE);
  if (rfile && wfile) {
    while (rfile.available()) {
      String line = rfile.readStringUntil('\n');
      if (line.length() > 0) {
        line.replace("\"sent\":false", "\"sent\":true");
        wfile.println(line);
      }
    }
  }
  if (rfile) rfile.close();
  if (wfile) wfile.close();
  SPIFFS.remove("/history.jsonl");
  SPIFFS.rename("/history_temp.jsonl", "/history.jsonl");
}

// ===== WIFI / UPLOAD =====
void attempt_wifi_upload() {
  File file = SPIFFS.open("/history.jsonl", FILE_READ);
  if (!file) {
    Serial.println("[WIFI ] No history file found, skipping upload");
    return;
  }

  DynamicJsonDocument doc(16384);
  JsonArray records = doc.createNestedArray("records");
  doc["device_id"] = DEVICE_ID;

  int count = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0 && line.indexOf("\"sent\":false") > 0) {
      StaticJsonDocument<256> lineDoc;
      deserializeJson(lineDoc, line);
      records.add(lineDoc);
      count++;
    }
  }
  file.close();

  if (count == 0) {
    Serial.println("[WIFI ] No unsent records - skipping upload");
    return;
  }

  Serial.printf("[WIFI ] %d unsent record(s). Connecting to \"%s\"...\n", count, WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start_m = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_m < 10000) {
    led_double_blip(); // Rapid double-blink = connecting
    delay(300);
  }
  digitalWrite(LED_PIN, LOW);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI ] Connected! IP: %s\n", WiFi.localIP().toString().c_str());

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = String("https://") + FIREBASE_HOST + "/history.json?auth=" + FIREBASE_AUTH;
    Serial.printf("[WIFI ] POSTing to: %s\n", url.c_str());
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    String payload;
    serializeJson(doc, payload);

    if (DEBUG_MODE) {
      Serial.println("[WIFI ] Payload:");
      Serial.println(payload);
    }

    Serial.printf("[WIFI ] Sending %d records (%d bytes)...\n", count, payload.length());
    int code = http.POST(payload);
    http.end();

    if (code == 200) {
      Serial.printf("[WIFI ] Upload OK! %d records sent (HTTP 200)\n", count);
      led_blink(5, 80, 80);        // 5 rapid blinks
      digitalWrite(LED_PIN, HIGH);
      delay(500);                  // then hold ON = all good
      digitalWrite(LED_PIN, LOW);
      mark_records_as_sent();
    } else {
      Serial.printf("[WIFI ] Upload FAILED - HTTP code: %d\n", code);
      led_blink(3, 400, 200);      // 3 slow blinks = upload failed
    }
  } else {
    Serial.printf("[WIFI ] Could not connect to \"%s\" (10s timeout)\n", WIFI_SSID);
    led_blink(3, 400, 200);        // 3 slow blinks = no connection
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("[WIFI ] WiFi off");
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("\n\n========================================");
  Serial.println("  LostAndNeverFound - Tracker Firmware");
  Serial.println("========================================");
  Serial.printf("  Device ID    : %s\n", DEVICE_ID);
  Serial.printf("  Battery Mode : %s\n", SAVE_BATTERY_MODE ? "ON (MPU-gated GPS)" : "OFF (GPS every 30s)");
  Serial.printf("  Firebase     : %s\n", FIREBASE_HOST);
  Serial.printf("  WiFi SSID    : %s\n", WIFI_SSID);
  Serial.println("========================================\n");

  led_blink(3, 100, 100); // 3 fast blinks = boot OK

  btStop();
  WiFi.mode(WIFI_OFF);
  Serial.println("[INIT ] Bluetooth stopped, WiFi off");

  pinMode(GPS_POWER_PIN, OUTPUT);
  digitalWrite(GPS_POWER_PIN, LOW);
  Serial.printf("[INIT ] GPS power pin (GPIO %d) -> LOW\n", GPS_POWER_PIN);

  if (!SAVE_BATTERY_MODE) {
    current_gps_interval = 30000;
    Serial.println("[INIT ] GPS interval: 30s (battery save OFF)");
  } else {
    Serial.println("[INIT ] GPS interval: 5min stationary / 30s moving");
  }

  Serial.println("[INIT ] Mounting SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("[INIT ] ERROR: SPIFFS mount failed!");
    digitalWrite(LED_PIN, HIGH);
    delay(2000);
    digitalWrite(LED_PIN, LOW);
  } else {
    size_t total = SPIFFS.totalBytes();
    size_t used  = SPIFFS.usedBytes();
    Serial.printf("[INIT ] SPIFFS OK - %u KB used / %u KB total\n", used/1024, total/1024);
    led_blink(2, 200, 150); // 2 medium blinks = SPIFFS OK
  }

  if (SAVE_BATTERY_MODE) {
    Serial.println("[INIT ] Initializing MPU6050...");
    mpu_init();
  } else {
    Serial.println("[INIT ] MPU6050 skipped (battery save OFF)");
  }

  Serial.printf("\n[INIT ] Ready! First GPS fix in ~%.0fs\n\n",
                current_gps_interval / 1000.0);
}

// ===== LOOP =====
void loop() {
  uint32_t now = millis();

  // MPU polling (only in battery save mode)
  if (SAVE_BATTERY_MODE && (now - last_mpu_poll >= current_loop_delay)) {
    mpu_read_and_process();
    last_mpu_poll = millis();
  }

  // Countdown print every 5s so you know it's alive
  if (now - last_countdown_print >= 5000) {
    uint32_t time_since = now - last_gps_fix_attempt;
    int32_t  secs_left  = ((int32_t)current_gps_interval - (int32_t)time_since) / 1000;
    if (secs_left < 0) secs_left = 0;
    Serial.printf("[LOOP ] Next GPS in %ds | moving=%s | uptime=%lus\n",
                  secs_left, is_moving ? "yes" : "no", now / 1000);
    last_countdown_print = millis();
  }

  // Heartbeat blink every 5s (tiny blip so you know the board is alive)
  if (now - last_heartbeat >= 5000) {
    digitalWrite(LED_PIN, HIGH); delay(30); digitalWrite(LED_PIN, LOW);
    last_heartbeat = millis();
  }

  // GPS fix + upload cycle
  if (now - last_gps_fix_attempt >= current_gps_interval) {
    Serial.println("\n[LOOP ] ---- GPS cycle starting ----");
    bool got_fix = attempt_gps_fix();

    if (!is_moving && SAVE_BATTERY_MODE) {
      Serial.println("[GPS ] Stationary + battery save: powering GPS off");
      digitalWrite(GPS_POWER_PIN, LOW);
      Serial2.end();
    }

    // Always save a record — fix=true has coords, fix=false has nulls
    save_record(got_fix);

    attempt_wifi_upload();

    last_gps_fix_attempt = millis();
    Serial.println("[LOOP ] ---- GPS cycle done ----\n");
  }

  delay(50);
}
