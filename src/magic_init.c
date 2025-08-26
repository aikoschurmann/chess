#include "magic_bitboards.h"
#include <stdlib.h>
#include <string.h>

// Include the generated data (after magic_bitboards.h to avoid typedef conflicts)
#include "../magic_data_generated_rook.h"
#include "../magic_data_generated_bishop.h"  
#include "../magic_attacks_rook_generated.h"
#include "../magic_attacks_bishop_generated.h"

// Direction vectors
const int ROOK_DIRS[4][2] = {
    { 0,  1}, { 0, -1},
    { 1,  0}, {-1,  0}
};
const int BISHOP_DIRS[4][2] = {
    { 1,  1}, { 1, -1},
    {-1,  1}, {-1, -1}
};

// Initialize magic bitboard data
void initialize_magic_bitboards() {
    // Copy the precomputed magic entries
    memcpy(rook_magics, precomputed_rook_magics, sizeof(precomputed_rook_magics));
    memcpy(bishop_magics, precomputed_bishop_magics, sizeof(precomputed_bishop_magics));
    
    // Allocate and copy attack tables
    rook_attacks = malloc(sizeof(precomputed_rook_attacks));
    bishop_attacks = malloc(sizeof(precomputed_bishop_attacks));
    
    if (rook_attacks && bishop_attacks) {
        memcpy(rook_attacks, precomputed_rook_attacks, sizeof(precomputed_rook_attacks));
        memcpy(bishop_attacks, precomputed_bishop_attacks, sizeof(precomputed_bishop_attacks));
    }
}

// Get rook attacks using magic bitboards
uint64_t get_rook_attack(int square, uint64_t blockers) {
    MagicEntry *entry = &rook_magics[square];
    uint64_t masked_blockers = blockers & entry->mask;
    uint32_t index = ((masked_blockers * entry->magic) >> entry->shift) + entry->offset;
    return rook_attacks[index];
}

// Get bishop attacks using magic bitboards
uint64_t get_bishop_attack(int square, uint64_t blockers) {
    MagicEntry *entry = &bishop_magics[square];
    uint64_t masked_blockers = blockers & entry->mask;
    uint32_t index = ((masked_blockers * entry->magic) >> entry->shift) + entry->offset;
    return bishop_attacks[index];
}

// Get queen attacks (combination of rook and bishop)
uint64_t get_queen_attack(int square, uint64_t blockers) {
    return get_rook_attack(square, blockers) | get_bishop_attack(square, blockers);
}

// Cleanup function
void cleanup_magic_bitboards() {
    free(rook_attacks);
    free(bishop_attacks);
    rook_attacks = NULL;
    bishop_attacks = NULL;
}

// Helper function for magic index calculation
uint32_t magic_index(MagicEntry *entry, uint64_t blockers) {
    uint64_t masked_blockers = blockers & entry->mask;
    return (masked_blockers * entry->magic) >> entry->shift;
}

// Helper functions for debugging/testing (from the magic generator)
uint64_t mask_sliding(int square, const int dirs[4][2]) {
    uint64_t mask = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    for (int d = 0; d < 4; d++) {
        int dr = dirs[d][0];
        int df = dirs[d][1];
        int r = rank + dr;
        int f = file + df;
        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            // stop before edge (standard magic mask)
            if (dr == 0 && (f == 0 || f == 7)) break;
            if (df == 0 && (r == 0 || r == 7)) break;
            if (dr != 0 && df != 0 && (r == 0 || r == 7 || f == 0 || f == 7)) break;

            mask |= (1ULL << (r * 8 + f));
            r += dr;
            f += df;
        }
    }
    return mask;
}

void generate_occupancies(uint64_t mask, uint64_t *occupancies) {
    int bits = __builtin_popcountll(mask);
    uint32_t permutations = 1u << bits;
    int bit_positions[64];
    int count = 0;
    for (int i = 0; i < 64; i++) {
        if (mask & (1ULL << i)) bit_positions[count++] = i;
    }
    for (uint32_t i = 0; i < permutations; i++) {
        uint64_t occ = 0ULL;
        for (int j = 0; j < bits; j++) {
            if (i & (1u << j)) occ |= (1ULL << bit_positions[j]);
        }
        occupancies[i] = occ;
    }
}

uint64_t generate_attacks(int square, uint64_t blockers, const int dirs[4][2]) {
    uint64_t attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    for (int d = 0; d < 4; d++) {
        int dr = dirs[d][0];
        int df = dirs[d][1];
        int r = rank + dr;
        int f = file + df;
        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            int idx = r * 8 + f;
            uint64_t bit = 1ULL << idx;
            attacks |= bit;
            if (blockers & bit) break;
            r += dr; f += df;
        }
    }
    return attacks;
}
