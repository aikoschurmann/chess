#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>

typedef uint64_t Bitboard;

// Bit manipulation functions
static inline void set_bit(Bitboard *bb, int sq) {
    *bb |= (1ULL << sq);
}

static inline void clear_bit(Bitboard *bb, int sq) {
    *bb &= ~(1ULL << sq);
}

static inline int is_set(Bitboard bb, int sq) {
    return (bb & (1ULL << sq)) != 0;
}

#endif // BITBOARD_H
