#!/usr/bin/env python3
"""test_magic.py (fixed multiplication mask)

Check whether a 64-bit magic number is valid for a given square (0-based, a1=0).
Usage: python3 test_magic.py [magic_hex] [square]
If no args provided, defaults to magic 0xe106003c7e02ffed and square 9.
"""

import sys

ROOK_DIRS = [(0,1),(0,-1),(1,0),(-1,0)]
BISHOP_DIRS = [(1,1),(1,-1),(-1,1),(-1,-1)]

def mask_sliding(square, dirs):
    mask = 0
    rank = square // 8
    file = square % 8
    for dr, df in dirs:
        r = rank + dr
        f = file + df
        while 0 <= r < 8 and 0 <= f < 8:
            # stop before the edge square (standard magic mask behavior)
            if dr == 0 and (f == 0 or f == 7):
                break
            if df == 0 and (r == 0 or r == 7):
                break
            if dr != 0 and df != 0 and (r == 0 or r == 7 or f == 0 or f == 7):
                break
            mask |= (1 << (r * 8 + f))
            r += dr
            f += df
    return mask

def generate_occupancies(mask):
    positions = [i for i in range(64) if (mask >> i) & 1]
    n = len(positions)
    occs = []
    for i in range(1 << n):
        occ = 0
        for j in range(n):
            if (i >> j) & 1:
                occ |= (1 << positions[j])
        occs.append(occ)
    return occs

def generate_attacks(square, blockers, dirs):
    attacks = 0
    rank = square // 8
    file = square % 8
    for dr, df in dirs:
        r = rank + dr
        f = file + df
        while 0 <= r < 8 and 0 <= f < 8:
            idx = r * 8 + f
            bit = 1 << idx
            attacks |= bit
            if blockers & bit:
                break
            r += dr
            f += df
    return attacks

def test_magic(square, dirs, magic):
    mask = mask_sliding(square, dirs)
    bits = mask.bit_count()
    shift = 64 - bits
    occs = generate_occupancies(mask)
    attacks = [generate_attacks(square, o, dirs) for o in occs]
    size = 1 << bits
    used = [None] * size
    for i, occ in enumerate(occs):
        prod = (occ * magic) & ((1 << 64) - 1)   # emulate uint64 overflow
        idx = prod >> shift
        if idx >= size:
            return False, f"Index out of range: {idx} >= {size} (bits={bits})"
        if used[idx] is None:
            used[idx] = attacks[i]
        elif used[idx] != attacks[i]:
            return False, {
                "collision_index": idx,
                "occupancy_index": i,
                "occupancy": occ,
                "expected_attacks": attacks[i],
                "previous_attacks": used[idx],
                "bits": bits,
                "mask": mask,
                "shift": shift
            }
    return True, {
        "bits": bits,
        "mask": mask,
        "shift": shift,
        "table_size": size
    }

def pretty_hex(x):
    return f"0x{x:016x}"

def main(argv):
    default_magic = 0x0200020020910814
    magic = default_magic
    square = 9
    if len(argv) >= 2:
        try:
            magic = int(argv[1], 0)
        except Exception as e:
            print("Invalid magic argument; please pass hex like 0x..., or leave blank.", e)
            return 1
    if len(argv) >= 3:
        square = int(argv[2], 0)
    print(f"Testing magic {pretty_hex(magic)} for square {square} (0=a1).\n")
    for name, dirs in (('Rook', ROOK_DIRS), ('Bishop', BISHOP_DIRS)):
        ok, info = test_magic(square, dirs, magic)
        if ok:
            print(f"{name}: VALID -- bits={info['bits']}, mask={pretty_hex(info['mask'])}, shift={info['shift']}, table_entries={info['table_size']}")
        else:
            print(f"{name}: INVALID -- details:\n{info}")
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
