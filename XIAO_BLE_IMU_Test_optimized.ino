#include <bluefruit.h>
#include "LSM6DS3.h"
#include "Wire.h"

// IMU instance
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// BLE UART service
BLEUart bleuart;

// Target sample interval = 1 / 6600 Hz ≈ 151 microseconds
const unsigned long SAMPLE_INTERVAL_US = 151;

struct ImuSample {
  float ax, ay, az;
  float gx, gy, gz;
};

ImuSample imuSample;

unsigned long lastSampleTime = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Starting XIAO + LSM6DS3 + BLE UART");

  // Initialize IMU
  if (myIMU.begin() != 0) {
    Serial.println("IMU initialization failed!");
    while (1);
  }

  // Configure IMU to max ODR 6.6kHz
  myIMU.setAccelODR(6600);  // Check your library supports this or set registers manually
  myIMU.setGyroODR(6600);

  // Optional: set full-scale ranges for accuracy/performance
  myIMU.setAccelRange(LSM6DS3_ACC_RANGE_2G);
  myIMU.setGyroRange(LSM6DS3_GYRO_RANGE_2000DPS);

  // Initialize BLE
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("XIAO-IMU");
  bleuart.begin();

  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);

  Serial.println("BLE Advertising started");
}

void loop() {
  unsigned long now = micros();

  // Sample at ~6.6kHz interval
  if ((now - lastSampleTime) >= SAMPLE_INTERVAL_US) {
    lastSampleTime = now;

    // Read accel + gyro floats from IMU
    imuSample.ax = myIMU.readFloatAccelX();
    imuSample.ay = myIMU.readFloatAccelY();
    imuSample.az = myIMU.readFloatAccelZ();

    imuSample.gx = myIMU.readFloatGyroX();
    imuSample.gy = myIMU.readFloatGyroY();
    imuSample.gz = myIMU.readFloatGyroZ();

    // Send raw binary data to BLE UART (24 bytes per sample)
    bleuart.write((uint8_t*)&imuSample, sizeof(imuSample));
  }
}

void connect_callback(uint16_t conn_handle) {
  Serial.println("BLE Connected");
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.println("BLE Disconnected");
}
