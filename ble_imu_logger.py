import asyncio
from bleak import BleakScanner, BleakClient
import pandas as pd
from datetime import datetime

# === CONFIG ===
TARGET_NAME = "XIAO_IMU"  # Change if your XIAO advertises differently
TARGET_ADDRESS = None    # Fill this in after scanning for more reliable connection
CSV_FILE = "imu_log.csv"

# === CSV SETUP ===
columns = ["Timestamp", "Accel_X", "Accel_Y", "Accel_Z", "Gyro_X", "Gyro_Y", "Gyro_Z"]
pd.DataFrame(columns=columns).to_csv(CSV_FILE, index=False)

async def notification_handler(sender, data):
    """Handles incoming BLE notifications."""
    try:
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

    except Exception as e:
        print("Parse error:", e)

async def main():
    print("🔍 Scanning for devices...")
    devices = await BleakScanner.discover()

    print("\n📋 Devices found:")
    for d in devices:
        print(f"- Name: {d.name} | Address: {d.address}")

    # Find XIAO by address (preferred) or name
    xiao_device = None
    for d in devices:
        if TARGET_ADDRESS and d.address == TARGET_ADDRESS:
            xiao_device = d
            break
        elif d.name and TARGET_NAME in d.name:
            xiao_device = d
            break

    if not xiao_device:
        print("\n❌ Device not found. Check name/address above.")
        return

    print(f"\n✅ Connecting to: {xiao_device.name} ({xiao_device.address})")

    async with BleakClient(xiao_device.address) as client:
        print("🔗 Connected!")

        print("\n📡 Services & Characteristics:")
        for service in client.services:
            print(f"[Service] {service.uuid}")
            for char in service.characteristics:
                print(f"  [Char] {char.uuid} ({char.properties})")
                # Subscribe to notify-enabled characteristics
                if "notify" in char.properties:
                    await client.start_notify(char.uuid, notification_handler)

        print("\n📄 Logging IMU data to CSV... Press Ctrl+C to stop.")
        while True:
            await asyncio.sleep(1)

if __name__ == "__main__":
    asyncio.run(main())
#python "c:\Hypersonics\IMU\xiao_imu_ble.py"
#change to python ["file path']
