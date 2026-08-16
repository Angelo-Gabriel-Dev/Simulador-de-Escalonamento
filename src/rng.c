#include <math.h>
#include "rng.h"

/* splitmix64: usado só para "esparramar" a seed inicial do usuário nos
 * dois words de 64 bits do estado do xorshift128+. */
static uint64_t splitmix64_next(uint64_t *x) {
    uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

void rng_seed(RngState *st, uint64_t seed) {
    uint64_t sm = seed;
    st->s[0] = splitmix64_next(&sm);
    st->s[1] = splitmix64_next(&sm);
    /* xorshift128+ exige estado != {0,0}; com splitmix64 isso é
     * astronomicamente improvável, mas garantimos mesmo assim. */
    if (st->s[0] == 0 && st->s[1] == 0) {
        st->s[0] = 0x9E3779B97F4A7C15ULL;
    }
}

static uint64_t xorshift128plus_next(RngState *st) {
    uint64_t x = st->s[0];
    const uint64_t y = st->s[1];
    st->s[0] = y;
    x ^= x << 23;
    x ^= x >> 17;
    x ^= y ^ (y >> 26);
    st->s[1] = x;
    return x + y;
}

uint32_t rng_next_u32(RngState *st) {
    return (uint32_t) (xorshift128plus_next(st) >> 32);
}

double rng_next_double(RngState *st) {
    /* 53 bits de mantissa -> double uniforme em [0,1) sem viés de arredondamento. */
    uint64_t v = xorshift128plus_next(st) >> 11; /* 53 bits úteis */
    return (double) v / (double) (1ULL << 53);
}

int rng_range_int(RngState *st, int min, int max) {
    if (max <= min) return min;
    int span = max - min + 1;
    return min + (int) (rng_next_double(st) * (double) span);
}

double rng_exponential(RngState *st, double mean) {
    double u = rng_next_double(st);
    /* evita log(0) no caso (raríssimo) de u == 0.0 */
    if (u <= 0.0) u = 1e-12;
    return -mean * log(u);
}
