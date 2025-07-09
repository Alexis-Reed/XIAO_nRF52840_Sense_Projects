#include <bluefruit.h>

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Starting BLE test...");

  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("XIAO-BLE-Test");

  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.start(0);

  Serial.println("BLE Advertising started.");
}

void loop() {
  // Nothing here
}
