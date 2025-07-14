#include <bluefruit.h>
#include "LSM6DS3.h"
#include "Wire.h"

// IMU Setup
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// BLE UART Service
BLEUart bleuart;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Starting IMU + BLE UART...");

  // Init IMU
  if (myIMU.begin() != 0) {
    Serial.println("IMU device error");
  } else {
    Serial.println("IMU initialized.");
  }

  // Init BLE
  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Max TX power
  Bluefruit.setName("XIAO-IMU");

  // BLE UART setup
  bleuart.begin();
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Start Advertising
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.setInterval(32, 244); // Fast advertising
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0); // 0 = forever

  Serial.println("BLE Advertising started...");
}

void loop() {
  // Read IMU data
  float ax = myIMU.readFloatAccelX();
  float ay = myIMU.readFloatAccelY();
  float az = myIMU.readFloatAccelZ();

  float gx = myIMU.readFloatGyroX();
  float gy = myIMU.readFloatGyroY();
  float gz = myIMU.readFloatGyroZ();

  float tempC = myIMU.readTempC();

  // Format as CSV or readable text
  String imuData = "Accel: " + String(ax, 2) + "," + String(ay, 2) + "," + String(az, 2) + " | " +
                   "Gyro: " + String(gx, 2) + "," + String(gy, 2) + "," + String(gz, 2) + " | " +
                   "Temp: " + String(tempC, 2) + " C";

  // Send to BLE UART
  bleuart.print(imuData + "\n");

  // Also print to Serial Monitor
  Serial.println(imuData);

  delay(500); // adjust as needed
}

// Optional callbacks
void connect_callback(uint16_t conn_handle) {
  Serial.println("BLE Connected");
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("BLE Disconnected");
}
