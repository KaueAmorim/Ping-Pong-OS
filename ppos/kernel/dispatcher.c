// Dispatcher: gerencia os estados das tarefas.

#include <stdlib.h>

#include "dispatcher.h"
#include "ctx.h"
#include "task.h"
#include "tcb.h"

extern struct task_t *task_kernel;
extern struct task_t *current_task;

void user_main(void *args);

void dispatcher_init()
{
}

void dispatcher_term()
{
}

void dispatcher()
{
    struct task_t *task_user = task_create("user_main", user_main, NULL);
    task_switch(task_user);
    task_destroy(task_user);
}

// transfere o processador para "task".
// Se task == NULL, transfere para a tarefa que criou a atual (parent).
int task_switch(struct task_t *task)
{
    if (task == NULL)
        task = current_task->parent;

    struct task_t *prev = current_task;
    current_task = task;

    // troca de contexto: salva o estado atual em prev e carrega o de task
    if (ctx_switch(&prev->context, &task->context) != 0)
    {
        // em erro, desfaz a atualização da tarefa corrente
        current_task = prev;
        return ERROR;
    }

    return NOERROR;
}

