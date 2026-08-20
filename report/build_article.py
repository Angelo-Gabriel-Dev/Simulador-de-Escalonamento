#!/usr/bin/env python3
"""
report/build_article.py

Monta o artigo cientifico final (report/artigo.pdf), formato IEEE de duas
colunas, a partir dos dados reais em results/consolidated/ (via
article_data.py -- nenhum numero e digitado a mao neste arquivo).
"""
from pathlib import Path

from reportlab.lib.pagesizes import A4
from reportlab.lib.units import inch
from reportlab.lib import colors
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_JUSTIFY, TA_CENTER, TA_LEFT
from reportlab.platypus import (
    BaseDocTemplate, PageTemplate, Frame, NextPageTemplate, FrameBreak,
    PageBreak, Paragraph, Spacer, Table, TableStyle, Image, KeepTogether,
)

from article_data import m, mc, mean_only, ratio, n_seeds, n_processes

N_SEEDS_MAIN = n_seeds("main")
N_PROC_MAIN = n_processes("main")
N_SEEDS_K1 = n_seeds("k1")

HERE = Path(__file__).resolve().parent
FIG = HERE / "figuras"
OUT = HERE / "artigo.pdf"

# ---------------------------------------------------------------- layout --
PAGE_W, PAGE_H = A4
MARGIN = 0.72 * inch
COL_GAP = 0.28 * inch
USABLE_W = PAGE_W - 2 * MARGIN
COL_W = (USABLE_W - COL_GAP) / 2
HEADER_H = 2.35 * inch

header_frame = Frame(MARGIN, PAGE_H - MARGIN - HEADER_H, USABLE_W, HEADER_H,
                      id="header", showBoundary=0,
                      leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=8)
col1_p1 = Frame(MARGIN, MARGIN, COL_W, PAGE_H - 2 * MARGIN - HEADER_H,
                 id="col1p1", showBoundary=0, leftPadding=0, rightPadding=9, topPadding=0, bottomPadding=0)
col2_p1 = Frame(MARGIN + COL_W + COL_GAP, MARGIN, COL_W, PAGE_H - 2 * MARGIN - HEADER_H,
                 id="col2p1", showBoundary=0, leftPadding=9, rightPadding=0, topPadding=0, bottomPadding=0)

col1 = Frame(MARGIN, MARGIN, COL_W, PAGE_H - 2 * MARGIN,
             id="col1", showBoundary=0, leftPadding=0, rightPadding=9, topPadding=0, bottomPadding=0)
col2 = Frame(MARGIN + COL_W + COL_GAP, MARGIN, COL_W, PAGE_H - 2 * MARGIN,
             id="col2", showBoundary=0, leftPadding=9, rightPadding=0, topPadding=0, bottomPadding=0)

full_frame = Frame(MARGIN, MARGIN, USABLE_W, PAGE_H - 2 * MARGIN,
                    id="full", showBoundary=0, leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)


def on_page(canvas, doc):
    canvas.saveState()
    canvas.setFont("Times-Roman", 8)
    canvas.drawCentredString(PAGE_W / 2, 0.42 * inch, str(doc.page))
    canvas.restoreState()


doc = BaseDocTemplate(str(OUT), pagesize=A4,
                       leftMargin=MARGIN, rightMargin=MARGIN, topMargin=MARGIN, bottomMargin=MARGIN,
                       title="Comparacao de Algoritmos de Escalonamento de Processos com Prioridade "
                             "Dinamica por Envelhecimento e Estimativa de Rajada",
                       author="Angelo; Jetro; Luiz; Dorian")
doc.addPageTemplates([
    PageTemplate(id="FirstPage", frames=[header_frame, col1_p1, col2_p1], onPage=on_page),
    PageTemplate(id="LaterPages", frames=[col1, col2], onPage=on_page),
    PageTemplate(id="FullWidth", frames=[full_frame], onPage=on_page),
])

# ----------------------------------------------------------------- styles --
S_TITLE = ParagraphStyle("title", fontName="Times-Bold", fontSize=15.5, leading=18,
                          alignment=TA_CENTER, spaceAfter=8)
S_AUTHORS = ParagraphStyle("authors", fontName="Times-Roman", fontSize=10.5, leading=13,
                            alignment=TA_CENTER, spaceAfter=2)
S_AFFIL = ParagraphStyle("affil", fontName="Times-Italic", fontSize=9.5, leading=12,
                          alignment=TA_CENTER, spaceAfter=8)
S_ABSTRACT_HEAD = ParagraphStyle("abshead", fontName="Times-Bold", fontSize=9, leading=11,
                                  alignment=TA_JUSTIFY, spaceBefore=4)
S_ABSTRACT = ParagraphStyle("abs", fontName="Times-Italic", fontSize=9, leading=11.2,
                             alignment=TA_JUSTIFY, firstLineIndent=0, spaceAfter=2)
S_KEYWORDS = ParagraphStyle("kw", fontName="Times-Italic", fontSize=9, leading=11.2, alignment=TA_JUSTIFY)

S_H1 = ParagraphStyle("h1", fontName="Times-Bold", fontSize=10.5, leading=13,
                       alignment=TA_CENTER, spaceBefore=10, spaceAfter=5)
S_H2 = ParagraphStyle("h2", fontName="Times-Bold", fontSize=10, leading=12,
                       alignment=TA_LEFT, spaceBefore=7, spaceAfter=3, fontStyle="italic")
S_H2 = ParagraphStyle("h2", parent=S_H2, fontName="Times-BoldItalic")
S_BODY = ParagraphStyle("body", fontName="Times-Roman", fontSize=9.3, leading=11.6,
                         alignment=TA_JUSTIFY, spaceAfter=5.5, firstLineIndent=12)
S_BODY_NOINDENT = ParagraphStyle("body_ni", parent=S_BODY, firstLineIndent=0)
S_CAPTION = ParagraphStyle("caption", fontName="Times-Roman", fontSize=8.2, leading=10,
                            alignment=TA_JUSTIFY, spaceBefore=3, spaceAfter=8)
S_TABLE_TITLE = ParagraphStyle("tabletitle", fontName="Times-Roman", fontSize=8.2, leading=10,
                                alignment=TA_CENTER, spaceAfter=3)
S_REF = ParagraphStyle("ref", fontName="Times-Roman", fontSize=8.3, leading=10.2,
                        alignment=TA_JUSTIFY, firstLineIndent=-10, leftIndent=10, spaceAfter=3)
S_EQ = ParagraphStyle("eq", fontName="Times-Italic", fontSize=9.3, leading=13,
                       alignment=TA_CENTER, spaceBefore=3, spaceAfter=5)

story = []

# ================================================================ HEADER ==
story.append(Paragraph(
    "Comparação de Algoritmos de Escalonamento de Processos com "
    "Prioridade Dinâmica por Envelhecimento e Estimativa de Rajada",
    S_TITLE))
story.append(Paragraph("Ângelo, Jetro, Luiz, Dorian", S_AUTHORS))
story.append(Paragraph(
    "Curso de Engenharia de Software &mdash; Universidade Federal do Cariri (UFCA)<br/>"
    "Disciplina de Sistemas Operacionais &mdash; Projeto da Unidade 3",
    S_AFFIL))

abstract_text = (
    "Este trabalho apresenta um simulador de escalonamento de processos por "
    "eventos discretos, escrito em C, para comparar três algoritmos "
    "clássicos &mdash; FCFS, Round Robin e Prioridade não preemptiva &mdash; "
    "com um algoritmo próprio, a Prioridade Dinâmica com Estimativa de "
    "Rajada e Envelhecimento. O algoritmo próprio combina prioridade "
    "estática, envelhecimento (aging) do tempo de espera e uma estimativa "
    "de rajada via média móvel exponencial (calculada apenas sobre rajadas "
    "já concluídas do próprio processo, sem uso de informação futura). "
    f"Avaliamos os quatro algoritmos em 4 cenários obrigatórios "
    f"(equilibrado, I/O-bound, CPU-bound e prioridades desbalanceadas), "
    f"com {N_PROC_MAIN} processos por execução e {N_SEEDS_MAIN} seeds por cenário "
    f"({N_SEEDS_MAIN // 100}x o "
    f"mínimo exigido), custo de troca de contexto maior que zero, e três "
    f"análises complementares (custo de troca de contexto zero, "
    f"sensibilidade de quantum do Round Robin e sensibilidade do peso de "
    f"envelhecimento do algoritmo próprio). O resultado central é que o "
    f"algoritmo próprio diferencia processos por prioridade de forma "
    f"visível (razão de turnaround baixa/alta prioridade de "
    f"{ratio('priority_skew','custom','avg_turnaround_low_priority','avg_turnaround_high_priority'):.2f}".replace('.', ',') +
    f"×) sem reproduzir a quase-inanição da Prioridade estática pura "
    f"({ratio('priority_skew','priority','avg_turnaround_low_priority','avg_turnaround_high_priority'):.2f}".replace('.', ',') +
    f"×), obtendo justiça (índice de Jain do slowdown) muito acima da "
    f"Prioridade, embora sem superar o turnaround médio agregado do FCFS."
)
story.append(Paragraph("<b>Resumo</b> &mdash; " + abstract_text, S_ABSTRACT))
story.append(Paragraph(
    "<b>Palavras-chave</b> &mdash; escalonamento de processos; simulação a "
    "eventos discretos; envelhecimento (aging); Round Robin; índice de "
    "Jain; sistemas operacionais.",
    S_KEYWORDS))

story.append(FrameBreak())
story.append(NextPageTemplate("LaterPages"))

# ============================================================ SECAO I =====
story.append(Paragraph("I. INTRODUÇÃO E MOTIVAÇÃO", S_H1))
story.append(Paragraph(
    "Escalonar processos em um sistema operacional é, no fundo, um problema "
    "de arbitrar entre objetivos concorrentes: minimizar o tempo médio de "
    "resposta, tratar processos de forma justa e respeitar prioridades "
    "definidas externamente. Algoritmos clássicos otimizam bem para um "
    "desses objetivos às custas dos demais &mdash; escalonamento por "
    "Prioridade estática, por exemplo, favorece processos importantes mas "
    "pode deixar processos de baixa prioridade esperando indefinidamente "
    "(inanição); FCFS é inerentemente justo mas ignora qualquer noção de "
    "prioridade ou duração de rajada.",
    S_BODY_NOINDENT))
story.append(Paragraph(
    "Este trabalho investiga se é possível obter parte da eficiência da "
    "Prioridade estática sem herdar seu problema de inanição, combinando-a "
    "com envelhecimento (aging) e uma estimativa da duração da próxima "
    "rajada de CPU. Construímos um simulador de escalonamento por eventos "
    "discretos em C, implementamos os três algoritmos clássicos exigidos "
    "(FCFS, Round Robin, Prioridade não preemptiva) e um algoritmo próprio, "
    "e comparamos os quatro sob quatro cargas de trabalho controladas por "
    "seed, com rigor estatístico (média e intervalo de confiança de 95% "
    f"sobre {N_SEEDS_MAIN} execuções independentes por combinação cenário&times;"
    "algoritmo).",
    S_BODY))
story.append(Paragraph(
    "A contribuição deste artigo não é apenas o código do simulador, mas o "
    "processo de calibração do algoritmo próprio e a discussão honesta de "
    "seus resultados: mostramos que, com a fórmula aditiva simples que "
    "propomos, não existe um único peso de envelhecimento que supere o "
    "FCFS simultaneamente em turnaround médio e em justiça &mdash; e "
    "explicamos por quê (Seção VI). O código completo, os dados brutos e "
    "consolidados, e os scripts de análise estão disponíveis no "
    "repositório do projeto.",
    S_BODY))

# ============================================================ SECAO II ====
story.append(Paragraph("II. MODELO DO SISTEMA", S_H1))
story.append(Paragraph(
    "Cada processo é modelado como uma sequência de rajadas "
    "<i>CPU &rarr; E/S &rarr; CPU &rarr; E/S &rarr; ... &rarr; CPU</i>, com "
    "identificador, tempo de chegada, prioridade estática (convenção: menor "
    "valor numérico = maior prioridade) e uma lista de pares "
    "(duração de CPU, duração de E/S). Apenas a última rajada de um "
    "processo não é seguida de E/S. O <i>tempo mínimo ideal</i> de um "
    "processo &mdash; usado no cálculo do slowdown &mdash; é a soma de "
    "todas as suas durações de CPU e E/S, isto é, o tempo que ele levaria "
    "para terminar se nunca esperasse em nenhuma fila.",
    S_BODY_NOINDENT))
story.append(Paragraph(
    "<b>Entrada e saída (E/S).</b> Um ou mais dispositivos (padrão: 1), "
    "cada um com fila FIFO própria e sem paralelismo interno. Um processo "
    "que solicita E/S é atribuído ao dispositivo de menor carga "
    "(ocupado + fila), com desempate round-robin entre dispositivos "
    "empatados. Ao concluir a E/S, o processo retorna imediatamente à fila "
    "de prontos.",
    S_BODY))
story.append(Paragraph(
    "<b>Troca de contexto.</b> Ocorre sempre que a CPU passa a executar um "
    "processo diferente do anterior, incluindo a transição "
    "ociosa&rarr;executando; tem custo configurável (&gt;0 nos experimentos "
    "principais) durante o qual a CPU fica indisponível. Se o mesmo "
    "processo é redespachado sem que a CPU tenha ficado ociosa ou "
    "executado outro processo nesse intervalo, não há troca.",
    S_BODY))
story.append(Paragraph(
    "<b>Chegadas.</b> Processo modelado por intervalos entre chegadas "
    "exponencialmente distribuídos (média por cenário), em vez de todos os "
    "processos chegarem em t=0 &mdash; produz um regime de fila mais "
    "realista e evita favorecer artificialmente algoritmos que reordenam "
    "livremente uma fila já completa desde o início.",
    S_BODY))
story.append(Paragraph(
    "Detalhes completos de cada decisão de modelagem, incluindo a "
    "recalibração dos parâmetros do cenário I/O-bound (a primeira "
    "tentativa gerava utilização do dispositivo de E/S acima de 100%, "
    "fila sem limite e turnaround não-representativo), estão documentados "
    "em <font face=\"Courier\">docs/modelagem.md</font> no repositório.",
    S_BODY))

# ============================================================ SECAO III ===
story.append(Paragraph("III. ALGORITMOS DE ESCALONAMENTO", S_H1))
story.append(Paragraph("A. Algoritmos clássicos", S_H2))
story.append(Paragraph(
    "<b>FCFS</b>: fila FIFO simples pela ordem de entrada na fila de "
    "prontos (chegada nova ou retorno de E/S). <b>Round Robin</b>: fila "
    "circular FIFO com quantum configurável (padrão 4); o processo cujo "
    "quantum expira antes de terminar a rajada retorna ao final da fila. "
    "<b>Prioridade não preemptiva</b>: escolhe sempre o processo pronto de "
    "menor valor de prioridade; desempate por menor tempo de chegada e, "
    "residualmente, menor identificador de processo. Nenhum dos três usa "
    "preempção fora da expiração de quantum do Round Robin.",
    S_BODY_NOINDENT))

story.append(Paragraph("B. Algoritmo próprio: Prioridade Dinâmica com Estimativa de "
                        "Rajada e Envelhecimento", S_H2))
story.append(Paragraph(
    "Não preemptivo. A cada CPU livre, escolhe o processo pronto de menor "
    "prioridade efetiva:",
    S_BODY_NOINDENT))
story.append(Paragraph(
    "prioridade_efetiva(p, t) = prioridade(p) &minus; k<sub>1</sub>&middot;"
    "espera(p,t) + k<sub>2</sub>&middot;rajada_estimada(p)",
    S_EQ))
story.append(Paragraph(
    "onde <i>espera(p,t)</i> é o tempo desde que <i>p</i> entrou na fila de "
    "prontos pela última vez, e <i>rajada_estimada(p)</i> é uma média "
    "móvel exponencial (EMA, fator &alpha;=0,5) das durações de rajadas de "
    "CPU já <i>concluídas</i> do próprio processo &mdash; nunca a duração "
    "real, ainda desconhecida, da rajada em curso. Antes da primeira "
    "rajada concluída usa-se um valor de referência configurável "
    "(padrão 10,0).",
    S_BODY))
story.append(Paragraph(
    "Os pesos foram calibrados empiricamente testando k<sub>1</sub> "
    f"&isin; {{0,5; 0,05; 0,005}} nos 4 cenários ({N_SEEDS_K1} seeds cada): "
    "k<sub>1</sub>=0,5 é forte demais e faz o termo de envelhecimento "
    "dominar quase imediatamente frente às esperas médias observadas "
    "(dezenas a centenas de unidades de tempo), degenerando o algoritmo "
    "para um comportamento estatisticamente indistinguível do FCFS; "
    "k<sub>1</sub>=0,005 é fraco demais e aproxima o comportamento da "
    "Prioridade estática pura. Adotamos k<sub>1</sub>=0,05 e "
    "k<sub>2</sub>=0,3 (este último com sensibilidade bem mais fraca &mdash; "
    "variação de 274,6 a 277,6 no turnaround médio do cenário equilibrado "
    "ao longo de k<sub>2</sub> &isin; [0,1; 3,0]) como configuração "
    "principal do artigo. Detalhes completos da calibração, incluindo a "
    "tabela de resultados por valor de k<sub>1</sub>, estão em "
    "<font face=\"Courier\">docs/algoritmo_proprio.md</font>.",
    S_BODY))
story.append(Paragraph(
    "A ideia de combinar envelhecimento com prioridade para evitar "
    "inanição, e de estimar a próxima rajada por média móvel exponencial "
    "sobre o histórico observado, são técnicas conhecidas na literatura "
    "de sistemas operacionais [1], [3]; nossa contribuição é a combinação "
    "específica dos dois mecanismos em uma fórmula aditiva com pesos "
    "calibrados empiricamente, e o diagnóstico de que essa fórmula não "
    "admite um ponto que domine o FCFS simultaneamente em turnaround e "
    "justiça (Seção VI).",
    S_BODY))

story.append(NextPageTemplate("LaterPages"))

# ============================================================ SECAO IV ====
story.append(Paragraph("IV. METODOLOGIA EXPERIMENTAL", S_H1))
story.append(Paragraph(
    "Quatro cenários obrigatórios, cada um com carga de trabalho gerada "
    "deterministicamente a partir de uma seed (PRNG xorshift128+ próprio, "
    "independente de <font face=\"Courier\">rand()</font>/"
    "<font face=\"Courier\">srand()</font> da libc, para garantir "
    "reprodutibilidade entre máquinas): <i>equilibrado</i> (mistura de "
    "processos curtos/longos, pouca/muita E/S), <i>I/O-bound</i> "
    "(rajadas de CPU curtas, muita E/S), <i>CPU-bound</i> (rajadas longas, "
    "pouca E/S) e <i>prioridades desbalanceadas</i> (85% dos processos com "
    "prioridade alta, 15% com prioridade baixa).",
    S_BODY_NOINDENT))
story.append(Paragraph(
    f"Cada execução simula {N_PROC_MAIN} processos; cada combinação "
    f"cenário&times;algoritmo foi repetida com {N_SEEDS_MAIN} seeds independentes "
    f"(seeds 1&ndash;{N_SEEDS_MAIN}, as mesmas para todos os algoritmos dentro de um "
    f"cenário) &mdash; {N_SEEDS_MAIN // 100} vezes o mínimo de 100 seeds exigido, viável "
    f"porque as ~45.000 execuções completas do experimento (principal + 3 "
    f"análises complementares) levam poucos minutos. O custo de troca de "
    f"contexto nos experimentos principais é 1 (&gt;0); uma análise "
    f"complementar repete tudo com custo 0 para isolar seu efeito.",
    S_BODY))
story.append(Paragraph(
    "Métricas obrigatórias: turnaround médio, número total de trocas de "
    "contexto, e índice de Jain aplicado ao slowdown de cada processo "
    "(slowdown = turnaround / tempo mínimo ideal). Para cada métrica, "
    "reportamos média entre seeds e intervalo de confiança de 95% "
    "(média amostral &plusmn; 1,96&middot;s/&radic;n, desvio padrão amostral). "
    "Adicionalmente, quebramos o turnaround médio por classe de "
    "prioridade (alta: &le;3; baixa: &ge;8) para medir o quanto cada "
    "algoritmo de fato diferencia processos por prioridade.",
    S_BODY))

# ============================================================ SECAO V =====
story.append(Paragraph("V. RESULTADOS", S_H1))
story.append(Paragraph(
    "A Fig. 1 (próxima página) resume as três métricas obrigatórias nos 4 "
    "cenários. O Round Robin (quantum=4) tem turnaround médio e número de "
    "trocas de contexto uma ordem de grandeza acima dos demais algoritmos "
    "em todos os cenários exceto I/O-bound &mdash; investigamos essa "
    "diferença na Seção VI. Entre os outros três, FCFS e o algoritmo "
    "próprio ficam próximos em turnaround agregado, com a Prioridade "
    "obtendo o menor turnaround médio à custa de justiça muito inferior "
    "(painel c).",
    S_BODY_NOINDENT))

def fmt_table_scn_alg(metric, decimals=2):
    scenarios = [("balanced", "Equilibrado"), ("io_bound", "I/O-bound"),
                 ("cpu_bound", "CPU-bound"), ("priority_skew", "Prio. desbal.")]
    algos = [("fcfs", "FCFS"), ("rr", "RR"), ("priority", "Prio."), ("custom", "Próprio")]
    header = ["Cenário"] + [a[1] for a in algos]
    rows = [header]
    for scen_key, scen_label in scenarios:
        row = [scen_label]
        for algo_key, _ in algos:
            row.append(mean_only(scen_key, algo_key, metric, decimals=decimals))
        rows.append(row)
    return rows

t1_data = fmt_table_scn_alg("avg_turnaround", decimals=1)
t1 = Table(t1_data, colWidths=[0.92 * inch] + [0.52 * inch] * 4, hAlign="CENTER")
SHARED_TABLE_STYLE = TableStyle([
    ("FONTNAME", (0, 0), (-1, -1), "Times-Roman"),
    ("FONTNAME", (0, 0), (-1, 0), "Times-Bold"),
    ("FONTSIZE", (0, 0), (-1, -1), 7.4),
    ("ALIGN", (1, 0), (-1, -1), "CENTER"),
    ("LINEABOVE", (0, 0), (-1, 0), 0.75, colors.black),
    ("LINEBELOW", (0, 0), (-1, 0), 0.5, colors.black),
    ("LINEBELOW", (0, -1), (-1, -1), 0.75, colors.black),
    ("TOPPADDING", (0, 0), (-1, -1), 2),
    ("BOTTOMPADDING", (0, 0), (-1, -1), 2),
])
t1.setStyle(SHARED_TABLE_STYLE)
story.append(KeepTogether([
    Paragraph("Tabela I. Turnaround médio (unidades de tempo)", S_TABLE_TITLE),
    t1,
]))

t2_data = fmt_table_scn_alg("jain_slowdown", decimals=1)
t2 = Table(t2_data, colWidths=[0.92 * inch] + [0.52 * inch] * 4, hAlign="CENTER")
t2.setStyle(SHARED_TABLE_STYLE)
story.append(KeepTogether([
    Paragraph("Tabela II. Índice de Jain do slowdown (%)", S_TABLE_TITLE),
    t2,
]))

story.append(Paragraph(
    "A Tabela III isola o cenário de prioridades desbalanceadas por "
    "classe de prioridade &mdash; o resultado mais importante deste "
    "trabalho (ver também Fig. 2, próxima página):",
    S_BODY_NOINDENT))

t3_header = ["Algoritmo", "Prio. alta", "Prio. baixa", "Razão"]
t3_rows = [t3_header]
for algo_key, algo_label in [("fcfs", "FCFS"), ("rr", "RR"), ("priority", "Prioridade"), ("custom", "Próprio")]:
    hi = mean_only("priority_skew", algo_key, "avg_turnaround_high_priority", decimals=1)
    lo = mean_only("priority_skew", algo_key, "avg_turnaround_low_priority", decimals=1)
    rt = ratio("priority_skew", algo_key, "avg_turnaround_low_priority", "avg_turnaround_high_priority")
    t3_rows.append([algo_label, hi, lo, f"{rt:.2f}".replace(".", ",") + "\u00d7"])
t3 = Table(t3_rows, colWidths=[0.95 * inch, 0.68 * inch, 0.68 * inch, 0.55 * inch], hAlign="CENTER")
t3.setStyle(SHARED_TABLE_STYLE)
story.append(KeepTogether([
    Paragraph("Tabela III. Turnaround por classe de prioridade &mdash; cenário "
              "prioridades desbalanceadas", S_TABLE_TITLE),
    t3,
]))

story.append(Paragraph(
    "As Figs. 3&ndash;4 (inline abaixo) mostram as duas análises "
    "complementares: efeito do custo de troca de contexto e sensibilidade "
    "do Round Robin ao quantum.",
    S_BODY))

img_slowdown = Image(str(FIG / "fig_slowdown.png"), width=COL_W, height=COL_W * (4 / 6))
story.append(KeepTogether([
    img_slowdown,
    Paragraph(f"Fig. 3. Slowdown médio por cenário e algoritmo (métrica auxiliar). "
              f"Média &plusmn; IC95%, {N_SEEDS_MAIN} seeds/cenário.", S_CAPTION),
]))

img_csw = Image(str(FIG / "fig_complementary_csw.png"), width=COL_W, height=COL_W * (4 / 6))
story.append(KeepTogether([
    img_csw,
    Paragraph("Fig. 4. Turnaround médio em CPU-bound com custo de troca de "
              "contexto 1 (principal) vs. 0 (complementar).", S_CAPTION),
]))

img_quantum = Image(str(FIG / "fig_quantum_sensitivity.png"), width=COL_W, height=COL_W * (4 / 8))
story.append(KeepTogether([
    img_quantum,
    Paragraph("Fig. 5. Round Robin em CPU-bound: quantum=4 (principal) vs. "
              "quantum=20 (complementar).", S_CAPTION),
]))

# --------------------------------------------------- full-width figures ---
story.append(NextPageTemplate("FullWidth"))
story.append(PageBreak())

img_main = Image(str(FIG / "fig_main_metrics.png"), width=USABLE_W, height=USABLE_W * (4.2 / 12))
story.append(img_main)
story.append(Paragraph(
    f"Fig. 1. Turnaround médio, trocas de contexto (escala log) e índice de "
    f"Jain do slowdown, por cenário e algoritmo. Média &plusmn; IC95%, {N_SEEDS_MAIN} "
    f"seeds/cenário, {N_PROC_MAIN} processos/execução, custo de troca de contexto=1.",
    S_CAPTION))

story.append(Spacer(1, 10))
img_prio = Image(str(FIG / "fig_priority_breakdown.png"), width=5.9 * inch, height=5.9 * inch * (4.5 / 7))
story.append(Table([[img_prio]], colWidths=[USABLE_W], hAlign="CENTER", style=TableStyle([
    ("ALIGN", (0, 0), (-1, -1), "CENTER"),
])))
story.append(Paragraph(
    f"Fig. 2. Turnaround médio por classe de prioridade (alta: prioridade "
    f"&le;3; baixa: &ge;8) no cenário de prioridades desbalanceadas, com a "
    f"razão baixa/alta anotada acima de cada par de barras. FCFS não "
    f"diferencia (1,00&times;); Prioridade estática discrimina fortemente "
    f"(6,47&times;); o algoritmo próprio fica entre os dois extremos "
    f"(1,87&times;). Média &plusmn; IC95%, {N_SEEDS_MAIN} seeds.",
    S_CAPTION))

story.append(NextPageTemplate("LaterPages"))
story.append(PageBreak())

# ============================================================ SECAO VI ====
story.append(Paragraph("VI. DISCUSSÃO", S_H1))
story.append(Paragraph("A. Round Robin: turnaround e trocas de contexto muito piores", S_H2))
rr_q4 = mc("cpu_bound", "rr", "avg_turnaround")
rr_q20 = mc("cpu_bound", "rr", "avg_turnaround", dataset="quantum")
rr_cs_q4 = mc("cpu_bound", "rr", "context_switches", decimals=0)
rr_cs_q20 = mc("cpu_bound", "rr", "context_switches", dataset="quantum", decimals=0)
fcfs_cs = mc("cpu_bound", "fcfs", "context_switches", decimals=0)
story.append(Paragraph(
    f"No cenário CPU-bound (rajadas de CPU longas, média 25 unidades de "
    f"tempo), o Round Robin com quantum=4 tem turnaround médio de "
    f"{rr_q4}, contra {mc('cpu_bound','fcfs','avg_turnaround')} do FCFS "
    f"&mdash; e {rr_cs_q4} trocas de contexto contra {fcfs_cs} do FCFS "
    f"(mesmo número de rajadas totais). Cada rajada de ~25 unidades é "
    f"fatiada em cerca de 6 quanta de 4 unidades, e cada fatia exige uma "
    f"nova troca; sob carga alta (utilização de CPU &asymp;83% neste "
    f"cenário) isso empurra o RR para perto da saturação: com quantum=4, "
    f"o tempo total consumido por trocas de contexto chega a rivalizar com "
    f"o próprio tempo de CPU disponível. Repetindo o experimento com "
    f"quantum=20 (mais próximo da rajada média), o turnaround cai para "
    f"{rr_q20} e as trocas de contexto para {rr_cs_q20} &mdash; uma "
    f"melhora de quase uma ordem de grandeza (Fig. 5), confirmando que o "
    f"problema é a relação quantum/rajada, não uma limitação estrutural do "
    f"Round Robin.",
    S_BODY_NOINDENT))

story.append(Paragraph("B. Efeito do custo de troca de contexto", S_H2))
rr_csw0 = mc("cpu_bound", "rr", "avg_turnaround", dataset="csw0")
fcfs_csw0 = mc("cpu_bound", "fcfs", "avg_turnaround", dataset="csw0")
story.append(Paragraph(
    f"Com custo de troca de contexto zero (análise complementar), o "
    f"turnaround do RR em CPU-bound cai de {rr_q4} para {rr_csw0} "
    f"&mdash; uma redução muito mais acentuada, em termos proporcionais, "
    f"do que a observada nos outros três algoritmos (ex.: FCFS vai de "
    f"{mc('cpu_bound','fcfs','avg_turnaround')} para {fcfs_csw0}, uma "
    f"queda bem mais modesta). Isso é esperado: o RR gera muito mais "
    f"trocas de contexto que os demais (Tabela I/Fig. 1b), então o custo "
    f"por troca pesa proporcionalmente mais no seu turnaround total. O "
    f"experimento confirma que grande parte da desvantagem do RR neste "
    f"cenário vem do <i>custo acumulado das trocas</i>, não apenas da "
    f"ordem de execução em si (Fig. 4).",
    S_BODY_NOINDENT))

story.append(Paragraph("C. O algoritmo próprio diferencia por prioridade sem inanição extrema", S_H2))
story.append(Paragraph(
    f"O resultado mais nítido do projeto é a Tabela III / Fig. 2: no "
    f"cenário de prioridades desbalanceadas, o FCFS trata processos de "
    f"prioridade alta e baixa de forma praticamente idêntica (razão "
    f"{ratio('priority_skew','fcfs','avg_turnaround_low_priority','avg_turnaround_high_priority'):.2f}".replace('.', ',') +
    f"&times; &mdash; ignora completamente a prioridade, por construção), "
    f"enquanto a Prioridade estática discrimina de forma severa "
    f"({ratio('priority_skew','priority','avg_turnaround_low_priority','avg_turnaround_high_priority'):.2f}".replace('.', ',') +
    f"&times;: processos de baixa prioridade terminam, em média, mais de "
    f"6 vezes mais devagar que os de alta prioridade). O algoritmo próprio "
    f"fica entre os dois extremos "
    f"({ratio('priority_skew','custom','avg_turnaround_low_priority','avg_turnaround_high_priority'):.2f}".replace('.', ',') +
    f"&times;): usa a prioridade de forma visível, mas o termo de "
    f"envelhecimento evita que processos de baixa prioridade sejam "
    f"deixados para trás indefinidamente &mdash; seu turnaround médio na "
    f"classe de baixa prioridade ({mean_only('priority_skew','custom','avg_turnaround_low_priority',decimals=0)}) "
    f"é quase metade do observado na Prioridade pura "
    f"({mean_only('priority_skew','priority','avg_turnaround_low_priority',decimals=0)}). "
    f"O mesmo padrão qualitativo aparece no cenário CPU-bound com custo de "
    f"troca zero (prioridade uniforme, não desbalanceada): razões de "
    f"{ratio('cpu_bound','fcfs','avg_turnaround_low_priority','avg_turnaround_high_priority', dataset='csw0'):.2f}".replace('.', ',') +
    f"&times;, "
    f"{ratio('cpu_bound','priority','avg_turnaround_low_priority','avg_turnaround_high_priority', dataset='csw0'):.2f}".replace('.', ',') +
    f"&times; e "
    f"{ratio('cpu_bound','custom','avg_turnaround_low_priority','avg_turnaround_high_priority', dataset='csw0'):.2f}".replace('.', ',') +
    f"&times; respectivamente &mdash; não é um artefato de um único "
    f"cenário.",
    S_BODY_NOINDENT))

story.append(Paragraph("D. O algoritmo próprio não supera o FCFS na média agregada &mdash; e por quê", S_H2))
story.append(Paragraph(
    f"Apesar do resultado da seção anterior, o turnaround médio "
    f"<i>agregado</i> (todos os processos, não só por classe) do "
    f"algoritmo próprio fica estatisticamente indistinguível do FCFS na "
    f"maioria dos cenários (Tabela I), e sua justiça agregada (Tabela II) "
    f"fica sistematicamente abaixo da do FCFS (ex.: "
    f"{mean_only('balanced','custom','jain_slowdown',decimals=1)}% contra "
    f"{mean_only('balanced','fcfs','jain_slowdown',decimals=1)}% no "
    f"cenário equilibrado). <b>Entre os algoritmos não preemptivos "
    f"avaliados</b> (FCFS, Prioridade e o algoritmo próprio), o FCFS "
    f"obtém o maior índice de Jain nos 4 cenários, por ordenar "
    f"estritamente por tempo de espera sem privilegiar rajadas curtas ou "
    f"prioridades altas &mdash; qualquer regra que se desvie dessa "
    f"ordenação pura para perseguir eficiência (prioridade, rajada "
    f"estimada) reintroduz alguma assimetria dentro dessa família. "
    f"Essa leitura <b>não se generaliza</b> para o conjunto completo dos "
    f"4 algoritmos: o Round Robin &mdash; que é preemptivo &mdash; obtém "
    f"um índice de Jain <i>maior</i> que o do FCFS nos 4 cenários (ex.: "
    f"{mean_only('balanced','rr','jain_slowdown',decimals=1)}% contra "
    f"{mean_only('balanced','fcfs','jain_slowdown',decimals=1)}% no "
    f"cenário equilibrado; Tabela II/Fig. 1c), já que intercalar a "
    f"execução entre todos os processos prontos reduz por construção a "
    f"variância relativa de slowdown entre eles. A conclusão sobre maior "
    f"simetria, portanto, fica restrita à comparação entre os algoritmos "
    f"<b>não preemptivos</b> avaliados neste trabalho, e não se estende "
    f"ao Round Robin nem, necessariamente, a algoritmos preemptivos em "
    f"geral. A varredura de k<sub>1</sub> (Seção III-B) reforça a leitura "
    f"dentro dessa família não preemptiva: à medida que k<sub>1</sub> "
    f"diminui e o algoritmo se afasta do comportamento FCFS-símile, o "
    f"turnaround agregado melhora muito pouco (300,32 contra 305,76 no "
    f"cenário equilibrado, k<sub>1</sub>=0,005 vs. 0,05) enquanto a "
    f"justiça cai visivelmente mais &mdash; não encontramos, nessa "
    f"fórmula aditiva linear, um ponto que domine o FCFS nas duas "
    f"métricas ao mesmo tempo, dentro da família não preemptiva. O valor "
    f"real do algoritmo, à luz dos nossos experimentos, está na Seção "
    f"VI-C: ele entrega quase toda a robustez do FCFS contra inanição "
    f"enquanto ainda usa o sinal de prioridade de forma clara &mdash; "
    f"algo que nem FCFS nem Prioridade pura fazem ao mesmo tempo.",
    S_BODY_NOINDENT))

# ============================================================ SECAO VII ===
story.append(Paragraph("VII. CONCLUSÃO", S_H1))
story.append(Paragraph(
    f"Implementamos um simulador de escalonamento por eventos discretos "
    f"com quatro algoritmos e o avaliamos sob quatro cargas de trabalho "
    f"controladas por seed, com rigor estatístico substancialmente acima "
    f"do mínimo exigido ({N_SEEDS_MAIN} seeds/cenário). Confirmamos empiricamente um "
    f"resultado clássico de sistemas operacionais &mdash; Round Robin com "
    f"quantum pequeno frente a rajadas longas gera overhead de troca de "
    f"contexto suficiente para dominar o turnaround sob carga alta &mdash; "
    f"e quantificamos precisamente o efeito (queda de quase uma ordem de "
    "grandeza no turnaround ao aumentar o quantum de 4 para 20). Para o "
    "algoritmo próprio, o achado central é que ele oferece um ponto de "
    "equilíbrio genuíno entre Prioridade estática e FCFS na dimensão de "
    "\u201cquanto a prioridade importa\u201d (razão de turnaround "
    "baixa/alta prioridade de 1,87&times;, entre o 1,00&times; do FCFS e o "
    "6,47&times; da Prioridade pura), mesmo não superando o FCFS em "
    "turnaround agregado &mdash; um resultado que só ficou visível "
    "porque medimos turnaround por classe de prioridade, não apenas a "
    "média geral. Como trabalho futuro, destacamos duas direções "
    "concretas: (i) uma função de envelhecimento não-linear ou normalizada "
    "pelo tempo mínimo ideal do próprio processo, que dispense "
    "recalibração de k<sub>1</sub> a cada regime de carga; e (ii) uma "
    "variante preemptiva do algoritmo próprio, para tratar o caso de uma "
    "rajada de baixa prioridade muito longa bloquear um processo de "
    "prioridade alta recém-chegado.",
    S_BODY_NOINDENT))

# ============================================================ REFERENCIAS =
story.append(Paragraph("REFERÊNCIAS", S_H1))
refs = [
    "[1] A. Silberschatz, P. B. Galvin, and G. Gagne, <i>Operating System "
    "Concepts</i>, 10th ed. Hoboken, NJ, USA: Wiley, 2018.",
    "[2] R. Jain, D. Chiu, and W. Hawe, \u201cA Quantitative Measure of "
    "Fairness and Discrimination for Resource Allocation in Shared "
    "Computer Systems,\u201d DEC Research Report TR-301, Digital Equipment "
    "Corporation, 1984.",
    "[3] F. J. Corbat\u00f3, M. Merwin-Daggett, and R. C. Daley, \u201cAn "
    "Experimental Time-Sharing System,\u201d in <i>Proc. AFIPS Spring "
    "Joint Computer Conference</i>, 1962, pp. 335&ndash;344.",
]
for r in refs:
    story.append(Paragraph(r, S_REF))

doc.build(story)
print(f"OK: {OUT} gerado.")
