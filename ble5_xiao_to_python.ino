#include <ArduinoBLE.h>
#include "LSM6DS3.h"
#include "Wire.h"

// IMU
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// BLE Service & Characteristics
BLEService imuService("180C");
BLECharacteristic imuDataChar("2A56", BLERead | BLENotify, 244); // Binary data
BLECharacteristic controlChar("12345678-1234-5678-1234-56789abcdef1", BLEWrite, 16);

// Config
const int SAMPLES_PER_PACKET = 10;      // Adjust for MTU
bool imuActive = false;

// BLE 5 Parameters
#define MIN_CONN_INTERVAL 6   // 7.5 ms
#define MAX_CONN_INTERVAL 12  // 15 ms

// Buffer for binary samples
int16_t imuBuffer[SAMPLES_PER_PACKET][6]; // 6 values/sample

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

  BLE.setDeviceName("XIAO_IMU");
  BLE.setLocalName("XIAO_IMU");
  BLE.setConnectionInterval(MIN_CONN_INTERVAL, MAX_CONN_INTERVAL);
  BLE.setAdvertisedService(imuService);

  imuService.addCharacteristic(imuDataChar);
  imuService.addCharacteristic(controlChar);
  BLE.addService(imuService);

  controlChar.setEventHandler(BLEWritten, onControlWritten);

  imuDataChar.writeValue((uint8_t*)"WAIT", 4);
  BLE.advertise();

  Serial.println("BLE 5 Device Ready - Binary Mode");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    imuActive = false;

    while (central.connected()) {
      if (imuActive) {
        for (int i = 0; i < SAMPLES_PER_PACKET; i++) {
          imuBuffer[i][0] = myIMU.readRawAccelX();
          imuBuffer[i][1] = myIMU.readRawAccelY();
          imuBuffer[i][2] = myIMU.readRawAccelZ();
          imuBuffer[i][3] = myIMU.readRawGyroX();
          imuBuffer[i][4] = myIMU.readRawGyroY();
          imuBuffer[i][5] = myIMU.readRawGyroZ();
        }

        // Send raw bytes
        imuDataChar.writeValue((uint8_t*)imuBuffer, sizeof(imuBuffer));
      } else {
        delay(50);
      }
    }

    Serial.println("Disconnected from central");
    imuActive = false;
  }
}
