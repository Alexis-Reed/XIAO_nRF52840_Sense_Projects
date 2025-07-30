import serial
import struct
import csv
from datetime import datetime
import time
import os

PORT = "COM5"   # Adjust as needed
BAUD = 921600

SYNC_MARKER = b'\xAA\x55'
PACKET_SIZE = 4 + (6 * 2)  # timestamp + 6x int16
FMT = "<Ihhhhhh"

# Fixed save location
save_dir = r"C:\Hypersonics\IMU\logs"
os.makedirs(save_dir, exist_ok=True)
csv_filename = os.path.join(save_dir, f"imu_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv")

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

    except serial.SerialException as e:
        print(f"[ERROR] Could not open serial port {PORT}: {e}")

if __name__ == "__main__":
    main()


#python "C:\Hypersonics\IMU\imu_logger.py"
