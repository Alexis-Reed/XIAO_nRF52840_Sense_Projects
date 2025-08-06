import asyncio
from bleak import BleakScanner, BleakClient
import pandas as pd
from datetime import datetime
import time
import os
import threading
import struct

# === CONFIG ===
TARGET_NAME = "XIAO_IMU"  # BLE device name
TARGET_ADDRESS = None     # Optional: Use MAC address for stable connection
LOG_DIR = r"C:\Hypersonics\IMU\logs"
CONTROL_CHAR_UUID = "12345678-1234-5678-1234-56789abcdef1"  # Control commands UUID

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
client = None  # Global for command thread

async def notification_handler(sender, data):
    """Handles incoming BLE binary notifications."""
    global measurement_count, start_time

    try:
        if start_time is None:
            start_time = time.time()

        sample_size = 12  # 6 values × 2 bytes each
        sample_count = len(data) // sample_size
        valid_rows = []

        for i in range(sample_count):
            ax, ay, az, gx, gy, gz = struct.unpack_from("<hhhhhh", data, i * sample_size)
            valid_rows.append({
                "Timestamp": datetime.now(),
                "Accel_X": ax,
                "Accel_Y": ay,
                "Accel_Z": az,
                "Gyro_X": gx,
                "Gyro_Y": gy,
                "Gyro_Z": gz,
            })

        if valid_rows:
            measurement_count += len(valid_rows)
            pd.DataFrame(valid_rows).to_csv(CSV_FILE, mode='a', header=False, index=False)

        if measurement_count % 200 == 0:
            elapsed_time = time.time() - start_time
            freq = measurement_count / elapsed_time
            print(f"\n📊 Samples: {measurement_count} | Elapsed: {elapsed_time:.2f}s | Frequency: {freq:.2f} Hz\n")

    except Exception as e:
        print("Parse error:", e)

async def ble_main():
    global client
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
    client = BleakClient(xiao_device.address)
    await client.connect()
    print("🔗 Connected!")

    # Subscribe to notifications
    for service in client.services:
        for char in service.characteristics:
            if "notify" in char.properties:
                await client.start_notify(char.uuid, notification_handler)

    print(f"\n📄 Logging IMU data to: {CSV_FILE}")
    print("Type 'start' to start IMU, 'stop' to stop IMU.")

    # Keep BLE alive
    while True:
        await asyncio.sleep(1)

def command_loop():
    """Thread to listen for start/stop commands from user."""
    global client
    print("Type 'start' to start IMU, 'stop' to stop IMU (no quotes).")
    while True:
        cmd = input("> ").strip().lower()
        cmd = cmd.strip("'\"")  # Remove accidental quotes
        if client and client.is_connected:
            if cmd == "start":
                asyncio.run_coroutine_threadsafe(client.write_gatt_char(CONTROL_CHAR_UUID, b"START"), loop)
                print("▶️ Sent START command")
            elif cmd == "stop":
                asyncio.run_coroutine_threadsafe(client.write_gatt_char(CONTROL_CHAR_UUID, b"STOP"), loop)
                print("⏹ Sent STOP command")
            else:
                print("⚠️ Unknown command. Type 'start' or 'stop'.")
        else:
            print("⚠️ Not connected yet, please wait...")

if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    threading.Thread(target=command_loop, daemon=True).start()
    loop.run_until_complete(ble_main())


#python "c:\Hypersonics\IMU\xiao_imu_ble.py"
