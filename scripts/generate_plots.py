#!/usr/bin/env python3
"""
scripts/generate_plots.py

Gera os graficos com media + IC95% exigidos pela Secao 10 do enunciado,
comparando o algoritmo proprio com os 3 classicos nos 4 cenarios
obrigatorios. Le os CSVs consolidados por analyze_results.py (nunca
recalcula estatistica a partir dos dados brutos aqui, para garantir que os
numeros do grafico batem exatamente com os das tabelas do artigo).

Uso:
    python3 scripts/generate_plots.py
"""
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

CONSOLIDATED_DIR = Path("results/consolidated")
FIG_DIR = Path("report/figuras")
FIG_DIR.mkdir(parents=True, exist_ok=True)

ALGO_ORDER = ["fcfs", "rr", "priority", "custom"]
ALGO_LABELS = {
    "fcfs": "FCFS",
    "rr": "Round Robin",
    "priority": "Prioridade",
    "custom": "Algoritmo proprio",
}
ALGO_COLORS = {
    "fcfs": "#4C72B0",
    "rr": "#DD8452",
    "priority": "#55A868",
    "custom": "#C44E52",
}
SCEN_ORDER = ["balanced", "io_bound", "cpu_bound", "priority_skew"]
SCEN_LABELS = {
    "balanced": "Equilibrado",
    "io_bound": "I/O-bound",
    "cpu_bound": "CPU-bound",
    "priority_skew": "Prio.\ndesbalanc.",
}

METRIC_INFO = {
    "avg_turnaround": ("Turnaround medio", "unidades de tempo"),
    "context_switches": ("Trocas de contexto", "quantidade (total por execucao)"),
    "jain_slowdown": ("Indice de Jain do slowdown", "%"),
    "avg_slowdown": ("Slowdown medio", "razao (turnaround / tempo ideal)"),
}


def n_seeds_of(df, context=""):
    """Le a quantidade real de seeds a partir dos dados consolidados, em vez
    de assumir um numero fixo (ver docs/decisoes_projeto.md -- item sobre
    rotulos de grafico dinamicos). Se o CSV consolidado contiver mais de um
    valor de n_seeds (sintoma de mistura de execucoes com configuracoes
    diferentes -- ver Secao 4 do relatorio de revisao), a geracao do
    grafico PARA com um erro claro em vez de exibir um numero errado."""
    valores = df["n_seeds"].unique()
    if len(valores) != 1:
        raise ValueError(
            f"[generate_plots.py{' - ' + context if context else ''}] "
            f"O CSV consolidado contem mais de um valor de n_seeds ({sorted(valores)}). "
            f"Isso normalmente indica que resultados de execucoes diferentes (ex.: 100 "
            f"seeds numa vez, 1000 seeds noutra) foram misturados antes da consolidacao. "
            f"Rode scripts/run_experiments.sh de novo (ele agora limpa results/raw/ antes "
            f"de cada execucao) e depois scripts/analyze_results.py antes de gerar os graficos."
        )
    return int(valores[0])


def grouped_bar(ax, df, metric, title, ylabel, legend=False, log_scale=False):
    x = np.arange(len(SCEN_ORDER))
    width = 0.2
    for i, algo in enumerate(ALGO_ORDER):
        means, cis = [], []
        for scen in SCEN_ORDER:
            row = df[(df.scenario == scen) & (df.algorithm == algo) & (df.metric == metric)]
            if len(row) == 0:
                means.append(np.nan); cis.append(0.0)
            else:
                means.append(float(row["mean"].iloc[0]))
                cis.append(float(row["ci95"].iloc[0]))
        offset = (i - 1.5) * width
        ax.bar(x + offset, means, width, yerr=cis, capsize=3,
               label=ALGO_LABELS[algo], color=ALGO_COLORS[algo],
               edgecolor="white", linewidth=0.5)
    ax.set_xticks(x)
    ax.set_xticklabels([SCEN_LABELS[s] for s in SCEN_ORDER], fontsize=9)
    ax.set_title(title, fontsize=11, fontweight="bold")
    ax.set_ylabel(ylabel, fontsize=9)
    ax.grid(axis="y", alpha=0.3, linewidth=0.5)
    ax.set_axisbelow(True)
    if log_scale:
        ax.set_yscale("log")
        ax.set_ylabel(ylabel + " (escala log)", fontsize=9)
    if legend:
        ax.legend(fontsize=8, ncol=2, loc="upper center", bbox_to_anchor=(0.5, -0.18))


def plot_main_panel(df):
    """Figura principal: 1 linha x 3 paineis (turnaround, trocas de contexto, Jain),
    cada um comparando os 4 algoritmos nos 4 cenarios. Pensada para ocupar as
    duas colunas do artigo IEEE."""
    n_seeds = n_seeds_of(df, context="fig_main_metrics")
    fig, axes = plt.subplots(1, 3, figsize=(12, 4.2))
    grouped_bar(axes[0], df, "avg_turnaround", "(a) Turnaround medio", "unidades de tempo", log_scale=True)
    grouped_bar(axes[1], df, "context_switches", "(b) Trocas de contexto", "quantidade", log_scale=True)
    grouped_bar(axes[2], df, "jain_slowdown", "(c) Indice de Jain do slowdown", "%", legend=True)
    fig.suptitle(f"Comparacao entre algoritmos por cenario (media \u00b1 IC95%, {n_seeds} seeds/cenario)",
                 fontsize=12, fontweight="bold", y=1.04)
    fig.tight_layout()
    out = FIG_DIR / "fig_main_metrics.png"
    fig.savefig(out, dpi=200, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {out} ({n_seeds} seeds/cenario)")


def plot_slowdown(df):
    n_seeds = n_seeds_of(df, context="fig_slowdown")
    fig, ax = plt.subplots(1, 1, figsize=(6, 4))
    grouped_bar(ax, df, "avg_slowdown", f"Slowdown medio por cenario e algoritmo ({n_seeds} seeds/cenario)",
                "razao (turnaround / tempo ideal)", legend=True)
    fig.tight_layout()
    out = FIG_DIR / "fig_slowdown.png"
    fig.savefig(out, dpi=200, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {out} ({n_seeds} seeds/cenario)")


def plot_complementary_csw(df_main, df_csw0):
    """Compara turnaround medio com custo de troca de contexto = 1 (principal)
    vs = 0 (complementar), para todos os algoritmos, no cenario mais sensivel
    a trocas de contexto (cpu_bound, onde RR sofre mais)."""
    algos = ALGO_ORDER
    scen = "cpu_bound"
    means1, cis1, means0, cis0 = [], [], [], []
    for algo in algos:
        r1 = df_main[(df_main.scenario == scen) & (df_main.algorithm == algo) & (df_main.metric == "avg_turnaround")]
        r0 = df_csw0[(df_csw0.scenario == scen) & (df_csw0.algorithm == algo) & (df_csw0.metric == "avg_turnaround")]
        means1.append(float(r1["mean"].iloc[0]) if len(r1) else np.nan)
        cis1.append(float(r1["ci95"].iloc[0]) if len(r1) else 0.0)
        means0.append(float(r0["mean"].iloc[0]) if len(r0) else np.nan)
        cis0.append(float(r0["ci95"].iloc[0]) if len(r0) else 0.0)

    x = np.arange(len(algos))
    width = 0.32
    fig, ax = plt.subplots(figsize=(6, 4))
    ax.bar(x - width / 2, means1, width, yerr=cis1, capsize=3, label="Custo de troca = 1 (principal)",
           color="#4C72B0", edgecolor="white", linewidth=0.5)
    ax.bar(x + width / 2, means0, width, yerr=cis0, capsize=3, label="Custo de troca = 0 (complementar)",
           color="#8FBBE8", edgecolor="white", linewidth=0.5)
    ax.set_xticks(x)
    ax.set_xticklabels([ALGO_LABELS[a] for a in algos])
    ax.set_ylabel("Turnaround medio (unidades de tempo)")
    ax.set_title(f"Efeito do custo de troca de contexto -- cenario {SCEN_LABELS[scen].strip()}",
                 fontsize=11, fontweight="bold")
    ax.grid(axis="y", alpha=0.3, linewidth=0.5)
    ax.set_axisbelow(True)
    ax.legend(fontsize=8)
    fig.tight_layout()
    out = FIG_DIR / "fig_complementary_csw.png"
    fig.savefig(out, dpi=200, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {out}")


def plot_quantum_sensitivity(df_main, df_q):
    """RR com quantum=4 (principal) vs quantum=20 (complementar) em cpu_bound:
    turnaround medio e trocas de contexto lado a lado."""
    r4_t = df_main[(df_main.scenario == "cpu_bound") & (df_main.algorithm == "rr") & (df_main.metric == "avg_turnaround")]
    r4_c = df_main[(df_main.scenario == "cpu_bound") & (df_main.algorithm == "rr") & (df_main.metric == "context_switches")]
    r20_t = df_q[(df_q.metric == "avg_turnaround")]
    r20_c = df_q[(df_q.metric == "context_switches")]

    labels = ["quantum=4\n(principal)", "quantum=20\n(complementar)"]
    turnaround_vals = [float(r4_t["mean"].iloc[0]), float(r20_t["mean"].iloc[0])]
    turnaround_cis = [float(r4_t["ci95"].iloc[0]), float(r20_t["ci95"].iloc[0])]
    cs_vals = [float(r4_c["mean"].iloc[0]), float(r20_c["mean"].iloc[0])]
    cs_cis = [float(r4_c["ci95"].iloc[0]), float(r20_c["ci95"].iloc[0])]

    fig, axes = plt.subplots(1, 2, figsize=(8, 4))
    axes[0].bar(labels, turnaround_vals, yerr=turnaround_cis, capsize=4,
                color=["#DD8452", "#F2C29A"], edgecolor="white")
    axes[0].set_title("Turnaround medio", fontsize=10, fontweight="bold")
    axes[0].set_ylabel("unidades de tempo")
    axes[0].grid(axis="y", alpha=0.3)
    axes[0].set_axisbelow(True)

    axes[1].bar(labels, cs_vals, yerr=cs_cis, capsize=4,
                color=["#DD8452", "#F2C29A"], edgecolor="white")
    axes[1].set_title("Trocas de contexto", fontsize=10, fontweight="bold")
    axes[1].set_ylabel("quantidade")
    axes[1].grid(axis="y", alpha=0.3)
    axes[1].set_axisbelow(True)

    fig.suptitle("Sensibilidade do Round Robin ao quantum -- cenario CPU-bound", fontsize=11, fontweight="bold")
    fig.tight_layout()
    out = FIG_DIR / "fig_quantum_sensitivity.png"
    fig.savefig(out, dpi=200, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {out}")


def plot_priority_breakdown(df):
    """Turnaround medio para processos de prioridade alta (<=3) vs baixa
    (>=8), por algoritmo, no cenario priority_skew -- mostra se o
    algoritmo de fato USA a prioridade (FCFS nao usa; Priority usa demais;
    o algoritmo proprio fica no meio)."""
    n_seeds = n_seeds_of(df, context="fig_priority_breakdown")
    scen = "priority_skew"
    algos = ALGO_ORDER
    high_means, high_cis, low_means, low_cis = [], [], [], []
    for algo in algos:
        rh = df[(df.scenario == scen) & (df.algorithm == algo) & (df.metric == "avg_turnaround_high_priority")]
        rl = df[(df.scenario == scen) & (df.algorithm == algo) & (df.metric == "avg_turnaround_low_priority")]
        high_means.append(float(rh["mean"].iloc[0])); high_cis.append(float(rh["ci95"].iloc[0]))
        low_means.append(float(rl["mean"].iloc[0])); low_cis.append(float(rl["ci95"].iloc[0]))

    x = np.arange(len(algos))
    width = 0.32
    fig, ax = plt.subplots(figsize=(7, 4.5))
    ax.bar(x - width / 2, high_means, width, yerr=high_cis, capsize=3,
           label="Prioridade alta (<=3)", color="#55A868", edgecolor="white")
    ax.bar(x + width / 2, low_means, width, yerr=low_cis, capsize=3,
           label="Prioridade baixa (>=8)", color="#C44E52", edgecolor="white")
    ax.set_xticks(x)
    ax.set_xticklabels([ALGO_LABELS[a] for a in algos])
    ax.set_ylabel("Turnaround medio (unidades de tempo)")
    ax.set_title(f"Turnaround por classe de prioridade -- cenario prioridades desbalanceadas ({n_seeds} seeds)",
                 fontsize=11, fontweight="bold")
    ax.grid(axis="y", alpha=0.3, linewidth=0.5)
    ax.set_axisbelow(True)
    ax.legend(fontsize=9)

    # anota a razao baixa/alta acima de cada par de barras (mede o quanto
    # o algoritmo "discrimina" por prioridade)
    for i in range(len(algos)):
        ratio = low_means[i] / high_means[i] if high_means[i] > 0 else float("nan")
        ymax = max(high_means[i] + high_cis[i], low_means[i] + low_cis[i])
        ax.text(x[i], ymax * 1.03, f"{ratio:.2f}x", ha="center", fontsize=8.5, fontweight="bold")

    fig.tight_layout()
    out = FIG_DIR / "fig_priority_breakdown.png"
    fig.savefig(out, dpi=200, bbox_inches="tight")
    plt.close(fig)
    print(f"  -> {out} ({n_seeds} seeds)")


def main():
    main_path = CONSOLIDATED_DIR / "main_summary_long.csv"
    if not main_path.exists():
        print(f"[erro] {main_path} nao encontrado. Rode scripts/analyze_results.py primeiro.", file=sys.stderr)
        sys.exit(1)
    df_main = pd.read_csv(main_path)

    print("Gerando figuras principais...")
    plot_main_panel(df_main)
    plot_slowdown(df_main)
    plot_priority_breakdown(df_main)

    csw0_path = CONSOLIDATED_DIR / "complementary_csw0_summary_long.csv"
    if csw0_path.exists():
        df_csw0 = pd.read_csv(csw0_path)
        plot_complementary_csw(df_main, df_csw0)
    else:
        print(f"[aviso] {csw0_path} nao encontrado, pulando figura complementar de custo de troca.")

    q_path = CONSOLIDATED_DIR / "complementary_quantum_summary_long.csv"
    if q_path.exists():
        df_q = pd.read_csv(q_path)
        plot_quantum_sensitivity(df_main, df_q)
    else:
        print(f"[aviso] {q_path} nao encontrado, pulando figura de sensibilidade de quantum.")

    print("Concluido.")


if __name__ == "__main__":
    main()