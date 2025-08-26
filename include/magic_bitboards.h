#ifndef __MAGIC_BITBOARDS_H__
#define __MAGIC_BITBOARDS_H__

#include <stdint.h>
//#include "rook_masks.h"
//#include "bishop_masks.h"



// example mask for d4

//  8 | . . . . . . . .
//  7 | . . . x . . . .
//  6 | . . . x . . . .
//  5 | . . . x . . . .
//  4 | . x x . x x x .
//  3 | . . . x . . . .
//  2 | . . . x . . . .
//  1 | . . . . . . . .
//      a b c d e f g h


typedef struct MagicEntry {
    uint64_t mask;     // possible squares without blockers excluding the square itself and the edge squares
    uint64_t magic;    // magic multiplier
    uint32_t shift;    // bits to shift
    uint32_t offset;   // where in the attack table the square's data starts
} MagicEntry;

 // Direction vectors

extern const int ROOK_DIRS[4][2];
extern const int BISHOP_DIRS[4][2];

uint64_t new_rook_masks[64];   // precomputed masks for rooks
uint64_t new_bishop_masks[64]; // precomputed masks for bishops

// __attribute__((aligned(64))) is used to ensure that the data is aligned to 64 bytes for performance reasons
__attribute__((aligned(64))) MagicEntry rook_magics[64];      // magic numbers for rooks
__attribute__((aligned(64))) MagicEntry bishop_magics[64];    // magic numbers for bishops


uint64_t *rook_attacks;   // Flat attack table for rooks
uint64_t *bishop_attacks; // Flat attack table for bishops

uint64_t get_rook_attack(int square, uint64_t blockers);
uint64_t get_bishop_attack(int square, uint64_t blockers);
uint64_t get_queen_attack(int square, uint64_t blockers);

// Initialize and cleanup functions
void initialize_magic_bitboards();
void cleanup_magic_bitboards();

// Utility macros
#define LSB(x) __builtin_ctzll(x) // Least Significant Bit
#define POP_LSB(x) (x &= x - 1) // Pop the Least Significant Bit
#define COUNT_BITS(x) __builtin_popcountll(x) // Count the number of bits set to 1

#endif // __MAGIC_BITBOARDS_H__