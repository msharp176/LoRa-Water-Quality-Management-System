#!/usr/bin/env python3

import tkinter as tk
from tkinter import messagebox
from tkinter import ttk 
from PIL import Image, ImageTk
import csv
import os
import smtplib
from email.message import EmailMessage
from datetime import datetime
from collections import defaultdict

# ------------ Files and polling ------------
CSV_PATH = "payloads.csv"
IMG_PATH = "lwqms_logo.png"
POLL_MS = 5000
bgcolor = "#82F4FF"

# ------------ Email (SMTP) config ------------
SMTP_SERVER = "smtp.gmail.com"
SMTP_PORT = 587

# Gmail account that SENDS and RECEIVES alerts
SENDER_EMAIL = "lorawqms@gmail.com"
ALERT_EMAIL_TO = "lorawqms@gmail.com"

# Option 1: use environment variable for password
# SENDER_PASSWORD = os.environ.get("LWQMS_EMAIL_PASSWORD")

# Option 2 (less safe): hard-code your app password instead of env var
SENDER_PASSWORD = "kbbl ibkf soao gumk"
# gmail: lorawqms@gmail.com
# gmail password: Loraiscool123

# Sensor ranges (default values)
PH_MIN, PH_MAX = 6.5, 8.5
TEMP_MIN, TEMP_MAX = 0, 35        # degrees C
TURB_MIN, TURB_MAX = 0, 1.0       # NTU

# Node offline timeout (minutes)
NODE_TIMEOUT_MIN = 15.5

# Track which alerts are currently active so we do not spam
# Key is (node_id, "pH"/"temperature"/"turbidity")
active_alerts = {}


# ------------ Timestamp formatter ------------

def format_timestamp(ts_str):
    """Converts ISO Timestamp into nicer to read format"""
    if not ts_str: # empty string
        return "N/A"

    try:
        dt = datetime.fromisoformat(ts_str)
        return dt.strftime("%b %d, %Y %I:%M:%S %p")
        # If 24 hour time is preferred use:
        # return dt.strftime("%Y-%m-%d %H:%M:%S")
    except Exception:
        return ts_str


# ------------ Email helpers ------------

def send_email_alert(subject, body):
    """Send an email using SMTP."""
    if not SENDER_EMAIL or not SENDER_PASSWORD:
        print("Email not sent. Missing SENDER_EMAIL or SENDER_PASSWORD.")
        return

    try:
        msg = EmailMessage()
        msg["From"] = SENDER_EMAIL
        msg["To"] = ALERT_EMAIL_TO
        msg["Subject"] = subject
        msg.set_content(body)

        with smtplib.SMTP(SMTP_SERVER, SMTP_PORT) as server:    # connect to SMTP server
            server.starttls()                                   # secure connection
            server.login(SENDER_EMAIL, SENDER_PASSWORD)         # login to email account
            server.send_message(msg)                            # send the email

        print("Alert email sent:", subject)
    except Exception as e:
        print("Error sending email:", e)

# ------------ Alert checking ------------

def check_alerts_for_node(node_id, timestamp, ph_str, temp_str, turb_str):
    """Check ranges for one node and send alerts if something goes out of bounds."""
    try:
        ph   = float(ph_str)
        temp = float(temp_str)
        turb = float(turb_str)
    except (TypeError, ValueError):
        return

    # Helper to check one parameter(ph/temp/turb) and send alert if needed
    def check_param(name, value, min_val, max_val, units):
        global active_alerts
        key = (node_id, name)                       # unique key for this node and parameter
        in_range = (min_val <= value <= max_val)    # check if value is in range
        was_alert = active_alerts.get(key, False)   # was there an active alert before?

        # send alert if out of range and no active alert
        if not in_range and not was_alert:
            subject = f"LWQMS ALERT: {name} out of range at Node {node_id}"
            pretty_ts = format_timestamp(timestamp)
            body = (
                f"Node {node_id} has {name} out of range.\n\n"
                f"Timestamp: {pretty_ts}\n"
                f"pH: {ph}\n"
                f"Temperature: {temp} °C\n"
                f"Turbidity: {turb} NTU\n\n"
                f"Expected {name} range: {min_val} to {max_val} {units}"
            )
            send_email_alert(subject, body)
            active_alerts[key] = True   # mark alert as active

        # clear alert if back in range and was alerting
        elif in_range and was_alert:
            active_alerts[key] = False

    # Check each parameter
    check_param("pH", ph, PH_MIN, PH_MAX, "")
    check_param("temperature", temp, TEMP_MIN, TEMP_MAX, "°C")
    check_param("turbidity", turb, TURB_MIN, TURB_MAX, "NTU")


def check_node_offline(node_id, timestamp):

    global active_alerts

    key = (node_id, "offline")
    now = datetime.now()

    try:
        last_dt = datetime.fromisoformat(timestamp)
    except Exception:
        return
    
    age_sec = (now - last_dt).total_seconds()
    age_min = age_sec / 60.0

    is_offline = age_min > NODE_TIMEOUT_MIN
    was_alert = active_alerts.get(key, False)

    if is_offline and not was_alert:
        pretty_ts = format_timestamp(timestamp)
        subject = f"LWQMS ALERT: Node {node_id} Offline"
        body = (
            f"Node {node_id} has not sent data for over {NODE_TIMEOUT_MIN} minutes.\n\n"
            f"Last received timestamp: {pretty_ts}\n"
            f"Please check the node's connectivity and power status."
        )
        send_email_alert(subject, body)
        active_alerts[key] = True

    elif not is_offline and was_alert:
        active_alerts[key] = False


# ------------ Settings window ------------

def open_settings_window():
    """Open a window to edit min/max thresholds for pH, Temp, Turbidity."""
    settings = tk.Toplevel(root)
    settings.title("Set Alert Thresholds")
    settings.configure(bg=bgcolor)

    font_settings = ("Segoe UI", 14)

    # Labels
    tk.Label(settings, text="Parameter", font=font_settings, bg=bgcolor).grid(row=0, column=0, padx=10, pady=(10, 5))
    tk.Label(settings, text="Min", font=font_settings, bg=bgcolor).grid(row=0, column=1, padx=10, pady=(10, 5))
    tk.Label(settings, text="Max", font=font_settings, bg=bgcolor).grid(row=0, column=2, padx=10, pady=(10, 5))

    # pH
    tk.Label(settings, text="pH", font=font_settings, bg=bgcolor).grid(row=1, column=0, padx=10, pady=5, sticky="w")
    ph_min_entry = tk.Entry(settings, font=font_settings, width=8)
    ph_max_entry = tk.Entry(settings, font=font_settings, width=8)
    ph_min_entry.grid(row=1, column=1, padx=10, pady=5)
    ph_max_entry.grid(row=1, column=2, padx=10, pady=5)

    # Temperature
    tk.Label(settings, text="Temperature (°C)", font=font_settings, bg=bgcolor).grid(row=2, column=0, padx=10, pady=5, sticky="w")
    temp_min_entry = tk.Entry(settings, font=font_settings, width=8)
    temp_max_entry = tk.Entry(settings, font=font_settings, width=8)
    temp_min_entry.grid(row=2, column=1, padx=10, pady=5)
    temp_max_entry.grid(row=2, column=2, padx=10, pady=5)

    # Turbidity
    tk.Label(settings, text="Turbidity (NTU)", font=font_settings, bg=bgcolor).grid(row=3, column=0, padx=10, pady=5, sticky="w")
    turb_min_entry = tk.Entry(settings, font=font_settings, width=8)
    turb_max_entry = tk.Entry(settings, font=font_settings, width=8)
    turb_min_entry.grid(row=3, column=1, padx=10, pady=5)
    turb_max_entry.grid(row=3, column=2, padx=10, pady=5)

    # Pre-fill current values
    ph_min_entry.insert(0, str(PH_MIN))
    ph_max_entry.insert(0, str(PH_MAX))
    temp_min_entry.insert(0, str(TEMP_MIN))
    temp_max_entry.insert(0, str(TEMP_MAX))
    turb_min_entry.insert(0, str(TURB_MIN))
    turb_max_entry.insert(0, str(TURB_MAX))

    # Helper to save new thresholds
    def save_thresholds():
        global PH_MIN, PH_MAX, TEMP_MIN, TEMP_MAX, TURB_MIN, TURB_MAX

        try:
            new_ph_min = float(ph_min_entry.get())
            new_ph_max = float(ph_max_entry.get())
            new_temp_min = float(temp_min_entry.get())
            new_temp_max = float(temp_max_entry.get())
            new_turb_min = float(turb_min_entry.get())
            new_turb_max = float(turb_max_entry.get())
        except ValueError:
            messagebox.showerror("Invalid Input", "All values must be numeric.")
            return

        # Check that min is not greater than max
        if new_ph_min > new_ph_max or new_temp_min > new_temp_max or new_turb_min > new_turb_max:
            messagebox.showerror("Invalid Range", "Min value cannot be greater than Max value.")
            return

        # Update global thresholds
        PH_MIN, PH_MAX = new_ph_min, new_ph_max
        TEMP_MIN, TEMP_MAX = new_temp_min, new_temp_max
        TURB_MIN, TURB_MAX = new_turb_min, new_turb_max

        messagebox.showinfo("Thresholds Updated", "Alert thresholds have been updated.")
        settings.destroy() # close settings window

    # Save button will save the new thresholds when clicked
    save_button = tk.Button(settings, text="Save", font=font_settings, command=save_thresholds)
    save_button.grid(row=4, column=0, columnspan=3, pady=15)


# ------------ History Window ------------

def open_history_window(node_id):
    """Open a window showing historical readings for the given node ID."""
    if not os.path.exists(CSV_PATH):
        messagebox.showerror("File Not Found", f"{CSV_PATH} not found.")
        return

    # Read and extract history for the node
    history_rows = []
    try:
        with open(CSV_PATH, "r", encoding="utf-8", newline="") as f:
            reader = csv.reader(f)
            rows = list(reader)

            for row in rows[1:]:  
                if len(row) < 5:
                    continue
                timestamp = row[0]
                this_node = str(row[1]).strip()
                turb = row[2]
                temp = row[3]
                ph = row[4]

                if this_node == node_id:
                    history_rows.append((timestamp, ph, temp, turb))
    except Exception as e:
        messagebox.showerror("Error", f"Failed to read history: {e}")
        return

    # Create window
    win = tk.Toplevel(root)
    win.title(f"Node {node_id} History")
    win.configure(bg=bgcolor)
    win.geometry("800x400")

    # Title label
    title = tk.Label(
        win,
        text=f"Node {node_id} – History",
        font=("Segoe UI", 16, "bold"),
        bg=bgcolor
    )
    title.pack(pady=(10, 5))

    # Frame for Treeview + scrollbar
    table_frame = tk.Frame(win, bg=bgcolor)
    table_frame.pack(fill="both", expand=True, padx=10, pady=10)

    columns = ("time", "ph", "temp", "turb")
    tree = ttk.Treeview(table_frame, columns=columns, show="headings", height=15)

    tree.heading("time", text="Timestamp")
    tree.heading("ph", text="pH")
    tree.heading("temp", text="Temperature (°C)")
    tree.heading("turb", text="Turbidity (NTU)")

    tree.column("time", width=260, anchor="center")
    tree.column("ph", width=100, anchor="center")
    tree.column("temp", width=140, anchor="center")
    tree.column("turb", width=140, anchor="center")

    # Vertical scrollbar
    vsb = ttk.Scrollbar(table_frame, orient="vertical", command=tree.yview)
    tree.configure(yscrollcommand=vsb.set)

    tree.grid(row=0, column=0, sticky="nsew")
    vsb.grid(row=0, column=1, sticky="ns")

    table_frame.grid_rowconfigure(0, weight=1)
    table_frame.grid_columnconfigure(0, weight=1)

    # Insert data (newest first)
    for (timestamp, ph, temp, turb) in reversed(history_rows):
        tree.insert("", "end", values=(format_timestamp(timestamp), ph, temp, turb))

    # If no data, show a message
    if not history_rows:
        messagebox.showinfo("No Data", f"No readings found for Node {node_id}.")


# ------------ Tkinter setup ------------
root = tk.Tk()
root.title("LWQMS")
root.geometry("1600x900")
root.configure(bg=bgcolor)

# outer grid to center content vertically
root.grid_rowconfigure(0, weight=1)
root.grid_rowconfigure(1, weight=0)
root.grid_rowconfigure(2, weight=1)
root.grid_columnconfigure(0, weight=1)

# content frame in the middle
content = tk.Frame(root, bg=bgcolor)
content.grid(row=1, column=0, sticky="nsew")

for c in range(5):
    content.grid_columnconfigure(c, weight=1)
    
content.grid_columnconfigure(1, weight=3, minsize=550)
content.grid_columnconfigure(3, weight=2, minsize=350)

# fonts
font_title = ("Segoe UI", 28, "bold")
font_node = ("Segoe UI", 20, "bold")
font_time = ("Segoe UI", 16, "bold")
font_temp = ("Segoe UI", 16, "bold")
font_status = ("Segoe UI", 20, "bold")
font_small = ("Segoe UI", 14)
font_clock = ("Segoe UI", 16)

# load images
img = Image.open(IMG_PATH).resize((250, 250))
photo = ImageTk.PhotoImage(img)

# -------- Top row: images + title --------
top_frame = tk.Frame(content, bg=bgcolor)
top_frame.grid(row=0, column=0, columnspan=5, pady=(20, 10), sticky="ew")

# Side columns expand equally, middle stays centered
top_frame.grid_columnconfigure(0, weight=1)
top_frame.grid_columnconfigure(1, weight=0)   # title
top_frame.grid_columnconfigure(2, weight=1)

left_img_label = tk.Label(top_frame, image=photo, bg=bgcolor)
left_img_label.grid(row=0, column=0, padx=(40, 20))

title_label = tk.Label(
    top_frame,
    text="LoRa Water Quality Management System",
    font=font_title,
    fg="white",
    bg="black",
    padx=10,
    pady=5
)
title_label.grid(row=0, column=1, padx=20)

right_img_label = tk.Label(top_frame, image=photo, bg=bgcolor)
right_img_label.grid(row=0, column=2, padx=(20, 40))

# -------- Node 1 row (row 1) --------
node1_name_label = tk.Label(content, text="Node 1", font=font_node, bg=bgcolor)
node1_name_label.grid(row=1, column=0, pady=(40, 10))

node1_time_label = tk.Label(content, text="Last Updated: N/A", font=font_time, bg=bgcolor, anchor="w")
node1_time_label.grid(row=1, column=1, pady=(40, 10), padx=(10, 10), sticky="w")

node1_ph_label = tk.Label(content, text="pH: N/A", font=font_node, bg=bgcolor)
node1_ph_label.grid(row=1, column=2, pady=(40, 10))

node1_temp_label = tk.Label(content, text="Temperature (°C): N/A", font=font_temp, bg=bgcolor)
node1_temp_label.grid(row=1, column=3, pady=(40, 10))

node1_turb_label = tk.Label(content, text="Turbidity (NTU): N/A", font=font_node, bg=bgcolor)
node1_turb_label.grid(row=1, column=4, pady=(40, 10))

# -------- Spacer between Node 1 and 2 (row 2) --------
spacer = tk.Label(content, text="", bg=bgcolor)
spacer.grid(row=2, column=0, columnspan=5, pady=(10, 10))

# -------- Node 2 row (row 3) --------
node2_name_label = tk.Label(content, text="Node 2", font=font_node, bg=bgcolor)
node2_name_label.grid(row=3, column=0, pady=(10, 40))

node2_time_label = tk.Label(content, text="Last Updated: N/A", font=font_time, bg=bgcolor, anchor="w")
node2_time_label.grid(row=3, column=1, pady=(10, 40), padx=(10, 10), sticky="w")

node2_ph_label = tk.Label(content, text="pH: N/A", font=font_node, bg=bgcolor)
node2_ph_label.grid(row=3, column=2, pady=(10, 40))

node2_temp_label = tk.Label(content, text="Temperature (°C): N/A", font=font_temp, bg=bgcolor)
node2_temp_label.grid(row=3, column=3, pady=(10, 40))

node2_turb_label = tk.Label(content, text="Turbidity (NTU): N/A", font=font_node, bg=bgcolor)
node2_turb_label.grid(row=3, column=4, pady=(10, 40))

# -------- Status Bar (row 4) --------
status_label = tk.Label(
    content,
    text="Status: All readings normal",
    font=font_status,
    bg="green",
    fg="white",
    padx=10,
    pady=5
)
status_label.grid(row=4, column=0, columnspan=5, pady=(10, 10), sticky="ew")

# -------- Bottom: settings button + clock (row 5) --------
button_width=14

settings_button = tk.Button(
    content,
    text="Set Alert Thresholds",
    font=font_small,
    width=20,
    command=open_settings_window
)
settings_button.grid(row=5, column=0, padx=20, pady=(10, 20), sticky="w")

node1_history_button = tk.Button(
    content,
    text="Node 1 History",
    font=font_small,
    width=14,
    command=lambda: open_history_window("1")
)
node1_history_button.grid(row=5, column=1, padx=10, pady=(10, 20))

node2_history_button = tk.Button(
    content,
    text="Node 2 History",
    font=font_small,
    width=14,
    command=lambda: open_history_window("2")
)
node2_history_button.grid(row=5, column=2, padx=10, pady=(10, 20))

clock_label = tk.Label(
    content,
    text="",
    font=font_clock,
    bg="black",
    fg="white",
    padx=10,
    pady=5
)
clock_label.grid(row=5, column=4, padx=20, pady=(10, 20), sticky="e")


# -------- Status Bar Updating --------

def update_status_bar():
    """Update status bar text/color based on active_alerts."""
    alerts_by_node = defaultdict(list) # temporary dict of active alerts for display

    # Group active alerts by node
    for (node_id, param), is_active in active_alerts.items(): # iterate active alerts
        if not is_active:
            continue

        if param == "pH":
            pname = "pH"
        elif param == "temperature":
            pname = "Temperature"
        elif param == "turbidity":
            pname = "Turbidity"
        elif param == "offline":
            pname = "No recent data"
        else:
            pname = param

        alerts_by_node[node_id].append(pname)

    if not alerts_by_node:
        status_label.config(
            text="Status: All readings normal",
            bg="green",
            fg="white",
        )
    else:
        # Build multi-line text 
        lines = ["ALERTS:"]
        for node_id in sorted(alerts_by_node.keys(), key=int):
            params = alerts_by_node[node_id]
            
            offline_present = "No recent data" in params
            range_params = [p for p in params if p != "No recent data"]

            if offline_present and range_params:
                lines.append(
                    f"Node {node_id}: {', '.join(range_params)} out of range; "
                    f"No recent data ({NODE_TIMEOUT_MIN}+ min)"
                )
            elif offline_present:
                lines.append(
                    f"Node {node_id}: No recent data ({NODE_TIMEOUT_MIN}+ min)"
                )
            else:
                lines.append(
                    f"Node {node_id}: {', '.join(params)} out of range"
                )

        status_text = "\n".join(lines)

        status_label.config(
            text=status_text,
            bg="red",
            fg="white",
            justify="center"
        )


# -------- Clock Updating --------

def update_clock():
    now = datetime.now().strftime("%b %d, %Y %I:%M:%S %p")
    clock_label.config(text=f"Current Time: {now}")
    root.after(1000, update_clock) # update every second


# ------------ GUI updating ------------

def update_node_labels(node_id, latest):
    if node_id == "1":
        time_label = node1_time_label
        ph_label = node1_ph_label
        temp_label = node1_temp_label
        turb_label = node1_turb_label
    else:
        time_label = node2_time_label
        ph_label = node2_ph_label
        temp_label = node2_temp_label
        turb_label = node2_turb_label

    if latest is None:
        time_label.config(text="Last Updated: N/A")
        ph_label.config(text="pH: N/A")
        temp_label.config(text="Temperature (°C): N/A")
        turb_label.config(text="Turbidity (NTU): N/A")
    else:
        timestamp, ph, temp, turb = latest
        nice_ts = format_timestamp(timestamp)
        
        # Format to 3 decimal places
        ph_fmt = f"{float(ph):.3f}"
        temp_fmt = f"{float(temp):.3f}"
        turb_fmt = f"{float(turb):.3f}"
        
        time_label.config(text=f"Last Updated: {nice_ts}")
        ph_label.config(text=f"pH: {ph_fmt}")
        temp_label.config(text=f"Temperature (°C): {temp_fmt}")
        turb_label.config(text=f"Turbidity (NTU): {turb_fmt}")


def refresh():
    try:
        if not os.path.exists(CSV_PATH):
            raise FileNotFoundError(f"{CSV_PATH} not found")

        latest_by_node = {"1": None, "2": None}

        with open(CSV_PATH, "r", encoding="utf-8", newline="") as f:
            reader = csv.reader(f)
            rows = list(reader)

            if len(rows) > 1:
                for row in rows[1:]:  # skip header
                    if len(row) < 5:
                        continue
                    # CSV order: time, sensor node ID, turbidity, temp, pH
                    timestamp, node_id, ph, temp, turb = row[0], row[1], row[4], row[3], row[2]
                    node_id = str(node_id).strip()
                    if node_id in latest_by_node:
                        latest_by_node[node_id] = (timestamp, ph, temp, turb)

        # Update labels
        update_node_labels("1", latest_by_node["1"])
        update_node_labels("2", latest_by_node["2"])

        # Check for alerts
        if latest_by_node["1"]:
            ts, ph, temp, turb = latest_by_node["1"]
            check_alerts_for_node("1", ts, ph, temp, turb)
            check_node_offline("1", ts)
        if latest_by_node["2"]:
            ts, ph, temp, turb = latest_by_node["2"]
            check_alerts_for_node("2", ts, ph, temp, turb)
            check_node_offline("2", ts)

        # Update status bar
        update_status_bar()

    except Exception as e:
        err_text = f"Error: {e}"
        node1_time_label.config(text=err_text)
        node1_ph_label.config(text="")
        node1_temp_label.config(text="")
        node1_turb_label.config(text="")
        node2_time_label.config(text=err_text)
        node2_ph_label.config(text="")
        node2_temp_label.config(text="")
        node2_turb_label.config(text="")
        status_label.config(text=err_text, bg="red", fg="white")

    root.after(POLL_MS, refresh)


# Start the periodic updates
update_clock()
refresh()
root.mainloop()
