/**
 * @file mpu6050_test.ino
 * @brief MPU6050 I2C test for ESP32 Dev Module
 * 
 * WIRING (MPU6050 to ESP32 Dev Module):
 *   GND  → GND
 *   VCC  → 3.3V
 *   SDA  → GPIO 21 (I2C SDA)
 *   SCL  → GPIO 22 (I2C SCL)
 *   INT  → Not connected (optional)
 *   AD0  → GND (sets I2C address to 0x68)
 */

#include <Wire.h>

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.
const int I2C_SDA = 21;
const int I2C_SCL = 22;

int16_t accelerometer_x, accelerometer_y, accelerometer_z;
int16_t gyro_x, gyro_y, gyro_z;
int16_t temperature;

void setup() {
  Serial.begin(115200);
  // Wait for Serial to be ready (useful for USB CDC on ESP32)
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  
  // Initialize I2C bus for ESP32
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Verify connection
  Wire.beginTransmission(MPU_ADDR);
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.println("\nMPU6050 found!");
  } else {
    Serial.println("\nMPU6050 not found! Check wiring.");
    Serial.println("ESP32 I2C pins: SDA=GPIO21, SCL=GPIO22");
    while (1) {
      delay(1000); // Halt if not found
    }
  }

  // Wake up the MPU-6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // Set to 0 to wake up the sensor
  Wire.endTransmission(true);

  // Configure Gyro (Optional, default is +/- 250 deg/s)
  // Wire.beginTransmission(MPU_ADDR);
  // Wire.write(0x1B); // GYRO_CONFIG register
  // Wire.write(0x00); // 0 = +/- 250 deg/s, 8 = +/- 500 deg/s, 16 = +/- 1000 deg/s, 24 = +/- 2000 deg/s
  // Wire.endTransmission(true);

  // Configure Accelerometer (Optional, default is +/- 2g)
  // Wire.beginTransmission(MPU_ADDR);
  // Wire.write(0x1C); // ACCEL_CONFIG register
  // Wire.write(0x00); // 0 = +/- 2g, 8 = +/- 4g, 16 = +/- 8g, 24 = +/- 16g
  // Wire.endTransmission(true);

  Serial.println("MPU6050 initialized.");
}

void loop() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // Start with ACCEL_XOUT_H register (0x3B)
  Wire.endTransmission(false); // Restart condition for continuous read
  
  // Request 14 bytes from the sensor (6 bytes Accel + 2 bytes Temp + 6 bytes Gyro)
  Wire.requestFrom(MPU_ADDR, 14, true); 
  
  if (Wire.available() == 14) {
    // Read registers: high byte shifted left by 8, then bitwise OR with low byte
    accelerometer_x = Wire.read() << 8 | Wire.read();
    accelerometer_y = Wire.read() << 8 | Wire.read();
    accelerometer_z = Wire.read() << 8 | Wire.read();
    
    temperature = Wire.read() << 8 | Wire.read();
    
    gyro_x = Wire.read() << 8 | Wire.read();
    gyro_y = Wire.read() << 8 | Wire.read();
    gyro_z = Wire.read() << 8 | Wire.read();
    
    // Print formatted data
    Serial.print("Accel [X: ");
    Serial.print(accelerometer_x);
    Serial.print("\tY: ");
    Serial.print(accelerometer_y);
    Serial.print("\tZ: ");
    Serial.print(accelerometer_z);
    
    // Convert RAW temperature to Celsius (formula from MPU6050 datasheet)
    float tempC = (temperature / 340.00) + 36.53;
    Serial.print("]\tTemp: ");
    Serial.print(tempC);
    Serial.print(" C");
    
    Serial.print("\tGyro [X: ");
    Serial.print(gyro_x);
    Serial.print("\tY: ");
    Serial.print(gyro_y);
    Serial.print("\tZ: ");
    Serial.print(gyro_z);
    Serial.println("]");
  }
  
  delay(50); // Sample at ~20Hz
}
