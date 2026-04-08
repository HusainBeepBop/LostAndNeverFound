// TODO: Replace delay-based idle with ESP32 light sleep in v2 for better battery life
// Requirements: ArduinoJson library (install via Arduino Library Manager)

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ===== CONFIG =====
const bool SAVE_BATTERY_MODE = false; // Set to false to force GPS / Server updates every 30 seconds

const char* WIFI_SSID = "Google Pixel 8";
const char* WIFI_PASS = "easyentry";
const char* FIREBASE_HOST = "lostandneverfound-18437-default-rtdb.firebaseio.com"; // Without https:// and trailing /
const char* FIREBASE_AUTH = "lostandneverfound-4bf46.firebaseapp.com"; // Find in Firebase -> Project Settings -> Service accounts -> Database secrets
const char* DEVICE_ID = "MASTER";

const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;
#define GPS_BAUD 230400
#define GPS_POWER_PIN 4 // Optional: HIGH = GPS on, LOW = GPS off
#define LED_PIN 2       // Built-in LED on ESP32 Dev Module

const int MPU_ADDR = 0x68;
const int I2C_SDA = 21;
const int I2C_SCL = 22;

const int SENSITIVITY = 5;
const long ACCEL_THRESHOLD = 500 * SENSITIVITY;
const long GYRO_THRESHOLD = 400 * SENSITIVITY;
const bool DEBUG_MODE = false;
const int STATIONARY_DEBOUNCE = 20;

const int MAX_RECORDS = 500;

// ===== GLOBALS =====
bool is_moving = false;
int stationary_counter = STATIONARY_DEBOUNCE;
uint32_t last_mpu_poll = 0;
uint32_t last_gps_fix_attempt = 0;

uint32_t current_loop_delay = 2000;      // 500ms moving, 2000ms stationary
uint32_t current_gps_interval = 300000;  // 30s moving, 5 mins (300000ms) stationary

// ===== MPU6050 =====
int16_t ax, ay, az;
int16_t gx, gy, gz;

float grav_x = 0;
float grav_y = 0;
float grav_z = 0;

void mpu_init() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println("MPU6050 not found! Check wiring.");
  } else {
    // Wake up the MPU6050
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); // PWR_MGMT_1
    Wire.write(0);
    Wire.endTransmission(true);
    
    // Initial read
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 14, true);
    if (Wire.available() == 14) {
      grav_x = Wire.read() << 8 | Wire.read();
      grav_y = Wire.read() << 8 | Wire.read();
      grav_z = Wire.read() << 8 | Wire.read();
    }
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
    Wire.read(); Wire.read(); // Skip temp
    gx = Wire.read() << 8 | Wire.read();
    gy = Wire.read() << 8 | Wire.read();
    gz = Wire.read() << 8 | Wire.read();
  }

  grav_x = (grav_x * 0.9) + (ax * 0.1);
  grav_y = (grav_y * 0.9) + (ay * 0.1);
  grav_z = (grav_z * 0.9) + (az * 0.1);

  long dyn_x = abs(ax - (long)grav_x);
  long dyn_y = abs(ay - (long)grav_y);
  long dyn_z = abs(az - (long)grav_z);
  long total_dyn_accel = dyn_x + dyn_y + dyn_z;
  long total_gyro = abs(gx) + abs(gy) + abs(gz);

  bool moving_now = (total_dyn_accel > ACCEL_THRESHOLD) || (total_gyro > GYRO_THRESHOLD);

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
      Serial.println("[MOTION] Moving");
      current_loop_delay = 500;
      current_gps_interval = 30000;
    } else {
      Serial.println("[MOTION] Stationary");
      current_loop_delay = 2000;
      current_gps_interval = SAVE_BATTERY_MODE ? 300000 : 30000;
    }
  }
}

// ===== GPS =====
struct {
  float   lat, lon, alt_m, speed_kn, hdop;
  uint8_t fix, sats;
  uint8_t hour, min, sec, day, month;
  uint16_t year;
  bool    has_fix, has_data;
  uint32_t last_update;
} gps;

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
    gps.hdop  = *(uint16_t*)&p[76] / 100.0f;
    gps.has_data = true;
    gps.last_update = millis();
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
    gps.fix  = atoi(fld[6]);
    gps.has_fix = (gps.fix > 0);
    gps.sats = atoi(fld[7]);
    if (strlen(fld[8]) > 0) gps.hdop = atof(fld[8]);
    if (strlen(fld[9]) > 0) gps.alt_m = atof(fld[9]);
    gps.has_data = true;
    gps.last_update = millis();
  }
  else if (strncmp(t, "RMC", 3) == 0 && n >= 12) {
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
  digitalWrite(GPS_POWER_PIN, HIGH);
  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  uint32_t start_m = millis();
  gps.has_data = false;
  gps.has_fix = false;

  while (millis() - start_m < 60000) {
    while (Serial2.available()) {
      uint8_t c = Serial2.read();
      if (ubx_feed(c)) { ubx_handle(); }
      if (c == '$') { nmea_idx = 0; nmea_buf[nmea_idx++] = c; }
      else if (c == '\n' || c == '\r') {
        if (nmea_idx > 5) { nmea_buf[nmea_idx] = '\0'; nmea_parse(nmea_buf); }
        nmea_idx = 0;
      }
      else if (nmea_idx > 0 && nmea_idx < NMEA_BUF - 1) nmea_buf[nmea_idx++] = c;
    }
    
    if (gps.has_fix && gps.sats >= 4) {
      Serial.printf("[GPS] Fix acquired: %.6f, %.6f | sats=%d | age=%dms\n", 
                    gps.lat, gps.lon, gps.sats, (int)(millis() - gps.last_update));
      return true;
    }
    delay(10);
  }
  
  Serial.println("[GPS] No fix after 60s");
  return false;
}

// ===== STORAGE =====
void maintain_record_limit() {
  File file = SPIFFS.open("/history.jsonl", FILE_READ);
  if (!file) return;
  int lines = 0;
  while (file.available()) {
    if (file.read() == '\n') lines++;
  }
  file.close();

  if (lines > MAX_RECORDS) {
    File rfile = SPIFFS.open("/history.jsonl", FILE_READ);
    File wfile = SPIFFS.open("/history_temp.jsonl", FILE_WRITE);
    if(rfile && wfile) {
      bool first_skipped = false;
      while(rfile.available()) {
        String line = rfile.readStringUntil('\n');
        if(!first_skipped) { first_skipped = true; continue; }
        wfile.println(line);
      }
    }
    if (rfile) rfile.close();
    if (wfile) wfile.close();
    SPIFFS.remove("/history.jsonl");
    SPIFFS.rename("/history_temp.jsonl", "/history.jsonl");
  }
}

void save_record() {
  char ts[32];
  snprintf(ts, sizeof(ts), "%04d-%02d-%02dT%02d:%02d:%02dZ", gps.year, gps.month, gps.day, gps.hour, gps.min, gps.sec);
  
  // Format JSON
  StaticJsonDocument<256> doc;
  doc["ts"] = ts;
  doc["lat"] = gps.lat;
  doc["lon"] = gps.lon;
  doc["alt"] = gps.alt_m;
  doc["spd"] = gps.speed_kn;
  doc["sats"] = gps.sats;
  doc["moving"] = is_moving;
  doc["sent"] = false;

  String line;
  serializeJson(doc, line);

  File file = SPIFFS.open("/history.jsonl", FILE_APPEND);
  if (!file) file = SPIFFS.open("/history.jsonl", FILE_WRITE);

  if (file) {
    file.println(line);
    file.close();
    Serial.println("[STORE] Saved record to SPIFFS");
  }
  maintain_record_limit();
}

void mark_records_as_sent() {
  File rfile = SPIFFS.open("/history.jsonl", FILE_READ);
  File wfile = SPIFFS.open("/history_temp.jsonl", FILE_WRITE);
  if (rfile && wfile) {
    while(rfile.available()) {
      String line = rfile.readStringUntil('\n');
      if (line.length() > 0) {
        line.replace("\"sent\":false", "\"sent\":true");
        wfile.println(line);
      }
    }
  }
  if(rfile) rfile.close();
  if(wfile) wfile.close();
  SPIFFS.remove("/history.jsonl");
  SPIFFS.rename("/history_temp.jsonl", "/history.jsonl");
}

// ===== WIFI / UPLOAD =====
void attempt_wifi_upload() {
  File file = SPIFFS.open("/history.jsonl", FILE_READ);
  if (!file) return;

  DynamicJsonDocument doc(16384);
  JsonArray records = doc.createNestedArray("records");
  doc["device_id"] = DEVICE_ID;
  
  int count = 0;
  while(file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0 && line.indexOf("\"sent\":false") > 0) {
      StaticJsonDocument<256> lineDoc;
      deserializeJson(lineDoc, line);
      records.add(lineDoc);
      count++;
    }
  }
  file.close();

  if (count == 0) return;

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t start_m = millis();
  while(WiFi.status() != WL_CONNECTED && millis() - start_m < 10000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected");
    
    WiFiClientSecure client;
    client.setInsecure(); // Disable SSL certificate checking
    HTTPClient http;
    
    // Firebase Realtime DB URL format: https://<HOST>/history.json?auth=<AUTH>
    String url = String("https://") + FIREBASE_HOST + "/history.json?auth=" + FIREBASE_AUTH;
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    String payload;
    serializeJson(doc, payload);
    int httpResponseCode = http.POST(payload);

    if (httpResponseCode == 200) {
      Serial.printf("[WIFI] Upload OK (%d records sent)\n", count);
      // Blink LED on success
      digitalWrite(LED_PIN, HIGH);
      delay(1000);
      digitalWrite(LED_PIN, LOW);
      
      mark_records_as_sent();
    } else {
      Serial.println("[WIFI] Upload failed");
    }
    http.end();
  } else {
    Serial.println("[WIFI] No connection");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // LED off initially

  // Power off BT since not used
  btStop();
  WiFi.mode(WIFI_OFF);

  pinMode(GPS_POWER_PIN, OUTPUT);
  digitalWrite(GPS_POWER_PIN, LOW); // GPS off initially

  if (!SAVE_BATTERY_MODE) {
    current_gps_interval = 30000;
  }

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  }

  if (SAVE_BATTERY_MODE) {
    mpu_init();
  }
}

// ===== LOOP =====
void loop() {
  // 1. MPU Polling (runs every iteration of the delay-based loop)
  if (SAVE_BATTERY_MODE && (millis() - last_mpu_poll >= current_loop_delay)) {
    mpu_read_and_process();
    last_mpu_poll = millis();
  }

  // 2. GPS Fix & Wifi Upload (runs based on current_gps_interval)
  if (millis() - last_gps_fix_attempt >= current_gps_interval) {
    bool got_fix = attempt_gps_fix();
    
    // Shut off GPS if we are stationary AND battery saving is enabled
    if (!is_moving && SAVE_BATTERY_MODE) {
      digitalWrite(GPS_POWER_PIN, LOW);
      Serial2.end();
    }
    
    if (got_fix) {
      save_record();
    }
    
    // Always attempt Wi-Fi upload, in case we have unsent records 
    // even if we couldn't get a new GPS lock this time.
    attempt_wifi_upload();
    
    last_gps_fix_attempt = millis();
  }
  
  delay(50);
}
