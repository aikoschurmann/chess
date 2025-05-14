/*
 * magic_generator.c
 * Magic Bitboard Generator for Rooks
 * Continuously refines magic numbers to minimize storage
 * Shows per-square and cumulative KB progress
 * Can load existing magics to start from
 * Writes results to "magic_data.h" when done
 *
 * Author: Your Name
 * Date: 2025-05-14
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>

#define ATTEMPT_SLICE 1000  // Refinement attempts per iteration
#define ENTRY_SIZE    sizeof(uint64_t)
#define KB(x)         ((x) / 1024.0)

typedef struct {
    uint64_t mask;
    uint64_t magic;
    uint32_t shift;
    int      bits;
    size_t   table_size;   // number of entries needed: max_index + 1
} MagicEntry;

static MagicEntry rook_magics[64];
static size_t    total_squares = 64;

// Fast popcount
static inline int popcount(uint64_t x) {
    return __builtin_popcountll(x);
}

// Generate occupancy subset from mask
static uint64_t set_occupancy(int idx, int bits, uint64_t mask) {
    uint64_t occ = 0ULL;
    uint64_t m = mask;
    for (int i = 0; i < bits; i++) {
        int bit = __builtin_ctzll(m);
        m &= (m - 1);
        if (idx & (1 << i)) occ |= (1ULL << bit);
    }
    return occ;
}

// Build rook mask (excluding edges)
static uint64_t rook_mask(int sq) {
    int r = sq >> 3, f = sq & 7;
    uint64_t mask = 0ULL;
    for (int i = r+1; i <= 6; i++) mask |= (1ULL << (i*8 + f));
    for (int i = r-1; i >= 1;   i--) mask |= (1ULL << (i*8 + f));
    for (int i = f+1; i <= 6;   i++) mask |= (1ULL << (r*8 + i));
    for (int i = f-1; i >= 1;   i--) mask |= (1ULL << (r*8 + i));
    return mask;
}

// Compute rook attacks given blockers
static uint64_t rook_attack(int sq, uint64_t blockers) {
    uint64_t atk = 0ULL;
    int r = sq >> 3, f = sq & 7;
    for (int i = r+1; i < 8; i++) { uint64_t b = 1ULL << (i*8 + f); atk |= b; if (blockers & b) break; }
    for (int i = r-1; i >= 0; i--) { uint64_t b = 1ULL << (i*8 + f); atk |= b; if (blockers & b) break; }
    for (int i = f+1; i < 8; i++) { uint64_t b = 1ULL << (r*8 + i); atk |= b; if (blockers & b) break; }
    for (int i = f-1; i >= 0; i--) { uint64_t b = 1ULL << (r*8 + i); atk |= b; if (blockers & b) break; }
    return atk;
}

// Generate a sparse random 64-bit number
static inline uint64_t random_magic(void) {
    return ((uint64_t)rand() & 0xFFFFULL) |
           (((uint64_t)rand() & 0xFFFFULL) << 16) |
           (((uint64_t)rand() & 0xFFFFULL) << 32) |
           (((uint64_t)rand() & 0xFFFFULL) << 48);
}

// Non-blocking keypress detect
static int kbhit(void) {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) > 0;
}

// Toggle raw mode for stdin
static void set_raw_mode(int enable) {
    static struct termios oldt;
    struct termios newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

// Compute storage usage (KB)
static double compute_total_kb() {
    size_t total_bytes = 0;
    for (int i = 0; i < total_squares; i++) {
        total_bytes += rook_magics[i].table_size * ENTRY_SIZE;
    }
    return KB(total_bytes);
}

// Refine magic to minimize max index (and thus table_size)
static uint64_t refine_magic(int sq, int bits,
                             uint64_t *occ, uint64_t *atk,
                             int count, size_t *out_table_size)
{
    uint64_t best = rook_magics[sq].magic;
    size_t best_size = SIZE_MAX;
    int *used = malloc(count * sizeof(int));

    // initial size
    for (int i = 0; i < count; i++) used[i] = -1;
    for (int i = 0; i < count; i++) {
        int idx = (int)((occ[i] * best) >> (64 - bits));
        if ((size_t)idx + 1 > best_size) best_size = idx + 1;
    }

    set_raw_mode(1);
    printf("Refining square %2d: bits=%d, start size=%.2f KB, total=%.2f KB\n",
           sq, bits,
           KB(best_size * ENTRY_SIZE),
           compute_total_kb());

    while (!kbhit()) {
        for (int a = 0; a < ATTEMPT_SLICE; a++) {
            uint64_t m = random_magic() & random_magic() & random_magic();
            size_t max_idx = 0;
            for (int i = 0; i < count; i++) {
                int idx = (int)((occ[i] * m) >> (64 - bits));
                if ((size_t)idx > max_idx) max_idx = idx;
            }
            if (max_idx + 1 < best_size) {
                best_size = max_idx + 1;
                best = m;
                if (best_size == (size_t)count) goto done;
            }
        }
        printf("  best size=%.2f KB, total=%.2f KB\r",
               KB(best_size * ENTRY_SIZE),
               compute_total_kb());
        fflush(stdout);
    }
done:
    printf("\nAccepted magic=0x%016" PRIx64 " size=%.2f KB\n",
           best, KB(best_size * ENTRY_SIZE));
    set_raw_mode(0);
    *out_table_size = best_size;
    free(used);
    return best;
}

// Load existing magics from header file
static void load_magics(const char *fname) {
    FILE *f = fopen(fname, "r");
    if (!f) return;
    for (int i = 0; i < 64; i++) {
        uint64_t m;
        if (fscanf(f, " {0x%*16[0-9a-f] , 0x%" SCNx64 ", %*u, %*u, %*d},",
                   &m) == 1) {
            rook_magics[i].magic = m;
        }
    }
    fclose(f);
}

// Write header with final data
static void write_header(const char *fname) {
    FILE *f = fopen(fname, "w");
    fprintf(f, "// Generated magic bitboard data\n");
    fprintf(f, "#ifndef MAGIC_DATA_H\n#define MAGIC_DATA_H\n\n");
    fprintf(f, "#include <stdint.h>\n#define ROOK_MAGIC_COUNT 64\n\n");
    fprintf(f, "typedef struct { uint64_t mask, magic; uint32_t shift; int bits; } MagicEntry;\n\n");
    fprintf(f, "static const MagicEntry rook_magics[ROOK_MAGIC_COUNT] = {\n");
    for (int sq = 0; sq < 64; sq++) {
        fprintf(f, "    {0x%016" PRIx64 ", 0x%016" PRIx64 ", %u, %d},\n",
                rook_magics[sq].mask,
                rook_magics[sq].magic,
                rook_magics[sq].shift,
                rook_magics[sq].bits);
    }
    fprintf(f, "};\n\n");
    fclose(f);
}

//int main(int argc, char **argv) {
//    srand((unsigned)time(NULL));
//    // init masks, shifts
//    for (int sq = 0; sq < 64; sq++) {
//        rook_magics[sq].mask = rook_mask(sq);
//        rook_magics[sq].bits = popcount(rook_magics[sq].mask);
//        rook_magics[sq].shift = 64 - rook_magics[sq].bits;
//        rook_magics[sq].magic = random_magic();
//    }
//    // load if provided
//    if (argc > 1) load_magics(argv[1]);
//
//    // refine each square
//    for (int sq = 0; sq < 64; sq++) {
//        int bits  = rook_magics[sq].bits;
//        int count = 1 << bits;
//        uint64_t *occ = malloc(count * sizeof(uint64_t));
//        uint64_t *atk = malloc(count * sizeof(uint64_t));
//        for (int i = 0; i < count; i++) {
//            occ[i] = set_occupancy(i, bits, rook_magics[sq].mask);
//            atk[i] = rook_attack(sq, occ[i]);
//        }
//        size_t size;
//        rook_magics[sq].magic = refine_magic(sq, bits, occ, atk, count, &size);
//        rook_magics[sq].table_size = size;
//        free(occ);
//        free(atk);
//    }
//    write_header("magic_data.h");
//    printf("\nDone. Total storage: %.2f KB\n", compute_total_kb());
//    return 0;
//}