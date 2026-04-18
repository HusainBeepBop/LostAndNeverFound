// ============================================================
//  BLE Proximity Tag — ESP32 Firmware
//  Works with python_tracker.py (Flet + Bleak)
//
//  SETUP FOR EACH PHYSICAL TAG:
//    1. Change TAG_NAME to a unique value (must match a key
//       in the KNOWN_TAGS dict inside python_tracker.py)
//    2. Flash. Done.
//
//  SIGNAL CALIBRATION NOTES:
//    - TX power is set to max (+9 dBm) for best range.
//    - Zone thresholds are tuned in python_tracker.py → ZONES.
//    - To calibrate: hold the tag at a known distance, read the
//      smoothed dBm value in the app, then update the thresholds.
//    - Typical real-world readings (varies with walls/body):
//        ~0.5 m  → -45 to -55 dBm  (Very Near)
//        ~1-2 m  → -55 to -65 dBm  (Near)
//        ~3-4 m  → -65 to -73 dBm  (Fairly Near)
//        ~5-8 m  → -73 to -82 dBm  (Far)
//        >8 m    → < -82 dBm       (Very Far)
//    - Advertising interval is set to ~100ms for snappy updates.
//      Increase ADV_INTERVAL_MIN/MAX to save battery.
// ============================================================

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

// --- TAG IDENTITY (change this per physical tag) ---
#define TAG_NAME "Aastha-Bag"
// Examples for other tags:
//   "Aastha-Backpack"
//   "Aastha-Case"
//   "Aastha-Wallet"

// Shared service UUID — identifies all your tags to the app.
// Must match TRACKER_SERVICE_UUID in python_tracker.py exactly.
#define TRACKER_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

#define LED_PIN 2

// Advertising interval in units of 0.625 ms
// 160 × 0.625 = 100 ms | 200 × 0.625 = 125 ms
#define ADV_INTERVAL_MIN 160
#define ADV_INTERVAL_MAX 200

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  Serial.printf("\n[TAG] Initializing: %s\n", TAG_NAME);

  // --- BLE Stack Init ---
  BLEDevice::init(TAG_NAME);

  // Maximum TX power for best RSSI consistency at range.
  // Drop to ESP_PWR_LVL_N0 (0 dBm) if you want battery savings.
  BLEDevice::setPower(ESP_PWR_LVL_P9);

  BLEAdvertising *pAdv = BLEDevice::getAdvertising();

  // Primary advertisement packet:
  // Flags + Service UUID — this is what BleakScanner filters on.
  BLEAdvertisementData advData;
  advData.setFlags(0x06); // LE General Discoverable | No BR/EDR
  advData.setPartialServices(BLEUUID(TRACKER_SERVICE_UUID));
  pAdv->setAdvertisementData(advData);

  // Scan response packet:
  // Full device name — this is what the app uses to tell tags apart.
  // BleakScanner requests this via active scan automatically.
  BLEAdvertisementData scanResp;
  scanResp.setName(TAG_NAME);
  pAdv->setScanResponseData(scanResp);

  pAdv->setMinInterval(ADV_INTERVAL_MIN);
  pAdv->setMaxInterval(ADV_INTERVAL_MAX);
  pAdv->start();

  Serial.println("[TAG] Advertising started. Visible to python_tracker.py");

  // 3 quick blinks = boot success
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(80);
    digitalWrite(LED_PIN, LOW);  delay(80);
  }
}

void loop() {
  // BLE radio handles advertising autonomously.
  // Heartbeat blink every 5 s confirms the tag is alive.
  delay(5000);
  digitalWrite(LED_PIN, HIGH); delay(40);
  digitalWrite(LED_PIN, LOW);
}
