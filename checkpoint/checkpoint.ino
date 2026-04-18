// LostAndNeverFound - Home Base Checkpoint Scanner
// Runs on a second ESP32 connected via USB
//
// ============================================================
//  MODES
//  FINDMY_MODE true  : Detects tracker by BLE MAC address
//                      (derived from the Find My public key)
//  FINDMY_MODE false : Detects tracker by BLE device name
//                      (classic "LNF-MASTER" beacon)
//
//  HOME BASE ALERT
//  If the tracker is not seen for LOST_TIMEOUT_MS (default 10 min),
//  the checkpoint sends a webhook notification (Telegram, IFTTT, etc).
//  When the tracker reappears, a "found" notification is sent.
//
//  CALIBRATION (do this once):
//  Hold the bag tag exactly 1 meter from this ESP32.
//  Read the RSSI printed in Serial Monitor.
//  Set that value as RSSI_AT_1M below.
// ============================================================

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "findmy_key.h"

// ===== CONFIG =====
const bool FINDMY_MODE = true;  // true = match by MAC (Find My), false = match by name

const char* TARGET_NAME   = "LNF-MASTER";  // Classic mode: BLE beacon name to scan for
const int   RSSI_AT_1M    = -59;           // Calibrate: RSSI when tag is exactly 1m away
const float PATH_LOSS_N   = 2.5;           // 2.0 = open air, 2.5-3.0 = indoors
const int   RSSI_NEAR     = -72;           // dBm threshold to enter PRESENT state
const int   RSSI_FAR      = -80;           // dBm threshold to exit PRESENT -> PASSED
const int   CONFIRM_COUNT = 3;             // Consecutive reads needed to change state
const int   COOLDOWN_MS   = 10000;         // ms before checkpoint can trigger again
const int   SCAN_WINDOW   = 1;             // BLE scan window in seconds

// ===== HOME BASE CONFIG =====
const char* HB_WIFI_SSID = "Google Pixel 8";   // WiFi for webhook notifications
const char* HB_WIFI_PASS = "easyentry";

// Webhook URL — pick ONE and configure:
//   Telegram: https://api.telegram.org/bot<TOKEN>/sendMessage
//   IFTTT:    https://maker.ifttt.com/trigger/<EVENT>/with/key/<KEY>
//   Generic:  any URL that accepts POST with JSON body
const char* WEBHOOK_URL = "";  // <-- SET THIS

// For Telegram: set your chat ID here (leave empty for IFTTT/generic)
const char* TELEGRAM_CHAT_ID = "";

const uint32_t LOST_TIMEOUT_MS     = 600000;  // 10 minutes
const uint32_t FOUND_COOLDOWN_MS   = 30000;   // Don't spam "found" alerts
const uint32_t WEBHOOK_RETRY_MS    = 300000;   // Re-alert every 5 min if still lost

// ===== DERIVED: Expected BLE MAC from Find My key =====
char expected_mac[18];  // "XX:XX:XX:XX:XX:XX"

void derive_expected_mac() {
  uint8_t a[6];
  a[0] = findmy_public_key[0] | 0xC0;
  memcpy(&a[1], &findmy_public_key[1], 5);
  sprintf(expected_mac, "%02x:%02x:%02x:%02x:%02x:%02x",
          a[0], a[1], a[2], a[3], a[4], a[5]);
}

// ===== RSSI SMOOTHING =====
#define SMOOTH_N 5
int rssi_buf[SMOOTH_N] = {-100, -100, -100, -100, -100};
int rssi_idx = 0;

void rssi_push(int val) {
  rssi_buf[rssi_idx % SMOOTH_N] = val;
  rssi_idx++;
}

float rssi_avg() {
  float sum = 0;
  for (int i = 0; i < SMOOTH_N; i++) sum += rssi_buf[i];
  return sum / SMOOTH_N;
}

void rssi_reset() {
  for (int i = 0; i < SMOOTH_N; i++) rssi_buf[i] = -100;
  rssi_idx = 0;
}

// ===== DISTANCE ESTIMATE =====
float rssi_to_distance(float rssi) {
  return pow(10.0, (RSSI_AT_1M - rssi) / (10.0 * PATH_LOSS_N));
}

// ===== STATE MACHINE =====
enum State { IDLE, PRESENT, PASSED };
State state          = IDLE;
int   near_counter   = 0;
int   far_counter    = 0;
uint32_t passed_time = 0;
bool  tag_seen       = false;
int   raw_rssi       = -100;

// ===== HOME BASE STATE =====
uint32_t last_seen_time    = 0;
bool     tracker_lost      = false;
bool     lost_alert_sent   = false;
uint32_t last_alert_time   = 0;

// ===== BLE SCAN CALLBACK =====
class ScanCallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) {
    bool match = false;

    if (FINDMY_MODE) {
      // Match by BLE MAC address (derived from Find My public key)
      String addr = String(dev.getAddress().toString().c_str());
      if (addr.equalsIgnoreCase(String(expected_mac))) {
        match = true;
      }
    } else {
      // Match by device name (classic beacon mode)
      if (dev.haveName() && String(dev.getName().c_str()) == String(TARGET_NAME)) {
        match = true;
      }
    }

    if (match) {
      raw_rssi = dev.getRSSI();
      rssi_push(raw_rssi);
      tag_seen = true;
    }
  }
};

BLEScan* pScanner;

// ===== WEBHOOK =====
bool send_webhook(const char* message) {
  if (strlen(WEBHOOK_URL) == 0) {
    Serial.println("[HOOK ] No webhook URL configured - skipping");
    return false;
  }

  Serial.printf("[HOOK ] Connecting to WiFi \"%s\"...\n", HB_WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(HB_WIFI_SSID, HB_WIFI_PASS);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(300);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HOOK ] WiFi connect failed!");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return false;
  }

  Serial.printf("[HOOK ] WiFi OK. Sending: %s\n", message);

  HTTPClient http;
  http.begin(WEBHOOK_URL);
  http.addHeader("Content-Type", "application/json");

  // Build JSON body — Telegram format if chat_id is set, generic otherwise
  String body;
  if (strlen(TELEGRAM_CHAT_ID) > 0) {
    // Telegram Bot API format
    body = String("{\"chat_id\":\"") + TELEGRAM_CHAT_ID +
           "\",\"text\":\"" + message +
           "\",\"parse_mode\":\"HTML\"}";
  } else {
    // Generic / IFTTT format
    body = String("{\"value1\":\"") + message + "\"}";
  }

  int code = http.POST(body);
  http.end();

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  if (code >= 200 && code < 300) {
    Serial.printf("[HOOK ] Webhook OK (HTTP %d)\n", code);
    return true;
  } else {
    Serial.printf("[HOOK ] Webhook FAILED (HTTP %d)\n", code);
    return false;
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);

  // Derive expected MAC from Find My key
  derive_expected_mac();

  Serial.println("\n\n============================================");
  Serial.println("  LostAndNeverFound - Home Base Checkpoint");
  Serial.println("============================================");
  Serial.printf("  Mode         : %s\n", FINDMY_MODE ? "Find My (MAC match)" : "Classic (name match)");
  if (FINDMY_MODE) {
    Serial.printf("  Expected MAC : %s\n", expected_mac);
  } else {
    Serial.printf("  Target tag   : %s\n", TARGET_NAME);
  }
  Serial.printf("  RSSI @ 1m    : %d dBm\n", RSSI_AT_1M);
  Serial.printf("  NEAR thresh  : %d dBm\n", RSSI_NEAR);
  Serial.printf("  FAR  thresh  : %d dBm\n", RSSI_FAR);
  Serial.printf("  Lost timeout : %us\n", LOST_TIMEOUT_MS / 1000);
  Serial.printf("  Webhook      : %s\n", strlen(WEBHOOK_URL) > 0 ? "configured" : "NOT SET");
  Serial.printf("  Cooldown     : %ds\n", COOLDOWN_MS / 1000);
  Serial.println("============================================\n");
  Serial.println("Scanning... (bring the bag tag close to trigger)");
  Serial.println("--------------------------------------------");

  BLEDevice::init("LNF-HomeBase");
  pScanner = BLEDevice::getScan();
  pScanner->setAdvertisedDeviceCallbacks(new ScanCallback(), true);
  pScanner->setActiveScan(!FINDMY_MODE);  // Active scan only needed for name matching
  pScanner->setInterval(100);
  pScanner->setWindow(99);

  last_seen_time = millis();  // Assume tracker is home at boot
}

// ===== LOOP =====
void loop() {
  // Run a short BLE scan each iteration
  tag_seen = false;
  pScanner->start(SCAN_WINDOW, false);
  pScanner->clearResults();

  float avg  = rssi_avg();
  float dist = rssi_to_distance(avg);
  uint32_t now = millis();

  // Update last-seen time whenever tag is detected
  if (tag_seen) {
    last_seen_time = now;

    // If it was lost, send "found" notification
    if (tracker_lost) {
      uint32_t lost_duration = (now - last_alert_time) / 1000;
      Serial.println("\n============================================");
      Serial.println("  TRACKER FOUND - back in range!");
      Serial.printf("  Was lost for: ~%u seconds\n", lost_duration);
      Serial.println("============================================\n");

      char msg[128];
      snprintf(msg, sizeof(msg),
               "LNF Tracker FOUND - back in Home Base range (was lost ~%um)",
               lost_duration / 60);
      send_webhook(msg);

      tracker_lost    = false;
      lost_alert_sent = false;
    }
  }

  // ---- LOST DETECTION ----
  uint32_t since_seen = now - last_seen_time;

  if (since_seen > LOST_TIMEOUT_MS && !tracker_lost) {
    // First time crossing the lost threshold
    tracker_lost = true;
    Serial.println("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("  TRACKER LOST - not seen for 10+ minutes");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    send_webhook("LNF ALERT: Tracker has been out of Home Base range for 10+ minutes!");
    lost_alert_sent = true;
    last_alert_time = now;
  }

  // Re-alert periodically if still lost
  if (tracker_lost && (now - last_alert_time > WEBHOOK_RETRY_MS)) {
    uint32_t mins_lost = since_seen / 60000;
    char msg[128];
    snprintf(msg, sizeof(msg),
             "LNF REMINDER: Tracker still missing (%u min)", mins_lost);
    send_webhook(msg);
    last_alert_time = now;
  }

  // ---- CHECKPOINT STATE MACHINE ----
  switch (state) {

    case IDLE:
      if (tag_seen) {
        Serial.printf("[SCAN ] Tag spotted | RSSI: %d dBm (avg %.0f) | ~%.1f m\n",
                      raw_rssi, avg, dist);
      } else {
        rssi_reset();
      }

      if (avg > RSSI_NEAR) {
        near_counter++;
        if (near_counter >= CONFIRM_COUNT) {
          state = PRESENT;
          near_counter = 0;
          Serial.println("\n[STATE] >>> BAG IS NEAR THE CHECKPOINT <<<");
          Serial.println("[STATE] Monitoring... waiting for it to pass");
          Serial.println("--------------------------------------------");
        }
      } else {
        near_counter = 0;
      }
      break;

    case PRESENT:
      if (tag_seen) {
        Serial.printf("[TRACK] RSSI: %d dBm (avg %.0f) | Distance: ~%.1f m | %s\n",
                      raw_rssi, avg, dist,
                      avg > RSSI_NEAR ? "CLOSE" : "moving away...");
      } else {
        Serial.println("[TRACK] Tag not seen this scan - may be passing");
      }

      if (avg < RSSI_FAR) {
        far_counter++;
        if (far_counter >= CONFIRM_COUNT) {
          state = PASSED;
          far_counter = 0;
          passed_time = now;

          unsigned long s = millis() / 1000;
          unsigned int  m = s / 60;
          unsigned int  h = m / 60;

          Serial.println("\n============================================");
          Serial.printf( "  BAG PASSED CHECKPOINT\n");
          Serial.printf( "  Tag      : %s\n", FINDMY_MODE ? expected_mac : TARGET_NAME);
          Serial.printf( "  Uptime   : %02u:%02u:%02u\n", h % 24, m % 60, (unsigned int)(s % 60));
          Serial.printf( "  Peak est.: was within ~%.1f m\n", rssi_to_distance(RSSI_NEAR));
          Serial.println("============================================\n");
        }
      } else {
        far_counter = 0;
      }
      break;

    case PASSED:
      {
        uint32_t elapsed   = now - passed_time;
        uint32_t remaining = (COOLDOWN_MS - elapsed) / 1000;

        if (elapsed < COOLDOWN_MS) {
          static uint32_t last_cd_print = 0;
          if (now - last_cd_print > 2000) {
            Serial.printf("[COOL ] Cooldown: %us remaining before next detection\n", remaining);
            last_cd_print = now;
          }
        } else {
          state = IDLE;
          near_counter = 0;
          far_counter  = 0;
          rssi_reset();
          Serial.println("[STATE] Cooldown done - back to IDLE, ready for next bag");
          Serial.println("--------------------------------------------");
        }
      }
      break;
  }
}
