const pptxgen = require("pptxgenjs");
const path = require("path");
const fs = require("fs");

const FIG = path.join(__dirname, "..", "report", "figuras");

// Le a quantidade real de seeds a partir dos dados consolidados, em vez de
// hardcoded no texto dos slides (mesma logica de scripts/generate_plots.py
// e report/article_data.py -- ver Secao 3 do relatorio de revisao). Falha
// alto se o CSV nao existir ou tiver mais de um valor de n_seeds (sintoma
// de mistura de execucoes com configuracoes diferentes).
function readConsolidatedColumn(csvPath, colName) {
  const text = fs.readFileSync(csvPath, "utf8").trim();
  const lines = text.split("\n");
  const header = lines[0].split(",");
  const idx = header.indexOf(colName);
  if (idx === -1) throw new Error(`coluna ${colName} nao encontrada em ${csvPath}`);
  const valores = new Set(lines.slice(1).map((l) => l.split(",")[idx]));
  if (valores.size !== 1) {
    throw new Error(
      `${csvPath} contem mais de um valor de ${colName} (${[...valores].join(", ")}) -- ` +
      "resultados de execucoes diferentes parecem ter sido misturados. Rode " +
      "scripts/run_experiments.sh e scripts/analyze_results.py de novo antes " +
      "de gerar os slides."
    );
  }
  return parseInt([...valores][0], 10);
}

const MAIN_CSV = path.join(__dirname, "..", "results", "consolidated", "main_summary_long.csv");
const N_SEEDS_MAIN = readConsolidatedColumn(MAIN_CSV, "n_seeds");
const N_PROC_MAIN = readConsolidatedColumn(MAIN_CSV, "n_processes");

// ---------------------------------------------------------------- palette
const NAVY = "1E2761";
const NAVY_DARK = "141B47";
const ICE = "CADCFC";
const WHITE = "FFFFFF";
const DARKTEXT = "1A1A2E";
const MUTED = "5B6472";
const LIGHTBG = "F7F9FC";
const CARD = "FFFFFF";

const FCFS_C = "4C72B0";
const RR_C = "DD8452";
const PRIO_C = "55A868";
const CUSTOM_C = "C44E52";

const F_HEAD = "Cambria";
const F_BODY = "Calibri";

const pres = new pptxgen();
pres.defineLayout({ name: "WIDE", width: 13.333, height: 7.5 });
pres.layout = "WIDE";

const PAGE_W = 13.333;
const PAGE_H = 7.5;

function pageNumber(slide, n, dark) {
  slide.addText(String(n), {
    x: PAGE_W - 0.7, y: PAGE_H - 0.45, w: 0.5, h: 0.3,
    fontFace: F_BODY, fontSize: 10, color: dark ? "8891C4" : "A9B0BC",
    align: "right",
  });
}

function kicker(slide, text, dark) {
  slide.addText(text.toUpperCase(), {
    x: 0.7, y: 0.5, w: 8, h: 0.35,
    fontFace: F_BODY, fontSize: 12.5, color: dark ? ICE : NAVY,
    charSpacing: 2, bold: true,
  });
}

function slideTitle(slide, text, dark, y = 0.82) {
  slide.addText(text, {
    x: 0.7, y, w: 11.9, h: 0.9,
    fontFace: F_HEAD, fontSize: 30, bold: true,
    color: dark ? WHITE : DARKTEXT, align: "left",
  });
}

// ============================================================ SLIDE 1 ===
{
  const s = pres.addSlide();
  s.background = { color: NAVY };
  // subtle geometric motif: layered circles bottom-right
  s.addShape("ellipse", { x: 9.6, y: 4.6, w: 5.2, h: 5.2, fill: { color: NAVY_DARK }, line: { type: "none" } });
  s.addShape("ellipse", { x: 10.6, y: 5.6, w: 3.6, h: 3.6, fill: { color: "273174" }, line: { type: "none" } });

  s.addText("SISTEMAS OPERACIONAIS \u2014 PROJETO DA UNIDADE 3", {
    x: 0.9, y: 2.15, w: 10, h: 0.4, fontFace: F_BODY, fontSize: 14,
    color: ICE, charSpacing: 2, bold: true,
  });
  s.addText("Escalonamento de Processos com\nPrioridade Din\u00e2mica por Envelhecimento\ne Estimativa de Rajada", {
    x: 0.9, y: 2.6, w: 10.5, h: 2.5, fontFace: F_HEAD, fontSize: 40, bold: true,
    color: WHITE, lineSpacing: 46,
  });
  s.addText("\u00c2ngelo  \u00b7  Jetro  \u00b7  Luiz  \u00b7  Dorian", {
    x: 0.9, y: 5.35, w: 8, h: 0.5, fontFace: F_BODY, fontSize: 18, color: ICE,
  });
  s.addText("Curso de Engenharia de Software \u2014 Universidade Federal do Cariri (UFCA)", {
    x: 0.9, y: 5.85, w: 9, h: 0.4, fontFace: F_BODY, fontSize: 13, color: "9AA6D9", italic: true,
  });
  s.addNotes("Bom dia/tarde. Somos o Grupo [N], e vamos apresentar nosso simulador de escalonamento de processos, com foco no algoritmo próprio que desenvolvemos: Prioridade Dinâmica com Estimativa de Rajada e Envelhecimento. Apresentação de 10-12 minutos, todos participam.");
}

// ============================================================ SLIDE 2 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "O problema", false);
  slideTitle(s, "Escalonadores cl\u00e1ssicos otimizam um\nobjetivo \u00e0s custas de outro", false);

  const cards = [
    { title: "FCFS", sub: "Justo, mas cego a prioridade e a dura\u00e7\u00e3o de rajada", color: FCFS_C },
    { title: "Prioridade est\u00e1tica", sub: "Eficiente, mas pode deixar processos de baixa prioridade esperando indefinidamente", color: PRIO_C },
    { title: "Round Robin", sub: "Justo por constru\u00e7\u00e3o, mas caro em trocas de contexto se o quantum n\u00e3o casar com a rajada", color: RR_C },
  ];
  const cardW = 3.55, gap = 0.35, startX = 0.7, y0 = 2.55;
  cards.forEach((c, i) => {
    const x = startX + i * (cardW + gap);
    s.addShape("roundRect", {
      x, y: y0, w: cardW, h: 3.05, rectRadius: 0.08,
      fill: { color: CARD }, line: { color: "E3E7EF", width: 1 },
      shadow: { type: "outer", color: "364258", opacity: 0.18, blur: 8, offset: 3, angle: 90 },
    });
    s.addShape("rect", { x: x + 0.35, y: y0 + 0.35, w: 0.55, h: 0.12, fill: { color: c.color }, line: { type: "none" } });
    s.addText(c.title, {
      x: x + 0.35, y: y0 + 0.55, w: cardW - 0.7, h: 0.55,
      fontFace: F_HEAD, fontSize: 19, bold: true, color: DARKTEXT,
    });
    s.addText(c.sub, {
      x: x + 0.35, y: y0 + 1.15, w: cardW - 0.7, h: 1.7,
      fontFace: F_BODY, fontSize: 13.5, color: MUTED, lineSpacing: 18,
    });
  });

  s.addText("Pergunta do projeto: d\u00e1 para aproximar a efici\u00eancia da Prioridade sem herdar sua inani\u00e7\u00e3o?", {
    x: 0.7, y: 5.95, w: 11.9, h: 0.7, fontFace: F_HEAD, fontSize: 17, italic: true, color: NAVY,
  });
  s.addNotes("FCFS é justo mas ignora prioridade e tamanho de rajada. Prioridade estática é eficiente mas pode deixar processos de baixa prioridade esperando indefinidamente -- inanição. Round Robin é justo por construção, mas paga caro em trocas de contexto se o quantum não casar com o tamanho das rajadas. Nossa pergunta: dá para pegar a eficiência da Prioridade sem herdar sua inanição?");
  pageNumber(s, 2, false);
}

// ============================================================ SLIDE 3 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "O que constru\u00edmos", false);
  slideTitle(s, "Simulador de escalonamento por eventos\ndiscretos, em C, ponta a ponta", false);

  const steps = ["Seed", "Carga de\ntrabalho", "Simula\u00e7\u00e3o\n(C)", "M\u00e9tricas\npor seed", "IC95%\nentre seeds", "Gr\u00e1ficos +\nartigo"];
  const n = steps.length;
  const boxW = 1.75, boxH = 1.15, gap = 0.28;
  const totalW = n * boxW + (n - 1) * gap;
  let x = (PAGE_W - totalW) / 2;
  const y = 3.0;
  steps.forEach((label, i) => {
    s.addShape("roundRect", {
      x, y, w: boxW, h: boxH, rectRadius: 0.1,
      fill: { color: i === n - 1 ? NAVY : WHITE },
      line: { color: NAVY, width: 1.5 },
    });
    s.addText(label, {
      x, y, w: boxW, h: boxH, fontFace: F_BODY, fontSize: 12.5, bold: true,
      color: i === n - 1 ? WHITE : NAVY, align: "center", valign: "middle", lineSpacing: 14,
    });
    if (i < n - 1) {
      s.addText("\u2192", { x: x + boxW, y: y - 0.02, w: gap, h: boxH, fontSize: 18, color: MUTED, align: "center", valign: "middle" });
    }
    x += boxW + gap;
  });

  const facts = [
    ["4", "algoritmos"], ["4", "cen\u00e1rios obrigat\u00f3rios"], [String(N_PROC_MAIN), "processos / execu\u00e7\u00e3o"], [String(N_SEEDS_MAIN), "seeds / cen\u00e1rio"],
  ];
  const fW = 2.75, fGap = 0.25;
  const fTotalW = facts.length * fW + (facts.length - 1) * fGap;
  let fx = (PAGE_W - fTotalW) / 2;
  facts.forEach(([num, label]) => {
    s.addText(num, { x: fx, y: 4.85, w: fW, h: 0.85, fontFace: F_HEAD, fontSize: 40, bold: true, color: NAVY, align: "center" });
    s.addText(label, { x: fx, y: 5.7, w: fW, h: 0.5, fontFace: F_BODY, fontSize: 13, color: MUTED, align: "center" });
    fx += fW + fGap;
  });
  s.addNotes(`Construímos um simulador de eventos discretos em C -- sem depender de rand()/srand() da libc, PRNG próprio para garantir que a mesma seed sempre gera a mesma carga em qualquer máquina. Pipeline completo: seed -> geração de carga -> simulação -> métricas por seed -> agregação estatística com IC95% -> gráficos e artigo. Escala: ${N_PROC_MAIN} processos por execução, ${N_SEEDS_MAIN} seeds por cenário -- ${Math.round(N_SEEDS_MAIN / 100)}x o mínimo exigido, porque o pipeline roda ~45 mil execuções em poucos minutos.`);
  pageNumber(s, 3, false);
}

// ============================================================ SLIDE 4 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Modelo do sistema", false);
  slideTitle(s, "Processo = sequ\u00eancia de rajadas de\nCPU e E/S", false);

  const seq = [
    { label: "CPU", color: NAVY }, { label: "E/S", color: ICE },
    { label: "CPU", color: NAVY }, { label: "E/S", color: ICE },
    { label: "CPU", color: NAVY },
  ];
  const bw = 1.55, bh = 0.95, gap = 0.55;
  const totalW = seq.length * bw + (seq.length - 1) * gap;
  let x = (PAGE_W - totalW) / 2;
  const y = 2.5;
  seq.forEach((b, i) => {
    s.addShape("roundRect", {
      x, y, w: bw, h: bh, rectRadius: 0.09,
      fill: { color: b.color }, line: { type: "none" },
    });
    s.addText(b.label, {
      x, y, w: bw, h: bh, fontFace: F_BODY, fontSize: 15, bold: true,
      color: b.color === NAVY ? WHITE : NAVY, align: "center", valign: "middle",
    });
    if (i < seq.length - 1) {
      s.addText("\u2192", { x: x + bw, y: y - 0.05, w: gap, h: bh, fontSize: 20, color: MUTED, align: "center", valign: "middle" });
    }
    x += bw + gap;
  });
  s.addText("\u00daltima rajada nunca \u00e9 seguida de E/S \u2014 o processo termina ali.", {
    x: 0.7, y: 3.75, w: 11.9, h: 0.4, fontFace: F_BODY, fontSize: 13, italic: true, color: MUTED, align: "center",
  });

  const bullets = [
    { t: "Conven\u00e7\u00e3o de prioridade", d: "menor valor num\u00e9rico = maior prioridade (estilo nice do Unix)" },
    { t: "E/S", d: "dispositivos com fila FIFO pr\u00f3pria; atribui\u00e7\u00e3o ao de menor carga" },
    { t: "Troca de contexto", d: "conta em toda mudan\u00e7a de processo, inclusive ociosa\u2192executando; custo > 0 nos experimentos principais" },
    { t: "Chegadas", d: "intervalos exponenciais (n\u00e3o tudo em t=0) \u2014 fila realista" },
  ];
  const colW = 5.75;
  bullets.forEach((b, i) => {
    const col = i % 2, row = Math.floor(i / 2);
    const bx = 0.7 + col * (colW + 0.4);
    const by = 4.5 + row * 1.3;
    s.addShape("rect", { x: bx, y: by + 0.06, w: 0.06, h: 0.95, fill: { color: NAVY }, line: { type: "none" } });
    s.addText(b.t, { x: bx + 0.25, y: by, w: colW - 0.25, h: 0.35, fontFace: F_BODY, fontSize: 14, bold: true, color: DARKTEXT });
    s.addText(b.d, { x: bx + 0.25, y: by + 0.36, w: colW - 0.25, h: 0.65, fontFace: F_BODY, fontSize: 12, color: MUTED, lineSpacing: 15 });
  });
  s.addNotes("Cada processo é uma sequência de rajadas CPU->E/S->CPU->...->CPU; a última rajada nunca tem E/S depois, é quando o processo termina. Prioridade: menor número = maior prioridade. E/S com fila FIFO por dispositivo. Troca de contexto conta em toda mudança de processo, inclusive ociosa->executando, com custo configurável (>0 nos experimentos principais). Chegadas exponenciais, não tudo em t=0, para simular fila realista.");
  pageNumber(s, 4, false);
}

// ============================================================ SLIDE 5 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Algoritmos cl\u00e1ssicos", false);
  slideTitle(s, "Tr\u00eas baselines, mesma modelagem de\nE/S e troca de contexto", false);

  const algos = [
    { name: "FCFS", color: FCFS_C, d: "Fila FIFO pela ordem de entrada na fila de prontos (chegada ou retorno de E/S)." },
    { name: "Round Robin", color: RR_C, d: "Fila circular + quantum configur\u00e1vel (padr\u00e3o 4). Quem estoura o quantum volta ao final da fila." },
    { name: "Prioridade", color: PRIO_C, d: "N\u00e3o preemptivo. Sempre escolhe o menor valor de prioridade pronto; desempate por chegada." },
  ];
  const cardW = 3.55, gap = 0.35, startX = 0.7, y0 = 2.5;
  algos.forEach((a, i) => {
    const x = startX + i * (cardW + gap);
    s.addShape("roundRect", {
      x, y: y0, w: cardW, h: 3.6, rectRadius: 0.08,
      fill: { color: WHITE }, line: { color: "E3E7EF", width: 1 },
      shadow: { type: "outer", color: "364258", opacity: 0.16, blur: 7, offset: 3, angle: 90 },
    });
    s.addShape("ellipse", { x: x + 0.35, y: y0 + 0.35, w: 0.5, h: 0.5, fill: { color: a.color }, line: { type: "none" } });
    s.addText(a.name, {
      x: x + 0.35, y: y0 + 1.0, w: cardW - 0.7, h: 0.5,
      fontFace: F_HEAD, fontSize: 18, bold: true, color: DARKTEXT,
    });
    s.addText(a.d, {
      x: x + 0.35, y: y0 + 1.55, w: cardW - 0.7, h: 1.9,
      fontFace: F_BODY, fontSize: 13, color: MUTED, lineSpacing: 17,
    });
  });
  s.addNotes("Os três algoritmos clássicos usam exatamente a mesma modelagem de E/S e troca de contexto -- isso é importante para a comparação ser justa. FCFS: fila FIFO simples. Round Robin: quantum configurável, padrão 4. Prioridade: não preemptiva, sempre escolhe o menor valor de prioridade disponível.");
  pageNumber(s, 5, false);
}

// ============================================================ SLIDE 6 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Algoritmo pr\u00f3prio", false);
  slideTitle(s, "Prioridade Din\u00e2mica com Estimativa de\nRajada e Envelhecimento", false);

  s.addShape("roundRect", {
    x: 0.9, y: 2.55, w: 11.5, h: 1.35, rectRadius: 0.1,
    fill: { color: NAVY }, line: { type: "none" },
  });
  s.addText([
    { text: "prioridade_efetiva(p,t) = prioridade(p)  \u2212  k", options: { color: WHITE } },
    { text: "1", options: { color: WHITE, subscript: true } },
    { text: "\u00b7espera(p,t)  +  k", options: { color: WHITE } },
    { text: "2", options: { color: WHITE, subscript: true } },
    { text: "\u00b7rajada_estimada(p)", options: { color: WHITE } },
  ], {
    x: 0.9, y: 2.55, w: 11.5, h: 1.35, fontFace: "Courier New", fontSize: 20, bold: true,
    align: "center", valign: "middle",
  });

  const inputs = [
    { t: "Prioridade est\u00e1tica", d: "definida na chegada do processo" },
    { t: "Envelhecimento (k\u2081)", d: "tempo de espera na fila de prontos desde que ficou pronto" },
    { t: "Rajada estimada (k\u2082)", d: "m\u00e9dia m\u00f3vel exponencial de rajadas J\u00c1 CONCLU\u00cdDAS do pr\u00f3prio processo \u2014 nunca a rajada atual" },
  ];
  const colW = 3.65, gap = 0.35, startX = 0.9, y0 = 4.3;
  inputs.forEach((inp, i) => {
    const x = startX + i * (colW + gap);
    s.addText(String(i + 1), {
      x, y: y0, w: 0.6, h: 0.6, fontFace: F_HEAD, fontSize: 22, bold: true, color: CUSTOM_C,
    });
    s.addText(inp.t, { x: x, y: y0 + 0.55, w: colW, h: 0.4, fontFace: F_BODY, fontSize: 14.5, bold: true, color: DARKTEXT });
    s.addText(inp.d, { x: x, y: y0 + 0.95, w: colW, h: 1.3, fontFace: F_BODY, fontSize: 12, color: MUTED, lineSpacing: 15 });
  });
  s.addText("N\u00e3o preemptivo \u00b7 menor prioridade_efetiva vence \u00b7 nunca usa dura\u00e7\u00e3o real da rajada em curso", {
    x: 0.9, y: 6.75, w: 11.5, h: 0.4, fontFace: F_BODY, fontSize: 12, italic: true, color: MUTED, align: "center",
  });
  s.addNotes("Este é o coração do projeto. A prioridade efetiva combina três coisas: a prioridade estática do processo, um termo de envelhecimento que reduz o valor (favorece) quanto mais tempo o processo espera, e a rajada de CPU estimada via média móvel exponencial. Importante: a estimativa usa SÓ rajadas já concluídas do próprio processo -- nunca informação futura. Não preemptivo, igual à Prioridade clássica, para isolar o efeito dos dois novos mecanismos.");
  pageNumber(s, 6, false);
}

// ============================================================ SLIDE 7 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Calibra\u00e7\u00e3o", false);
  slideTitle(s, "Calibrando k\u2081: nem forte demais, nem\nfraco demais", false);

  const points = [
    { k: "k\u2081 = 0,5", res: "Domina a f\u00f3rmula quase de imediato", tag: "\u2248 FCFS", color: FCFS_C, x: 0.9 },
    { k: "k\u2081 = 0,05", res: "Ainda diferencia por prioridade, sem inani\u00e7\u00e3o extrema", tag: "ESCOLHIDO", color: CUSTOM_C, x: 5.0 },
    { k: "k\u2081 = 0,005", res: "Mal influencia a decis\u00e3o", tag: "\u2248 Prioridade", color: PRIO_C, x: 9.1 },
  ];
  // spectrum line
  s.addShape("line", { x: 1.3, y: 3.35, w: 10.7, h: 0, line: { color: "C7CEDA", width: 2 } });
  points.forEach((p) => {
    const cx = p.x + 1.55;
    s.addShape("ellipse", { x: cx - 0.09, y: 3.26, w: 0.18, h: 0.18, fill: { color: p.color }, line: { color: WHITE, width: 2 } });
    s.addShape("roundRect", {
      x: p.x, y: 3.7, w: 3.3, h: 2.35, rectRadius: 0.08,
      fill: { color: WHITE }, line: { color: p.color, width: p.tag === "ESCOLHIDO" ? 2.25 : 1 },
      shadow: p.tag === "ESCOLHIDO" ? { type: "outer", color: "364258", opacity: 0.22, blur: 9, offset: 3, angle: 90 } : undefined,
    });
    s.addText(p.k, { x: p.x + 0.25, y: 3.85, w: 2.8, h: 0.45, fontFace: F_HEAD, fontSize: 19, bold: true, color: DARKTEXT });
    s.addText(p.tag, {
      x: p.x + 0.25, y: 4.3, w: 2.8, h: 0.35, fontFace: F_BODY, fontSize: 11.5, bold: true,
      color: p.color, charSpacing: 1,
    });
    s.addText(p.res, { x: p.x + 0.25, y: 4.75, w: 2.8, h: 1.15, fontFace: F_BODY, fontSize: 12.5, color: MUTED, lineSpacing: 16 });
  });

  s.addText("k\u2082 (peso da rajada estimada) \u00e9 bem menos sens\u00edvel: turnaround varia s\u00f3 de 274,6 a 277,6 ao longo de k\u2082 \u2208 [0,1; 3,0]. Mantido em 0,3.", {
    x: 0.9, y: 6.45, w: 11.5, h: 0.6, fontFace: F_BODY, fontSize: 12.5, italic: true, color: MUTED, align: "center",
  });
  s.addNotes(`Testamos k1 em 0,5 / 0,05 / 0,005 nos 4 cenários com ${N_SEEDS_MAIN} seeds cada. k1=0,5 é forte demais: o envelhecimento domina a fórmula quase imediatamente frente às esperas observadas (centenas de unidades de tempo), e o algoritmo vira essencialmente um FCFS. k1=0,005 é fraco demais, aproxima da Prioridade pura. k1=0,05 ficou no meio: ainda diferencia por prioridade sem inanição extrema. Esse processo de calibração está documentado por completo no repositório.`);
  pageNumber(s, 7, false);
}

// ============================================================ SLIDE 8 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Metodologia", false);
  slideTitle(s, "Rigor estat\u00edstico: 10\u00d7 o m\u00ednimo exigido", false);

  const scenarios = [
    { t: "Equilibrado", d: "mix de curtos/longos, pouca/muita E/S" },
    { t: "I/O-bound", d: "rajadas curtas, muita E/S" },
    { t: "CPU-bound", d: "rajadas longas, pouca E/S" },
    { t: "Prioridades\ndesbalanceadas", d: "85% alta / 15% baixa prioridade" },
  ];
  const cw = 2.75, cgap = 0.25, sx = 0.7, sy = 2.5;
  scenarios.forEach((sc, i) => {
    const x = sx + i * (cw + cgap);
    s.addShape("roundRect", { x, y: sy, w: cw, h: 1.55, rectRadius: 0.08, fill: { color: WHITE }, line: { color: "E3E7EF", width: 1 } });
    s.addText(sc.t, { x: x + 0.2, y: sy + 0.15, w: cw - 0.4, h: 0.65, fontFace: F_BODY, fontSize: 14, bold: true, color: NAVY, lineSpacing: 15 });
    s.addText(sc.d, { x: x + 0.2, y: sy + 0.8, w: cw - 0.4, h: 0.65, fontFace: F_BODY, fontSize: 10.5, color: MUTED, lineSpacing: 12.5 });
  });

  const facts2 = [[String(N_PROC_MAIN), "processos / execu\u00e7\u00e3o"], [String(N_SEEDS_MAIN), "seeds / cen\u00e1rio"], ["45 mil", "execu\u00e7\u00f5es totais"], ["95%", "intervalo de confian\u00e7a"]];
  const fW = 2.75, fGap = 0.25;
  const fTotalW = facts2.length * fW + (facts2.length - 1) * fGap;
  let fx = (PAGE_W - fTotalW) / 2;
  facts2.forEach(([num, label]) => {
    s.addText(num, { x: fx, y: 4.55, w: fW, h: 0.85, fontFace: F_HEAD, fontSize: 36, bold: true, color: NAVY, align: "center" });
    s.addText(label, { x: fx, y: 5.35, w: fW, h: 0.6, fontFace: F_BODY, fontSize: 12.5, color: MUTED, align: "center" });
    fx += fW + fGap;
  });

  s.addText("M\u00e9tricas: turnaround m\u00e9dio \u00b7 trocas de contexto \u00b7 \u00edndice de Jain do slowdown \u00b7 turnaround por classe de prioridade", {
    x: 0.7, y: 6.35, w: 11.9, h: 0.5, fontFace: F_BODY, fontSize: 13, italic: true, color: NAVY, align: "center",
  });
  s.addNotes(`Quatro cenários obrigatórios cobrindo perfis bem diferentes de carga. ${N_PROC_MAIN} processos por execução, ${N_SEEDS_MAIN} seeds por cenário -- mesmas seeds para todos os algoritmos, para comparação justa. Custo de troca de contexto sempre >0 nos experimentos principais. Métricas: turnaround médio, trocas de contexto, índice de Jain do slowdown, e uma métrica extra que criamos: turnaround por classe de prioridade -- é essa última que revela o achado mais interessante.`);
  pageNumber(s, 8, false);
}

// ============================================================ SLIDE 9 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Resultados", false);
  slideTitle(s, "Vis\u00e3o geral: RR se destaca (para pior) em\nturnaround e trocas de contexto", false);
  s.addImage({ path: path.join(FIG, "fig_main_metrics.png"), x: 0.55, y: 2.05, w: 12.2, h: 12.2 / 2.7855 });
  s.addText(`M\u00e9dia \u00b1 IC95%, ${N_SEEDS_MAIN} seeds/cen\u00e1rio, ${N_PROC_MAIN} processos/execu\u00e7\u00e3o, custo de troca de contexto = 1. Escalas (a) e (b) em log.`, {
    x: 0.7, y: 6.55, w: 11.9, h: 0.4, fontFace: F_BODY, fontSize: 11.5, italic: true, color: MUTED, align: "center",
  });
  s.addNotes("Escalas em log nos painéis (a) e (b) porque o Round Robin fica uma ordem de grandeza acima dos outros três em turnaround e trocas de contexto, em quase todos os cenários. Entre os outros três, FCFS e algoritmo próprio ficam próximos, com a Prioridade tendo o menor turnaround mas a pior justiça (painel c). Vamos entender por que o RR se comporta assim no próximo slide.");
  pageNumber(s, 9, false);
}

// =========================================================== SLIDE 10 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Resultados", false);
  slideTitle(s, "RR: o problema \u00e9 quantum pequeno frente\na rajadas longas, n\u00e3o o algoritmo em si", false);
  s.addImage({ path: path.join(FIG, "fig_quantum_sensitivity.png"), x: 2.42, y: 2.15, w: 8.5, h: 8.5 / 1.9924 });
  s.addText("CPU-bound, quantum=4 (principal) vs. quantum=20 (complementar): turnaround cai quase 9\u00d7, trocas de contexto ~4\u00d7.", {
    x: 0.7, y: 6.75, w: 11.9, h: 0.5, fontFace: F_BODY, fontSize: 13, italic: true, color: MUTED, align: "center",
  });
  s.addNotes("No cenário CPU-bound as rajadas são longas, média 25 unidades de tempo. Com quantum=4, cada rajada é fatiada em ~6 pedaços, cada um exigindo uma troca de contexto -- sob carga alta isso quase satura a CPU só com overhead de troca. Aumentando o quantum para 20, mais perto do tamanho da rajada, o turnaround cai quase 9 vezes e as trocas de contexto caem 4 vezes. Confirma que o problema é a relação quantum/rajada, não uma falha do Round Robin em si.");
  pageNumber(s, 10, false);
}

// =========================================================== SLIDE 11 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Resultados \u2014 achado central", false);
  slideTitle(s, "O algoritmo pr\u00f3prio usa prioridade sem\nreproduzir inani\u00e7\u00e3o severa", false);
  s.addImage({ path: path.join(FIG, "fig_priority_breakdown.png"), x: 2.62, y: 1.95, w: 8.1, h: 8.1 / 1.6173 });
  s.addNotes("Este é o resultado mais importante do projeto. Quebramos o turnaround por classe de prioridade no cenário desbalanceado. FCFS trata processos de prioridade alta e baixa de forma praticamente idêntica -- razão 1,00x, porque literalmente ignora prioridade. A Prioridade estática discrimina muito forte -- 6,47x, processos de baixa prioridade terminam 6 vezes mais devagar. Nosso algoritmo fica no meio -- 1,87x: usa a prioridade de forma visível, mas o envelhecimento evita a quase-inanição. O turnaround da classe baixa prioridade no nosso algoritmo é quase metade do da Prioridade pura.");
  pageNumber(s, 11, false);
}

// =========================================================== SLIDE 12 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Discuss\u00e3o", false);
  slideTitle(s, "Por que o algoritmo pr\u00f3prio n\u00e3o supera o\nFCFS na m\u00e9dia agregada", false);

  s.addShape("roundRect", {
    x: 0.7, y: 2.5, w: 11.9, h: 1.65, rectRadius: 0.08,
    fill: { color: WHITE }, line: { color: "E3E7EF", width: 1 },
  });
  s.addText("Entre os algoritmos N\u00c3O PREEMPTIVOS, o FCFS maximiza a simetria \u2014 exatamente o que o \u00edndice de Jain premia. (O Round Robin, preemptivo, \u00e9 ainda mais justo: 71,6% vs. 43,1%.)", {
    x: 1.0, y: 2.7, w: 11.3, h: 1.25, fontFace: F_HEAD, fontSize: 16, italic: true, color: NAVY, valign: "middle", lineSpacing: 21,
  });

  s.addText("Entre os n\u00e3o preemptivos, qualquer desvio dessa ordena\u00e7\u00e3o pura \u2014 para perseguir efici\u00eancia via prioridade ou rajada estimada \u2014 reintroduz alguma assimetria.", {
    x: 0.7, y: 4.45, w: 11.9, h: 0.6, fontFace: F_BODY, fontSize: 14, color: DARKTEXT,
  });

  const twoCol = [
    { t: "O que perdemos", d: "Justi\u00e7a agregada abaixo do FCFS (ex.: 37,9% vs. 43,1% no cen\u00e1rio equilibrado)", color: CUSTOM_C },
    { t: "O que ganhamos", d: "Diferencia\u00e7\u00e3o real por prioridade (1,87\u00d7) sem a quase-inani\u00e7\u00e3o da Prioridade pura (6,47\u00d7)", color: PRIO_C },
  ];
  twoCol.forEach((c, i) => {
    const x = 0.7 + i * 6.0;
    s.addShape("rect", { x, y: 5.3, w: 0.06, h: 1.3, fill: { color: c.color }, line: { type: "none" } });
    s.addText(c.t, { x: x + 0.25, y: 5.25, w: 5.6, h: 0.4, fontFace: F_BODY, fontSize: 15, bold: true, color: DARKTEXT });
    s.addText(c.d, { x: x + 0.25, y: 5.65, w: 5.6, h: 0.95, fontFace: F_BODY, fontSize: 12.5, color: MUTED, lineSpacing: 16 });
  });
  s.addNotes("Sermos honestos: na média agregada (todos os processos juntos, não só por classe), nosso algoritmo NÃO supera o FCFS -- turnaround fica estatisticamente igual, e a justiça agregada fica um pouco abaixo. Por quê? Entre os algoritmos NÃO PREEMPTIVOS (FCFS, Prioridade, nosso algoritmo), o FCFS ordena estritamente por tempo de espera, e isso já maximiza a simetria entre eles -- exatamente o que o índice de Jain premia. IMPORTANTE: isso não vale para todos os 4 algoritmos -- o Round Robin, sendo preemptivo, tem justiça AINDA MAIOR que o FCFS em todos os cenários (71,6% vs 43,1% no equilibrado), porque intercalar execução entre todo mundo reduz a variância de slowdown por construção. Voltando à comparação relevante (algoritmo próprio vs FCFS, ambos não preemptivos): qualquer desvio da ordem pura, buscando eficiência, reintroduz assimetria dentro dessa família. O valor real do nosso algoritmo está no slide anterior: ele usa prioridade de forma visível sem a discriminação severa da Prioridade pura.");
  pageNumber(s, 12, false);
}

// =========================================================== SLIDE 13 ===
{
  const s = pres.addSlide();
  s.background = { color: LIGHTBG };
  kicker(s, "Conclus\u00e3o", false);
  slideTitle(s, "O que fica deste projeto", false);

  const items = [
    "Confirmamos experimentalmente um resultado cl\u00e1ssico: RR com quantum pequeno frente a rajadas longas satura em trocas de contexto sob carga alta.",
    "O algoritmo pr\u00f3prio entrega um ponto de equil\u00edbrio real entre FCFS e Prioridade na dimens\u00e3o \u201cquanto a prioridade importa\u201d \u2014 vis\u00edvel s\u00f3 porque medimos por classe de prioridade, n\u00e3o s\u00f3 a m\u00e9dia geral.",
    "Com a f\u00f3rmula aditiva linear, n\u00e3o existe k\u2081 que supere o FCFS em turnaround E justi\u00e7a ao mesmo tempo \u2014 uma limita\u00e7\u00e3o estrutural, n\u00e3o s\u00f3 de calibra\u00e7\u00e3o.",
  ];
  let y = 2.55;
  items.forEach((txt, i) => {
    s.addShape("ellipse", { x: 0.7, y: y + 0.05, w: 0.5, h: 0.5, fill: { color: NAVY }, line: { type: "none" } });
    s.addText(String(i + 1), { x: 0.7, y: y + 0.05, w: 0.5, h: 0.5, fontFace: F_HEAD, fontSize: 16, bold: true, color: WHITE, align: "center", valign: "middle" });
    s.addText(txt, { x: 1.4, y, w: 11.2, h: 1.0, fontFace: F_BODY, fontSize: 14.5, color: DARKTEXT, valign: "middle", lineSpacing: 18 });
    y += 1.2;
  });

  s.addShape("roundRect", { x: 0.7, y: 6.2, w: 11.9, h: 0.85, rectRadius: 0.08, fill: { color: ICE }, line: { type: "none" } });
  s.addText("Trabalho futuro: envelhecimento n\u00e3o-linear (dispensa recalibrar k\u2081) e uma variante preemptiva do algoritmo pr\u00f3prio.", {
    x: 1.0, y: 6.2, w: 11.3, h: 0.85, fontFace: F_BODY, fontSize: 13, italic: true, color: NAVY, valign: "middle",
  });
  s.addNotes("Resumindo: confirmamos experimentalmente o efeito clássico de quantum pequeno com rajadas longas no Round Robin. Nosso algoritmo próprio entrega um ponto de equilíbrio real entre FCFS e Prioridade -- um resultado que só ficou visível porque medimos por classe de prioridade. E documentamos honestamente uma limitação estrutural: com essa fórmula aditiva linear simples, não existe k1 que vença o FCFS nas duas métricas ao mesmo tempo. Trabalho futuro: envelhecimento não-linear e uma variante preemptiva.");
  pageNumber(s, 13, false);
}

// =========================================================== SLIDE 14 ===
{
  const s = pres.addSlide();
  s.background = { color: NAVY };
  s.addShape("ellipse", { x: -1.5, y: -1.8, w: 5.2, h: 5.2, fill: { color: NAVY_DARK }, line: { type: "none" } });
  s.addText("Obrigado.", {
    x: 0.9, y: 2.9, w: 10, h: 1.2, fontFace: F_HEAD, fontSize: 46, bold: true, color: WHITE,
  });
  s.addText("Perguntas? \u2014 c\u00f3digo, dados brutos/consolidados, docs de modelagem e o artigo completo est\u00e3o no reposit\u00f3rio do projeto.", {
    x: 0.9, y: 4.05, w: 10.5, h: 0.7, fontFace: F_BODY, fontSize: 15, color: ICE,
  });
  s.addText("\u00c2ngelo  \u00b7  Jetro  \u00b7  Luiz  \u00b7  Dorian  \u2014  UFCA, Engenharia de Software", {
    x: 0.9, y: 6.6, w: 9, h: 0.4, fontFace: F_BODY, fontSize: 12, color: "9AA6D9", italic: true,
  });
  s.addNotes("Obrigado. Abrimos para perguntas -- todo o código, dados brutos e consolidados, documentação de modelagem e o artigo completo estão no repositório GitHub do projeto, com instruções de build e reprodução no README.");
}

pres.writeFile({ fileName: path.join(__dirname, "apresentacao.pptx") }).then(() => {
  console.log("OK: apresentacao.pptx gerado.");
});
