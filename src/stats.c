#include <math.h>
#include "stats.h"

void stats_ci95(const double *values, int n, double *mean_out, double *ci_out) {
    if (n <= 0) {
        *mean_out = 0.0;
        *ci_out = 0.0;
        return;
    }

    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += values[i];
    double mean = sum / n;

    if (n <= 1) {
        *mean_out = mean;
        *ci_out = 0.0; /* desvio padrão indefinido com 1 amostra só */
        return;
    }

    double sum_sq_dev = 0.0;
    for (int i = 0; i < n; i++) {
        double d = values[i] - mean;
        sum_sq_dev += d * d;
    }
    double variance = sum_sq_dev / (n - 1); /* desvio padrão amostral (n-1) */
    double s = sqrt(variance);

    *mean_out = mean;
    *ci_out = 1.96 * s / sqrt((double) n);
}
