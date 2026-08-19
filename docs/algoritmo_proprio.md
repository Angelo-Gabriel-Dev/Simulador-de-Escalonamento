# Algoritmo próprio: Prioridade Dinâmica com Estimativa de Rajada e Envelhecimento

## 1. Qual problema o algoritmo tenta resolver

Os dois algoritmos clássicos que usam informação de prioridade/tamanho de
rajada têm defeitos complementares nos nossos experimentos (ver
`report/` para os números completos):

- **Prioridade estática não preemptiva** produz o menor turnaround médio
  entre os algoritmos "razoáveis" (FCFS, Prioridade, algoritmo próprio),
  mas à custa de justiça: no cenário de prioridades desbalanceadas, o
  índice de Jain do slowdown fica entre **7% e 19%**, e o turnaround médio
  dos processos de prioridade baixa chega a ser **6,47x** maior que o dos
  processos de prioridade alta (946 vs. 146 unidades de tempo, médias de
  1000 seeds). Processos de baixa prioridade ficam sistematicamente para
  trás sempre que há processos de alta prioridade chegando.
- **FCFS** é muito mais justo (Jain 37–57%, razão baixa/alta prioridade
  ≈ 1,00x — FCFS literalmente ignora prioridade), mas paga por isso com
  turnaround médio pior que a Prioridade em quase todos os cenários
  (ex.: 305 vs. 267 no cenário equilibrado).

O algoritmo próprio tenta ocupar um ponto intermediário: **usar a
prioridade estática e uma estimativa de rajada (como a Prioridade faz),
mas sem permitir que processos de baixa prioridade fiquem esperando
indefinidamente** (o defeito da Prioridade pura), aplicando envelhecimento
(*aging*) sobre o tempo de espera.

## 2. Quais informações o algoritmo usa

Para cada processo pronto `p`, no instante `now`:

- `priority(p)`: prioridade estática do processo (dado do processo,
  conhecido desde a chegada).
- `waiting_time(p, now) = now - ready_since(p)`: há quanto tempo `p` está
  na fila de prontos *desde a última vez que entrou nela* (reinicia a cada
  retorno de E/S — ver `docs/modelagem.md`, Seção 6).
- `estimated_next_burst(p)`: uma média móvel exponencial (EMA) das
  durações das rajadas de CPU **já concluídas** de `p`. Antes da primeira
  rajada concluída, usa um valor de *fallback* configurável
  (`--initial-burst-estimate`, padrão 10.0).

**Nenhuma outra informação é usada** — em particular, nunca a duração real
da rajada atual (que só é conhecida quando ela termina) nem dados de
outros processos além do que já está na fila de prontos.

## 3. Como o próximo processo é escolhido

Não preemptivo: só decide quando a CPU fica livre. Escolhe o processo
pronto de **menor** prioridade efetiva:

```
prioridade_efetiva(p, now) = priority(p)
                              - k1 * waiting_time(p, now)
                              + k2 * estimated_next_burst(p)
```

(menor valor = executa primeiro, mesma convenção da prioridade estática).
Empate: menor `ready_since`, depois menor `pid` (ver `docs/modelagem.md`).

`k1` controla o quanto o envelhecimento "perdoa" prioridade baixa conforme
o tempo passa; `k2` controla o quanto uma rajada estimada curta (efeito
tipo SJF) adianta um processo na fila.

## 4. Calibração de k1 e k2 (por que os valores padrão são k1=0,05 e k2=0,3)

Esta seção documenta o processo de calibração, não só o resultado — é
importante para entender **por que** o algoritmo se comporta como se
comporta nos resultados do artigo.

### k1 (peso do envelhecimento)

Testamos k1 em `{0,5; 0,05; 0,005}` (k2 fixo em 0,3) nos 4 cenários, 1000
seeds cada (dados brutos em
`results/raw/all_runs_complementary_k1.csv`, consolidado em
`results/consolidated/complementary_k1_summary_long.csv`).

| k1 | Turnaround médio (equilibrado) | Observação |
|---|---|---|
| 0,5 | 305,90 ± 7,16 | praticamente igual ao FCFS (305,85 ± 7,15) |
| 0,05 | 305,76 ± 7,17 | Jain 37,90 ± 0,33 — padrão escolhido |
| 0,005 | 300,32 ± 7,20 | mais próximo do comportamento da Prioridade |

**k1 = 0,5 é forte demais**: os tempos médios de espera observados nesses
cenários (dezenas a centenas de unidades de tempo, já que o turnaround
médio de ~300 é bem maior que o tempo mínimo ideal de um processo típico)
fazem o termo `-k1 * waiting_time` dominar a fórmula quase imediatamente
— qualquer diferença de prioridade (no máximo 9, já que a faixa é 1–10) ou
de rajada estimada é irrelevante perto de `0,5 * 100 = 50`. Na prática, a
prioridade efetiva vira essencialmente "quem chegou primeiro na fila", e o
algoritmo **degenera para um comportamento estatisticamente indistinguível
do FCFS** (turnaround e Jain praticamente idênticos ao FCFS em todos os
cenários com carga significativa).

**k1 = 0,005 é fraco demais**: o envelhecimento mal influencia a decisão
a não ser depois de uma espera muito longa, então o comportamento se
aproxima mais do que a Prioridade estática produziria.

**k1 = 0,05** ficou como padrão por ser o ponto em que o algoritmo ainda
apresenta uma diferenciação clara por prioridade (ver Seção 5 abaixo),
sem reproduzir a discriminação extrema da Prioridade pura.

### k2 (peso da rajada estimada)

Testamos k2 em `{0,1; 0,3; 1,0; 3,0}` com k1=0,05 fixo, no cenário
equilibrado (seed única, varredura exploratória): turnaround variou
suavemente de 274,6 a 277,6 e Jain de 30,5% a 38,0% — sensibilidade bem
mais fraca que a de k1, sem comportamento degenerado em nenhum extremo
testado. Mantivemos k2 = 0,3 (contribuição da ordem de poucas unidades na
fórmula, dado que as rajadas estimadas nos cenários ficam tipicamente
entre 2 e 25 unidades de tempo).

## 5. O resultado mais importante: diferenciação por classe de prioridade

Com os parâmetros calibrados, o achado mais nítido do projeto (1000 seeds,
cenário de prioridades desbalanceadas, ver
`report/figuras/fig_priority_breakdown.png`):

| Algoritmo | Turnaround prioridade alta (≤3) | Turnaround prioridade baixa (≥8) | Razão baixa/alta |
|---|---|---|---|
| FCFS | 305,22 ± 6,92 | 304,68 ± 6,84 | **1,00x** (ignora prioridade) |
| Prioridade | 146,17 ± 1,32 | 946,25 ± 30,88 | **6,47x** (discriminação severa) |
| **Algoritmo próprio** | 270,04 ± 6,61 | 505,40 ± 9,05 | **1,87x** |

O algoritmo próprio **usa** a prioridade (diferentemente do FCFS: a razão
sai de 1,00x para 1,87x) sem reproduzir a quase-inanição que a Prioridade
estática impõe aos processos de baixa prioridade (a razão fica em 1,87x,
não 6,47x — os processos de baixa prioridade do algoritmo próprio terminam,
em média, **quase 2x mais rápido** que os da Prioridade pura: 505 vs. 946).
O mesmo padrão qualitativo se repete no cenário CPU-bound (que usa
prioridade uniforme, não desbalanceada) com custo de troca zero: FCFS
1,00x, Prioridade 4,92x, algoritmo próprio 1,67x — não é um artefato de um
único cenário.

## 6. Discussão honesta: o algoritmo melhora sobre FCFS?

Turnaround médio agregado (todos os processos, não só por classe) do
algoritmo próprio fica **estatisticamente indistinguível do FCFS** na
maioria dos cenários (ex.: equilibrado 305,76 ± 7,17 vs. 305,85 ± 7,15;
prioridades desbalanceadas 305,31 ± 6,90 vs. 305,13 ± 6,88), e sua justiça
(Jain) fica **abaixo** da do FCFS (ex.: 37,9% vs. 43,1% no equilibrado).
Ou seja: **na média agregada, o algoritmo próprio não domina o FCFS** — ele
entrega justiça parecida, mas ligeiramente pior, sem ganho de turnaround
agregado que compense.

Interpretamos isso como uma propriedade estrutural **dentro da família de
algoritmos não preemptivos avaliados** (FCFS, Prioridade e o algoritmo
próprio), não um bug: o FCFS, ordenando estritamente por tempo de espera,
maximiza a simetria entre esses três (é o próprio conceito que o índice de
Jain do slowdown premia). Qualquer regra que se desvie da ordenação pura
por tempo de espera para perseguir eficiência (prioridade estática, rajada
estimada) necessariamente reintroduz alguma assimetria dentro dessa
família.

**Essa leitura não se estende ao conjunto completo dos 4 algoritmos.** O
Round Robin — que é preemptivo — obtém um índice de Jain **maior** que o
do FCFS nos 4 cenários (ex.: 71,6% vs. 43,1% no cenário equilibrado; ver
`results/consolidated/main_summary_long.csv`), porque intercalar a
execução entre todos os processos prontos reduz por construção a
variância relativa de slowdown entre eles. Ou seja: o FCFS maximiza
simetria *entre os não preemptivos*, mas não é o algoritmo mais justo do
experimento como um todo — essa distinção importa e deve ser mantida
explícita em qualquer discussão do artigo ou dos slides.

Voltando à comparação relevante (algoritmo próprio vs. FCFS, ambos não
preemptivos): a pergunta interessante não é "o algoritmo bate o FCFS nas
duas métricas ao mesmo tempo" (não bate, com esta fórmula aditiva simples,
dentro da família não preemptiva), e sim **qual troca ele oferece frente
aos dois extremos clássicos**. Nesse recorte, o resultado é claro: o
algoritmo entrega quase toda a robustez do FCFS contra inanição (razão de
prioridade 1,87x, muito mais perto do FCFS que da Prioridade) enquanto
ainda usa o sinal de prioridade de forma visível — algo que nem FCFS nem
Prioridade pura fazem
ao mesmo tempo.

## 7. Limitações

- **Fórmula aditiva linear**: `k1` precisa ser recalibrado se a escala de
  tempo do sistema mudar (número de processos, duração média das rajadas,
  etc.), porque o termo de envelhecimento cresce sem limite com o tempo de
  espera. Uma versão futura poderia normalizar o envelhecimento pelo tempo
  mínimo ideal do próprio processo, ou usar uma função não-linear (ex.:
  logarítmica) para o termo de espera, evitando a necessidade de
  recalibrar `k1` para cada regime de carga. Fica como trabalho futuro;
  não implementamos essa variante aqui por restrição de tempo do projeto,
  mas o mesmo processo de calibração documentado na Seção 4 se aplicaria.
- **Não preemptivo**: uma rajada de CPU longa de baixa prioridade, uma vez
  iniciada, não é interrompida mesmo que um processo de prioridade muito
  alta chegue logo em seguida. Escolha deliberada (mantém o algoritmo
  diretamente comparável ao baseline não preemptivo de Prioridade,
  isolando o efeito do envelhecimento e da estimativa de rajada), mas
  significa que o algoritmo não resolve o problema clássico de inversão de
  prioridade em rajadas muito longas. Nenhum experimento com uma variante
  preemptiva foi executado.
- **Estimativa de rajada por processo, não por classe**: cada processo
  começa sem histórico (usa o valor de *fallback*); processos com poucas
  rajadas totais (ex.: cenário CPU-bound, 1–3 rajadas) mal chegam a ter
  uma estimativa própria "madura" antes de terminar.
- **k1 e k2 foram calibrados nos nossos 4 cenários**; não testamos
  robustez sob distribuições de carga muito diferentes (ex.: workloads
  bimodais de rajada, picos de chegada muito mais extremos).

## 8. Inspiração e originalidade

A combinação de envelhecimento com prioridade dinâmica é uma técnica
conhecida na literatura de sistemas operacionais (usada, por exemplo, para
evitar inanição em variantes de escalonamento por prioridade); a
estimativa de rajada via média móvel exponencial também é uma técnica
clássica (usada de forma similar em aproximações de SJN/SRTN). A
contribuição própria da equipe está na **combinação específica dos dois
mecanismos em uma única fórmula aditiva com dois pesos independentes
calibrados empiricamente**, e no diagnóstico de que, com essa fórmula
aditiva simples, não existe uma escolha de `k1` que supere o FCFS
simultaneamente em turnaround e justiça — o próprio ponto de calibração
escolhido (k1=0,05) e a análise de sensibilidade que o sustenta são a
contribuição de engenharia deste trabalho, não uma cópia direta de uma
implementação existente.
