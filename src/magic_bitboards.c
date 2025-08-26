// magic_generator.c
// Single-file magic number finder + attack table generator
// Compile: gcc -O3 -march=native magic_generator.c -lpthread -o magic_generator

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <errno.h>

// --------------------------- Config ---------------------------
#ifndef COUNT_BITS
  #define COUNT_BITS(x) __builtin_popcountll(x)
#endif

#ifndef MAX_ATTEMPTS
  #define MAX_ATTEMPTS 10000000U  // Increased from 2M to 10M
#endif

// Sample size for cheap early rejection (reduce full-checks)
#ifndef SAMPLE_SIZE
  #define SAMPLE_SIZE 16  // Reduced from 64 for faster early rejection
#endif

// 0 => autodetect CPU count, otherwise set explicit worker count
#ifndef MAX_WORKERS
  #define MAX_WORKERS 0
#endif

// ------------------------ Data types --------------------------
typedef struct {
    uint64_t mask;
    uint64_t magic;
    uint32_t shift;
    uint32_t offset;
} MagicEntry;

// Global containers for magics (filled by generator)
static MagicEntry rook_magics[64];
static MagicEntry bishop_magics[64];

// Attack tables allocated at runtime
static uint64_t *rook_attacks = NULL;
static uint64_t *bishop_attacks = NULL;

// ------------------------ Directions --------------------------
static const int ROOK_DIRS[4][2] = {
    { 0,  1}, { 0, -1},
    { 1,  0}, {-1,  0}
};
static const int BISHOP_DIRS[4][2] = {
    { 1,  1}, { 1, -1},
    {-1,  1}, {-1, -1}
};

// ------------------------ RNG (xorshift64*) --------------------
static inline uint64_t rng_next(uint64_t *state) {
    uint64_t x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * 2685821657736338717ULL;
}
static inline void rng_seed_from(uint64_t *state, uint64_t seed) {
    *state = seed ? seed : 0x9E3779B97F4A7C15ULL;
}

// Generate sparse magic numbers (crucial for magic bitboards)
static inline uint64_t random_sparse_magic(uint64_t *state) {
    return rng_next(state) & rng_next(state) & rng_next(state);
}

#define RANDOM_MAGIC_FROM_STATE(s) random_sparse_magic(&(s))

// --------------------- Utility functions ----------------------
static uint64_t mask_sliding(int square, const int dirs[4][2]) {
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

static void generate_occupancies(uint64_t mask, uint64_t *occupancies) {
    int bits = COUNT_BITS(mask);
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

static uint64_t generate_attacks(int square, uint64_t blockers, const int dirs[4][2]) {
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

// ------------------- Magic-finding (sampled + threaded) -------------------

typedef struct {
    int square;
    const int (*dirs)[2];
    int endless;
    MagicEntry *dst;
} WorkItem;

typedef struct {
    WorkItem items[64];
    int head;
    int tail;
    pthread_mutex_t lock;
} WorkQueue;

static WorkQueue workq;
static atomic_int squares_done;

// WorkQueue helpers
static void workq_init(void) {
    pthread_mutex_init(&workq.lock, NULL);
    workq.head = workq.tail = 0;
}
static void workq_push(WorkItem it) {
    pthread_mutex_lock(&workq.lock);
    workq.items[workq.tail++] = it;
    pthread_mutex_unlock(&workq.lock);
}
static int workq_pop(WorkItem *out) {
    pthread_mutex_lock(&workq.lock);
    if (workq.head >= workq.tail) { pthread_mutex_unlock(&workq.lock); return 0; }
    *out = workq.items[workq.head++];
    pthread_mutex_unlock(&workq.lock);
    return 1;
}

// Per-square magic finder with randomized sampling early-reject
static uint64_t find_magic_for_square_sampled(int square, int endless, uint64_t mask, const int dirs[4][2], uint64_t rng_state) {
    int bits = COUNT_BITS(mask);
    uint32_t permutations = 1u << bits;
    if (permutations == 0) return 0;

    uint64_t *occupancies = malloc(sizeof(uint64_t) * permutations);
    uint64_t *attacks = malloc(sizeof(uint64_t) * permutations);
    if (!occupancies || !attacks) { free(occupancies); free(attacks); return 0; }

    generate_occupancies(mask, occupancies);
    for (uint32_t i = 0; i < permutations; i++) attacks[i] = generate_attacks(square, occupancies[i], dirs);

    // sample indices
    int sample_n = SAMPLE_SIZE < (int)permutations ? SAMPLE_SIZE : (int)permutations;
    uint32_t *sample_idx = malloc(sizeof(uint32_t) * sample_n);
    if (!sample_idx) { free(occupancies); free(attacks); return 0; }
    for (int i = 0; i < sample_n; i++) sample_idx[i] = (uint32_t)(rng_next(&rng_state) % permutations);

    uint32_t shift = 64 - bits;
    uint64_t *used = malloc(sizeof(uint64_t) * permutations);
    if (!used) { free(occupancies); free(attacks); free(sample_idx); return 0; }

    for (uint32_t attempt = 0; endless || attempt < MAX_ATTEMPTS; attempt++) {
        uint64_t magic = RANDOM_MAGIC_FROM_STATE(rng_state);

        // fast sample check
        for (uint32_t i = 0; i < permutations; i++) used[i] = UINT64_MAX;
        int sample_fail = 0;
        for (int s = 0; s < sample_n; s++) {
            uint32_t i = sample_idx[s];
            uint32_t index = (uint32_t)((occupancies[i] * magic) >> shift);
            uint64_t cur = used[index];
            if (cur == UINT64_MAX) used[index] = attacks[i];
            else if (cur != attacks[i]) { sample_fail = 1; break; }
        }
        if (sample_fail) continue;

        // full verify
        for (uint32_t i = 0; i < permutations; i++) used[i] = UINT64_MAX;
        int fail = 0;
        for (uint32_t i = 0; i < permutations; i++) {
            uint32_t index = (uint32_t)((occupancies[i] * magic) >> shift);
            uint64_t cur = used[index];
            if (cur == UINT64_MAX) used[index] = attacks[i];
            else if (cur != attacks[i]) { fail = 1; break; }
        }
        if (!fail) {
            free(used);
            free(occupancies);
            free(attacks);
            free(sample_idx);
            return magic;
        }
    }

    free(used);
    free(occupancies);
    free(attacks);
    free(sample_idx);
    return 0;
}

// worker thread function
static void *worker_fn(void *arg) {
    (void)arg;
    WorkItem it;
    uint64_t thread_seed = (uint64_t)time(NULL) ^ (uintptr_t)pthread_self();
    rng_seed_from(&thread_seed, thread_seed);
    while (workq_pop(&it)) {
        int sq = it.square;
        uint64_t mask = mask_sliding(sq, it.dirs);
        uint64_t magic = find_magic_for_square_sampled(sq, it.endless, mask, it.dirs, thread_seed);
        it.dst[sq].mask = mask;
        it.dst[sq].shift = 64 - COUNT_BITS(mask);
        if (!magic) {
            fprintf(stderr, "Square %2d: failed to find magic (increase MAX_ATTEMPTS or set endless=1)\n", sq);
            it.dst[sq].magic = 0;
            it.dst[sq].offset = 0; // offsets will be fixed later
        } else {
            it.dst[sq].magic = magic;
            // offset set by coordinator after all squares processed
            printf("Square %2d: found magic 0x%016" PRIx64 "\n", sq, magic);
        }
        atomic_fetch_add(&squares_done, 1);
    }
    return NULL;
}

// parallel compute magics for entire board
static void compute_magics_parallel(MagicEntry *data, int endless, const int dirs[4][2]) {
    workq_init();
    for (int s = 0; s < 64; s++) {
        WorkItem it = { .square = s, .dirs = dirs, .endless = endless, .dst = data };
        workq_push(it);
    }

    int cpu_count = MAX_WORKERS ? MAX_WORKERS : (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count <= 0) cpu_count = 1;
    pthread_t *workers = malloc(sizeof(pthread_t) * cpu_count);
    if (!workers) cpu_count = 1;

    atomic_init(&squares_done, 0);
    for (int i = 0; i < cpu_count; i++) pthread_create(&workers[i], NULL, worker_fn, NULL);
    for (int i = 0; i < cpu_count; i++) pthread_join(workers[i], NULL);
    free(workers);

    // compute offsets after all squares have mask/shift
    uint32_t offset = 0;
    for (int s = 0; s < 64; s++) {
        int bits = 64 - data[s].shift;
        data[s].offset = offset;
        offset += (1u << bits);
    }
}

// Public API to find magics
static void find_magic_numbers(int endless) {
    compute_magics_parallel(rook_magics, endless, ROOK_DIRS);
    compute_magics_parallel(bishop_magics, endless, BISHOP_DIRS);
}

// ------------------- Compute flat attack tables -------------------
static uint32_t calculate_table_size(MagicEntry *data) {
    return data[63].offset + (1u << (64 - data[63].shift));
}
static float calculate_table_kb_size(MagicEntry *data) {
    return calculate_table_size(data) * sizeof(uint64_t) / 1024.0f;
}

static void compute_attacks_table(MagicEntry *data, uint64_t *attacks_out, const int dirs[4][2]) {
    for (int square = 0; square < 64; square++) {
        int bits = 64 - data[square].shift;
        uint32_t base = data[square].offset;
        uint32_t count = 1u << bits;

        uint64_t *occ = malloc(sizeof(uint64_t) * count);
        if (!occ) { fprintf(stderr, "OOM generating occupancies\n"); exit(1); }
        generate_occupancies(data[square].mask, occ);

        for (uint32_t i = 0; i < count; i++) {
            // FIXED: Use magic multiplier to compute the correct index
            uint32_t magic_index = (uint32_t)((occ[i] * data[square].magic) >> data[square].shift);
            attacks_out[base + magic_index] = generate_attacks(square, occ[i], dirs);
        }
        free(occ);
    }
}

// ------------------------ Write outputs ------------------------
static void write_magic_data(const char *fn, MagicEntry *data, const char *array_name) {
    FILE *f = fopen(fn, "w");
    if (!f) { fprintf(stderr, "Failed to open %s: %s\n", fn, strerror(errno)); return; }
    
    // Use different header guards for rook and bishop
    const char *header_guard = strstr(fn, "rook") ? "MAGIC_DATA_GENERATED_ROOK_H" : "MAGIC_DATA_GENERATED_BISHOP_H";
    
    fprintf(f,
        "#ifndef %s\n"
        "#define %s\n\n"
        "#include <stdint.h>\n"
        "#include \"include/magic_bitboards.h\"\n\n"
        "static const MagicEntry %s[64] = {\n", header_guard, header_guard, array_name);

    for (int i = 0; i < 64; i++) {
        fprintf(f, "    { 0x%016" PRIx64 ", 0x%016" PRIx64 ", %u, %u }%s\n",
                data[i].mask, data[i].magic, data[i].shift, data[i].offset, (i == 63) ? "" : ",");
    }
    fprintf(f, "};\n\n#endif // %s\n", header_guard);
    fclose(f);
}

static void write_attack_tables(const char *fn, uint64_t *attacks, MagicEntry *data, const char *arrname) {
    uint32_t size = calculate_table_size(data);
    FILE *f = fopen(fn, "w");
    if (!f) { fprintf(stderr, "Failed to open %s: %s\n", fn, strerror(errno)); return; }
    
    // Use different header guards for rook and bishop attacks
    const char *header_guard = strstr(fn, "rook") ? "MAGIC_ATTACKS_ROOK_GENERATED_H" : "MAGIC_ATTACKS_BISHOP_GENERATED_H";
    
    fprintf(f,
        "#ifndef %s\n"
        "#define %s\n\n"
        "#include <stdint.h>\n\n"
        "static const uint64_t %s[%u] = {\n", header_guard, header_guard, arrname, size);

    for (uint32_t i = 0; i < size; i++) {
        fprintf(f, "    0x%016" PRIx64 "%s\n", attacks[i], (i + 1) == size ? "" : ",");
    }
    fprintf(f, "};\n\n#endif // %s\n", header_guard);
    fclose(f);
}

// ----------------------------- main -----------------------------
int main(void) {
    printf("Starting magic generation (multi-threaded, sampled check)\n");
    find_magic_numbers(0); // endless=0 (use MAX_ATTEMPTS instead of infinite search)

    // allocate flat attack tables
    uint32_t total_rook_entries = calculate_table_size(rook_magics);
    uint32_t total_bishop_entries = calculate_table_size(bishop_magics);

    rook_attacks   = malloc((size_t)total_rook_entries * sizeof(uint64_t));
    bishop_attacks = malloc((size_t)total_bishop_entries * sizeof(uint64_t));
    if (!rook_attacks || !bishop_attacks) {
        fprintf(stderr, "OOM when allocating attack tables\n");
        return 1;
    }

    // compute attacks
    compute_attacks_table(rook_magics, rook_attacks, ROOK_DIRS);
    compute_attacks_table(bishop_magics, bishop_attacks, BISHOP_DIRS);

    printf("Rook attack table: %u entries (%.2f KB)\n", total_rook_entries, calculate_table_kb_size(rook_magics));
    printf("Bishop attack table: %u entries (%.2f KB)\n", total_bishop_entries, calculate_table_kb_size(bishop_magics));

    // write outputs
    write_magic_data("magic_data_generated_rook.h", rook_magics, "precomputed_rook_magics");
    write_magic_data("magic_data_generated_bishop.h", bishop_magics, "precomputed_bishop_magics");
    write_attack_tables("magic_attacks_rook_generated.h", rook_attacks, rook_magics, "precomputed_rook_attacks");
    write_attack_tables("magic_attacks_bishop_generated.h", bishop_attacks, bishop_magics, "precomputed_bishop_attacks");

    free(rook_attacks);
    free(bishop_attacks);
    printf("Done.\n");
    return 0;
}
