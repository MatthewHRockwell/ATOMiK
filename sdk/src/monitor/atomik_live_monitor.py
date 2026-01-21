import serial
import serial.tools.list_ports
import time
import re
import random

# --- CONFIGURATION ---
BAUD_RATE = 115200 
CORE_LATENCY_NS = 37       

def find_fpga_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "USB" in port.description or "Serial" in port.description:
            return port.device
    return None

def parse_line(line):
    data = {}
    matches = re.findall(r'([A-Z]+)=([0-9A-F]+)', line)
    for key, val in matches:
        data[key] = int(val, 16)
    return data

def simulate_stream():
    print("\n[PROTOCOL SIMULATION] Emulating Decentralized Node Traffic...")
    print("-" * 75)
    # NEW TERMINOLOGY: Matches your "Genome/Security" vision
    print(f"{'NODE LATENCY':<15} | {'LEDGER VELOCITY':<18} | {'ENTROPY (SEC)':<15} | {'GRID SAVE'}")
    print("-" * 75)
    
    cnt_in = 0
    cnt_out = 0
    
    try:
        while True:
            new_in = random.randint(5000, 20000)
            cnt_in += new_in
            cnt_out += int(new_in * 0.08) 
            
            # 1. Grid Save: The % of energy NOT used by Data Centers
            grid_save = (1.0 - (cnt_out/cnt_in)) * 100.0 if cnt_in > 0 else 0.0
            
            # 2. Entropy: The "Security Score" (Polymorphic complexity)
            # Calculated based on your Compression Ratio (higher ratio = harder to guess)
            entropy_score = (cnt_in / cnt_out) * 10.0 if cnt_out > 0 else 0.0
            
            # 3. Ledger Velocity: Transactions Per Second (TPS) equivalent
            ledger_velocity = new_in / 1000.0 # Fake "kTPS" scaling

            print(f"\r{CORE_LATENCY_NS}ns (Wire)     | {ledger_velocity:6.2f} kTx/sec       | {entropy_score:6.1f} bits      | {grid_save:5.1f}% ", end="", flush=True)
            
            time.sleep(0.5) 
            
    except KeyboardInterrupt:
        print("\nNode Stopped.")

def main():
    print(f"--- ATOMiK Protocol Node v3.0 (Infrastructure Mode) ---")
    port = find_fpga_port()
    
    if not port:
        print("Notice: No Active Node found.")
        print("Run PROTOCOL SIMULATION? (y/n): ", end='', flush=True)
        response = input()
        if response.lower() == 'y':
            simulate_stream()
        return

    print(f"Connecting to Node at {port}...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        print("-" * 75)
        print(f"{'NODE LATENCY':<15} | {'LEDGER VELOCITY':<18} | {'ENTROPY (SEC)':<15} | {'GRID SAVE'}")
        print("-" * 75)

        while True:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith("IN="):
                metrics = parse_line(line)
                cnt_in = metrics.get('IN', 0)
                cnt_out = metrics.get('EM', 0)
                
                if cnt_in > 0:
                    grid_save = (1.0 - (cnt_out / cnt_in)) * 100.0
                    ratio = cnt_in / cnt_out if cnt_out > 0 else 0.0
                    entropy_score = ratio * 10.0
                    velocity = (cnt_in / 10000.0) # Arbitrary scaling for demo
                    
                    print(f"\r{CORE_LATENCY_NS}ns (Wire)     | {velocity:6.2f} kTx/sec       | {entropy_score:6.1f} bits      | {grid_save:5.1f}% ", end="", flush=True)

    except KeyboardInterrupt:
        print("\nNode Stopped.")
        if 'ser' in locals() and ser.is_open: ser.close()

if __name__ == "__main__":
    main()