import json
import struct
import sys
import os

# --- THE BASE-4 PHYSICS ENGINE ---
TAG_VOID   = 0  # 00
TAG_INGEST = 1  # 01
TAG_MUTATE = 2  # 10
TAG_EMIT   = 3  # 11

def translate_to_dna(instruction):
    cmd = instruction.upper()
    if "FILTER" in cmd:   return [TAG_INGEST, TAG_MUTATE, TAG_EMIT]
    elif "ENCRYPT" in cmd: return [TAG_INGEST, TAG_MUTATE, TAG_MUTATE, TAG_EMIT]
    elif "STORE" in cmd:   return [TAG_INGEST, TAG_VOID, TAG_EMIT]
    else:                  return [TAG_VOID]

def compile_genome(json_file):
    print(f"--- ATOMiK GENOME TRANSLATOR v1.1 (OTP Support) ---")
    print(f" [IN]  Reading Schema: {json_file}")
    
    with open(json_file, 'r') as f:
        schema = json.load(f)

    name = schema['meta']['name']
    security = schema['policy'].get('security_level', 'UNKNOWN')
    
    # NEW: Burn-on-Read Logic
    burn_on_read = schema['policy'].get('burn_on_read', False)
    
    print(f" [NFO] Target: {name} ({security})")
    print(f" [SEC] Burn-After-Reading: {'ENABLED' if burn_on_read else 'DISABLED'}")

    # Synthesize DNA
    raw_dna = []
    for reg, op in schema['dna'].items():
        raw_dna.extend(translate_to_dna(op))
        
    # Pack Binary (4 tags per byte)
    packed_bytes = bytearray()
    current_byte = 0
    shift = 6
    for tag in raw_dna:
        current_byte |= (tag << shift)
        shift -= 2
        if shift < 0:
            packed_bytes.append(current_byte)
            current_byte = 0
            shift = 6
    if shift != 6: packed_bytes.append(current_byte)

    # Save .gnm File
    output_filename = schema['meta']['id'] + ".gnm"
    
    # HEADER v2: 
    # [ATOM (4b)] + [Ver (1b)] + [Freq (4b)] + [Policy (1b)]
    poly_freq = schema['mutation'].get('scramble_freq_ms', 0)
    policy_byte = 1 if burn_on_read else 0
    
    # Struct format: 4s (ATOM) B (Ver) I (Freq) B (Policy)
    header = struct.pack('<4sBIB', b'ATOM', 1, poly_freq, policy_byte)
    
    with open(output_filename, 'wb') as out:
        out.write(header)
        out.write(packed_bytes)

    print(f" [OUT] Synthesized: {output_filename}")
    print(f" [DNA] {len(raw_dna)} tags -> {len(packed_bytes)} bytes payload")
    print("-" * 40)

if __name__ == "__main__":
    if len(sys.argv) < 2: print("Usage: python agc.py <schema.json>")
    else: compile_genome(sys.argv[1])