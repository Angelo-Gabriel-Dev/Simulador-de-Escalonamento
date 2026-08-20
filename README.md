# Simulador de Escalonamento de Processos

Projeto da Unidade 3 de Sistemas Operacionais — simulador de escalonamento
de processos por eventos discretos, em C, com 3 algoritmos clássicos
(FCFS, Round Robin, Prioridade não preemptiva) e um algoritmo próprio
(Prioridade Dinâmica com Estimativa de Rajada e Envelhecimento), mais
pipeline de experimentos/análise estatística em Python.

## Requisitos

- `gcc` (padrão C11) e `make`
- Python 3.10+ com `pip install -r scripts/requirements.txt`
  (pandas, numpy, scipy, matplotlib, seaborn)
- Bash (`scripts/run_experiments.sh` usa `bash`, não `/bin/sh` puro)

**No Windows**, recomenda-se rodar tudo dentro do WSL2 (não no PowerShell/cmd
direto — o simulador em C e os scripts Bash não têm suporte nativo fora de
um ambiente Unix-like). O repositório tem um `.gitattributes` forçando final
de linha LF para `.sh`/`.py`/`Makefile`/etc., então um `git clone` normal
dentro do WSL já traz os scripts com final de linha correto, mesmo que o
`core.autocrlf` do Git no Windows esteja ligado. Se algum script falhar com
erro do tipo `bash: ...: No such file or directory` ou `$'\r': command not
found`, verifique o final de linha com `file scripts/run_experiments.sh`
(deve indicar `ASCII text`, não `... with CRLF line terminators`).

## Build

```bash
make all          # compila bin/scheduler-sim
make test         # compila e roda a suíte de testes unitários (tests/test_*.c)
make clean        # remove bin/ e build/
```

## Execução manual de uma simulação

```bash
./bin/scheduler-sim --scenario balanced --algorithm custom --seed 1 \
    --n-processes 1000 --context-switch-cost 1 --io-devices 1 --verbose
```

`--help` lista todas as opções (cenário, algoritmo, seed, quantum do RR,
custo de troca de contexto, nº de dispositivos de E/S, pesos k1/k2 e
estimativa inicial de rajada do algoritmo próprio). Cada execução imprime
uma linha CSV em stdout com as métricas daquela seed.

## Rodando o experimento completo

```bash
pip install -r scripts/requirements.txt --break-system-packages   # se necessário
bash scripts/run_experiments.sh
python3 scripts/analyze_results.py --raw results/raw/all_runs_main.csv --out-prefix main
python3 scripts/analyze_results.py --raw results/raw/all_runs_complementary_csw0.csv --out-prefix complementary_csw0
python3 scripts/analyze_results.py --raw results/raw/all_runs_complementary_quantum.csv --out-prefix complementary_quantum
python3 scripts/analyze_results.py --raw results/raw/all_runs_complementary_k1.csv --out-prefix complementary_k1 --group-col k1
python3 scripts/generate_plots.py
```

Isso roda, por padrão, **1000 processos por execução e 1000 seeds por
cenário** (o mínimo exigido pelo enunciado é 1000 processos e 100 seeds —
usamos o valor "meta recomendada" porque o tempo de execução permite:
os ~45.000 runs completos do experimento principal + 3 análises
complementares levam cerca de 3 minutos nesta máquina de referência,
1 núcleo). Para reduzir para o mínimo obrigatório ou aumentar ainda mais:

```bash
SEEDS_PER_SCENARIO=100 bash scripts/run_experiments.sh    # mínimo obrigatório
SEEDS_PER_SCENARIO=1000 N_PROCESSES=10000 bash scripts/run_experiments.sh   # configuração ampliada
```

Variáveis de ambiente aceitas: `N_PROCESSES`, `SEEDS_PER_SCENARIO`,
`SEED_START`, `CONTEXT_SWITCH_COST`, `IO_DEVICES`, `QUANTUM`, `K1`, `K2`,
`INITIAL_BURST_ESTIMATE`, `EMA_ALPHA`, `PARALLEL_JOBS` (padrão: todos os
núcleos disponíveis, via `nproc`).

O script roda automaticamente:
1. **Experimento principal**: 4 cenários x 4 algoritmos x N seeds, custo de
   troca de contexto = 1 (> 0, como exige o enunciado).
2. **Complementar 1**: mesmos parâmetros, custo de troca de contexto = 0
   (isola o efeito da troca de contexto).
3. **Complementar 2**: Round Robin em CPU-bound com quantum = 20 em vez de
   4 (investiga a sensibilidade do RR ao quantum).
4. **Complementar 3**: algoritmo próprio com k1 ∈ {0,5; 0,05; 0,005}
   (documenta a calibração do peso de envelhecimento — ver
   `docs/algoritmo_proprio.md`).

## Reprodutibilidade

Toda carga de trabalho é determinística: mesma seed + mesmo cenário +
mesmo número de processos ⇒ exatamente os mesmos processos, sempre (PRNG
próprio, xorshift128+, não depende de `rand()`/`srand()` da libc — ver
`docs/decisoes_projeto.md`). Verificado automaticamente em
`tests/test_workload_reproducibility.c` e `tests/test_rng.c`.

Os resultados brutos individuais (`results/raw/`) não são versionados por
volume (dezenas de milhares de arquivos pequenos na configuração de 1000
seeds — ver `.gitignore`); os consolidados
(`results/consolidated/*.csv`) e as figuras (`report/figuras/*.png`) são
versionados. Para regenerar tudo do zero: `make clean && make all && bash
scripts/run_experiments.sh && python3 scripts/analyze_results.py [...] &&
python3 scripts/generate_plots.py` (ver comandos completos acima).

`scripts/run_experiments.sh` limpa automaticamente `results/raw/` no início
de cada execução (evita misturar CSVs de uma rodada com parâmetros
diferentes da rodada anterior — desative com `CLEAN_BEFORE_RUN=0` se
precisar acumular deliberadamente). Como camada extra de segurança,
`scripts/analyze_results.py` valida que todas as linhas de um dataset bruto
compartilham a mesma configuração (`n_processes`, `quantum`,
`context_switch_cost`, `io_devices`, `k1`/`k2`) antes de consolidar, e
recusa continuar (com um erro explicando o motivo) se detectar
inconsistência.

## Estrutura do repositório

```
src/                  # simulador em C
  algorithms/         # fcfs, round_robin, priority, custom_algo
config/               # scenarios.json, experiments.yaml
scripts/              # run_experiments.sh, analyze_results.py, generate_plots.py
results/
  raw/                # csv bruto por (cenário, algoritmo, seed) -- gerado, não versionado
  consolidated/        # csv consolidado (média + IC95%) -- versionado
docs/
  modelagem.md         # decisões de modelagem (E/S, troca de contexto, chegadas, desempates)
  algoritmo_proprio.md # motivação, calibração e limitações do algoritmo próprio
  decisoes_projeto.md  # decisões de arquitetura/engenharia
report/
  figuras/              # gráficos gerados por generate_plots.py
  artigo.pdf            # artigo científico final (formato IEEE, 2 colunas)
slides/
  apresentacao.pptx     # slides da apresentação (10-12 min)
tests/                  # testes unitários (make test)
responsabilidades.md    # divisão de tarefas entre os 4 integrantes
```

## Testes

```bash
make test
```

Roda 7 suítes (`tests/test_rng.c`, `test_event_queue.c`,
`test_workload_reproducibility.c`, `test_algorithms.c`, `test_metrics.c`,
`test_stats.c`, `test_custom_algo.c`), cobrindo determinismo de seed,
ordenação da fila de eventos, os 3 algoritmos clássicos, fórmulas de
métricas (incluindo casos conhecidos do índice de Jain), IC95%, e as
propriedades do algoritmo próprio (atualização da EMA, efeito do
envelhecimento, não uso de informação futura). Compilado e executado
também sob AddressSanitizer/UndefinedBehaviorSanitizer durante o
desenvolvimento (sem erros em nenhuma das 16 combinações
cenário×algoritmo, incluindo parâmetros não-padrão de custo de troca e
nº de dispositivos de E/S).

## Equipe e fluxo de trabalho Git

Ver `responsabilidades.md` para a divisão de tarefas completa. Branches:
`main` (protegida) ← `develop` ← `feature/<nome>-<módulo>` (uma por
integrante). O histórico de commits neste repositório foi montado seguindo
o backlog combinado pela equipe (mensagens no padrão Conventional
Commits); **antes de enviar para o repositório GitHub definitivo, cada
integrante deve revisar os commits do seu módulo e, se necessário,
recommitar com sua própria identidade Git (`git config user.name` /
`user.email`) para que a participação individual apareça corretamente no
histórico do GitHub**, conforme pede o critério de avaliação de
reprodutibilidade/organização do repositório.
