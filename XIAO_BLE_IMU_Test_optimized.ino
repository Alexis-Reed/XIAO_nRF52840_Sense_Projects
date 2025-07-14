#include <bluefruit.h>
#include "LSM6DS3.h"
#include <Wire.h>

// ───────────────────────────────── IMU ─────────────────────────────────
LSM6DS3 myIMU(I2C_MODE, 0x6A);           // If your scan ever shows 0x6B, change it

// 6.6 kHz → 1 / 6600 Hz ≈ 151 µs
const uint32_t SAMPLE_INTERVAL_US = 151;

struct ImuSample {
  float ax, ay, az;
  float gx, gy, gz;
};

// ─────────────────────────────── BLE UART ─────────────────────────────
BLEUart bleuart;

// ─────────────────────────── Globals / helpers ────────────────────────
ImuSample   imuSample;
uint32_t    lastSampleTimeUs = 0;

void writeRegister(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(0x6A);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void configureLSM6DS3_HighSpeed()
{
  // Accel: 6.66 kHz, ±2 g  -> 0xA0
  writeRegister(0x10, 0xA0);

  // Gyro : 6.66 kHz, ±2000 dps -> 0xAC
  writeRegister(0x11, 0xAC);
}

// ───────────────────────────────── Setup ──────────────────────────────
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("XIAO‑BLE 6.6 kHz IMU stream");

  Wire.begin();
  Wire.setClock(400000);                 // 400 kHz I²C = ~8 µs for 6‑byte read

  if (myIMU.begin() != 0) {
    Serial.println("IMU init failed!");
    while (1);
  }
  Serial.println("IMU OK");

  configureLSM6DS3_HighSpeed();

  // ── BLE init ────────────────────────────────────────────────────────
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("XIAO-IMU-6k");

  bleuart.begin();                       // must precede Advertising

  Bluefruit.Periph.setConnectCallback([](uint16_t) {
    Serial.println("BLE connected");
  });
  Bluefruit.Periph.setDisconnectCallback([](uint16_t, uint8_t) {
    Serial.println("BLE disconnected");
  });

  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);     // fast, then slow
  Bluefruit.Advertising.start(0);

  Serial.println("Advertising…");
}

// ───────────────────────────────── Loop ───────────────────────────────
void loop() {
  uint32_t now = micros();
  if ((now - lastSampleTimeUs) >= SAMPLE_INTERVAL_US) {
    lastSampleTimeUs = now;

    imuSample.ax = myIMU.readFloatAccelX();
    imuSample.ay = myIMU.readFloatAccelY();
    imuSample.az = myIMU.readFloatAccelZ();

    imuSample.gx = myIMU.readFloatGyroX();
    imuSample.gy = myIMU.readFloatGyroY();
    imuSample.gz = myIMU.readFloatGyroZ();

    // Print to Serial to confirm values
    Serial.print(imuSample.ax); Serial.print(", ");
    Serial.print(imuSample.ay); Serial.print(", ");
    Serial.print(imuSample.az); Serial.print(", ");
    Serial.print(imuSample.gx); Serial.print(", ");
    Serial.print(imuSample.gy); Serial.print(", ");
    Serial.println(imuSample.gz);

    // Send to BLE only if connected and notify is enabled
    if (bleuart.notifyEnabled()) {
      bleuart.write((uint8_t*)&imuSample, sizeof(imuSample));
      Serial.println("Sent over BLE");
    }
  }
}
