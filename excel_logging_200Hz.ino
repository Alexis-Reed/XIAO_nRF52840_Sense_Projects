#include "LSM6DS3.h"
#include <Wire.h>

// ───────────────────────────────── IMU ─────────────────────────────────
LSM6DS3 myIMU(I2C_MODE, 0x6A);  // I²C device address

// Packet marker for Python sync
const uint8_t SYNC_MARKER[2] = {0xAA, 0x55};

// 6.6 kHz target sampling
const uint32_t SAMPLE_INTERVAL_US = 151;
uint32_t lastSampleTimeUs = 0;

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(0x6A);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void configureLSM6DS3_HighSpeed() {
  // Accelerometer: 6.66 kHz, ±2g
  writeRegister(0x10, 0xA0);
  // Gyroscope: 6.66 kHz, ±2000 dps
  writeRegister(0x11, 0xAC);
}

void setup() {
  Serial.begin(921600);
  while (!Serial) {}

  Wire.begin();
  Wire.setClock(400000);

  if (myIMU.begin() != 0) {
    while (1);  // Halt if IMU init fails
  }

  configureLSM6DS3_HighSpeed();
}

void loop() {
  uint32_t now = micros();
  if ((now - lastSampleTimeUs) >= SAMPLE_INTERVAL_US) {
    lastSampleTimeUs = now;

    // Read floats
    float ax_f = myIMU.readFloatAccelX();
    float ay_f = myIMU.readFloatAccelY();
    float az_f = myIMU.readFloatAccelZ();
    float gx_f = myIMU.readFloatGyroX();
    float gy_f = myIMU.readFloatGyroY();
    float gz_f = myIMU.readFloatGyroZ();

    // Convert floats to int16_t (scaled)
    int16_t ax = (int16_t)(ax_f * 1000);  // mg
    int16_t ay = (int16_t)(ay_f * 1000);
    int16_t az = (int16_t)(az_f * 1000);
    int16_t gx = (int16_t)(gx_f * 100);   // dps × 100
    int16_t gy = (int16_t)(gy_f * 100);
    int16_t gz = (int16_t)(gz_f * 100);

    uint32_t timestamp = now;

    // Send binary packet
    Serial.write(SYNC_MARKER, 2);
    Serial.write((uint8_t*)&timestamp, sizeof(timestamp));
    Serial.write((uint8_t*)&ax, sizeof(ax));
    Serial.write((uint8_t*)&ay, sizeof(ay));
    Serial.write((uint8_t*)&az, sizeof(az));
    Serial.write((uint8_t*)&gx, sizeof(gx));
    Serial.write((uint8_t*)&gy, sizeof(gy));
    Serial.write((uint8_t*)&gz, sizeof(gz));
  }
}
