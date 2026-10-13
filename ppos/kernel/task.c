// Gerência básica de tarefas.

#include "task.h"
#include "tcb.h"
#include "memory.h"
#include "dispatcher.h"
#include "hardware/cpu.h"
#include "time.h"
#include "lib/pplibc.h"

// tamanho padrão das pilhas de tarefas de usuário
#define STACK_SIZE 32 * 1024

// contador de IDs: 0 é reservado para a tarefa do núcleo;
// task_create atribui IDs a partir de 1.
static int id = 1;

// tarefa do núcleo (não usa pilha própria nem função de entrada)
struct task_t task_kernel;

// tarefa atualmente em execução (ponte de task_switch)
struct task_t *current_task = NULL;

extern struct queue_t *ready_queue;
extern struct queue_t *waiting_queue;
extern int user_tasks;

void task_init()
{
    task_kernel.id = 0;
    task_kernel.name = "kernel";
    task_kernel.status = RUNNING;
    task_kernel.parent = NULL;
    task_kernel.queue = NULL;
    task_kernel.quantum = 0;
    task_kernel.is_system = 1;   // núcleo/dispatcher: nunca preemptável
    task_kernel.waited = NULL;
    task_kernel.start_time = 0;  // núcleo existe desde a inicialização
    task_kernel.cpu_time = 0;
    task_kernel.activations = 0;
    task_kernel.exit_code = 0;
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
    task->prio_static = 0;
    task->prio_dynamic = 0;
    task->quantum = 0;
    task->is_system = 0;     // tarefa de usuário (preemptável)
    task->parent = current_task;
    task->waited = NULL;
    // contabilização: a vida da tarefa começa na criação
    task->start_time = time();
    task->cpu_time = 0;
    task->activations = 0;
    task->exit_code = 0;

    // insere na fila de prontas e contabiliza como tarefa de usuário
    queue_add(ready_queue, task);
    task->queue = ready_queue;
    user_tasks++;

    return task;
}

int task_destroy(struct task_t *task)
{
    if (task == NULL)
        return ERROR;
    if (task == current_task)
        return ERROR;

    // se a tarefa ainda estiver em alguma fila, retira-a
    if (task->queue != NULL)
    {
        queue_del(task->queue, task);
        task->queue = NULL;
    }

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

void task_yield()
{
    // evita preempção dentro do núcleo
    current_task->is_system = 1;

    current_task->status = READY;
    queue_add(ready_queue, current_task);
    current_task->queue = ready_queue;

    // retorna ao dispatcher
    task_switch(&task_kernel);
}

int task_wait(struct task_t *task)
{
    if (task == NULL)
        return ERROR;

    // se a tarefa já encerrou, retorna imediatamente com seu exit code
    if (task->status == TERMINATED)
        return task->exit_code;

    // evita preempção durante a seção crítica check+suspend
    current_task->is_system = 1;

    // registra quem se aguarda e se suspende na fila de espera
    current_task->waited = task;
    task_suspend(waiting_queue);

    return task->exit_code;
}

void task_exit(int exit_code)
{
    // evita preempção dentro do núcleo
    current_task->is_system = 1;

    // contabilização: registra o código de encerramento
    current_task->exit_code = exit_code;

    current_task->status = TERMINATED;
    user_tasks--;

    // retorna ao dispatcher
    task_switch(&task_kernel);
}

// imprime a contabilização de uma tarefa encerrada:
// tempo de vida, tempo de CPU e número de ativações.
void task_report(struct task_t *task)
{
    if (task == NULL)
        return;

    unsigned int life = time() - task->start_time;

    printk("PPOS: task %3d (%s), %5u ms run, %5u ms cpu, %5u acts, exit code %3d\n",
           task->id, task->name, life, task->cpu_time,
           task->activations, task->exit_code);
}
