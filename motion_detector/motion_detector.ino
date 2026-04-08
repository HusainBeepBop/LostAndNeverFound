/**
 * @file motion_detector.ino
 * @brief Motion Detection for lost items using MPU6050 + ESP32 Dev Module
 * 
 * WIRING (MPU6050 to ESP32 Dev Module):
 *   GND  → GND
 *   VCC  → 3.3V
 *   SDA  → GPIO 21 (I2C SDA)
 *   SCL  → GPIO 22 (I2C SCL)
 *   INT  → Not connected (optional)
 *   AD0  → GND (sets I2C address to 0x68)
 * 
 * LED Output:
 *   LED_BUILTIN (GPIO 2) → ON when stationary, OFF when moving
 */

#include <Wire.h>

const int MPU_ADDR = 0x68; // Standard I2C address for MPU6050
const int I2C_SDA = 21;
const int I2C_SCL = 22;

int16_t ax, ay, az;
int16_t gx, gy, gz;

// Low-Pass Filter variables to track the gravity vector
float grav_x = 0;
float grav_y = 0;
float grav_z = 0;

// ==========================================
// ====== SENSITIVITY CONFIGURATION =========
// ==========================================
// We use a "Dynamic Acceleration" method. This ignores gravity and the orientation 
// of the sensor (e.g. turning it over slowly won't trigger it), but it looks for 
// sudden movements/spikes like walking or grabbing a bag.
// 
// SENSITIVITY: 
// 1 = Extremely sensitive
// 5 = Medium (Detects most handling / walking)
// 10 = Very insensitive (Needs a hard jolt)
const int SENSITIVITY = 5; 

// If you want even finer control, you can manually change these base values:
const long ACCEL_THRESHOLD = 500 * SENSITIVITY;  
const long GYRO_THRESHOLD = 400 * SENSITIVITY;   

// Enable DEBUG_MODE to print the raw movement delta values to the Serial Monitor/Plotter.
const bool DEBUG_MODE = false;

// ==========================================
// ============ DEBOUNCING ==================
// ==========================================
int stationary_counter = 0;
// Since walking produces cyclical bumps (meaning there are moments of zero acceleration 
// between footsteps), we need a longer debounce to consider a bag/pocket "Stationary".
// 20 loops * 50ms = 1 full second of NO movement before declaring it Stationary.
const int STATIONARY_DEBOUNCE = 20;

// ESP32 Dev Module built-in LED on GPIO 2
const int LED_PIN = 2;

void readSensor() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // Start at ACCEL_XOUT_H
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
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  while (!Serial && millis() < 5000) { delay(10); }

  // Initialize I2C for ESP32
  Wire.begin(I2C_SDA, I2C_SCL);
  
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

  // Perform an initial read and set the baseline gravity
  readSensor();
  grav_x = ax;
  grav_y = ay;
  grav_z = az;

  Serial.println("Pocket/Bag Motion Detection Initialized.");
}

void loop() {
  readSensor();

  // 1. Update the Low-Pass Filter to find where "Down" (gravity) currently is
  // This smoothly absorbs rotations over time, so slowly turning it over won't be seen as a "spike".
  grav_x = (grav_x * 0.9) + (ax * 0.1);
  grav_y = (grav_y * 0.9) + (ay * 0.1);
  grav_z = (grav_z * 0.9) + (az * 0.1);

  // 2. Subtract gravity from the current reading to find only the Dynamic Acceleration (the jolt)
  long dyn_x = abs(ax - (long)grav_x);
  long dyn_y = abs(ay - (long)grav_y);
  long dyn_z = abs(az - (long)grav_z);
  long total_dyn_accel = dyn_x + dyn_y + dyn_z;

  // 3. For gyroscope, we don't need a filter, just check the absolute magnitude of rotation rate
  long total_gyro = abs(gx) + abs(gy) + abs(gz);

  if (DEBUG_MODE) {
    Serial.print("Dyn_Accel:"); Serial.print(total_dyn_accel);
    Serial.print("\tGyro_Mag:"); Serial.print(total_gyro);
    Serial.print("\tAccel_Thresh:"); Serial.print(ACCEL_THRESHOLD);
    Serial.print("\tGyro_Thresh:"); Serial.println(GYRO_THRESHOLD);
  }

  // Check if the change exceeds our thresholds
  bool isMoving = (total_dyn_accel > ACCEL_THRESHOLD) || (total_gyro > GYRO_THRESHOLD);

  if (!DEBUG_MODE) {
    if (isMoving) {
      stationary_counter = 0; // Reset debounce
      Serial.println("Moving");
      digitalWrite(LED_PIN, LOW); // Turn off LED when moving
    } else {
      // We didn't detect movement this cycle, increment debounce counter
      stationary_counter++;
      
      if (stationary_counter >= STATIONARY_DEBOUNCE) {
         Serial.println("Stationary");
         digitalWrite(LED_PIN, HIGH); // Light up LED when truly stationary
         stationary_counter = STATIONARY_DEBOUNCE; // Prevent integer overflow
      } else {
         // We are in the debounce period. Since we are tuning for a pocket/bag, 
         // it's safer to assume it's still moving until it fully proves it's stationary.
         Serial.println("Moving"); 
         digitalWrite(LED_PIN, LOW); 
      }
    }
  }

  delay(50); // Sample at roughly 20Hz
}
