// LostAndNeverFound - Checkpoint Scanner
// Runs on a second ESP32 connected to laptop via USB / Serial Monitor
// Scans for the bag tag (LNF-MASTER) and detects when it passes by
//
// ============================================================
//  HOW IT WORKS
//  1. Continuously BLE-scans for the bag tag by name
//  2. Smooths noisy RSSI over last 5 readings
//  3. Converts RSSI to an estimated distance (rough, ±40%)
//  4. State machine:
//       IDLE    → bag comes close  → PRESENT (prints live distance)
//       PRESENT → bag moves away   → PASSED  (fires checkpoint event!)
//       PASSED  → 10s cooldown     → IDLE
//
//  CALIBRATION (do this once):
//  Hold the bag tag exactly 1 meter from this ESP32.
//  Read the RSSI printed in Serial Monitor.
//  Set that value as RSSI_AT_1M below.
// ============================================================

#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ===== CONFIG =====
const char* TARGET_NAME   = "LNF-MASTER";  // Must match BLE_BEACON_NAME in tracker_main
const int   RSSI_AT_1M    = -59;           // Calibrate: RSSI when tag is exactly 1m away
const float PATH_LOSS_N   = 2.5;           // 2.0 = open air, 2.5–3.0 = indoors
const int   RSSI_NEAR     = -72;           // dBm threshold to enter PRESENT state
const int   RSSI_FAR      = -80;           // dBm threshold to exit PRESENT → PASSED
const int   CONFIRM_COUNT = 3;             // Consecutive reads needed to change state
const int   COOLDOWN_MS   = 10000;         // ms before checkpoint can trigger again
const int   SCAN_WINDOW   = 1;            // BLE scan window in seconds

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

// ===== DISTANCE ESTIMATE =====
// Returns estimated meters from RSSI. Rough but directionally correct.
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

// ===== BLE SCAN CALLBACK =====
class ScanCallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) {
    if (!dev.haveName()) return;
    if (String(dev.getName().c_str()) != String(TARGET_NAME)) return;

    raw_rssi = dev.getRSSI();
    rssi_push(raw_rssi);
    tag_seen = true;
  }
};

BLEScan* pScanner;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n\n============================================");
  Serial.println("  LostAndNeverFound - Checkpoint Scanner");
  Serial.println("============================================");
  Serial.printf("  Target tag   : %s\n", TARGET_NAME);
  Serial.printf("  RSSI @ 1m    : %d dBm\n", RSSI_AT_1M);
  Serial.printf("  NEAR thresh  : %d dBm\n", RSSI_NEAR);
  Serial.printf("  FAR  thresh  : %d dBm\n", RSSI_FAR);
  Serial.printf("  Cooldown     : %ds\n", COOLDOWN_MS / 1000);
  Serial.println("============================================\n");
  Serial.println("Scanning... (bring the bag tag close to trigger)");
  Serial.println("--------------------------------------------");

  BLEDevice::init("LNF-Checkpoint");
  pScanner = BLEDevice::getScan();
  pScanner->setAdvertisedDeviceCallbacks(new ScanCallback(), true);
  pScanner->setActiveScan(true);
  pScanner->setInterval(100);
  pScanner->setWindow(99);
}

void loop() {
  // Run a short BLE scan each iteration
  tag_seen = false;
  pScanner->start(SCAN_WINDOW, false);
  pScanner->clearResults();

  float avg  = rssi_avg();
  float dist = rssi_to_distance(avg);

  // ---- STATE MACHINE ----
  switch (state) {

    case IDLE:
      if (tag_seen) {
        Serial.printf("[SCAN ] Tag spotted | RSSI: %d dBm (avg %.0f) | ~%.1f m\n",
                      raw_rssi, avg, dist);
      } else {
        // Reset smoothing buffer when tag not seen so stale values don't linger
        for (int i = 0; i < SMOOTH_N; i++) rssi_buf[i] = -100;
        rssi_idx = 0;
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
        // Print live distance while bag is present
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
          passed_time = millis();

          // Get current time for the event log
          unsigned long s = millis() / 1000;
          unsigned int  m = s / 60;
          unsigned int  h = m / 60;

          Serial.println("\n============================================");
          Serial.printf( "  ✅ BAG PASSED CHECKPOINT\n");
          Serial.printf( "  Tag      : %s\n", TARGET_NAME);
          Serial.printf( "  Uptime   : %02u:%02u:%02u\n", h % 24, m % 60, (unsigned int)(s % 60));
          Serial.printf( "  Peak est.: was within ~%.1f m\n", rssi_to_distance(RSSI_NEAR));
          Serial.println("============================================\n");
        }
      } else {
        far_counter = 0;
      }
      break;

    case PASSED:
      // Cooldown — ignore everything, just count down
      {
        uint32_t elapsed   = millis() - passed_time;
        uint32_t remaining = (COOLDOWN_MS - elapsed) / 1000;

        if (elapsed < COOLDOWN_MS) {
          static uint32_t last_cd_print = 0;
          if (millis() - last_cd_print > 2000) {
            Serial.printf("[COOL ] Cooldown: %us remaining before next detection\n", remaining);
            last_cd_print = millis();
          }
        } else {
          state = IDLE;
          near_counter = 0;
          far_counter  = 0;
          for (int i = 0; i < SMOOTH_N; i++) rssi_buf[i] = -100;
          rssi_idx = 0;
          Serial.println("[STATE] Cooldown done - back to IDLE, ready for next bag");
          Serial.println("--------------------------------------------");
        }
      }
      break;
  }
}
