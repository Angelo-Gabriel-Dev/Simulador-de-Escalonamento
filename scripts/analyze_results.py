#!/usr/bin/env python3
"""
scripts/analyze_results.py

Consolida os CSVs brutos (um valor-resumo por seed) em media + IC95% por
combinacao (cenario, algoritmo), para cada metrica obrigatoria (Secao 8/9
do enunciado). O IC95% e calculado ENTRE seeds (n = numero de seeds), nunca
entre processos dentro de uma execucao -- exatamente como pede a Secao 9.

IC95% = media +- 1.96 * desvio_padrao_amostral / sqrt(n_seeds)

(mesma formula, e mesmo resultado numerico, que stats.c / test_stats.c)

Uso:
    python3 scripts/analyze_results.py
    python3 scripts/analyze_results.py --raw results/raw/all_runs_main.csv --out-prefix main
"""
import argparse
import sys
from pathlib import Path

import numpy as np
import pandas as pd

COLUMNS = [
    "scenario", "algorithm", "seed", "n_processes", "quantum",
    "context_switch_cost", "io_devices", "k1", "k2",
    "avg_turnaround", "context_switches", "jain_slowdown", "avg_slowdown",
    "avg_turnaround_high_priority", "avg_turnaround_low_priority",
]

METRICS = {
    "avg_turnaround": "Turnaround medio",
    "context_switches": "Trocas de contexto",
    "jain_slowdown": "Indice de Jain do slowdown (%)",
    "avg_slowdown": "Slowdown medio (auxiliar)",
    "avg_turnaround_high_priority": "Turnaround medio -- prioridade alta (<=3)",
    "avg_turnaround_low_priority": "Turnaround medio -- prioridade baixa (>=8)",
}

ALGO_LABELS = {
    "fcfs": "FCFS",
    "rr": "Round Robin",
    "priority": "Prioridade",
    "custom": "Algoritmo proprio (Aging+Rajada Estimada)",
}
SCENARIO_LABELS = {
    "balanced": "Aleatorio equilibrado",
    "io_bound": "I/O-bound",
    "cpu_bound": "CPU-bound",
    "priority_skew": "Prioridades desbalanceadas",
}


def ci95(values: np.ndarray):
    n = len(values)
    mean = float(np.mean(values))
    if n <= 1:
        return mean, 0.0
    s = float(np.std(values, ddof=1))  # desvio padrao amostral (n-1)
    ci = 1.96 * s / np.sqrt(n)
    return mean, ci


def load_raw(path: Path) -> pd.DataFrame:
    if not path.exists():
        print(f"[aviso] arquivo bruto nao encontrado: {path}", file=sys.stderr)
        return pd.DataFrame(columns=COLUMNS)
    df = pd.read_csv(path, header=None, names=COLUMNS)
    return df


def check_config_consistency(df: pd.DataFrame, group_col: str = "algorithm"):
    """Verifica se cada grupo (cenario, group_col) tem uma configuracao
    experimental CONSTANTE entre as seeds (n_processes, quantum,
    context_switch_cost, io_devices, e k1/k2 quando fizer sentido). Se nao
    for, os dados brutos provavelmente misturam execucoes com parametros
    diferentes (ex.: uma rodada com 100 seeds e outra com 1000, ou quantum
    diferente) que acabaram no mesmo diretorio de saida antes da
    consolidacao -- ver docs/decisoes_projeto.md. Levanta erro em vez de
    calcular uma media silenciosamente errada sobre dados incompativeis."""
    always_check = ["n_processes", "context_switch_cost", "io_devices", "quantum"]
    check_cols = [c for c in always_check if c != group_col]
    if group_col != "k1":
        check_cols.append("k1")
    if group_col != "k2":
        check_cols.append("k2")

    problems = []
    for (scenario, group_val), group in df.groupby(["scenario", group_col]):
        for col in check_cols:
            n_unique = group[col].nunique()
            if n_unique > 1:
                problems.append(
                    f"  cenario={scenario} {group_col}={group_val}: coluna '{col}' tem "
                    f"{n_unique} valores diferentes ({sorted(group[col].unique())})"
                )
    if problems:
        raise ValueError(
            "Configuracao experimental inconsistente dentro de um ou mais grupos "
            "(sintoma de mistura de execucoes com parametros diferentes -- rode "
            "scripts/run_experiments.sh de novo, que agora limpa results/raw/ "
            "antes de cada execucao, e evite copiar CSVs brutos de execucoes "
            "diferentes para o mesmo diretorio):\n" + "\n".join(problems)
        )
    print(f"[ok] configuracao experimental consistente em todas as {df.groupby(['scenario', group_col]).ngroups} combinacoes (cenario x {group_col}).")


def consolidate(df: pd.DataFrame, group_col: str = "algorithm") -> pd.DataFrame:
    rows = []
    for (scenario, group_val), group in df.groupby(["scenario", group_col]):
        n_seeds = len(group)
        for metric in METRICS:
            mean, ci = ci95(group[metric].to_numpy(dtype=float))
            rows.append({
                "scenario": scenario,
                "scenario_label": SCENARIO_LABELS.get(scenario, scenario),
                "algorithm": group_val,
                "algorithm_label": ALGO_LABELS.get(group_val, str(group_val)),
                "metric": metric,
                "metric_label": METRICS[metric],
                "mean": mean,
                "ci95": ci,
                "n_seeds": n_seeds,
                # parametros da execucao (ja validados como constantes dentro do
                # grupo por check_config_consistency) -- ver Secao 3 do relatorio
                # de revisao: evita numeros hardcoded no artigo/graficos.
                "n_processes": int(group["n_processes"].iloc[0]),
                "quantum": int(group["quantum"].iloc[0]),
                "context_switch_cost": int(group["context_switch_cost"].iloc[0]),
                "io_devices": int(group["io_devices"].iloc[0]),
            })
    out = pd.DataFrame(rows)
    return out.sort_values(["scenario", "metric", "algorithm"]).reset_index(drop=True)


def check_seed_counts(df: pd.DataFrame, expected_min: int = 100, group_col: str = "algorithm"):
    counts = df.groupby(["scenario", group_col]).size()
    problems = counts[counts < expected_min]
    if len(problems) > 0:
        print(f"[aviso] combinacoes com menos de {expected_min} seeds:", file=sys.stderr)
        print(problems, file=sys.stderr)
    else:
        print(f"[ok] todas as {len(counts)} combinacoes (cenario x {group_col}) tem >= {expected_min} seeds.")
    return counts


def make_wide_tables(consolidated: pd.DataFrame, out_dir: Path, prefix: str):
    """Uma tabela por metrica: linhas = cenario, colunas = algoritmo (media e IC95% lado a lado).
    Formato pronto para colar no artigo (Secao 10 do enunciado)."""
    for metric in consolidated["metric"].unique():
        sub = consolidated[consolidated["metric"] == metric].copy()
        sub["mean_ci"] = sub.apply(lambda r: f"{r['mean']:.3f} +/- {r['ci95']:.3f}", axis=1)
        wide = sub.pivot(index="scenario_label", columns="algorithm_label", values="mean_ci")
        out_path = out_dir / f"{prefix}_{metric}_wide.csv"
        wide.to_csv(out_path)
        print(f"  -> {out_path}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--raw", default="results/raw/all_runs_main.csv")
    ap.add_argument("--out-prefix", default="main")
    ap.add_argument("--min-seeds", type=int, default=100)
    ap.add_argument("--group-col", default="algorithm",
                     help="coluna usada para agrupar/comparar (default: algorithm; use 'k1' para a sensibilidade de aging)")
    args = ap.parse_args()

    raw_path = Path(args.raw)
    out_dir = Path("results/consolidated")
    out_dir.mkdir(parents=True, exist_ok=True)

    df = load_raw(raw_path)
    if df.empty:
        print(f"[erro] nenhum dado em {raw_path}; rode scripts/run_experiments.sh primeiro.", file=sys.stderr)
        sys.exit(1)

    print(f"Carregado {len(df)} execucoes de {raw_path}")
    check_seed_counts(df, expected_min=args.min_seeds, group_col=args.group_col)
    check_config_consistency(df, group_col=args.group_col)

    consolidated = consolidate(df, group_col=args.group_col)
    long_path = out_dir / f"{args.out_prefix}_summary_long.csv"
    consolidated.to_csv(long_path, index=False)
    print(f"Resumo (formato longo) salvo em {long_path}")

    print("Gerando tabelas largas (uma por metrica, prontas para o artigo):")
    make_wide_tables(consolidated, out_dir, args.out_prefix)

    print("\n=== Resumo (media +/- IC95%, agregado entre seeds) ===")
    for scenario in consolidated["scenario"].unique():
        print(f"\n--- Cenario: {SCENARIO_LABELS.get(scenario, scenario)} ---")
        sub = consolidated[consolidated["scenario"] == scenario]
        for metric in METRICS:
            msub = sub[sub["metric"] == metric]
            print(f"  {METRICS[metric]}:")
            for _, row in msub.iterrows():
                print(f"    {row['algorithm_label']:<40s} {row['mean']:>12.3f} +/- {row['ci95']:.3f}  (n={row['n_seeds']})")


if __name__ == "__main__":
    main()
