#include <bluefruit.h>
#include <Wire.h>
#include <Arduino_LSM6DS3.h>  // Use this IMU library (default with XIAO Sense)

// BLE service and characteristic
BLEService imuService = BLEService(0x180D);
BLECharacteristic imuDataChar = BLECharacteristic(0x2A58);  // Notify IMU data

#define SAMPLE_RATE_HZ     6600
#define SAMPLES_PER_BATCH  300
#define BYTES_PER_SAMPLE   12  // 6 x int16_t
#define MAX_PACKET_SIZE    244

uint8_t imuBuffer[SAMPLES_PER_BATCH * BYTES_PER_SAMPLE];
volatile uint16_t sampleIndex = 0;
bool streaming = true;

unsigned long lastSampleMicros = 0;
const unsigned long sampleIntervalMicros = 1000000UL / SAMPLE_RATE_HZ;

void sampleIMU() {
  if (!streaming || sampleIndex >= SAMPLES_PER_BATCH) return;

  float ax, ay, az, gx, gy, gz;

  if (IMU.accelerationAvailable()) IMU.readAcceleration(ax, ay, az);
  if (IMU.gyroscopeAvailable()) IMU.readGyroscope(gx, gy, gz);

  int16_t ax_i = (int16_t)(ax * 1000);
  int16_t ay_i = (int16_t)(ay * 1000);
  int16_t az_i = (int16_t)(az * 1000);
  int16_t gx_i = (int16_t)(gx * 100);
  int16_t gy_i = (int16_t)(gy * 100);
  int16_t gz_i = (int16_t)(gz * 100);

  uint8_t* ptr = imuBuffer + sampleIndex * BYTES_PER_SAMPLE;
  memcpy(ptr, &ax_i, 2); memcpy(ptr + 2, &ay_i, 2); memcpy(ptr + 4, &az_i, 2);
  memcpy(ptr + 6, &gx_i, 2); memcpy(ptr + 8, &gy_i, 2); memcpy(ptr + 10, &gz_i, 2);

  sampleIndex++;

  if (sampleIndex >= SAMPLES_PER_BATCH) {
    transmitIMUData();
    sampleIndex = 0;
  }
}

void transmitIMUData() {
  uint16_t offset = 0;
  while (offset < sizeof(imuBuffer)) {
    uint16_t chunk = min(MAX_PACKET_SIZE, sizeof(imuBuffer) - offset);
    imuDataChar.notify(imuBuffer + offset, chunk);
    offset += chunk;
    delayMicroseconds(500);  // Prevent BLE congestion
  }
}

void setupBLE() {
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("XIAO-IMU");
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  imuService.begin();
  imuDataChar.setProperties(CHR_PROPS_NOTIFY);
  imuDataChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  imuDataChar.setMaxLen(MAX_PACKET_SIZE);
  imuDataChar.begin();

  Bluefruit.Advertising.addService(imuService);
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.start(0);

  Serial.println("✅ BLE advertising started");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("🔧 Serial started");

  Wire.begin();
  if (!IMU.begin()) {
    Serial.println("❌ IMU failed to initialize");
    while (1);
  }
  Serial.println("✅ IMU initialized");

  setupBLE();
}

void loop() {
  if (streaming && (micros() - lastSampleMicros >= sampleIntervalMicros)) {
    lastSampleMicros = micros();
    sampleIMU();
  }
}
