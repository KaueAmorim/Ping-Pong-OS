// Descritor de tarefas (TCB - Task Control Block).

#ifndef __PPOS_TCB__
#define __PPOS_TCB__

#include "ctx.h"

// estados possíveis de uma tarefa
#define TASK_READY      0
#define TASK_RUNNING    1
#define TASK_SUSPENDED  2
#define TASK_TERMINATED 3

// Task Control Block (TCB), infos sobre uma tarefa
struct task_t
{
    int id;                 // identificador da tarefa
    char *name;             // nome da tarefa
    struct ctx_t context;   // contexto da CPU (embutido)
    int status;             // estado atual da tarefa

    struct task_t *parent;  // tarefa que criou esta tarefa
    void *stack;            // pilha da tarefa (NULL na tarefa do núcleo)
    unsigned long stack_size; // tamanho da pilha, em bytes
};

#endif
