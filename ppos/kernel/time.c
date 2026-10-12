// Gerência do tempo: relógio (ticks) e preempção por quantum.

#include "time.h"
#include "hardware/cpu.h"
#include "tcb.h"
#include "dispatcher.h"

// relógio do sistema, em milissegundos (1 tick == 1 ms)
static volatile unsigned int uptime = 0;

extern struct task_t *current_task;
extern struct task_t task_kernel;
extern void task_yield();

// tratador de interrupção do temporizador (definido abaixo)
void tick_handler(int irq);

void time_init()
{
    // registra o tratador do tick do temporizador
    hw_irq_handle(IRQ_TIMER, tick_handler);

    // arma o temporizador: 1º disparo em TICK ms, depois a cada TICK ms
    hw_timer(TICK, TICK);
}

void time_term()
{
    // desabilita as IRQs, desarma o temporizador e cancela o tratador,
    // evitando ticks pendentes após o encerramento
    hw_irq_enable(0);
    hw_timer(0, 0);
    hw_irq_handle(IRQ_TIMER, NULL);
}

unsigned int time()
{
    return (uptime);
}

void tick_handler(int irq)
{
    (void)irq; // IRQ_TIMER, não usado

    // conta o tempo decorrido desde a inicialização
    uptime++;

    // contabiliza 1 ms de CPU para a tarefa que estava rodando no tick,
    // pois a interrupção do temporizador a interrompeu
    if (current_task != NULL)
        current_task->cpu_time++;

    // se não há tarefa corrente ou ela é de sistema, não preempta
    if (current_task == NULL)
        return;
    if (current_task == &task_kernel)
        return;
    if (current_task->is_system)
        return;

    // decrementa o quantum da tarefa de usuário em execução
    if (current_task->quantum > 0)
        current_task->quantum--;

    // ao esgotar o quantum, devolve a CPU ao dispatcher
    if (current_task->quantum == 0)
        task_yield();
}
