// Escalonador de tarefas prontas.

#include "scheduler.h"
#include "tcb.h"

void sched_init()
{
}

void sched_term()
{
}

// escalonador FCFS: devolve a primeira tarefa da fila de prontas.
struct task_t *scheduler(struct queue_t *ready_queue)
{
    if (ready_queue == NULL)
        return NULL;

    return (struct task_t *)queue_head(ready_queue);
}
