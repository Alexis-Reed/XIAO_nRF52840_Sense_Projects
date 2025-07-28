#include <Wire.h>
#include "LSM6DS3.h"
#include <ArduinoBLE.h>

// IMU object
LSM6DS3 imu(I2C_MODE, 0x6A);

// BLE service & characteristics
BLEService imuService("180C"); // Custom service
BLEStringCharacteristic imuDataChar("2A56", BLERead | BLENotify, 50);
BLEStringCharacteristic freqChar("2A57", BLERead | BLENotify, 20);

// Frequency tracking
unsigned long lastRatePrint = 0;
unsigned long sampleCount = 0;
float lastFrequency = 0.0;

void setup() {
  Serial.begin(921600);
  while (!Serial);  

  Wire.begin();
  Wire.setClock(1000000);

  if (imu.begin() != 0) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // IMU config (3.33kHz accel/gyro)
  imu.writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, 0b11000000);
  imu.writeRegister(LSM6DS3_ACC_GYRO_CTRL2_G,  0b11000000);

  // Start BLE
  if (!BLE.begin()) {
    Serial.println("BLE init failed!");
    while (1);
  }

  BLE.setLocalName("XIAO-IMU");
  BLE.setAdvertisedService(imuService);

  imuService.addCharacteristic(imuDataChar);
  imuService.addCharacteristic(freqChar);
  BLE.addService(imuService);

  imuDataChar.writeValue("Waiting...");
  freqChar.writeValue("0.000 kHz");

  BLE.advertise();
  Serial.println("BLE ready. Connect with LightBlue.");
}

void loop() {
  BLE.poll();

  int16_t ax, ay, az, gx, gy, gz;

  // Burst read from IMU
  Wire.beginTransmission(0x6A);
  Wire.write(0x80 | LSM6DS3_ACC_GYRO_OUTX_L_XL);
  Wire.endTransmission(false);
  Wire.requestFrom(0x6A, 12, true);

  ax = Wire.read() | (Wire.read() << 8);
  ay = Wire.read() | (Wire.read() << 8);
  az = Wire.read() | (Wire.read() << 8);
  gx = Wire.read() | (Wire.read() << 8);
  gy = Wire.read() | (Wire.read() << 8);
  gz = Wire.read() | (Wire.read() << 8);

  sampleCount++;

  // Send IMU values via BLE
  char imuPacket[50];
  snprintf(imuPacket, sizeof(imuPacket),
           "%d,%d,%d,%d,%d,%d",
           ax, ay, az, gx, gy, gz);
  imuDataChar.writeValue(imuPacket);

  // Every second, calculate and send frequency
  unsigned long now = millis();
  if (now - lastRatePrint >= 1000) {
    lastFrequency = float(sampleCount) / 1000.0;

    char freqPacket[20];
    snprintf(freqPacket, sizeof(freqPacket),
             "%.3f kHz", lastFrequency);
    freqChar.writeValue(freqPacket);

    sampleCount = 0;
    lastRatePrint = now;
  }
}
