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
    int quantum;            // quantum restante (em ticks)
    int is_system;          // 1: tarefa de sistema (não preemptável); 0: de usuário
    struct task_t *parent;  // tarefa que criou esta tarefa
    struct queue_t *queue;  // fila onde a tarefa está (NULL se pronta)
    struct task_t *waited;  // tarefa aguardada por esta (task_wait)
    unsigned int start_time;   // tick em que a tarefa foi criada
    unsigned int cpu_time;     // ms de processador acumulados pela tarefa
    unsigned int activations;  // nº de vezes que a tarefa recebeu a CPU
    int exit_code;             // código de encerramento (via task_exit)
};

#endif
