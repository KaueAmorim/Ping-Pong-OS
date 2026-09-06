// Dispatcher: gerencia os estados das tarefas.

#include "dispatcher.h"
#include "ctx.h"
#include "task.h"
#include "tcb.h"
#include "scheduler.h"

// fila de tarefas prontas
struct queue_t *ready_queue = NULL;

// contador de tarefas de usuário ativas (criadas e não terminadas)
int user_tasks = 0;

extern struct task_t *current_task;
extern struct task_t task_kernel;

void user_main(void *args);

void dispatcher_init()
{
    ready_queue = queue_create();
}

void dispatcher_term()
{
    queue_destroy(ready_queue);
    ready_queue = NULL;
}

void dispatcher()
{
    // cria a tarefa inicial de usuário, que executará user_main()
    struct task_t *task_user = task_create("user_main", user_main, NULL);

    // enquanto houver tarefas de usuário e tarefas na fila de prontas
    while (user_tasks > 0 && queue_size(ready_queue) > 0)
    {
        // escolhe a próxima tarefa a executar (FCFS: cabeça da fila)
        struct task_t *next = scheduler(ready_queue);

        if (next != NULL)
        {
            // transfere o controle para a próxima tarefa
            task_run(next);

            // ao voltar ao dispatcher, trata a tarefa conforme seu estado
            switch (next->status)
            {
            case READY:
                // já foi reenfileirada por task_yield; nada a fazer
                break;
            case TERMINATED:
                // tarefa encerrou; nada a fazer aqui (task_exit já tratou)
                break;
            case SUSPENDED:
                // tarefa se suspendeu; foi inserida em outra fila
                break;
            default:
                break;
            }
        }
    }

    task_destroy(task_user);
}

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

void task_run(struct task_t *task)
{
    if (task == NULL)
        return;

    queue_del(ready_queue, task);
    task->queue = NULL;
    task->status = RUNNING;
    task_switch(task);
}

void task_suspend(struct queue_t *queue)
{
    current_task->status = SUSPENDED;
    if (queue != NULL)
    {
        queue_add(queue, current_task);
        current_task->queue = queue;
    }

    // retorna ao dispatcher
    task_switch(&task_kernel);
}

void task_awake(struct task_t *task)
{
    if (task == NULL)
        return;

    // se a tarefa está suspensa em alguma fila, retira-a dessa fila
    if (task->queue != NULL)
    {
        queue_del(task->queue, task);
        task->queue = NULL;
    }

    task->status = READY;
    queue_add(ready_queue, task);
    task->queue = ready_queue;
}
