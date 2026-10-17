// Semáforos e spinlocks

#include "semaphore.h"
#include "tcb.h"
#include "memory.h"
#include "dispatcher.h"
#include "lib/map.h"
#include "lib/queue.h"

#define MAX_SEMS 16  // número máximo de semáforos simultâneos

struct semaphore_t
{
    int id;                 // UID do semáforo (posição no mapa)
    int value;              // contador do semáforo (negativo: nº de aguardantes)
    struct queue_t *queue;  // fila de tarefas aguardando o semáforo
    int lock;               // spinlock: protege o acesso ao semáforo
};

extern struct task_t *current_task;

// mapa de semáforos: UID -> struct semaphore_t*
static struct map_t *sem_map = NULL;

// spinlock de proteção do mapa de semáforos
static int sem_map_lock = 0;

void sem_init()
{
    sem_map = map_create(MAX_SEMS);
}

void sem_term()
{
    if (sem_map == NULL)
        return;

    // destrói os semáforos remanescentes
    for (int i = 0; i < map_size(sem_map); i++)
    {
        struct semaphore_t *sem = (struct semaphore_t *)map_del(sem_map, i);
        if (sem != NULL)
        {
            queue_destroy(sem->queue);
            mem_free(sem);
        }
    }

    map_destroy(sem_map);
    sem_map = NULL;
}

void spin_lock(int *lock)
{
    // Test-and-Set Lock (TSL)
    while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE));
}

void spin_unlock(int *lock)
{
    __atomic_clear(lock, __ATOMIC_RELEASE);
}

int sem_create(int value)
{
    if (value < 0 || sem_map == NULL)
        return ERROR;

    struct semaphore_t *sem = mem_alloc(sizeof(struct semaphore_t));
    if (sem == NULL)
        return ERROR;

    sem->queue = queue_create();
    if (sem->queue == NULL)
    {
        mem_free(sem);
        return ERROR;
    }

    // registra o semáforo no mapa, sob o UID atribuído
    spin_lock(&sem_map_lock);
    int id = map_put(sem_map, sem);
    spin_unlock(&sem_map_lock);
    if (id < 0)
    {
        queue_destroy(sem->queue);
        mem_free(sem);
        return ERROR;
    }

    sem->id    = id;
    sem->value = value;
    sem->lock  = 0;

    return id;
}

int sem_destroy(int sem_id)
{
    if (sem_id < 0)
        return ERROR;

    // evita preempção durante a seção crítica
    int prev = current_task->is_system;
    current_task->is_system = 1;

    // localiza o semáforo no mapa
    spin_lock(&sem_map_lock);
    struct semaphore_t *sem = (struct semaphore_t *)map_get(sem_map, sem_id);
    if (sem == NULL)
    {
        spin_unlock(&sem_map_lock);
        current_task->is_system = prev;
        return ERROR;
    }

    // retira o semáforo do mapa, liberando seu UID
    map_del(sem_map, sem_id);
    spin_unlock(&sem_map_lock);

    spin_lock(&sem->lock);

    // acorda todas as tarefas aguardando; como o semáforo não está mais
    // no mapa, elas retornarão da operação Down com código de erro
    struct task_t *t = (struct task_t *)queue_head(sem->queue);
    while (t != NULL)
    {
        task_awake(t);
        t = (struct task_t *)queue_item(sem->queue);
    }
    spin_unlock(&sem->lock);

    queue_destroy(sem->queue);
    mem_free(sem);

    current_task->is_system = prev;
    return NOERROR;
}

int sem_down(int sem_id)
{
    if (sem_id < 0)
        return ERROR;

    // evita preempção durante a seção crítica check+suspend
    int prev = current_task->is_system;
    current_task->is_system = 1;

    // localiza o semáforo no mapa
    spin_lock(&sem_map_lock);
    struct semaphore_t *sem = (struct semaphore_t *)map_get(sem_map, sem_id);
    spin_unlock(&sem_map_lock);
    if (sem == NULL)
    {
        current_task->is_system = prev;
        return ERROR;
    }

    // decrementa o contador e suspende a tarefa caso ele se torne negativo
    spin_lock(&sem->lock);
    sem->value--;

    if (sem->value >= 0)
    {
        // semáforo livre: continua a execução sem se suspender
        spin_unlock(&sem->lock);
        current_task->is_system = prev;
        return NOERROR;
    }

    // contador negativo: suspende a tarefa no final da fila do semáforo
    spin_unlock(&sem->lock);
    task_suspend(sem->queue);

    // acordada (por sem_up ou sem_destroy): sucesso somente se o
    // semáforo - o mesmo objeto - ainda existir no mapa
    struct semaphore_t *s = (struct semaphore_t *)map_get(sem_map, sem_id);
    return (s == sem) ? NOERROR : ERROR;
}

int sem_up(int sem_id)
{
    if (sem_id < 0)
        return ERROR;

    // evita preempção durante a seção crítica
    int prev = current_task->is_system;
    current_task->is_system = 1;

    // localiza o semáforo no mapa
    spin_lock(&sem_map_lock);
    struct semaphore_t *sem = (struct semaphore_t *)map_get(sem_map, sem_id);
    spin_unlock(&sem_map_lock);
    if (sem == NULL)
    {
        current_task->is_system = prev;
        return ERROR;
    }

    // incrementa o contador e, se havia tarefas aguardando, acorda a primeira da fila
    spin_lock(&sem->lock);
    sem->value++;

    if (sem->value <= 0)
    {
        struct task_t *t = (struct task_t *)queue_head(sem->queue);
        if (t != NULL)
            task_awake(t);
    }
    spin_unlock(&sem->lock);

    current_task->is_system = prev;
    return NOERROR;
}
