#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEBeacon.h>

#define GPIO_DEEP_SLEEP_DURATION 10 // Sleep duration in seconds (optional for later)
#define Led_pin 2

// Unique UUID for your tracker (kept from your original code)
#define DEVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

void setup() {
  Serial.begin(115200);
  pinMode(Led_pin, OUTPUT);
  Serial.println("Starting iBeacon Tracker Mode...");

  // 1. Initialize the BLE Hardware
  BLEDevice::init("ESP32_Tracker");

  // 2. Create the Beacon Object
  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x4C00); // Fake Apple's ID to ensure standard iBeacon compatibility
  oBeacon.setProximityUUID(BLEUUID(DEVICE_UUID));
  
  // Major and Minor can be used to identify different bags
  oBeacon.setMajor(0x01); 
  oBeacon.setMinor(0x01);
  
  // Signal strength at 1 meter (Used by your Expo app for distance math)
  // You may need to tune this value (-59 is a standard baseline)
  oBeacon.setSignalPower(-59);

  // 3. Create the Advertising Data
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  oAdvertisementData.setFlags(0x04); // BR_EDR_NOT_SUPPORTED 
  
  String strServiceData = "";
  strServiceData += (char)26;     // Length
  strServiceData += (char)0xFF;   // Type
  strServiceData += oBeacon.getData(); 
  oAdvertisementData.addData(strServiceData);

  // 4. Start Advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("Beacon is broadcasting! Use nRF Connect or your Expo app to see RSSI.");
  
  // Visual indicator that beacon is active
  digitalWrite(Led_pin, HIGH);
  delay(500);
  digitalWrite(Led_pin, LOW);
}

void loop() {
  // In pure beacon mode, we don't need code in the loop.
  // The radio hardware handles the broadcasting in the background.
  delay(5000); 
}