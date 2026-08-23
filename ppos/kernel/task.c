// Gerência básica de tarefas.

#define _XOPEN_SOURCE 700   // expõe posix_memalign no std=c99
#include <stdlib.h>

#include "tcb.h"
#include "macros.h"
#include "memory.h"

// tamanho padrão das pilhas de tarefas de usuário
#define TASK_STACK_SIZE 8192

// contador de IDs: 0 é reservado para a tarefa do núcleo;
// task_create atribui IDs a partir de 1.
static int next_task_id = 1;

// tarefa do núcleo (não usa pilha própria nem função de entrada)
static struct task_t task_kernel;

// tarefa atualmente em execução (ponte de task_switch)
static struct task_t *current_task = NULL;

void task_init()
{
    task_kernel.id = 0;
    task_kernel.name = "kernel";
    task_kernel.status = TASK_RUNNING;
    task_kernel.parent = NULL;
    task_kernel.stack = NULL;
    task_kernel.stack_size = 0;

    // o núcleo está rodando; é a tarefa corrente até outra ser ativada
    current_task = &task_kernel;
}

void task_term()
{
    current_task = NULL;
}

struct task_t *task_create(char *name, void (*entry)(void *), void *arg)
{
    check_parm(entry == NULL, "entry function is NULL", NULL);

    struct task_t *task = mem_alloc(sizeof(struct task_t));
    check_parm(task == NULL, "can't allocate TCB", NULL);

    // aloca pilha alinhada a 16 bytes (ctx_create exige esse alinhamento)
    void *stack = NULL;
    if (posix_memalign(&stack, 16, TASK_STACK_SIZE) != 0)
    {
        mem_free(task);
        return NULL;
    }

    // cria o contexto, o TCB embute o struct ctx_t
    if (ctx_create(&task->context, entry, arg, stack, TASK_STACK_SIZE) != 0)
    {
        free(stack);
        mem_free(task);
        return NULL;
    }

    task->id = next_task_id++;
    task->name = name;
    task->status = TASK_READY;
    task->parent = current_task;
    task->stack = stack;
    task->stack_size = TASK_STACK_SIZE;

    return task;
}

int task_destroy(struct task_t *task)
{
    check_parm(task == NULL, "task is NULL", ERROR);
    check_parm(task == current_task, "can't destroy the running task", ERROR);

    free(task->stack);
    mem_free(task);

    return NOERROR;
}

int task_id(struct task_t *task)
{
    if (task == NULL)
        task = current_task;

    if (task == NULL)
        return ERROR;

    return task->id;
}

char *task_name(struct task_t *task)
{
    if (task == NULL)
        task = current_task;

    if (task == NULL)
        return NULL;

    return task->name;
}

// transfere o processador para "task".
// Se task == NULL, transfere para a tarefa que criou a atual (parent).
// Retorna NOERROR ou ERROR.
int task_switch(struct task_t *task)
{
    if (task == NULL)
        task = current_task->parent;

    check_parm(task == NULL, "no task to switch to", ERROR);
    check_parm(task == current_task, "switching to the current task", ERROR);

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
