#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Unique UUIDs for the service and characteristic
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define Led_pin 2

bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println(">>> Device Connected!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println(">>> Device Disconnected.");
      // CRITICAL: Restart advertising so it can be found again
      pServer->getAdvertising()->start();
      Serial.println(">>> Advertising restarted...");
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Test Work!");

  pinMode(Led_pin, OUTPUT);

  // Initialize the BLE device
  BLEDevice::init("ESP32_Test_Node");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // helps with iPhone connection issues
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now scan it with your phone.");
}

void loop() {
  if (deviceConnected) {
    // You can add logic here to blink an LED to show active connection
    digitalWrite(Led_pin, HIGH);
    delay(1000);
    digitalWrite(Led_pin, LOW);
    delay(1000);
  }
  delay(1000);
}