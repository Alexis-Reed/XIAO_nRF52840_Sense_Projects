#include <ArduinoBLE.h>
#include "LSM6DS3.h"
#include "Wire.h"

// IMU
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// BLE Service & Characteristic
BLEService imuService("180C");  // custom IMU service UUID
BLEStringCharacteristic imuDataChar("2A56", BLERead | BLENotify, 64); // 64-byte buffer

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Start IMU
  if (myIMU.begin() != 0) {
    Serial.println("IMU Device error");
  } else {
    Serial.println("IMU Device OK!");
  }

  // Start BLE
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }
  
  BLE.setLocalName("XIAO_IMU");
  BLE.setAdvertisedService(imuService);

  imuService.addCharacteristic(imuDataChar);
  BLE.addService(imuService);

  imuDataChar.writeValue("Waiting for data...");
  BLE.advertise();

  Serial.println("BLE Device Ready - Connect via LightBlue");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) { // connected
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {
      // Read IMU data
      float ax = myIMU.readFloatAccelX();
      float ay = myIMU.readFloatAccelY();
      float az = myIMU.readFloatAccelZ();
      float gx = myIMU.readFloatGyroX();
      float gy = myIMU.readFloatGyroY();
      float gz = myIMU.readFloatGyroZ();

      // Create a compact string
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "A:%.2f,%.2f,%.2f G:%.2f,%.2f,%.2f",
               ax, ay, az, gx, gy, gz);

      // Send over BLE
      imuDataChar.writeValue(buffer);

      // Debug on Serial
      Serial.println(buffer);

      delay(200); // update every 200ms
    }

    Serial.println("Disconnected from central");
  }
}
