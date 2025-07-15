#include "LSM6DS3.h"
#include <Wire.h>

// ───────────────────────────────── IMU ─────────────────────────────────
LSM6DS3 myIMU(I2C_MODE, 0x6A);           // Use 0x6B if needed

// 6.6 kHz → 1 / 6600 Hz ≈ 151 µs
const uint32_t SAMPLE_INTERVAL_US = 151;

struct ImuSample {
  float ax, ay, az;
  float gx, gy, gz;
};

ImuSample   imuSample;
uint32_t    lastSampleTimeUs = 0;

void writeRegister(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(0x6A);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void configureLSM6DS3_HighSpeed()
{
  // Accel: 6.66 kHz, ±2 g  → 0xA0
  writeRegister(0x10, 0xA0);

  // Gyro : 6.66 kHz, ±2000 dps → 0xAC
  writeRegister(0x11, 0xAC);
}

// ───────────────────────────────── Setup ──────────────────────────────
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("XIAO 6.6 kHz IMU stream (Serial Output)");

  Wire.begin();
  Wire.setClock(400000);  // Fast I²C

  if (myIMU.begin() != 0) {
    Serial.println("IMU init failed!");
    while (1);
  }
  Serial.println("IMU OK");

  configureLSM6DS3_HighSpeed();
}

// ───────────────────────────────── Loop ───────────────────────────────
void loop() {
  uint32_t now = micros();
  if ((now - lastSampleTimeUs) >= SAMPLE_INTERVAL_US) {
    lastSampleTimeUs = now;

    imuSample.ax = myIMU.readFloatAccelX();
    imuSample.ay = myIMU.readFloatAccelY();
    imuSample.az = myIMU.readFloatAccelZ();

    imuSample.gx = myIMU.readFloatGyroX();
    imuSample.gy = myIMU.readFloatGyroY();
    imuSample.gz = myIMU.readFloatGyroZ();

    // Output as comma-separated values
    char payload[64];
    snprintf(payload, sizeof(payload), "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
             imuSample.ax, imuSample.ay, imuSample.az,
             imuSample.gx, imuSample.gy, imuSample.gz);
    Serial.print(payload);
  }
}
