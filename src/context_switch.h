#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

/*
 * Modelagem de troca de contexto (decisão de projeto documentada em
 * docs/modelagem.md, Seção 4.3 do prompt mestre):
 *  - Ocorre toda vez que a CPU passa a executar um processo DIFERENTE do
 *    anterior, incluindo a transição ociosa -> executando.
 *  - Duração configurável (`cost`), > 0 nos experimentos principais.
 *  - Durante a troca, a CPU fica indisponível (nenhum processo executa).
 *  - Mesmo valor de custo para todos os algoritmos dentro de um experimento.
 *
 * Implementação: mantemos `last_run_pid` = pid do último processo que
 * ocupou a CPU, com o sentinela NONE (-1) representando "CPU ociosa desde
 * a última execução". Sempre que a CPU fica genuinamente ociosa (ninguém
 * pronto para rodar), resetamos para NONE — isso faz com que a PRÓXIMA
 * vez que alguém for despachado, mesmo que seja o mesmo pid de antes, seja
 * tratada como uma transição ociosa->executando (portanto com troca),
 * exatamente como pede o enunciado. Se, em vez disso, o mesmo processo é
 * redespachado imediatamente sem a CPU passar por um período ocioso real
 * (ex.: RR com um único processo pronto, cujo quantum expira e ele é o
 * próprio próximo da fila), não há troca, pois não houve de fato "um
 * processo diferente" ocupando a CPU nesse intervalo.
 */

#define CS_NONE (-1)

typedef struct {
    int last_run_pid;
    int cost;
} ContextSwitchState;

void cs_init(ContextSwitchState *cs, int cost);

/* Marca a CPU como ociosa (nenhum processo rodando no momento). */
void cs_mark_idle(ContextSwitchState *cs);

/* Retorna 1 se despachar `next_pid` agora exige troca de contexto. */
int cs_needs_switch(const ContextSwitchState *cs, int next_pid);

/* Registra que `next_pid` passou a ocupar a CPU. */
void cs_set_running(ContextSwitchState *cs, int next_pid);

#endif /* CONTEXT_SWITCH_H */

