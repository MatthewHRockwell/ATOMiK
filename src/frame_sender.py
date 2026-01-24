import serial
import time

COM_PORT = "COM6"      # change as needed
BAUD     = 115200

# Frame format based on your loader FSM:
# "ATOM" + version(1) + poly_freq(4) + policy(1) + dna(32)
version = bytes([0x01])

poly_freq = bytes([0x00, 0x00, 0x00, 0x00])   # 0 for now
policy    = bytes([0x01])                     # otp_en = 1 (LSB)

dna = bytes(range(32))                        # deterministic 0..31

frame = b"ATOM" + version + poly_freq + policy + dna

with serial.Serial(COM_PORT, BAUD, timeout=1) as s:
    time.sleep(0.2)
    s.write(frame)
    s.flush()
print(frame)
