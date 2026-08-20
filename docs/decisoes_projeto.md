# Decisões de projeto

Decisões de engenharia que não são sobre modelagem de sistema operacional
em si (essas estão em `docs/modelagem.md`) nem sobre o algoritmo próprio
(essas estão em `docs/algoritmo_proprio.md`), mas afetam a arquitetura do
repositório e merecem registro.

## Por que os cenários são presets em C, e não JSON lido em tempo de execução

`config/scenarios.json` documenta os 4 cenários obrigatórios de forma
legível e é lido pelos scripts Python (rotulagem de gráficos/tabelas). Em
tempo de execução, porém, `src/workload_generator.c` usa presets
`if/else` nomeados (`scenario_get_by_name`) em vez de parsear o JSON.

Motivo: escrever (ou trazer como dependência) um parser de JSON em C só
para ler 4 registros estáticos, conhecidos em tempo de compilação, não
melhora a reprodutibilidade — o parser em si viraria mais uma peça de
código para validar, e o enunciado pede explicitamente "sem dependências
exóticas" no simulador C. O preço dessa escolha é ter que manter
`scenarios.json` e os presets em `workload_generator.c` sincronizados
manualmente; isso é mitigado por (a) um teste automatizado
(`tests/test_workload_reproducibility.c`) que verifica as propriedades
estruturais de cada preset (rajadas sempre ≥1, só a última rajada sem E/S,
chegadas não-decrescentes) e (b) `scenarios.json` conter um campo
`_readme` explícito apontando `workload_generator.c` como fonte de
verdade.

## Por que `config/experiments.yaml` não é parseado por `run_experiments.sh`

Pelo mesmo raciocínio acima: `experiments.yaml` é a referência legível e
versionada dos parâmetros do experimento principal, mas
`scripts/run_experiments.sh` define os mesmos valores como variáveis de
shell no topo do arquivo (sobrescrevíveis por variáveis de ambiente, ex.:
`SEEDS_PER_SCENARIO=1000 ./scripts/run_experiments.sh`). Escrever um
parser de YAML em Bash para meia dúzia de escalares não trouxe benefício
de reprodutibilidade que justificasse a dependência extra (`yq` ou
similar). Se os dois arquivos divergirem, o comportamento real do
experimento é sempre o que está em `run_experiments.sh` — é o que
efetivamente roda.

## PRNG próprio (xorshift128+) em vez de `rand()`/`srand()` da libc

A qualidade estatística de `rand()` e o comportamento de `srand()` variam
entre implementações de libc (glibc, musl, etc.) e até entre versões da
mesma libc. Como a garantia central do projeto é "mesma seed, mesmo
cenário => mesma carga de trabalho, em qualquer máquina da equipe",
depender da libc arriscava quebrar essa garantia se dois integrantes
compilassem em ambientes diferentes. Implementamos xorshift128+
(`src/rng.c`), semeado via splitmix64 a partir da seed do usuário —
totalmente autocontido, sem dependência de plataforma, com teste de
determinismo automatizado (`tests/test_rng.c`).

## Critério de desempate global por sequência de inserção na fila de eventos

Documentado tecnicamente em `docs/modelagem.md` (Seção 7), mas vale
registrar a motivação: precisávamos de **um único critério determinístico**
que funcionasse para qualquer tipo de empate de tempo (chegada vs.
chegada, chegada vs. fim de E/S, fim de E/S vs. fim de E/S), sem precisar
de lógica especial por tipo de evento. Usar a ordem de inserção
(implementada como um contador monotônico `seq` comparado após `time` no
heap — ver `event_queue.c`) resolve isso de forma uniforme e barata (O(log
n) por inserção, sem custo extra no desempate).

## Estrutura de dados da fila de prontos por algoritmo

FCFS e Round Robin usam fila circular FIFO (O(1) por inserção/remoção).
Prioridade e o algoritmo próprio usam um array não ordenado com busca
linear pelo melhor candidato a cada `select_next` (O(k) onde k = tamanho
da fila de prontos naquele instante). Optamos por não manter uma
estrutura ordenada (ex.: heap por prioridade) para esses dois porque (a) a
prioridade efetiva do algoritmo próprio depende de `now`, que muda
continuamente mesmo sem novos eventos — um heap ordenado por prioridade
ficaria desatualizado a cada tick sem um evento correspondente, exigindo
reordenação completa de qualquer forma; e (b) nas escalas testadas (até
10.000 processos), o custo total de busca linear acumulado ao longo de uma
execução completa é desprezível frente ao tempo de simulação (execuções de
1000 processos levam poucos milissegundos — ver `results/`).

## Por que `k1`, `k2` e as demais constantes do algoritmo próprio viraram colunas no CSV bruto

Inicialmente o CSV de saída do simulador não incluía os hiperparâmetros do
algoritmo próprio (só apareciam implicitamente, via qual script os havia
gerado). Isso quebrou a análise de sensibilidade de `k1` (Seção 4 de
`docs/algoritmo_proprio.md`), porque três lotes de execuções com `k1`
diferentes ficavam indistinguíveis depois de concatenados em um único CSV
consolidado. Corrigimos incluindo `k1,k2` como colunas explícitas na saída
de todo run (`src/main.c`), não só nos runs de sensibilidade — melhora a
rastreabilidade de qualquer CSV bruto gerado no futuro, mesmo fora de um
experimento formal.

## Formato dos arquivos brutos: um CSV por (cenário, algoritmo, seed)

Como sugerido na árvore de diretórios do enunciado
(`results/raw/ # csv bruto por (cenário, algoritmo, seed)`),
`scripts/run_experiments.sh` grava um arquivo por execução individual em
`results/raw/main/`, `results/raw/complementary_csw0/` etc., e depois
concatena tudo em um único `all_runs_*.csv` por lote (usado pelos scripts
de análise). Isso deixa qualquer execução individual auditável/reexecutável
isoladamente (útil para depuração), ao custo de gerar um volume grande de
arquivos pequenos (dezenas de milhares na configuração ampliada de 1000
seeds/cenário) — por isso `results/raw/` está no `.gitignore` (mantemos
apenas os `all_runs_*.csv` consolidados e `results/consolidated/`
versionados), como o próprio enunciado sugere ("gitignored se grande").