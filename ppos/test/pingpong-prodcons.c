// Sistema produtor/consumidor com buffer limitado

#include <assert.h>
#include "lib/pplibc.h"
#include "syscall.h"

#define NUM_PRODUTORES 3    // nº de tarefas produtoras
#define NUM_CONSUMIDORES 2  // nº de tarefas consumidoras
#define NUM_ITENS 10        // itens produzidos por cada produtor
#define BUFFER_SIZE 5       // capacidade do buffer (elementos)

// descritores de semáforos
int s_buffer;   // exclusão mútua de acesso ao buffer (valor inicial 1)
int s_item;     // nº de itens disponíveis no buffer (valor inicial 0)
int s_vaga;     // nº de vagas livres no buffer (valor inicial 5)

// buffer circular de inteiros, com política FIFO
int buffer[BUFFER_SIZE];
int buffer_head = 0;   // posição de retirada (FIFO)
int buffer_tail = 0;   // posição de inserção
int buffer_count = 0;  // nº de itens atualmente no buffer

// contador de itens já consumidos (protegido por s_buffer)
int consumidos = 0;

void produtor(void *arg)
{
    int status;

    for (int i = 0; i < NUM_ITENS; i++)
    {
        // produz um item a cada 1000 ms
        task_sleep(1000);
        int item = randnum() % 100;

        // espera por vaga no buffer
        status = sem_down(s_vaga);
        assert(status == NOERROR);

        // insere o item no buffer, em exclusão mútua
        status = sem_down(s_buffer);
        assert(status == NOERROR);
        buffer[buffer_tail] = item;
        buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
        buffer_count++;
        sem_up(s_buffer);

        // informa a produção antes de liberar o item para consumo,
        // para que a saída mostre cada item antes do seu consumo
        printk("%s produziu %d\n", task_name(NULL), item);

        // sinaliza um item disponível
        status = sem_up(s_item);
        assert(status == NOERROR);
    }

    task_exit(NOERROR);
}

// corpo de uma tarefa consumidora
void consumidor(void *arg)
{
    int status;

    while (1)
    {
        // espera por um item disponível
        status = sem_down(s_item);
        assert(status == NOERROR);

        // retira o item do buffer, em exclusão mútua
        status = sem_down(s_buffer);
        assert(status == NOERROR);
        int item = buffer[buffer_head];
        buffer_head = (buffer_head + 1) % BUFFER_SIZE;
        buffer_count--;
        consumidos++;
        sem_up(s_buffer);

        // libera a vaga ocupada pelo item
        status = sem_up(s_vaga);
        assert(status == NOERROR);

        printk("                             %s consumiu %d\n",
               task_name(NULL), item);

        // "processa" o item consumido
        task_sleep(1000);
    }
}

// corpo da tarefa principal
void user_main(void *arg)
{
    int status;
    char *name;

    randseed(1);

    name = task_name(NULL);
    printk("%5u ms: %s: inicio\n", time(), name);

    // inicia semáforos: 1 mutex, 0 itens, 5 vagas
    s_buffer = sem_create(1);
    assert(s_buffer >= 0);
    s_item   = sem_create(0);
    assert(s_item >= 0);
    s_vaga   = sem_create(BUFFER_SIZE);
    assert(s_vaga >= 0);

    // cria as tarefas produtoras e consumidoras
    struct task_t *prod[NUM_PRODUTORES];
    struct task_t *cons[NUM_CONSUMIDORES];
    for (int i = 0; i < NUM_PRODUTORES; i++)
    {
        char *nomes_p[] = {"p1", "p2", "p3"};
        prod[i] = task_create(nomes_p[i], produtor, NULL);
        assert(prod[i]);
    }
    for (int i = 0; i < NUM_CONSUMIDORES; i++)
    {
        char *nomes_c[] = {"c1", "c2"};
        cons[i] = task_create(nomes_c[i], consumidor, NULL);
        assert(cons[i]);
    }

    // aguarda todos os produtores encerrarem
    for (int i = 0; i < NUM_PRODUTORES; i++)
    {
        status = task_wait(prod[i]);
        assert(status == NOERROR);
    }

    // espera até que todos os itens produzidos tenham sido consumidos
    while (consumidos < NUM_PRODUTORES * NUM_ITENS)
        task_sleep(100);

    printk("%s: %d itens produzidos e consumidos\n",
           name, consumidos);

    // encerra as tarefas consumidoras (bloqueadas em s_item)
    for (int i = 0; i < NUM_CONSUMIDORES; i++)
    {
        status = task_destroy(cons[i]);
        assert(status == NOERROR);
    }

    // destrói os semáforos
    status = sem_destroy(s_buffer);
    assert(status == NOERROR);
    status = sem_destroy(s_item);
    assert(status == NOERROR);
    status = sem_destroy(s_vaga);
    assert(status == NOERROR);

    // libera as tarefas produtoras encerradas
    for (int i = 0; i < NUM_PRODUTORES; i++)
    {
        status = task_destroy(prod[i]);
        assert(status == NOERROR);
    }

    printk("%5u ms: %s: fim\n", time(), name);

    task_exit(NOERROR);
}
