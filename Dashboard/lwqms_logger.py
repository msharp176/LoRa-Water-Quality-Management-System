import serial
import csv
import os
import re
from datetime import datetime

# ============================
# User Configuration
# ============================
PORT = "/dev/ttyACM0"
BAUDRATE = 115200
TIMEOUT = 1.0          # Seconds to wait for serial input before looping again

PAYLOAD_FILE = "payloads.csv"
MESSAGE_FILE = "messages.csv"

# ============================
# Utility Functions
# ============================

def ensure_csv_header(filename, headers):
    """Ensure the CSV file exists and has the proper header."""
    if not os.path.exists(filename):
        with open(filename, mode='w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(headers)

def append_to_csv(filename, row):
    """Append a single row to the CSV file, safely."""
    with open(filename, mode='a', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(row)

def parse_payload(line):
    """
    Parse LWQMS_PLD packets like:
    LWQMS_PLD[1]: 0.000450 23.000000 7.000000 END
    """
    match = re.match(r"LWQMS_PLD\[(\d+)\]:\s*([\d\.\-Ee]+)\s+([\d\.\-Ee]+)\s+([\d\.\-Ee]+)\s+END", line)
    if match:
        node_id = int(match.group(1))
        turbidity = float(match.group(2))
        temperature = float(match.group(3))
        ph = float(match.group(4))
        timestamp = datetime.now().isoformat()
        return [timestamp, node_id, turbidity, temperature, ph]
    return None

def parse_message(line):
    """
    Parse LWQMS_MSG packets like:
    LWQMS_MSG[1]: Transmitter Ready END
    """
    match = re.match(r"LWQMS_MSG\[(\d+)\]:\s*(.*?)\s*END", line)
    if match:
        node_id = int(match.group(1))
        message = match.group(2)
        timestamp = datetime.now().isoformat()
        return [timestamp, node_id, message]
    return None

# ============================
# Main Loop
# ============================

def main():
    # Ensure CSV headers
    ensure_csv_header(PAYLOAD_FILE, ["Time", "Sensor Node ID", "Turbidity (NTU)", "Temperature (C)", "pH"])
    ensure_csv_header(MESSAGE_FILE, ["Time", "Sensor Node ID", "Message"])

    print(f"Listening on {PORT} at {BAUDRATE} baud...")
    try:
        with serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT) as ser:
            while True:
                try:
                    line = ser.readline().decode(errors='ignore').strip()
                    if not line:
                        continue

                    if "LWQMS_PLD" in line:
                        payload_data = parse_payload(line)
                        if payload_data:
                            append_to_csv(PAYLOAD_FILE, payload_data)
                            print(f"[PAYLOAD] {payload_data}")

                    elif "LWQMS_MSG" in line:
                        message_data = parse_message(line)
                        if message_data:
                            append_to_csv(MESSAGE_FILE, message_data)
                            print(f"[MESSAGE] {message_data}")

                except Exception as e:
                    print(f"Error parsing line: {e}")

    except serial.SerialException as e:
        print(f"Serial port error: {e}")
    except KeyboardInterrupt:
        print("\nStopped by user.")


if __name__ == "__main__":
    main()