import asyncio
from bleak import BleakScanner, BleakClient
import pandas as pd
from datetime import datetime
import time
import os

# === CONFIG ===
TARGET_NAME = "XIAO_IMU"  # Change if needed
TARGET_ADDRESS = None    # Use address for more reliable connection
LOG_DIR = r"C:\Hypersonics\IMU\logs"

# === Ensure log directory exists ===
os.makedirs(LOG_DIR, exist_ok=True)

# === Create timestamped CSV file ===
timestamp_str = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
CSV_FILE = os.path.join(LOG_DIR, f"imu_log_{timestamp_str}.csv")

# === CSV SETUP ===
columns = ["Timestamp", "Accel_X", "Accel_Y", "Accel_Z", "Gyro_X", "Gyro_Y", "Gyro_Z"]
pd.DataFrame(columns=columns).to_csv(CSV_FILE, index=False)

# === Measurement tracking ===
measurement_count = 0
start_time = None

async def notification_handler(sender, data):
    global measurement_count, start_time
    try:
        if start_time is None:
            start_time = time.time()

        measurement_count += 1

        text = data.decode("utf-8").strip()
        print("Received:", text)

        # Parse "A:x,y,z G:x,y,z"
        parts = text.replace("A:", "").replace("G:", "").split()
        accel = parts[0].split(",")
        gyro = parts[1].split(",")

        new_row = {
            "Timestamp": datetime.now(),
            "Accel_X": accel[0],
            "Accel_Y": accel[1],
            "Accel_Z": accel[2],
            "Gyro_X": gyro[0],
            "Gyro_Y": gyro[1],
            "Gyro_Z": gyro[2],
        }

        pd.DataFrame([new_row]).to_csv(CSV_FILE, mode='a', header=False, index=False)

        # Calculate frequency every 50 measurements
        if measurement_count % 50 == 0:
            elapsed_time = time.time() - start_time
            freq = measurement_count / elapsed_time
            print(f"\n📊 Samples: {measurement_count} | Elapsed: {elapsed_time:.2f}s | Frequency: {freq:.2f} Hz\n")

    except Exception as e:
        print("Parse error:", e)

async def main():
    print("🔍 Scanning for devices...")
    devices = await BleakScanner.discover()

    print("\n📋 Devices found:")
    for d in devices:
        print(f"- Name: {d.name} | Address: {d.address}")

    xiao_device = None
    for d in devices:
        if TARGET_ADDRESS and d.address == TARGET_ADDRESS:
            xiao_device = d
            break
        elif d.name and TARGET_NAME in d.name:
            xiao_device = d
            break

    if not xiao_device:
        print("\n❌ Device not found.")
        return

    print(f"\n✅ Connecting to: {xiao_device.name} ({xiao_device.address})")

    async with BleakClient(xiao_device.address) as client:
        print("🔗 Connected!")

        print("\n📡 Services & Characteristics:")
        for service in client.services:
            print(f"[Service] {service.uuid}")
            for char in service.characteristics:
                print(f"  [Char] {char.uuid} ({char.properties})")
                if "notify" in char.properties:
                    await client.start_notify(char.uuid, notification_handler)

        print(f"\n📄 Logging IMU data to: {CSV_FILE}")
        while True:
            await asyncio.sleep(1)

if __name__ == "__main__":
    asyncio.run(main())



#python "c:\Hypersonics\IMU\xiao_imu_ble.py"
