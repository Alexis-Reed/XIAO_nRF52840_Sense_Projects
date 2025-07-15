#define SAMPLES_PER_BATCH 300
char batchBuffer[SAMPLES_PER_BATCH][64];  // 64 bytes per line max
uint16_t sampleIndex = 0;
uint32_t lastBatchTimeUs = 0;

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

    snprintf(batchBuffer[sampleIndex], sizeof(batchBuffer[sampleIndex]),
             "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
             imuSample.ax, imuSample.ay, imuSample.az,
             imuSample.gx, imuSample.gy, imuSample.gz);

    sampleIndex++;

    // If we've collected 300 samples, send the batch
    if (sampleIndex >= SAMPLES_PER_BATCH) {
      for (uint16_t i = 0; i < SAMPLES_PER_BATCH; i++) {
        Serial.println(batchBuffer[i]);
      }
      sampleIndex = 0;
      lastBatchTimeUs = now;
    }
  }
}
