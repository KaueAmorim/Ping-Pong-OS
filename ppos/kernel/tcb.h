// Descritor de tarefas (TCB - Task Control Block).

#ifndef __PPOS_TCB__
#define __PPOS_TCB__

#include "ctx.h"
#include "lib/queue.h"

// estados possíveis de uma tarefa
enum STATUS_T{
    NEW,
    READY,
    RUNNING,
    SUSPENDED,
    TERMINATED
};

// Task Control Block (TCB), infos sobre uma tarefa
struct task_t
{
    int id;                 // identificador da tarefa
    char *name;             // nome da tarefa
    struct ctx_t context;   // contexto da CPU (embutido)
    int status;             // estado atual da tarefa
    int prio_static;        // prioridade estática (-20 a +20, estilo UNIX)
    int prio_dynamic;       // prioridade dinâmica (envelhecida)
    struct task_t *parent;  // tarefa que criou esta tarefa
    struct queue_t *queue;  // fila onde a tarefa está (NULL se pronta)
};

#endif
