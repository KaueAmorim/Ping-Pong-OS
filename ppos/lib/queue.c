// PingPongOS - PingPong Operating System
// © Prof. Carlos A. Maziero, DINF UFPR
// Versão 2.1 -- 06/2026

// Este arquivo PODE/DEVE ser alterado.

// Implementação do TAD fila genérica

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

struct node_t {
    struct node_t *next;            // Próximo elemento
    void *item;                     // Item (qualquer) apontado
};

struct queue_t{
    struct node_t *head;            // Primeiro elemento da fila
    struct node_t *tail;            // Último elemento da fila
    struct node_t *it;              // Elemento atual apontado
    int size;                       // Tamanho da fila
};

struct queue_t *queue_create(){
    struct queue_t *queue;

    if (!(queue = malloc(sizeof(struct queue_t)))) return NULL;

    queue->it = queue->tail = queue->head = NULL;
    queue->size = 0;

    return queue;
}

int queue_destroy(struct queue_t *queue){
    if (!queue) return ERROR;

    free(queue);
    return NOERROR;
}

int queue_add(struct queue_t *queue, void *item){
    if (!queue || !item) return ERROR;

    struct node_t *new = malloc(sizeof(struct node_t));
    if(!new) return ERROR;

    new->item = item;
    new->next = NULL;
    
    // Ajuste de ponteiros + iterador
    if (queue->size == 0){
        queue->it = queue->tail = queue->head = new;
    }
    // Ajuste de ponteiros
    else{
        queue->tail = queue->tail->next = new;
    }
    queue->size++;

    return NOERROR;
}

int queue_del(struct queue_t *queue, void *item){
    if (!queue || !item) return ERROR;

    struct node_t *curr, *prev;
    curr = queue->head;
    while(curr){
        if (curr->item == item) break;

        prev = curr;
        curr = curr->next;
    }

    // Não encontrado
    if (!curr) return ERROR;

    // Ajuste de iterador
    if (curr == queue->it){
        queue->it = curr->next;
    }

    // Casos particulares
    if (queue_size(queue) == 1){
        queue->it = queue->tail = queue->head = NULL;
    }
    else if (curr == queue->head) {
        queue->head = queue->head->next;
    }
    else if (curr == queue->tail) {
        queue->tail = prev;
        queue->tail->next = NULL;
    }
    // Caso geral
    else {
        prev->next = curr->next;
    }


    free(curr);
    curr = NULL;
    queue->size--;

    return NOERROR;
}

bool queue_has(struct queue_t *queue, void *item){
    if (!queue) return false;

    struct node_t *curr = queue->head;

    while(curr){
        if (curr->item == item) return true;
        curr = curr->next;
    }

    return false;
}

int queue_size(struct queue_t *queue){
    if (!queue) return ERROR;
    
    return queue->size;
}

void *queue_head(struct queue_t *queue){
    if (!queue || !queue_size(queue)) return NULL;
    
    queue->it = queue->head;
    return queue->it->item;
}

void *queue_next(struct queue_t *queue){
    if (!queue || !queue_size(queue) || !(queue->it = queue->it->next)) return NULL;

    return queue->it->item;
}

void *queue_item(struct queue_t *queue){
    if (!queue || !queue_size(queue) || !queue->it) return NULL;

    return queue->it->item;
}

void queue_print(char *name, struct queue_t *queue, void (*func)(void *)){
    if (func && queue){
        struct node_t *curr = queue->head;    
        printf("%s: [ ", name);
        for (int i = 0; i < queue_size(queue); ++i){
            func(curr->item);
            printf(" ");
            curr = curr->next;
        }
        printf("] (%d items)\n", queue_size(queue));
    }
    else if (!func && queue){
        printf("%s: [", name);
        for (int i = 0; i < queue_size(queue); ++i){
            printf(" undef");
        } 
        printf(" ] (%d items)\n", queue_size(queue));
    }else if (!queue){
        printf("%s: undef\n", name);
    }
}
