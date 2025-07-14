#include <Wire.h>
#include <ArduinoBLE.h>

#define ICM_ADDR      0x68  // Default I²C for ICM-42688-P (ADDR SEL jumper = GND)
#define WHO_AM_I      0x75
#define PWR_MGMT0     0x4E
#define ACCEL_XOUT_L  0x5F
#define GYRO_XOUT_L   0x33
#define ACCEL_CONFIG0 0x50
#define GYRO_CONFIG0  0x52

BLEService imuService("181A");  // Environmental Sensing service (example)
BLECharacteristic imuChar("2A58", BLERead | BLENotify, 20);

void writeReg(uint8_t r, uint8_t v) {
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(r);
  Wire.write(v);
  Wire.endTransmission();
}

void readRegs(uint8_t r, uint8_t *buf, int len) {
  Wire.beginTransmission(ICM_ADDR);
  Wire.write(r);
  Wire.endTransmission(false);
  Wire.requestFrom(ICM_ADDR, len);
  for (int i = 0; i < len; i++) buf[i] = Wire.read();
}

void setupICM() {
  uint8_t id = 0;
  readRegs(WHO_AM_I, &id, 1);
  if (id != 0x47) {
    Serial.print("WAI err: ");
    Serial.println(id, HEX);
    while (1);
  }

  writeReg(PWR_MGMT0, 0x01);          // Clock source = gyro
  writeReg(ACCEL_CONFIG0, 0x21);     // ODR=1kHz, FS=±4g
  writeReg(GYRO_CONFIG0, 0x21);      // ODR=1kHz, FS=±500dps
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!BLE.begin()) { while (1); }
  BLE.setLocalName("XIAO-ICM6DOF");
  BLE.setAdvertisedService(imuService);
  imuService.addCharacteristic(imuChar);
  BLE.addService(imuService);
  imuChar.writeValue("Init");
  BLE.advertise();

  setupICM();
  Serial.println("ICM-42688-P ready");
}

void loop() {
  BLEDevice central = BLE.central();
  if (!central) return;

  Serial.print("Connected: "); Serial.println(central.address());
  while (central.connected()) {
    uint8_t buf[12];
    readRegs(ACCEL_XOUT_L, buf, 6);
    readRegs(GYRO_XOUT_L, buf+6, 6);

    int16_t ax = (buf[1]<<8) | buf[0];
    int16_t ay = (buf[3]<<8) | buf[2];
    int16_t az = (buf[5]<<8) | buf[4];
    int16_t gx = (buf[7]<<8) | buf[6];
    int16_t gy = (buf[9]<<8) | buf[8];
    int16_t gz = (buf[11]<<8) | buf[10];

    char out[64];
    int n = snprintf(out, sizeof(out),
      "A:%.2fg,%.2fg,%.2fg G:%.2fdps,%.2fdps,%.2fdps",
      ax/4096.0, ay/4096.0, az/4096.0,
      gx/65.5, gy/65.5, gz/65.5
    );
    imuChar.writeValue((const uint8_t*)out, n);
    Serial.println(out);
    delay(100);
  }

  Serial.println("Disconnected");
}
