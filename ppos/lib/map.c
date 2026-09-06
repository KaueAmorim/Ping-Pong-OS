// Implementação do TAD Mapa de objetos

#include <stdio.h>
#include <stdlib.h>

#include "map.h"

struct map_t {
    void **items;
    int num_items;
    int capacity;
};

struct map_t *map_create(int size){
    if (size < 1) return NULL;
    struct map_t *map;
    if (!(map = malloc(sizeof(struct map_t))) || !(map->items = malloc(sizeof(void *)*size))) return NULL;


    for (int i = 0; i < size; i++){
        map->items[i] = NULL;
    }

    map->num_items = 0;
    map->capacity = size;

    return map;
}

int map_destroy(struct map_t *map){
    if(!map) return ERROR;

    if(map->items) free(map->items);

    free(map);
    return NOERROR;
}

int map_put(struct map_t *map, void *object){
    if (!map) return ERROR;

    int new_id = 0;
    while (new_id < map_size(map) && map->items[new_id] != NULL) {
        new_id++;
    }

    if (new_id >= map->capacity) return ERROR;

    map->items[new_id] = object;
    map->num_items++;

    return new_id;
}

void *map_get(struct map_t *map, int id){
    if (!map || id >= map_size(map)) return NULL;
    
    return map->items[id];
}

void *map_del(struct map_t *map, int id){
    if (!map || id >= map_size(map)) return NULL;

    void *del_item = map->items[id];
    map->items[id] = NULL;
    map->num_items--;

    return del_item;
}

int map_items(struct map_t *map){
    if (!map) return ERROR;
   
    return map->num_items;    
}

int map_size(struct map_t *map){
    if (!map) return ERROR;

    return map->capacity;
}

void map_print(char *name, struct map_t *map){
    if(map){
        printf("%s: [ ", name);
        for (int i = 0; i < map_size(map); i++){
            if (map_get(map, i) != NULL) printf("* ");
            else printf("- ");
        }
        printf("] (%d/%d)\n", map_items(map), map_size(map));
    }
    else{
        printf("%s: undef\n", name);
    }
}
