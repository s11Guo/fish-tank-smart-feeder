#!/usr/bin/env python3
"""
Fish Tank App Simulator
- Reads STM32 serial output (sensor data, debug)
- Connects to ESP8266 TCP server (192.168.4.1:9000)
- Sends commands, receives JSON, measures latency
"""

import socket
import json
import time
import threading
import sys
import argparse

# ── config ──
ESP_HOST = "192.168.4.1"
ESP_PORT = 9000
SERIAL_PORT = None  # set via CLI
SERIAL_BAUD = 115200

# ── TCP client ──
class TCPClient:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.sock = None
        self.buf = b""
        self.lock = threading.Lock()

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5)
        self.sock.connect((self.host, self.port))
        self.sock.settimeout(0.1)
        print(f"[TCP] Connected to {self.host}:{self.port}")

    def send(self, cmd):
        """Send a single-char command: u=up, d=down, e=enter, b=back, r=refresh"""
        if self.sock:
            self.sock.sendall(cmd.encode())
            print(f"[TCP] Sent: {cmd}")

    def recv_json(self):
        """Non-blocking: return latest JSON dict or None"""
        try:
            data = self.sock.recv(4096)
            if data:
                self.buf += data
        except socket.timeout:
            pass
        except Exception as e:
            print(f"[TCP] recv error: {e}")

        # Parse complete JSON objects
        result = None
        while b"\r\n" in self.buf:
            line, self.buf = self.buf.split(b"\r\n", 1)
            try:
                obj = json.loads(line.decode())
                result = obj  # keep latest
            except:
                pass
        return result

    def close(self):
        if self.sock:
            self.sock.close()
            self.sock = None


# ── Serial reader (optional, in separate thread) ──
def serial_reader(port, baud):
    try:
        import serial
        ser = serial.Serial(port, baud, timeout=0.1)
        print(f"[SER] Opened {port} at {baud}")
        while True:
            line = ser.readline()
            if line:
                print(f"[SER] {line.decode(errors='replace').rstrip()}")
    except ImportError:
        print("[SER] pyserial not installed, skipping serial monitor")
    except Exception as e:
        print(f"[SER] Error: {e}")


# ── main test loop ──
def main():
    parser = argparse.ArgumentParser(description="Fish Tank App Simulator")
    parser.add_argument("--serial", help="Serial port (e.g., COM5)")
    parser.add_argument("--host", default=ESP_HOST, help="ESP8266 TCP host")
    parser.add_argument("--port", type=int, default=ESP_PORT, help="ESP8266 TCP port")
    parser.add_argument("--loops", type=int, default=10, help="Number of test cycles")
    args = parser.parse_args()

    # Start serial monitor if port provided
    if args.serial:
        t = threading.Thread(target=serial_reader, args=(args.serial, SERIAL_BAUD), daemon=True)
        t.start()
        time.sleep(0.5)

    # Connect TCP
    tcp = TCPClient(args.host, args.port)
    try:
        tcp.connect()
    except Exception as e:
        print(f"[ERR] Cannot connect to ESP8266: {e}")
        print("Make sure you are on the ESP8266 WiFi network (FishTank)")
        if args.serial:
            print("Serial monitor is running. Press Ctrl+C to exit.")
            while True:
                time.sleep(1)
        return

    time.sleep(1)

    # Dump initial state
    obj = tcp.recv_json()
    if obj:
        print(f"[INIT] Page={obj.get('page','?')} cursor={obj.get('cursor','?')}")

    # ── Test sequence ──
    commands = [
        # Navigate from wherever we are back to Menu
        ('b', 'Back to menu'),
        ('b', 'Back again'),
        ('b', 'Back once more (ensure at top)'),
        # Enter Feed submenu
        ('e', 'Enter (Welcome→Menu)'),
        ('d', 'Down to Feed'),
        ('e', 'Enter Feed submenu'),
        # Navigate sub-items
        ('d', 'Down to Manual Feed'),
        ('d', 'Down to Auto Mode'),
        ('u', 'Up to Manual Feed'),
        ('u', 'Up to Train Mode'),
        # Enter Train Mode
        ('e', 'Enter Train Mode'),
        ('d', 'Down to START'),
        ('e', 'Enter train setting'),
        # Back
        ('b', 'Back to Train menu'),
        ('b', 'Back to Feed submenu'),
        ('b', 'Back to Menu'),
        # Go to Manual Feed
        ('e', 'Enter Feed submenu'),
        ('d', 'Down to Manual Feed'),
        ('e', 'Enter Manual Feed'),
        # Edit feed time
        ('e', 'Edit feed time'),
        ('u', 'Increase time'),
        ('u', 'Increase time'),
        ('d', 'Decrease time'),
        ('e', 'Confirm time'),
        # Back
        ('b', 'Back'),
        ('b', 'Back to Menu'),
        # Light page
        ('d', 'Down'),
        ('d', 'Down to Light'),
        ('e', 'Enter Light'),
        ('d', 'Down'),
        ('e', 'Toggle mode'),
        ('b', 'Back'),
        # Temp page
        ('d', 'Down'),
        ('d', 'Down'),
        ('d', 'Down to Temp'),
        ('e', 'Enter Temp'),
        ('e', 'Toggle heater'),
        ('b', 'Back'),
        # Pump page
        ('d', 'Down'),
        ('d', 'Down'),
        ('d', 'Down'),
        ('d', 'Down to Pump'),
        ('e', 'Enter Pump'),
        ('b', 'Back'),
    ]

    print(f"\n{'='*60}")
    print(f"Running {len(commands)} command tests ({args.loops} cycles each)")
    print(f"{'='*60}\n")

    for cycle in range(args.loops):
        print(f"\n--- Cycle {cycle+1}/{args.loops} ---")
        for cmd, desc in commands:
            # Send command
            t0 = time.time()
            tcp.send(cmd)

            # Wait for JSON response, measure latency
            obj = None
            timeout = time.time() + 3.0
            while time.time() < timeout:
                obj = tcp.recv_json()
                if obj:
                    break
                time.sleep(0.01)

            latency = (time.time() - t0) * 1000
            status = "OK" if obj else "TIMEOUT"
            page = obj.get('page', '?') if obj else '?'
            cursor = obj.get('cursor', -1) if obj else -1
            print(f"  [{status}] cmd={cmd:3s} → page={page:12s} cursor={cursor:3d} "
                  f"latency={latency:.0f}ms  ({desc})")

            time.sleep(0.3)  # gap between commands

    tcp.close()
    print(f"\n{'='*60}")
    print("Test complete.")
    print(f"{'='*60}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrupted.")
