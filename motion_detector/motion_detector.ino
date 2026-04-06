#include <Wire.h>

const int MPU_ADDR = 0x68; // Standard I2C address for MPU6050

int16_t ax, ay, az;
int16_t gx, gy, gz;

int16_t prev_ax, prev_ay, prev_az;
int16_t prev_gx, prev_gy, prev_gz;

// Sensitivity thresholds for movement detection. 
// These values are based on the raw sensor readings.
// You might need to adjust these upwards if it's too sensitive to vibrations,
// or downwards if it's not detecting slow movements.
const int ACCEL_THRESHOLD = 500;  
const int GYRO_THRESHOLD = 300;   

// Debounce counter to prevent the output from rapidly flickering
// between Stationary and Moving due to minor sensor noise.
int stationary_counter = 0;
const int STATIONARY_DEBOUNCE = 5; // Must be stationary for 5 consecutive loops (~250ms) to trigger

// Define LED_BUILTIN just in case it's missing on some board variants (like certain ESP32s).
// Pin 2 is the most common onboard LED pin for ESP32/ESP8266. Teensy handles LED_BUILTIN automatically.
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2
#endif

void readSensor() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // Start at ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);
  
  if (Wire.available() == 14) {
    ax = Wire.read() << 8 | Wire.read();
    ay = Wire.read() << 8 | Wire.read();
    az = Wire.read() << 8 | Wire.read();
    
    Wire.read(); Wire.read(); // Skip 2 bytes of temperature data
    
    gx = Wire.read() << 8 | Wire.read();
    gy = Wire.read() << 8 | Wire.read();
    gz = Wire.read() << 8 | Wire.read();
  }
}

void setup() {
  Serial.begin(115200);
  
  // Setup the onboard LED pin
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // Start with LED off

  while (!Serial && millis() < 5000) { delay(10); }

  Wire.begin();
  
  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println("MPU6050 not found! Check wiring.");
    while (1) { delay(1000); }
  }

  // Wake up the MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1
  Wire.write(0);
  Wire.endTransmission(true);

  // Perform an initial read to populate the `prev_` variables
  readSensor();
  prev_ax = ax; prev_ay = ay; prev_az = az;
  prev_gx = gx; prev_gy = gy; prev_gz = gz;

  Serial.println("Motion Detection Initialized.");
}

void loop() {
  readSensor();

  // Calculate the total absolute change in acceleration and gyroscope rotation
  long accel_diff = abs(ax - prev_ax) + abs(ay - prev_ay) + abs(az - prev_az);
  long gyro_diff = abs(gx - prev_gx) + abs(gy - prev_gy) + abs(gz - prev_gz);

  // Check if the change exceeds our thresholds
  bool isMoving = (accel_diff > ACCEL_THRESHOLD) || (gyro_diff > GYRO_THRESHOLD);

  if (isMoving) {
    stationary_counter = 0; // Reset debounce
    Serial.println("Moving");
    digitalWrite(LED_BUILTIN, LOW); // Turn off LED when moving
  } else {
    // We didn't detect movement this cycle, increment debounce counter
    stationary_counter++;
    
    if (stationary_counter >= STATIONARY_DEBOUNCE) {
       Serial.println("Stationary");
       digitalWrite(LED_BUILTIN, HIGH); // Light up LED when truly stationary
       stationary_counter = STATIONARY_DEBOUNCE; // Prevent integer overflow
    } else {
       // We are in the debounce period, print moving to be safe
       Serial.println("Moving"); 
       digitalWrite(LED_BUILTIN, LOW); 
    }
  }

  // Store the current values as previous for the next loop
  prev_ax = ax; prev_ay = ay; prev_az = az;
  prev_gx = gx; prev_gy = gy; prev_gz = gz;

  delay(50); // Sample at roughly 20Hz
}
