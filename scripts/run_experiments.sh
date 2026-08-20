#!/usr/bin/env bash
#
# scripts/run_experiments.sh
#
# Roda o experimento principal (>=1000 processos, >=100 seeds/cenario,
# custo de troca de contexto > 0, mesmos cenarios/seeds para todos os
# algoritmos -- Secoes 5, 7 e 2 do enunciado), mais tres analises
# complementares (custo de troca de contexto = 0, sensibilidade de
# quantum do RR, e sensibilidade de k1 do algoritmo proprio). Parametros
# documentados em config/experiments.yaml (mantidos aqui como variaveis
# de shell -- ver docs/decisoes_projeto.md).
#
# Uso:
#   ./scripts/run_experiments.sh                 # experimento principal + complementares (1000 seeds/cenario, o padrao)
#   ./scripts/run_experiments.sh --main-only      # so o experimento principal
#   SEEDS_PER_SCENARIO=100 ./scripts/run_experiments.sh
#       # minimo obrigatorio do enunciado (mais rapido, use para testes rapidos)
#   N_PROCESSES=10000 SEEDS_PER_SCENARIO=1000 ./scripts/run_experiments.sh
#       # configuracao ampliada (opcional, Secao 5)

set -euo pipefail
cd "$(dirname "$0")/.."

BIN=./bin/scheduler-sim
N_PROCESSES="${N_PROCESSES:-1000}"
SEEDS_PER_SCENARIO="${SEEDS_PER_SCENARIO:-1000}"
SEED_START="${SEED_START:-1}"
CONTEXT_SWITCH_COST="${CONTEXT_SWITCH_COST:-1}"
IO_DEVICES="${IO_DEVICES:-1}"
QUANTUM="${QUANTUM:-4}"
K1="${K1:-0.05}"
K2="${K2:-0.3}"
INITIAL_BURST_ESTIMATE="${INITIAL_BURST_ESTIMATE:-10.0}"
EMA_ALPHA="${EMA_ALPHA:-0.5}"
PARALLEL_JOBS="${PARALLEL_JOBS:-$(nproc)}"

SCENARIOS="balanced io_bound cpu_bound priority_skew"
ALGORITHMS="fcfs rr priority custom"

RAW_DIR=results/raw
CLEAN_BEFORE_RUN="${CLEAN_BEFORE_RUN:-1}"
mkdir -p "$RAW_DIR/main" "$RAW_DIR/complementary_csw0" "$RAW_DIR/complementary_quantum" "$RAW_DIR/complementary_k1"

if [ "$CLEAN_BEFORE_RUN" = "1" ]; then
  echo "== limpando resultados brutos de execucoes anteriores em $RAW_DIR/ =="
  echo "   (evita misturar CSVs de execucoes com parametros diferentes -- ver"
  echo "   docs/decisoes_projeto.md; desative com CLEAN_BEFORE_RUN=0 se tiver certeza"
  echo "   de que quer ACUMULAR arquivos de execucoes anteriores)"
  rm -f "$RAW_DIR"/main/*.csv "$RAW_DIR"/complementary_csw0/*.csv \
        "$RAW_DIR"/complementary_quantum/*.csv "$RAW_DIR"/complementary_k1/*.csv \
        "$RAW_DIR"/all_runs_*.csv 2>/dev/null || true
else
  echo "== CLEAN_BEFORE_RUN=0: mantendo arquivos brutos de execucoes anteriores =="
  echo "   ATENCAO: se os parametros (seeds, n_processes, quantum, custo de troca...)"
  echo "   mudaram desde a ultima execucao, a consolidacao pode misturar dados"
  echo "   incompativeis. scripts/analyze_results.py agora valida isso e recusa"
  echo "   consolidar caso detecte configuracao inconsistente, mas prefira nao"
  echo "   contar so com essa rede de seguranca."
fi

echo "== build =="
make all

seed_end=$((SEED_START + SEEDS_PER_SCENARIO - 1))
echo "== experimento principal: $N_PROCESSES processos, seeds $SEED_START..$seed_end, csw_cost=$CONTEXT_SWITCH_COST, io_devices=$IO_DEVICES, quantum=$QUANTUM, paralelismo=$PARALLEL_JOBS =="

run_one() {
  scenario="$1"; algorithm="$2"; seed="$3"
  out="$RAW_DIR/main/${scenario}__${algorithm}__seed${seed}.csv"
  "$BIN" --scenario "$scenario" --algorithm "$algorithm" --seed "$seed" \
         --n-processes "$N_PROCESSES" --quantum "$QUANTUM" \
         --context-switch-cost "$CONTEXT_SWITCH_COST" --io-devices "$IO_DEVICES" \
         --k1 "$K1" --k2 "$K2" --initial-burst-estimate "$INITIAL_BURST_ESTIMATE" \
         --ema-alpha "$EMA_ALPHA" > "$out"
}
export -f run_one
export BIN N_PROCESSES QUANTUM CONTEXT_SWITCH_COST IO_DEVICES K1 K2 INITIAL_BURST_ESTIMATE EMA_ALPHA RAW_DIR

jobs_file="$(mktemp)"
for scenario in $SCENARIOS; do
  for algorithm in $ALGORITHMS; do
    for seed in $(seq "$SEED_START" "$seed_end"); do
      echo "$scenario $algorithm $seed"
    done
  done
done > "$jobs_file"

total_jobs=$(wc -l < "$jobs_file")
echo "Total de execucoes (experimento principal): $total_jobs"

start_ts=$(date +%s)
xargs -a "$jobs_file" -P "$PARALLEL_JOBS" -L 1 bash -c 'run_one "$0" "$1" "$2"'
end_ts=$(date +%s)
echo "Experimento principal concluido em $((end_ts - start_ts))s."

cat "$RAW_DIR"/main/*.csv > "$RAW_DIR/all_runs_main.csv"
echo "Consolidado bruto: $RAW_DIR/all_runs_main.csv ($(wc -l < "$RAW_DIR/all_runs_main.csv") linhas)"

if [ "${1:-}" = "--main-only" ]; then
  echo "Modo --main-only: pulando analises complementares."
  rm -f "$jobs_file"
  exit 0
fi

echo "== analise complementar 1/2: custo de troca de contexto = 0 (mesmos cenarios/algoritmos/seeds) =="
run_one_csw0() {
  scenario="$1"; algorithm="$2"; seed="$3"
  out="$RAW_DIR/complementary_csw0/${scenario}__${algorithm}__seed${seed}.csv"
  "$BIN" --scenario "$scenario" --algorithm "$algorithm" --seed "$seed" \
         --n-processes "$N_PROCESSES" --quantum "$QUANTUM" \
         --context-switch-cost 0 --io-devices "$IO_DEVICES" \
         --k1 "$K1" --k2 "$K2" --initial-burst-estimate "$INITIAL_BURST_ESTIMATE" \
         --ema-alpha "$EMA_ALPHA" > "$out"
}
export -f run_one_csw0
xargs -a "$jobs_file" -P "$PARALLEL_JOBS" -L 1 bash -c 'run_one_csw0 "$0" "$1" "$2"'
cat "$RAW_DIR"/complementary_csw0/*.csv > "$RAW_DIR/all_runs_complementary_csw0.csv"
echo "Consolidado bruto: $RAW_DIR/all_runs_complementary_csw0.csv ($(wc -l < "$RAW_DIR/all_runs_complementary_csw0.csv") linhas)"
rm -f "$jobs_file"

echo "== analise complementar 2/2: sensibilidade de quantum do RR em cpu_bound (quantum=20) =="
qjobs_file="$(mktemp)"
for seed in $(seq "$SEED_START" "$seed_end"); do
  echo "$seed"
done > "$qjobs_file"
run_one_quantum() {
  seed="$1"
  out="$RAW_DIR/complementary_quantum/cpu_bound__rr_q20__seed${seed}.csv"
  "$BIN" --scenario cpu_bound --algorithm rr --seed "$seed" \
         --n-processes "$N_PROCESSES" --quantum 20 \
         --context-switch-cost "$CONTEXT_SWITCH_COST" --io-devices "$IO_DEVICES" > "$out"
}
export -f run_one_quantum
xargs -a "$qjobs_file" -P "$PARALLEL_JOBS" -L 1 bash -c 'run_one_quantum "$0"'
cat "$RAW_DIR"/complementary_quantum/*.csv > "$RAW_DIR/all_runs_complementary_quantum.csv"
echo "Consolidado bruto: $RAW_DIR/all_runs_complementary_quantum.csv ($(wc -l < "$RAW_DIR/all_runs_complementary_quantum.csv") linhas)"
rm -f "$qjobs_file"

echo "== analise complementar 3/3: sensibilidade de k1 (peso de aging) do algoritmo proprio =="
k1jobs_file="$(mktemp)"
for scenario in $SCENARIOS; do
  for k1val in 0.5 0.05 0.005; do
    for seed in $(seq "$SEED_START" "$seed_end"); do
      echo "$scenario $k1val $seed"
    done
  done
done > "$k1jobs_file"
run_one_k1() {
  scenario="$1"; k1val="$2"; seed="$3"
  out="$RAW_DIR/complementary_k1/${scenario}__k1_${k1val}__seed${seed}.csv"
  "$BIN" --scenario "$scenario" --algorithm custom --seed "$seed" \
         --n-processes "$N_PROCESSES" --context-switch-cost "$CONTEXT_SWITCH_COST" \
         --io-devices "$IO_DEVICES" --k1 "$k1val" --k2 "$K2" \
         --initial-burst-estimate "$INITIAL_BURST_ESTIMATE" --ema-alpha "$EMA_ALPHA" > "$out"
}
export -f run_one_k1
xargs -a "$k1jobs_file" -P "$PARALLEL_JOBS" -L 1 bash -c 'run_one_k1 "$0" "$1" "$2"'
cat "$RAW_DIR"/complementary_k1/*.csv > "$RAW_DIR/all_runs_complementary_k1.csv"
echo "Consolidado bruto: $RAW_DIR/all_runs_complementary_k1.csv ($(wc -l < "$RAW_DIR/all_runs_complementary_k1.csv") linhas)"
rm -f "$k1jobs_file"

echo "== tudo pronto. Proximo passo: python3 scripts/analyze_results.py =="