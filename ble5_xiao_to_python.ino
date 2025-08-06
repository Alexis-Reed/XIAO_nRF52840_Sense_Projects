#include <ArduinoBLE.h>
#include "LSM6DS3.h"
#include "Wire.h"

// IMU
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// BLE Service & Characteristics
BLEService imuService("180C");
BLEStringCharacteristic imuDataChar("2A56", BLERead | BLENotify, 244);  // Max 244 bytes for BLE 5
BLECharacteristic controlChar("12345678-1234-5678-1234-56789abcdef1", BLEWrite, 16);

// Config
const int SAMPLES_PER_PACKET = 12;  // Bigger batch due to larger MTU
char packetBuffer[244];             // Increase to BLE 5 MTU size
bool imuActive = false;

// BLE 5 Configuration Parameters
#define MIN_CONN_INTERVAL 6   // 7.5 ms (6 * 1.25ms)
#define MAX_CONN_INTERVAL 12  // 15 ms
#define SLAVE_LATENCY     0   // No latency for fast response
#define SUPERVISION_TIMEOUT 400 // 4 seconds

void onControlWritten(BLEDevice central, BLECharacteristic characteristic) {
  int length = characteristic.valueLength();
  const uint8_t* rawValue = characteristic.value();

  char commandBuffer[17];
  if (length > 16) length = 16;
  memcpy(commandBuffer, rawValue, length);
  commandBuffer[length] = '\0';

  String command(commandBuffer);
  command.trim();

  if (command.equalsIgnoreCase("START")) {
    imuActive = true;
    Serial.println("✅ IMU started");
  } else if (command.equalsIgnoreCase("STOP")) {
    imuActive = false;
    Serial.println("🛑 IMU stopped");
  } else {
    Serial.println("⚠️ Unknown command");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (myIMU.begin() != 0) {
    Serial.println("IMU Device error");
    while (1);
  }
  Serial.println("IMU Device OK!");

  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  // BLE 5 optimizations
  BLE.setDeviceName("XIAO_IMU");
  BLE.setLocalName("XIAO_IMU");
  BLE.setConnectionInterval(MIN_CONN_INTERVAL, MAX_CONN_INTERVAL);  
  BLE.setAdvertisedService(imuService);

  imuService.addCharacteristic(imuDataChar);
  imuService.addCharacteristic(controlChar);
  BLE.addService(imuService);

  controlChar.setEventHandler(BLEWritten, onControlWritten);

  imuDataChar.writeValue("Waiting...");
  BLE.advertise();

  Serial.println("BLE 5 Device Ready - Connect via LightBlue");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    imuActive = false;

    while (central.connected()) {
      if (imuActive) {
        packetBuffer[0] = '\0';
        for (int i = 0; i < SAMPLES_PER_PACKET; i++) {
          int16_t ax = myIMU.readRawAccelX();
          int16_t ay = myIMU.readRawAccelY();
          int16_t az = myIMU.readRawAccelZ();
          int16_t gx = myIMU.readRawGyroX();
          int16_t gy = myIMU.readRawGyroY();
          int16_t gz = myIMU.readRawGyroZ();

          char sample[30];
          snprintf(sample, sizeof(sample), "%d,%d,%d,%d,%d,%d;", ax, ay, az, gx, gy, gz);
          strncat(packetBuffer, sample, sizeof(packetBuffer) - strlen(packetBuffer) - 1);
        }

        imuDataChar.writeValue(packetBuffer);  // Send BLE 5 packet
        Serial.println(packetBuffer);
      } else {
        delay(50); // Idle
      }
    }

    Serial.println("Disconnected from central");
    imuActive = false;
  }
}
