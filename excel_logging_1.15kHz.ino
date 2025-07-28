#include <Wire.h>
#include "LSM6DS3.h"

#define IMU_ADDR 0x6A
const uint8_t SYNC_MARKER[2] = {0xAA, 0x55};

LSM6DS3 imu(I2C_MODE, IMU_ADDR);

void setup() {
  Serial.begin(921600);
  while (!Serial);

  Wire.begin();
  Wire.setClock(1000000);

  if (imu.begin() != 0) {
    while (1);
  }

  imu.writeRegister(LSM6DS3_ACC_GYRO_CTRL3_C, 0b01000100); // BDU
  imu.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, 0b11100000); // Accel 6.66kHz ±2g
  imu.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G,  0b11100000); // Gyro 6.66kHz ±245 dps
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;

  Wire.beginTransmission(IMU_ADDR);
  Wire.write(0x80 | LSM6DS3_ACC_GYRO_OUTX_L_G);
  Wire.endTransmission(false);
  Wire.requestFrom(IMU_ADDR, 12, true);

  gx = Wire.read() | (Wire.read() << 8);
  gy = Wire.read() | (Wire.read() << 8);
  gz = Wire.read() | (Wire.read() << 8);
  ax = Wire.read() | (Wire.read() << 8);
  ay = Wire.read() | (Wire.read() << 8);
  az = Wire.read() | (Wire.read() << 8);

  uint32_t timestamp = micros();

  Serial.write(SYNC_MARKER, 2); // Send exact sync marker bytes
  Serial.write((uint8_t*)&timestamp, sizeof(timestamp));
  Serial.write((uint8_t*)&ax, sizeof(ax));
  Serial.write((uint8_t*)&ay, sizeof(ay));
  Serial.write((uint8_t*)&az, sizeof(az));
  Serial.write((uint8_t*)&gx, sizeof(gx));
  Serial.write((uint8_t*)&gy, sizeof(gy));
  Serial.write((uint8_t*)&gz, sizeof(gz));
}
