#!/usr/bin/env python3

import serial
import csv
import os
import re
import time
import random
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
# Node 2 Simulation Settings
# ============================

NODE2_ID = 2
NODE2_INTERVAL_SECONDS = 15 * 60  # 15 minutes

# Base values
NODE2_BASE_PH = 7.83
NODE2_BASE_TEMP = 22.75
NODE2_BASE_TURB = 0.15

# Physical/global ranges
NODE2_PH_MIN, NODE2_PH_MAX = 6.5, 8.5
NODE2_TEMP_MIN, NODE2_TEMP_MAX = 0.0, 35.0
NODE2_TURB_MIN, NODE2_TURB_MAX = 0.0, 1.0

# Current state so values drift smoothly instead of jumping randomly
NODE2_STATE = {
    "ph": NODE2_BASE_PH,
    "temp": NODE2_BASE_TEMP,
    "turb": NODE2_BASE_TURB,
}

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
    match = re.match(
        r"LWQMS_PLD\[(\d+)\]:\s*([\d\.\-Ee]+)\s+([\d\.\-Ee]+)\s+([\d\.\-Ee]+)\s+END",
        line,
    )
    if match:
        node_id = int(match.group(1))
        turbidity = float(match.group(2))
        temperature = float(match.group(3))
        ph = float(match.group(4))
        timestamp = datetime.now().isoformat()
        # IMPORTANT: order matches your dashboard expectation:
        # [Time, Sensor Node ID, Turbidity (NTU), Temperature (C), pH]
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

def _bounded_random_step(current, base, frac_of_base_step, hard_min, hard_max):
    # Make a small random step around 'current' with limits based on base and hard bounds.
    max_step = base * frac_of_base_step
    delta = random.uniform(-max_step, max_step)
    new_val = current + delta

    # ±10% band around base
    band_min = base * 0.9
    band_max = base * 1.1

    # Respect both the band and the physical limits
    lower = max(hard_min, band_min)
    upper = min(hard_max, band_max)

    return max(lower, min(upper, new_val))

def generate_node2_payload():
    # Generate a single fake payload row for Node 2.
    # Returns [timestamp_iso, node_id, turbidity, temperature, ph]

    # Use and update global state so values drift smoothly
    NODE2_STATE["ph"] = _bounded_random_step(
        NODE2_STATE["ph"], NODE2_BASE_PH, 0.02, NODE2_PH_MIN, NODE2_PH_MAX
    )
    NODE2_STATE["temp"] = _bounded_random_step(
        NODE2_STATE["temp"], NODE2_BASE_TEMP, 0.02, NODE2_TEMP_MIN, NODE2_TEMP_MAX
    )
    NODE2_STATE["turb"] = _bounded_random_step(
        NODE2_STATE["turb"], NODE2_BASE_TURB, 0.05, NODE2_TURB_MIN, NODE2_TURB_MAX
    )

    timestamp = datetime.now().isoformat()
    turb = NODE2_STATE["turb"]
    temp = NODE2_STATE["temp"]
    ph = NODE2_STATE["ph"]
    
    turb_str = f"{turb:.6f}"
    temp_str = f"{temp:.6f}"
    ph_str   = f"{ph:.6f}"

    return [timestamp, NODE2_ID, turb_str, temp_str, ph_str]

# ============================
# Main Loop
# ============================

def main():
    # Ensure CSV headers
    ensure_csv_header(
        PAYLOAD_FILE,
        ["Time", "Sensor Node ID", "Turbidity (NTU)", "Temperature (C)", "pH"],
    )
    ensure_csv_header(MESSAGE_FILE, ["Time", "Sensor Node ID", "Message"])

    print(f"Listening on {PORT} at {BAUDRATE} baud...")

    # Initial Node 2 sample at startup
    startup_row = generate_node2_payload()
    append_to_csv(PAYLOAD_FILE, startup_row)
    print(f"[SIM NODE 2 STARTUP] {startup_row}")
    
    # Track when we last generated a Node 2 sample
    last_node2_time = time.time()

    try:
        with serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT) as ser:
            while True:
                try:
                    line = ser.readline().decode(errors="ignore").strip()

                    # 1) Handle real packets from the receiver (Node 1)
                    if line:
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

                    # 2) Periodically inject Node 2 simulated data
                    now = time.time()
                    if now - last_node2_time >= NODE2_INTERVAL_SECONDS:
                        node2_row = generate_node2_payload()
                        append_to_csv(PAYLOAD_FILE, node2_row)
                        print(f"[SIM NODE 2] {node2_row}")
                        last_node2_time = now

                except Exception as e:
                    print(f"Error inside main loop: {e}")

    except serial.SerialException as e:
        print(f"Serial port error: {e}")
    except KeyboardInterrupt:
        print("\nStopped by user.")

if __name__ == "__main__":
    main()
