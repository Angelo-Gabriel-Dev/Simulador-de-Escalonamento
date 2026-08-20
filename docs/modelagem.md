# Modelagem do simulador

Este documento descreve, de forma explícita, todas as escolhas de modelagem
exigidas pelo enunciado (Seção 4) e outras decisões de projeto que o
enunciado deixa em aberto. O objetivo é que qualquer pessoa consiga
reproduzir exatamente o comportamento do simulador sem precisar ler o
código-fonte.

## 1. Convenção de prioridade

**Menor valor numérico = maior prioridade** (mesma convenção do `nice` do
Unix). Um processo com `priority = 1` é atendido antes de um processo com
`priority = 10` no escalonamento por Prioridade e recebe peso favorável no
algoritmo próprio. Essa convenção é usada em **todos** os algoritmos e em
**todos** os cenários, sem exceção.

## 2. Modelo de processo e rajadas

Cada processo é gerado como uma sequência de rajadas `CPU -> E/S -> CPU ->
E/S -> ... -> CPU`. Por construção do gerador de cargas
(`workload_generator.c`):

- Toda rajada de CPU tem duração `>= 1`.
- Toda rajada de E/S (exceto a última do processo) tem duração `>= 1`.
- **A última rajada de cada processo nunca é seguida de E/S** (`io_time ==
  0`), ou seja, o processo termina imediatamente após concluir sua última
  rajada de CPU. Esta é a única situação em que `io_time == 0` ocorre.

O **tempo mínimo ideal** de um processo (usado no cálculo do slowdown) é a
soma de todas as suas rajadas de CPU e E/S: `min_ideal = sum(cpu_i) +
sum(io_i)`. Corresponde ao tempo que o processo levaria para terminar se
nunca esperasse na fila de prontos nem em fila de E/S, e se nunca houvesse
troca de contexto.

## 3. Modelagem de E/S (Seção 4.2 do enunciado)

- **Quantidade de dispositivos**: configurável via `--io-devices` (padrão
  = 1, seguindo a sugestão do enunciado).
- **Quando um processo solicita E/S**: imediatamente ao concluir uma
  rajada de CPU cuja rajada seguinte não é a última (`io_time > 0` para
  aquela rajada).
- **Fila por dispositivo**: cada dispositivo de E/S tem sua própria fila
  FIFO. **Não há paralelismo dentro de um mesmo dispositivo** (um processo
  por vez); dispositivos diferentes podem atender processos diferentes ao
  mesmo tempo.
- **Atribuição de dispositivo**: quando um processo solicita E/S, ele é
  atribuído ao dispositivo de **menor carga** no momento, onde carga =
  `(1 se ocupado) + tamanho da fila de espera`. Em caso de empate entre
  dois ou mais dispositivos, o desempate é round-robin (um ponteiro
  interno gira entre os dispositivos empatados a cada nova atribuição,
  para espalhar a carga ao longo do tempo).
- **Retorno à fila de prontos**: assim que a E/S termina, o processo volta
  imediatamente à fila de prontos do algoritmo de escalonamento ativo
  (chamando o mesmo `on_process_arrival` usado para chegadas novas — ver
  Seção 6 abaixo).
- Essa modelagem é **idêntica para todos os algoritmos** avaliados, como
  exigido.

### Calibração dos parâmetros de E/S por cenário

Os parâmetros de duração/frequência de E/S de cada cenário foram
calibrados para que a **utilização do dispositivo de E/S fique abaixo de
100%** (sistema estável). Na primeira tentativa de calibração do cenário
`io_bound`, usamos duração de E/S uniforme em `[5, 20]` com média de
chegada `20`; isso gerava utilização do único dispositivo de E/S de
aproximadamente **281%** (fila cresce sem limite, tempos de turnaround
artificialmente enormes e pouco informativos para comparar algoritmos).
Recalibramos para duração de E/S uniforme em `[3, 12]` e média de chegada
`45`, resultando em utilização aproximada de 75% do dispositivo de E/S e
37% da CPU — um cenário genuinamente dominado por E/S, porém estável. Ver
`config/scenarios.json` para os valores finais e `docs/decisoes_projeto.md`
para mais detalhes desse processo de calibração.

## 4. Modelagem de troca de contexto (Seção 4.3 do enunciado)

- **Quando ocorre**: toda vez que a CPU passa a executar um processo
  **diferente** do anterior, **incluindo a transição ociosa -> executando**.
- **Duração**: configurável via `--context-switch-cost` (padrão = 1 nos
  experimentos principais, sempre `> 0`; execuções complementares usam
  `= 0` para isolar o efeito da troca).
- **Disponibilidade da CPU durante a troca**: a CPU fica **totalmente
  indisponível** durante o intervalo de troca — nenhum processo executa
  nesse intervalo (modelado como um evento `EVENT_CONTEXT_SWITCH_DONE`
  entre o instante em que o despacho é decidido e o instante em que o
  processo de fato começa a rodar).
- **Mesmo processo redespachado sem a CPU ficar ociosa**: se, entre uma
  saída da CPU e a próxima decisão de despacho, **nenhum outro processo
  chegou a ocupar a CPU** (ex.: Round Robin com um único processo pronto,
  cujo quantum expira e ele mesmo é o próximo da fila), **não há troca de
  contexto**, pois não houve de fato "um processo diferente" ocupando a
  CPU nesse intervalo — a CPU nunca deixou de estar, na prática, dedicada
  àquele processo. Já se a CPU ficou genuinamente ociosa em algum momento
  (fila de prontos vazia), a próxima execução — mesmo que seja do mesmo
  pid de antes — é tratada como transição ociosa->executando e **conta**
  como troca, exatamente como pede o enunciado.
- **Mesmo custo para todos os algoritmos** dentro de um mesmo experimento,
  como exigido.

Implementação de referência: `src/context_switch.c` (comentários no
cabeçalho `context_switch.h` explicam o mecanismo do sentinela `CS_NONE`
usado para representar "CPU ociosa desde a última execução").

## 5. Modelo de chegada dos processos (Seção 5, observações)

O gerador de cargas (`workload_generator.c`) suporta quatro modelos de
chegada (`ArrivalModel`): `ARRIVAL_EXPONENTIAL`, `ARRIVAL_FIXED_INTERVAL`,
`ARRIVAL_BATCH` e `ARRIVAL_ALL_AT_ZERO`. **Os 4 cenários obrigatórios usam
`ARRIVAL_EXPONENTIAL`**: o primeiro processo chega em `t=0`; cada chegada
seguinte é `t[i] = t[i-1] + round(Exponencial(média))`, onde a média é
específica de cada cenário (ver `config/scenarios.json`). Escolhemos
chegada exponencial (intervalos aleatórios) — em vez de todos os processos
chegando em `t=0` — porque ela produz um regime de fila mais realista
(picos e vales de carga) e evita o caso degenerado de "todo mundo já está
pronto desde o início", que favoreceria artificialmente algoritmos capazes
de reordenar livremente uma fila grande logo no início (ver Seção 5,
observações, do enunciado: chegada em `t=0` só é permitida com
justificativa, e não é o padrão que usamos). O modelo `ARRIVAL_ALL_AT_ZERO`
existe no código para eventuais cenários adicionais/sensibilidade, mas
**não é usado em nenhum dos 4 cenários obrigatórios**.

O modelo de chegada é o mesmo para todos os algoritmos avaliados dentro de
um mesmo cenário, como exigido.

## 6. Ponto único de entrada na fila de prontos

O enunciado define o contrato dos algoritmos em termos de "chegada", mas
do ponto de vista de um algoritmo de escalonamento, uma chegada nova e um
retorno de E/S são o mesmo evento: *um processo passou a estar apto a usar
a CPU agora*. Por isso, `SchedulerAlgorithm.on_process_arrival` é chamado
nas duas situações (ver `src/scheduler.h`). Isso evita duplicar lógica de
fila em cada um dos 4 algoritmos e é a razão pela qual, por exemplo, o
FCFS ordena estritamente por "ordem de chegada **na fila de prontos**"
(que pode diferir da ordem de chegada original ao sistema, caso um
processo volte de E/S depois que outro, que chegou mais tarde, já esteja
pronto) — o próprio enunciado usa essa formulação exata na descrição do
FCFS.

## 7. Critérios de desempate

| Situação | Critério de desempate |
|---|---|
| Dois eventos com o mesmo `time` na fila de eventos | Ordem de inserção na fila (FIFO) — ver `event_queue.c`. Como todas as chegadas são pré-carregadas em ordem de pid (que é nao-decrescente em `arrival_time`), isso preserva "menor `arrival_time`, depois menor `pid`" para chegadas simultâneas, e prioriza eventos de chegada pré-carregados sobre eventos de E/S dinâmicos no mesmo instante. |
| FCFS: dois processos entram na fila de prontos ao mesmo tempo | Consequência direta do desempate acima (FIFO da própria fila de eventos). |
| Prioridade: prioridade estática igual | Menor `arrival_time`; empate residual, menor `pid`. |
| Algoritmo próprio: prioridade efetiva igual (raro, ponto flutuante) | Menor `ready_since` (entrou primeiro na fila de prontos); empate residual, menor `pid`. |

## 8. Não uso de informação futura

Nenhum algoritmo tem acesso a: (a) a duração real da rajada de CPU
**atual** antes de ela terminar, (b) chegadas futuras, ou (c) qualquer
dado computado após o instante de decisão `now`. O algoritmo próprio
estima a duração da próxima rajada via média móvel exponencial (EMA)
calculada **apenas sobre rajadas já concluídas do mesmo processo**
(`custom_algo_observe_completed_burst`, chamada somente depois que
`EVENT_CPU_BURST_DONE` já ocorreu de fato). Antes da primeira rajada
concluída de um processo, usa-se um valor de *fallback* configurável
(`--initial-burst-estimate`), nunca a duração real (ainda desconhecida) da
rajada em curso. Ver `tests/test_custom_algo.c` para um teste automatizado
dessa propriedade e `docs/algoritmo_proprio.md` para a discussão completa.

