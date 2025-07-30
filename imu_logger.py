import serial
import struct
import csv
from datetime import datetime
import time
import os
import pandas as pd
import numpy as np

PORT = "COM5"   # Adjust as needed
BAUD = 921600

SYNC_MARKER = b'\xAA\x55'
PACKET_SIZE = 4 + (6 * 2)  # timestamp + 6x int16
FMT = "<Ihhhhhh"

# Fixed save location
save_dir = r"C:\Hypersonics\IMU\logs"
os.makedirs(save_dir, exist_ok=True)
csv_filename = os.path.join(save_dir, f"imu_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv")

def process_imu_data(input_csv):
    output_csv = input_csv.replace('.csv', '_with_velocity.csv')

    # Load raw IMU CSV data
    df = pd.read_csv(input_csv)

    required_cols = {'time_us', 'ax', 'ay', 'az'}
    if not required_cols.issubset(df.columns):
        raise ValueError(f"CSV must contain columns: {required_cols}")

    # Convert time to seconds relative to first timestamp
    df['time_s'] = (df['time_us'] - df['time_us'][0]) / 1e6

    # Convert raw acceleration to m/s² (assuming ±2g mode, raw in mg)
    ACC_SCALE = 9.80665 / 1000  # mg → m/s²
    df['ax_mps2'] = df['ax'] * ACC_SCALE
    df['ay_mps2'] = df['ay'] * ACC_SCALE
    df['az_mps2'] = df['az'] * ACC_SCALE

    # Integrate acceleration to velocity using cumulative trapezoidal rule
    dt = np.gradient(df['time_s'])

    df['vx'] = np.cumsum(df['ax_mps2'] * dt)
    df['vy'] = np.cumsum(df['ay_mps2'] * dt)
    df['vz'] = np.cumsum(df['az_mps2'] * dt)

    df.to_csv(output_csv, index=False)
    print(f"[INFO] Velocity calculated and saved to {output_csv}")

def main():
    try:
        with serial.Serial(PORT, BAUD, timeout=1) as ser, open(csv_filename, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(["time_us", "ax", "ay", "az", "gx", "gy", "gz"])
            print(f"[INFO] Logging IMU data to:\n  {csv_filename}")

            packet_count = 0
            last_time = time.time()

            while True:
                # Sync search
                if ser.read(1) != SYNC_MARKER[0:1]:
                    continue
                if ser.read(1) != SYNC_MARKER[1:2]:
                    continue

                # Read packet
                data = ser.read(PACKET_SIZE)
                if len(data) != PACKET_SIZE:
                    continue

                try:
                    unpacked = struct.unpack(FMT, data)
                    writer.writerow(unpacked)
                    csvfile.flush()  # Ensure data is written immediately
                    packet_count += 1

                except struct.error:
                    pass

                # Show rate every second
                now = time.time()
                if now - last_time >= 1.0:
                    rate_hz = packet_count / (now - last_time)
                    print(f"[RATE] {rate_hz:.0f} Hz ({rate_hz/1000:.2f} kHz)")
                    packet_count = 0
                    last_time = now

    except KeyboardInterrupt:
        print("\n[INFO] Logging stopped by user.")
        print("[INFO] Processing recorded IMU data for velocity calculation...")
        process_imu_data(csv_filename)

    except serial.SerialException as e:
        print(f"[ERROR] Could not open serial port {PORT}: {e}")

if __name__ == "__main__":
    main()



#python "C:\Hypersonics\IMU\imu_logger.py"
