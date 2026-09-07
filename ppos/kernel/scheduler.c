// Escalonador de tarefas prontas com prioridades e envelhecimento.

#include "scheduler.h"
#include "tcb.h"
#include "lib/queue.h"

#define PRIO_DEFAULT 0  // prioridade estática default (estilo UNIX)
#define AGING       -1  // fator de envelhecimento (α = -1)

extern struct task_t *current_task;

void sched_init()
{
}

void sched_term()
{
}

void sched_setprio(struct task_t *task, int prio)
{
    if (task == NULL)
        task = current_task;
    if (task == NULL)
        return;

    // limita ao intervalo válido (-20 a +20)
    if (prio < -20)
        prio = -20;
    else if (prio > 20)
        prio = 20;

    task->prio_static  = prio;
    task->prio_dynamic = prio;
}

int sched_getprio(struct task_t *task)
{
    if (task == NULL)
        task = current_task;
    if (task == NULL)
        return ERROR;

    return task->prio_static;
}

// escalonador por prioridades com envelhecimento:
// percorre a fila de prontas, escolhe a de menor prioridade dinâmica
// (menor valor = mais prioritária), reseta sua prioridade dinâmica para
// a estática e envelhece todas as demais (α = -1).
struct task_t *scheduler(struct queue_t *ready_queue)
{
    if (ready_queue == NULL)
        return NULL;

    struct task_t *chosen = (struct task_t *)queue_head(ready_queue);
    if (chosen == NULL)
        return NULL;

    // varre a fila procurando a tarefa mais prioritária
    struct task_t *t = (struct task_t *)queue_next(ready_queue);
    while (t != NULL)
    {
        if (t->prio_dynamic < chosen->prio_dynamic)
            chosen = t;
        t = (struct task_t *)queue_next(ready_queue);
    }

    // a escolhida volta à prioridade estática
    chosen->prio_dynamic = chosen->prio_static;

    // envelhece todas as demais tarefas prontas
    for (struct task_t *p = (struct task_t *)queue_head(ready_queue); p != NULL; p = (struct task_t *)queue_next(ready_queue))
    {
        if (p == chosen)
            continue;

        p->prio_dynamic += AGING;
        if (p->prio_dynamic < -20)
            p->prio_dynamic = -20;
    }

    return chosen;
}
