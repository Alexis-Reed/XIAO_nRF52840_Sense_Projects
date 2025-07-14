#include <bluefruit.h>
#include "LSM6DS3.h"
#include "Wire.h"

// IMU
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// BLE UART Service
BLEUart bleuart;  // THIS handles notify/write/read under the hood

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Starting IMU + BLE UART...");

  // IMU Init
  if (myIMU.begin() != 0) {
    Serial.println("IMU device error");
  } else {
    Serial.println("IMU initialized.");
  }

  // BLE Init
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("XIAO-IMU");

  // Start UART service
  bleuart.begin();  // << THIS must be called BEFORE advertising!

  // Optional callbacks
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Advertising
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.addService(bleuart);  // << include UART service
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);

  Serial.println("BLE Advertising started...");
}

void loop() {
  float ax = myIMU.readFloatAccelX();
  float ay = myIMU.readFloatAccelY();
  float az = myIMU.readFloatAccelZ();

  float gx = myIMU.readFloatGyroX();
  float gy = myIMU.readFloatGyroY();
  float gz = myIMU.readFloatGyroZ();

  float tempC = myIMU.readTempC();

  // Send IMU data
  String imuData = String(ax, 2) + "," + String(ay, 2) + "," + String(az, 2) + "," +
                   String(gx, 2) + "," + String(gy, 2) + "," + String(gz, 2) + "," +
                   String(tempC, 2);
  bleuart.println(imuData);
  Serial.println(imuData);

  delay(500);
}

void connect_callback(uint16_t conn_handle) {
  Serial.println("BLE Connected");
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("BLE Disconnected");
}
