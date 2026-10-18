// Gerência de filas de mensagens

#include "mqueue.h"
#include "tcb.h"
#include "memory.h"
#include "dispatcher.h"
#include "semaphore.h"
#include "lib/map.h"
#include "lib/queue.h"
#include "lib/pplibc.h"

#define MAX_QUEUES 16  // número máximo de filas de mensagens simultâneas

struct mqueue_t
{
    int id;                 // UID da fila (posição no mapa)
    int max_msgs;           // capacidade máxima da fila (mensagens)
    int msg_size;           // tamanho de cada mensagem, em bytes
    int count;              // número de mensagens atualmente na fila
    struct queue_t *data;      // buffer da fila: ponteiros p/ mensagens copiadas
    struct queue_t *wait_send; // tarefas aguardando vaga para enviar
    struct queue_t *wait_recv; // tarefas aguardando mensagem para receber
    int lock;                  // spinlock: protege o acesso à fila
};

extern struct task_t *current_task;

// mapa de filas de mensagens: UID -> struct mqueue_t*
static struct map_t *mqueue_map = NULL;

// spinlock de proteção do mapa de filas
static int mqueue_map_lock = 0;

// libera as estruturas de uma fila de mensagens (sem acordar ninguém);
// a fila não deve estar mais no mapa quando esta função for chamada
static void mqueue_free(struct mqueue_t *q)
{
    if (q == NULL)
        return;

    // libera as mensagens remanescentes no buffer da fila
    void *buf;
    while ((buf = (void *)queue_head(q->data)) != NULL)
    {
        queue_del(q->data, buf);
        mem_free(buf);
    }

    queue_destroy(q->data);
    queue_destroy(q->wait_send);
    queue_destroy(q->wait_recv);
    mem_free(q);
}

void mqueue_init()
{
    mqueue_map = map_create(MAX_QUEUES);
}

void mqueue_term()
{
    if (mqueue_map == NULL)
        return;

    // destrói as filas remanescentes
    for (int i = 0; i < map_size(mqueue_map); i++)
    {
        struct mqueue_t *q = (struct mqueue_t *)map_del(mqueue_map, i);
        mqueue_free(q);
    }

    map_destroy(mqueue_map);
    mqueue_map = NULL;
}

int mqueue_create(int max_msgs, int msg_size)
{
    if (max_msgs < 1 || msg_size < 1 || mqueue_map == NULL)
        return ERROR;

    // evita preempção enquanto retiver os spinlocks
    int prev = current_task->is_system;
    current_task->is_system = 1;

    struct mqueue_t *q = mem_alloc(sizeof(struct mqueue_t));
    if (q == NULL)
    {
        current_task->is_system = prev;
        return ERROR;
    }

    q->data      = queue_create();
    q->wait_send = queue_create();
    q->wait_recv = queue_create();
    if (q->data == NULL || q->wait_send == NULL || q->wait_recv == NULL)
    {
        queue_destroy(q->data);
        queue_destroy(q->wait_send);
        queue_destroy(q->wait_recv);
        mem_free(q);
        current_task->is_system = prev;
        return ERROR;
    }

    // registra a fila no mapa, sob o UID atribuído
    spin_lock(&mqueue_map_lock);
    int id = map_put(mqueue_map, q);
    spin_unlock(&mqueue_map_lock);
    if (id < 0)
    {
        mqueue_free(q);
        current_task->is_system = prev;
        return ERROR;
    }

    q->id       = id;
    q->max_msgs = max_msgs;
    q->msg_size = msg_size;
    q->count    = 0;
    q->lock     = 0;

    current_task->is_system = prev;
    return id;
}

int mqueue_destroy(int queue_id)
{
    if (queue_id < 0)
        return ERROR;

    // evita preempção durante a seção crítica
    int prev = current_task->is_system;
    current_task->is_system = 1;

    // retira a fila do mapa, liberando seu UID
    spin_lock(&mqueue_map_lock);
    struct mqueue_t *q = (struct mqueue_t *)map_del(mqueue_map, queue_id);
    spin_unlock(&mqueue_map_lock);
    if (q == NULL)
    {
        current_task->is_system = prev;
        return ERROR;
    }

    spin_lock(&q->lock);

    // acorda todas as tarefas aguardando enviar ou receber; como a fila
    // não está mais no mapa, elas retornarão das suas chamadas com erro
    struct task_t *t = (struct task_t *)queue_head(q->wait_send);
    while (t != NULL)
    {
        task_awake(t);
        t = (struct task_t *)queue_item(q->wait_send);
    }
    t = (struct task_t *)queue_head(q->wait_recv);
    while (t != NULL)
    {
        task_awake(t);
        t = (struct task_t *)queue_item(q->wait_recv);
    }
    spin_unlock(&q->lock);

    // libera as mensagens e as estruturas da fila
    mqueue_free(q);

    current_task->is_system = prev;
    return NOERROR;
}

int mqueue_send(int queue_id, void *msg)
{
    if (queue_id < 0 || msg == NULL)
        return ERROR;

    // evita preempção durante as seções críticas check+suspend
    int prev = current_task->is_system;
    current_task->is_system = 1;

    // localiza a fila no mapa
    spin_lock(&mqueue_map_lock);
    struct mqueue_t *q = (struct mqueue_t *)map_get(mqueue_map, queue_id);
    spin_unlock(&mqueue_map_lock);
    if (q == NULL)
    {
        current_task->is_system = prev;
        return ERROR;
    }

    // espera por vaga: fila cheia suspende a tarefa até uma retirada
    spin_lock(&q->lock);
    while (q->count >= q->max_msgs)
    {
        // fila cheia: suspende a tarefa no final da fila de espera
        spin_unlock(&q->lock);
        task_suspend(q->wait_send);

        // acordada (por mqueue_recv ou mqueue_destroy): sucesso somente
        // se a fila - o mesmo objeto - ainda existir no mapa
        struct mqueue_t *s = (struct mqueue_t *)map_get(mqueue_map, queue_id);
        if (s != q)
        {
            current_task->is_system = prev;
            return ERROR;
        }

        // fila ainda existe: reavalia a disponibilidade de vagas
        spin_lock(&q->lock);
    }

    // copia a mensagem para um buffer próprio e o insere na fila
    void *buf = mem_alloc(q->msg_size);
    if (buf == NULL)
    {
        spin_unlock(&q->lock);
        current_task->is_system = prev;
        return ERROR;
    }
    mem_copy(msg, buf, q->msg_size);
    queue_add(q->data, buf);
    q->count++;

    // se havia tarefas aguardando, acorda a primeira da fila de recepção
    struct task_t *t = (struct task_t *)queue_head(q->wait_recv);
    if (t != NULL)
        task_awake(t);

    spin_unlock(&q->lock);
    current_task->is_system = prev;
    return NOERROR;
}

int mqueue_recv(int queue_id, void *msg)
{
    if (queue_id < 0 || msg == NULL)
        return ERROR;

    // evita preempção durante as seções críticas check+suspend
    int prev = current_task->is_system;
    current_task->is_system = 1;

    // localiza a fila no mapa
    spin_lock(&mqueue_map_lock);
    struct mqueue_t *q = (struct mqueue_t *)map_get(mqueue_map, queue_id);
    spin_unlock(&mqueue_map_lock);
    if (q == NULL)
    {
        current_task->is_system = prev;
        return ERROR;
    }

    // espera por mensagem: fila vazia suspende a tarefa até um envio
    spin_lock(&q->lock);
    while (q->count == 0)
    {
        // fila vazia: suspende a tarefa no final da fila de espera
        spin_unlock(&q->lock);
        task_suspend(q->wait_recv);

        // acordada (por mqueue_send ou mqueue_destroy): sucesso somente
        // se a fila - o mesmo objeto - ainda existir no mapa
        struct mqueue_t *s = (struct mqueue_t *)map_get(mqueue_map, queue_id);
        if (s != q)
        {
            current_task->is_system = prev;
            return ERROR;
        }

        // fila ainda existe: reavalia a disponibilidade de mensagens
        spin_lock(&q->lock);
    }

    // retira a primeira mensagem da fila e a copia para o buffer
    void *buf = (void *)queue_head(q->data);
    queue_del(q->data, buf);
    q->count--;
    mem_copy(buf, msg, q->msg_size);
    mem_free(buf);

    // se havia tarefas aguardando vaga, acorda a primeira da fila de envio
    struct task_t *t = (struct task_t *)queue_head(q->wait_send);
    if (t != NULL)
        task_awake(t);

    spin_unlock(&q->lock);
    current_task->is_system = prev;
    return NOERROR;
}

int mqueue_msgs(int queue_id)
{
    if (queue_id < 0)
        return ERROR;

    int prev = current_task->is_system;
    current_task->is_system = 1;

    struct mqueue_t *q = (struct mqueue_t *)map_get(mqueue_map, queue_id);

    current_task->is_system = prev;
    return (q == NULL) ? ERROR : q->count;
}
