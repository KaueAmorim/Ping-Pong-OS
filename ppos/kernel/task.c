// Gerência básica de tarefas.

#include "tcb.h"
#include "macros.h"
#include "memory.h"

// tamanho padrão das pilhas de tarefas de usuário
#define STACK_SIZE 32 * 1024

// contador de IDs: 0 é reservado para a tarefa do núcleo;
// task_create atribui IDs a partir de 1.
static int id = 1;

// tarefa do núcleo (não usa pilha própria nem função de entrada)
struct task_t task_kernel;

// tarefa atualmente em execução (ponte de task_switch)
struct task_t *current_task = NULL;

void task_init()
{
    task_kernel.id = 0;
    task_kernel.name = "kernel";
    task_kernel.status = RUNNING;
    task_kernel.parent = NULL;
    // o núcleo está rodando; é a tarefa corrente até outra ser ativada
    current_task = &task_kernel;
}

void task_term()
{
    current_task = NULL;
}

struct task_t *task_create(char *name, void (*entry)(void *), void *arg)
{
    if (entry == NULL)
        return NULL;

    struct task_t *task = mem_alloc(sizeof(struct task_t));
    if (task == NULL)
        return NULL;

    void *stack = mem_alloc(STACK_SIZE);
    if (stack == NULL)
    {
        mem_free(task);
        return NULL;
    }

    if (ctx_create(&task->context, entry, arg, stack, STACK_SIZE) != 0)
    {
        mem_free(stack);
        mem_free(task);
        return NULL;
    }

    task->id = id++;
    task->name = name;
    task->status = READY;
    task->parent = current_task;

    return task;
}

int task_destroy(struct task_t *task)
{
    if (task == NULL)
        return ERROR;
    if (task == current_task)
        return ERROR;

    mem_free(task->context.stack);
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
