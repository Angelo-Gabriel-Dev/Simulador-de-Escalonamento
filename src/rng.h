#ifndef RNG_H
#define RNG_H

#include <stdint.h>

/*
 * PRNG determinístico próprio (xorshift128+), independente da libc.
 * Não usamos rand()/srand() porque a implementação da libc varia entre
 * plataformas/versões (ver Seção 2 do prompt mestre) — isso quebraria a
 * garantia de "mesma seed => mesma carga de trabalho" entre máquinas
 * diferentes da equipe.
 */
typedef struct {
    uint64_t s[2];
} RngState;

/* Inicializa o estado a partir de uma seed de 64 bits usando splitmix64
 * (evita o estado-zero problemático do xorshift puro e espalha bem
 * seeds sequenciais como 1, 2, 3, ...). */
void rng_seed(RngState *st, uint64_t seed);

/* Próximo inteiro pseudo-aleatório de 32 bits. */
uint32_t rng_next_u32(RngState *st);

/* Próximo double pseudo-aleatório em [0, 1). */
double rng_next_double(RngState *st);

/* Inteiro uniforme em [min, max] (inclusive nos dois extremos). */
int rng_range_int(RngState *st, int min, int max);

/* Amostra de uma distribuição exponencial com média `mean` (usada para
 * tempos entre chegadas). mean deve ser > 0. */
double rng_exponential(RngState *st, double mean);

#endif /* RNG_H */
