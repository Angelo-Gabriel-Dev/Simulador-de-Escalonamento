#ifndef STATS_H
#define STATS_H

/*
 * IC95% = x_bar +- 1.96 * s / sqrt(n)
 * onde x_bar é a média amostral, s o desvio padrão amostral (n-1 no
 * denominador) e n o número de execuções independentes (seeds).
 *
 * A agregação "oficial" usada no artigo é feita em Python
 * (scripts/analyze_results.py, com pandas/numpy sobre todas as seeds de
 * cada combinação cenário x algoritmo). Esta implementação em C existe
 * porque a arquitetura do projeto (Seção 11) pede stats.h/c como parte do
 * módulo do Dorian, e serve como (a) referência independente testada em
 * tests/test_stats.c para validar a fórmula usada no Python, e (b) uma
 * função pronta caso um modo de execução em lote dentro de um único
 * processo C venha a ser útil no futuro.
 */
void stats_ci95(const double *values, int n, double *mean_out, double *ci_out);

#endif /* STATS_H */