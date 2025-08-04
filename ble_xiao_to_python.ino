#include <ArduinoBLE.h>
#include "LSM6DS3.h"
#include "Wire.h"

// IMU
LSM6DS3 myIMU(I2C_MODE, 0x6A);

// BLE Service & Characteristics
BLEService imuService("180C");
BLEStringCharacteristic imuDataChar("2A56", BLERead | BLENotify, 200);  // Data notifications
BLECharacteristic controlChar("12345678-1234-5678-1234-56789abcdef1", BLEWrite, 16); // Control commands

// Config
const int SAMPLES_PER_PACKET = 10; // Samples per BLE packet
char packetBuffer[200];             // Buffer for batched data

bool imuActive = false;             // Track IMU reading state

const int LED_PIN = LED_BUILTIN;     // Built-in LED (or change to custom pin)

// Event handler for BLE control commands
void onControlWritten(BLEDevice central, BLECharacteristic characteristic) {
  int length = characteristic.valueLength();
  char buffer[17]; // 16 bytes max + null terminator
  if (length > 16) length = 16;

  memcpy(buffer, characteristic.value(), length);
  buffer[length] = '\0';

  String command = String(buffer);
  command.trim();

  if (command.equalsIgnoreCase("START")) {
    imuActive = true;
    digitalWrite(LED_PIN, HIGH);  // LED ON
    Serial.println("Received START command - IMU active");
  } 
  else if (command.equalsIgnoreCase("STOP")) {
    imuActive = false;
    digitalWrite(LED_PIN, LOW);   // LED OFF
    Serial.println("Received STOP command - IMU inactive");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize IMU
  if (myIMU.begin() != 0) {
    Serial.println("IMU Device error");
    while (1);
  }
  Serial.println("IMU Device OK!");

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }

  BLE.setDeviceName("XIAO_IMU");
  BLE.setLocalName("XIAO_IMU");
  BLE.setAdvertisedService(imuService);

  imuService.addCharacteristic(imuDataChar);
  imuService.addCharacteristic(controlChar);
  BLE.addService(imuService);

  controlChar.setEventHandler(BLEWritten, onControlWritten); // Attach handler

  imuDataChar.writeValue("Waiting...");
  BLE.advertise();

  Serial.println("BLE Device Ready - Connect via LightBlue");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    imuActive = false;
    digitalWrite(LED_PIN, LOW); // Ensure LED is off initially

    while (central.connected()) {
      if (imuActive) {
        // Collect batch of samples
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

        imuDataChar.writeValue(packetBuffer);
        Serial.println(packetBuffer);

        delay(10); // Adjust as needed
      } else {
        delay(100); // Idle delay when inactive
      }
    }

    Serial.println("Disconnected from central");
    imuActive = false;
    digitalWrite(LED_PIN, LOW); // LED off when disconnected
  }
}
