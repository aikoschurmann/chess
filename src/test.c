/*
 * Optimized magic bitboard generator (Single-threaded)
 * - Precompute masks once
 * - Reuse occupancy/attack buffers
 * - Use __builtin_popcountll
 */
//#include "magic_bitboards.h"
//#include <stdio.h>
//#include <inttypes.h>
//#include <stdlib.h>
//#include <stdint.h>
//#include <string.h>
//#include "bishop_masks.h"
//#include "rook_masks.h"
//
//#define MAX_ATTEMPTS 1000000
//
//int ROOK_DIRS[4][2] = {{0,1},{0,-1},{1,0},{-1,0}};
//int BISHOP_DIRS[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
//
//// Precomputed masks
//uint64_t rook_masks[64];
//uint64_t bishop_masks[64];
//
//inline uint32_t magic_index2(const MagicEntry* e, uint64_t blockers) {
//    uint64_t occ = blockers & e->mask;
//    return (uint32_t)((occ * e->magic) >> e->shift) + e->offset;
//}
//
//uint64_t mask_sliding(int square, const int dirs[4][2]) {
//    uint64_t mask = 0ULL;
//    int rank = square / 8;
//    int file = square % 8;
//
//    for (int d = 0; d < 4; d++) {
//        int dr = dirs[d][0];
//        int df = dirs[d][1];
//        int r  = rank + dr;
//        int f  = file + df;
//
//        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
//            // If sliding horizontally, only stop at file edges:
//            if (dr == 0 && (f == 0 || f == 7)) break;
//            // If sliding vertically, only stop at rank edges:
//            if (df == 0 && (r == 0 || r == 7)) break;
//            // If sliding diagonally, stop at either edge:
//            if (dr != 0 && df != 0 && (r == 0 || r == 7 || f == 0 || f == 7)) break;
//
//            mask |= 1ULL << (r * 8 + f);
//            r += dr;
//            f += df;
//        }
//    }
//
//    return mask;
//}
//
//// Generate mask for sliding piece once
//static void init_masks() {
//    for(int sq=0; sq<64; ++sq) {
//        rook_masks[sq]   = mask_sliding(sq, ROOK_DIRS);
//        bishop_masks[sq] = mask_sliding(sq, BISHOP_DIRS);
//    }
//}
//
//// Generate all occupancies for a mask
//void generate_occupancies2(uint64_t mask, uint64_t* occs, int bits) {
//    int perms = 1<<bits;
//    for(int i=0; i<perms; ++i) {
//        uint64_t occ = 0ULL;
//        uint64_t m = mask;
//        int count = bits;
//        int idx = 0;
//        uint64_t temp = i;
//        // iterate bits of mask
//        while (m) {
//            int b = __builtin_ctzll(m);
//            if (temp & 1) occ |= (1ULL<<b);
//            temp >>= 1;
//            m &= m - 1;
//        }
//        occs[i] = occ;
//    }
//}
//
//// Generate ray attacks
//static uint64_t sliding_attacks(int sq, uint64_t blockers, const int dirs[4][2]) {
//    uint64_t at=0ULL;
//    int r0= sq/8, f0=sq%8;
//    for(int d=0; d<4; ++d) {
//        int r=r0+dirs[d][0], f=f0+dirs[d][1];
//        while(r>=0 && r<8 && f>=0 && f<8) {
//            int idx = r*8 + f;
//            uint64_t bit = 1ULL<<idx;
//            at |= bit;
//            if (blockers & bit) break;
//            r += dirs[d][0]; f += dirs[d][1];
//        }
//    }
//    return at;
//}
//
//// Find magic for one square
//static uint64_t find_magic(int sq, const int dirs[4][2], uint64_t mask) {
//    int bits = __builtin_popcountll(mask);
//    int perms = 1<<bits;
//    uint64_t *occs = malloc(perms*sizeof(uint64_t));
//    uint64_t *atts = malloc(perms*sizeof(uint64_t));
//    generate_occupancies2(mask, occs, bits);
//    for(int i=0; i<perms; ++i)
//        atts[i] = sliding_attacks(sq, occs[i], dirs);
//
//    uint64_t *used = calloc(perms, sizeof(uint64_t));
//    uint64_t magic = 0ULL;
//    for(int attempt=0; attempt<MAX_ATTEMPTS; ++attempt) {
//        int fail = 0;
//        magic = RANDOM_MAGIC();
//        memset(used, 0, perms*sizeof(uint64_t));
//        for(int i=0; i<perms; ++i) {
//            uint32_t idx = (uint32_t)((occs[i] * magic) >> (64 - bits));
//            if (!used[idx]) used[idx] = atts[i];
//            else if (used[idx] != atts[i]) { fail = 1; break; }
//        }
//        if (!fail) break;
//    }
//
//    free(used);
//    free(occs);
//    free(atts);
//    printf("Found magic number for square %d: 0x%016" PRIx64 "\n", sq, magic);
//    return magic;
//    
//}
//
//// Compute magics sequentially
//static void compute_magics(MagicEntry* data, const int dirs[4][2], uint64_t* masks) {
//    uint32_t offset = 0;
//    for(int sq=0; sq<64; ++sq) {
//        uint64_t mask = masks[sq];
//        uint64_t magic = find_magic(sq, dirs, mask);
//        int bits = __builtin_popcountll(mask);
//        data[sq].mask   = mask;
//        data[sq].magic  = magic;
//        data[sq].shift  = 64 - bits;
//        data[sq].offset = offset;
//        offset += (1u << bits);
//    }
//}

//int main(void) {
//    printf("Starting magic generation (single-thread)...\n");
//    init_masks();
//    compute_magics(rook_magics, ROOK_DIRS, rook_masks);
//    compute_magics(bishop_magics, BISHOP_DIRS, bishop_masks);
//    printf("Magic search complete.\n");
//    return 0;
//}