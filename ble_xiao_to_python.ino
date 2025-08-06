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
const int SAMPLES_PER_PACKET = 10;
char packetBuffer[200];

bool imuActive = false;

const int LED_PIN = LED_BUILTIN; // Built-in LED
#define LED_ON  LOW   // XIAO's LED is active low
#define LED_OFF HIGH

// Event handler for BLE control commands
void onControlWritten(BLEDevice central, BLECharacteristic characteristic) {
  int length = characteristic.valueLength();
  const uint8_t* rawValue = characteristic.value();

  // Make a safe buffer with null terminator
  char commandBuffer[17]; // 16 max + null
  if (length > 16) length = 16;
  memcpy(commandBuffer, rawValue, length);
  commandBuffer[length] = '\0';

  // Debug: print each byte
  Serial.print("Raw command bytes: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)rawValue[i]);
    Serial.print(" ");
  }
  Serial.println();

  // Debug: print parsed string
  Serial.print("Parsed command: '");
  Serial.print(commandBuffer);
  Serial.println("'");

  // Process commands
  String command(commandBuffer);
  command.trim();

  if (command.equalsIgnoreCase("START")) {
    imuActive = true;
    digitalWrite(LED_PIN, LED_ON);
    Serial.println("✅ IMU started - LED ON");
  } 
  else if (command.equalsIgnoreCase("STOP")) {
    imuActive = false;
    digitalWrite(LED_PIN, LED_OFF);
    Serial.println("🛑 IMU stopped - LED OFF");
  } 
  else {
    Serial.println("⚠️ Unknown command received");
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF); // Ensure LED is off at boot

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

  controlChar.setEventHandler(BLEWritten, onControlWritten);

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
    digitalWrite(LED_PIN, LED_OFF);

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

        imuDataChar.writeValue(packetBuffer);
        Serial.println(packetBuffer);

        delay(10);
      } else {
        delay(100);
      }
    }

    Serial.println("Disconnected from central");
    imuActive = false;
    digitalWrite(LED_PIN, LED_OFF);
  }
}
