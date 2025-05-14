#include "magic_bitboards.h"
#include <stdio.h>
#include <inttypes.h>  // Required for PRIx64
#include <string.h>
#include <stdlib.h>
#include "magic_data.h"

//uint64_t *rook_attacks;   // Flat attack table for rooks
//uint64_t *bishop_attacks; // Flat attack table for bishops

static inline uint32_t magic_index(MagicEntry *entry, uint64_t blockers) {
    // 1) Mask off irrelevant bits
    uint64_t occupancy_bits = blockers & entry->mask;
    // 2) Multiply by magic and shift down to get a compact index
    uint32_t idx = (uint32_t)((occupancy_bits * entry->magic) >> entry->shift);
    // 3) Add the entry’s offset into the global table

    // without this, the size of the table would be 64 * 2^n, with n is the highest shift across all squares
    // (e.g. 64 * 2^12 * uint64_t = 64 * 4096 * 8 = 2,097,152 bytes = 2MB)

    // (now we have a table of size 64 * 2^n, where n is individual for each square)
    // with optimised individual n's we can expect 800kB for the whole table
    return idx + entry->offset;
}

void generate_occupancies(uint64_t mask, uint64_t *occupancies) {
    int bits = COUNT_BITS(mask); 
    int permutations = 1 << bits;

    // Store all set bit positions
    int bit_positions[64];
    int count = 0;
    for (int i = 0; i < 64; i++) {
        if (mask & (1ULL << i)) {
            bit_positions[count++] = i;
        }
    }

    for (int i = 0; i < permutations; i++) {
        uint64_t occupancy = 0ULL;
        for (int j = 0; j < bits; j++) {
            if (i & (1 << j)) {
                occupancy |= (1ULL << bit_positions[j]);
            }
        }
        occupancies[i] = occupancy;
    }
}

static uint64_t mask_sliding(int square, const int dirs[4][2]) {
    uint64_t mask = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    for (int d = 0; d < 4; d++) {
        int dr = dirs[d][0];
        int df = dirs[d][1];
        int r  = rank + dr;
        int f  = file + df;

        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            // If sliding horizontally, only stop at file edges:
            if (dr == 0 && (f == 0 || f == 7)) break;
            // If sliding vertically, only stop at rank edges:
            if (df == 0 && (r == 0 || r == 7)) break;
            // If sliding diagonally, stop at either edge:
            if (dr != 0 && df != 0 && (r == 0 || r == 7 || f == 0 || f == 7)) break;

            mask |= 1ULL << (r * 8 + f);
            r += dr;
            f += df;
        }
    }

    return mask;
}

static void generate_rook_masks() {
    for (int square = 0; square < 64; square++) {
        rook_masks[square] = mask_sliding(square, ROOK_DIRS);
    }
}

static void generate_bishop_masks() {
    for (int square = 0; square < 64; square++) {
        bishop_masks[square] = mask_sliding(square, BISHOP_DIRS);
    }
}

static uint64_t generate_attacks(int square, uint64_t blockers,
                                 const int dirs[4][2]) {
    uint64_t attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    for (int d = 0; d < 4; d++) {
        int dr = dirs[d][0];
        int df = dirs[d][1];
        int r  = rank + dr;
        int f  = file + df;

        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            // before adding the square, check if stepping onto an *edge* in this direction
            // for horizontal (dr==0), only file-edge matters; for vertical (df==0), only rank-edge;
            // for diagonal, both edges.
            if ((dr == 0 && (f == 0 || f == 7)) ||
                (df == 0 && (r == 0 || r == 7)) ||
                (dr != 0 && df != 0 && (r == 0 || r == 7 || f == 0 || f == 7)))
            {
                break;
            }

            int idx = r * 8 + f;
            uint64_t bit = 1ULL << idx;
            attacks |= bit;

            // stop on blocker
            if (blockers & bit)
                break;

            r += dr;
            f += df;
        }
    }

    return attacks;
}



uint64_t generate_rook_attacks(int square, uint64_t blockers) {
    uint64_t attacks = 0ULL;

    // Horizontal (rank)
    for (int f = square % 8 + 1; f <= 6; f++) {
        attacks |= 1ULL << (square / 8 * 8 + f);
        if (blockers & (1ULL << (square / 8 * 8 + f))) break;
    }
    for (int f = square % 8 - 1; f >= 1; f--) {
        attacks |= 1ULL << (square / 8 * 8 + f);
        if (blockers & (1ULL << (square / 8 * 8 + f))) break;
    }

    // Vertical (file)
    for (int r = square / 8 + 1; r <= 6; r++) {
        attacks |= 1ULL << (r * 8 + square % 8);
        if (blockers & (1ULL << (r * 8 + square % 8))) break;
    }
    for (int r = square / 8 - 1; r >= 1; r--) {
        attacks |= 1ULL << (r * 8 + square % 8);
        if (blockers & (1ULL << (r * 8 + square % 8))) break;
    }

    return attacks;
    
}

u_int64_t find_magic_number(int square, int endless, uint64_t mask, int dirs[4][2]) {
    printf("Finding magic number for square %d\n", square);

    int bits = COUNT_BITS(mask);
    int permutations = 1 << bits; // 2^bits
    uint64_t *occupancies = malloc(permutations * sizeof(uint64_t));
    uint64_t *attacks = malloc(permutations * sizeof(uint64_t));
    generate_occupancies(mask, occupancies);

    // Precompute all attack maps
    for (int i = 0; i < permutations; i++) {
        attacks[i] = generate_attacks(square, occupancies[i], dirs);
    }

    int attempts = MAX_ATTEMPTS;

    while(endless || attempts > 0) {
        attempts--;
        uint64_t magic = RANDOM_MAGIC();
        if (COUNT_BITS((mask * magic) & 0xFF00000000000000ULL) < 6) continue;

        // Allocate a map to test collisions
        uint64_t *used = calloc(permutations, sizeof(uint64_t));

        int fail = 0;

        for (int i = 0; i < permutations; i++) {
            int index = (int)((occupancies[i] * magic) >> (64 - bits));
            if (used[index] == 0) {
                used[index] = attacks[i];
            } else if (used[index] != attacks[i]) {
                fail = 1;
                break;
            }
        }

        if (!fail) {
            free(used);
            free(occupancies);
            free(attacks);
            return magic;
        }
        free(used);
    }
    free(occupancies);
    free(attacks);
    return 0; // failed to find a magic
}

static void compute_magics(MagicEntry *data, int endless, int dirs[4][2]) {
    uint32_t offset = 0;
    for (int square = 0; square < 64; square++) {
        uint64_t mask = mask_sliding(square, dirs);
        uint64_t magic = find_magic_number(square, endless, mask, dirs);
        data[square].magic = magic;
        data[square].mask = mask;
        int bits = COUNT_BITS(data[square].mask);
        data[square].shift = 64 - bits;
        data[square].offset = offset;
        offset += (1 << bits);
    }
}

void find_magic_numbers(int endless) {
    compute_magics(rook_magics, endless, ROOK_DIRS);
    compute_magics(bishop_magics, endless, BISHOP_DIRS);
}


void compute_attacks(MagicEntry *data, uint64_t *attacks, int dirs[4][2]) {
    for (int square = 0; square < 64; square++) {
        int bits = 64 - data[square].shift;
        uint32_t base = data[square].offset;
        uint32_t count = 1u << bits;

        // generate occupancies once per square:
        uint64_t *occ = malloc(count * sizeof(uint64_t));
        generate_occupancies(data[square].mask, occ);

        for (uint32_t i = 0; i < count; i++) {
            attacks[base + i] = generate_attacks(square, occ[i], dirs);
        }
        free(occ);
    }
}
    

static void write_magic_data(const char *fn) {
    FILE *f = fopen(fn, "w");
    fprintf(f,
        "#ifndef MAGIC_DATA_H\n"
        "#define MAGIC_DATA_H\n"
        "#include <stdint.h>\n\n"
        "#include \"magic_bitboards.h\"\n\n"
        "__attribute__((aligned(64))) MagicEntry precomputed_rook_magics[64] = {\n");
    for (int i = 0; i < 64; i++) {
        fprintf(f, "    { 0x%016" PRIx64 ", 0x%016" PRIx64 ", %u, %u }%s\n",
            rook_masks[i],
            rook_magics[i].magic,
            rook_magics[i].shift,
            rook_magics[i].offset,
            i == 63 ? "" : ",");
    }
    fprintf(f,
        "};\n\n"
        "__attribute__((aligned(64))) MagicEntry precomputed_bishop_magics[64] = {\n");
    for (int i = 0; i < 64; i++) {
        fprintf(f, "    { 0x%016" PRIx64 ", 0x%016" PRIx64 ", %u, %u }%s\n",
            bishop_masks[i],
            bishop_magics[i].magic,
            bishop_magics[i].shift,
            bishop_magics[i].offset,
            i == 63 ? "" : ",");
    }
    fprintf(f,
        "};\n\n"
        "#endif // MAGIC_DATA_H\n");
    fclose(f);
}

static void write_attack_tables(const char *fn) {
    // We assume rook_attacks and bishop_attacks are already filled:
    size_t rost = rook_magics[63].offset + (1 << (64 - rook_magics[63].shift));
    size_t bist = bishop_magics[63].offset + (1 << (64 - bishop_magics[63].shift));
    FILE *f = fopen(fn, "w");
    fprintf(f,
        "#ifndef MAGIC_ATTACKS_H\n"
        "#define MAGIC_ATTACKS_H\n"
        "#include <stdint.h>\n\n"
        "__attribute__((aligned(64))) static const uint64_t rook_attacks[%zu] = {\n", rost);
    for (size_t i = 0; i < rost; i++) {
        fprintf(f, "    0x%016" PRIx64 "%s\n",
            rook_attacks[i],
            i + 1 == rost ? "" : ",");
    }
    fprintf(f,
        "};\n\n"
        "__attribute__((aligned(64))) static const uint64_t bishop_attacks[%zu] = {\n", bist);
    for (size_t i = 0; i < bist; i++) {
        fprintf(f, "    0x%016" PRIx64 "%s\n",
            bishop_attacks[i],
            i + 1 == bist ? "" : ",");
    }
    fprintf(f,
        "};\n\n"
        "#endif // MAGIC_ATTACKS_H\n");
    fclose(f);
}

inline int calculate_table_size(MagicEntry *data) {
    return data[63].offset + (1 << (64 - data[63].shift));
}

inline float calculate_table_kb_size(MagicEntry *data) {
    return calculate_table_size(data) * sizeof(uint64_t) / 1024.0f;
}



int main(void) {
    // 1) Masks
    
    // precomputed 
    //generate_rook_masks();
    //generate_bishop_masks();
    // print


    // 2) Magics, shifts & offsets
    // precomputed
    //find_magic_numbers(1);


    // 3) Total flat table sizes
    uint32_t total_rook_entries   = calculate_table_size(rook_magics);
    uint32_t total_bishop_entries = calculate_table_size(bishop_magics);

    // 4) Allocate flat attack arrays
    rook_attacks   = malloc(total_rook_entries   * sizeof(uint64_t));
    bishop_attacks = malloc(total_bishop_entries * sizeof(uint64_t));
    
    compute_attacks(rook_magics, rook_attacks, ROOK_DIRS);
    compute_attacks(bishop_magics, bishop_attacks, BISHOP_DIRS);

    printf("Rook attack table bytes (%.2f KB)\n", calculate_table_kb_size(rook_magics));
    printf("Bishop attack table bytes (%.2f KB)\n", calculate_table_kb_size(bishop_magics));

    // 7) Write out headers
    // precomputed
    // write_magic_data("include/magic_data.h");
    // write_attack_tables("include/magic_attacks.h");

    // Cleanup
    free(rook_attacks);
    free(bishop_attacks);
    return 0;
}
