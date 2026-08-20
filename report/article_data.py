"""
Carrega os CSVs consolidados e expõe helpers de formatação (padrão
brasileiro, vírgula decimal) para o script que monta o artigo
(build_article.py). Nenhum número no PDF final é digitado à mão: tudo vem
daqui, para garantir que o artigo sempre bata com os dados em
results/consolidated/.
"""
from pathlib import Path
import pandas as pd

CONS = Path(__file__).resolve().parent.parent / "results" / "consolidated"

_main = pd.read_csv(CONS / "main_summary_long.csv")
_csw0 = pd.read_csv(CONS / "complementary_csw0_summary_long.csv")
_quantum = pd.read_csv(CONS / "complementary_quantum_summary_long.csv")
_k1 = pd.read_csv(CONS / "complementary_k1_summary_long.csv")


def _lookup(df, scenario, algorithm, metric):
    row = df[(df.scenario == scenario) & (df.algorithm == algorithm) & (df.metric == metric)]
    if len(row) == 0:
        raise KeyError(f"nao encontrado: {scenario}/{algorithm}/{metric}")
    r = row.iloc[0]
    return float(r["mean"]), float(r["ci95"]), int(r["n_seeds"])


def m(scenario, algorithm, metric, dataset="main"):
    """Retorna (media, ic95, n_seeds) para uma combinacao."""
    df = {"main": _main, "csw0": _csw0, "quantum": _quantum, "k1": _k1}[dataset]
    return _lookup(df, scenario, algorithm, metric)


def br(x, decimals=2):
    """Formata um numero no padrao brasileiro (virgula decimal)."""
    s = f"{x:,.{decimals}f}"
    s = s.replace(",", "\ufffd").replace(".", ",").replace("\ufffd", ".")
    return s


def mc(scenario, algorithm, metric, dataset="main", decimals=2):
    """'305,76 \u00b1 7,17' pronto para o texto/tabelas."""
    mean, ci, n = m(scenario, algorithm, metric, dataset)
    return f"{br(mean, decimals)} \u00b1 {br(ci, decimals)}"


def mean_only(scenario, algorithm, metric, dataset="main", decimals=2):
    mean, ci, n = m(scenario, algorithm, metric, dataset)
    return br(mean, decimals)


def n_seeds(dataset="main"):
    """Le e valida a quantidade real de seeds usada no dataset (nunca
    hardcoded no texto do artigo -- ver Secao 3 do relatorio de revisao).
    Levanta erro se o dataset tiver mais de um valor de n_seeds (sintoma
    de mistura de execucoes com configuracoes diferentes)."""
    df = {"main": _main, "csw0": _csw0, "quantum": _quantum, "k1": _k1}[dataset]
    valores = df["n_seeds"].unique()
    if len(valores) != 1:
        raise ValueError(
            f"[article_data.py] dataset '{dataset}' contem mais de um valor de "
            f"n_seeds ({sorted(valores)}) -- resultados de execucoes diferentes "
            f"parecem ter sido misturados. Rode scripts/run_experiments.sh e "
            f"scripts/analyze_results.py de novo antes de gerar o artigo."
        )
    return int(valores[0])


def n_processes(dataset="main"):
    """Le e valida a quantidade real de processos por execucao usada no
    dataset (mesmo raciocinio de n_seeds -- nunca hardcoded no artigo)."""
    df = {"main": _main, "csw0": _csw0, "quantum": _quantum, "k1": _k1}[dataset]
    valores = df["n_processes"].unique()
    if len(valores) != 1:
        raise ValueError(
            f"[article_data.py] dataset '{dataset}' contem mais de um valor de "
            f"n_processes ({sorted(valores)}) -- resultados de execucoes diferentes "
            f"parecem ter sido misturados."
        )
    return int(valores[0])


def ratio(scenario, algorithm, metric_num, metric_den, dataset="main"):
    a, _, _ = m(scenario, algorithm, metric_num, dataset)
    b, _, _ = m(scenario, algorithm, metric_den, dataset)
    return a / b if b else float("nan")