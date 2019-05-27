#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "index.h"
#include "utils.h"

#define INIT_SIZE 6000

Index *index_create(){
    Index *i = (Index *)malloc(sizeof(Index));
    if (i == NULL) return NULL;

    i->max_size = INIT_SIZE;
    i->n_entries = 0;
    i->list = (IndexEntry **)malloc(INIT_SIZE*sizeof(IndexEntry *));

    return i;
}

IndexEntry *entry_create(char name[120], long long offset){
    IndexEntry *e = (IndexEntry *)malloc(sizeof(IndexEntry));
    if (e == NULL) return NULL;

    strcpy(e->name, name);
    e->offset = offset;
    return e;
}

IndexEntry *entry_create_fp(FILE *fp){
    IndexEntry *e = (IndexEntry *)malloc(sizeof(IndexEntry));
    if (e == NULL) return NULL;

    fread(e->name, sizeof(char), 120, fp);
    fread(&e->offset, sizeof(long long), 1, fp);
    return e;
}

void index_insert(Index *i, FILE *fp){
    if (i == NULL) return;
    if (i->n_entries + 1 == i->max_size){
        i->max_size += 200;
        i->list = realloc(i->list, i->max_size*sizeof(IndexEntry *));
    }
    i->list[ i->n_entries++ ] = entry_create_fp(fp);
}

void index_free(Index *idx){
    if (idx == NULL) return;
    for (int i = 0; i < idx->n_entries; i++) entry_free(idx->list[i]);
    free(idx->list);
    free(idx);
}

void entry_free(IndexEntry *e){
    if (e == NULL) return;
    free(e);
}

Index *index_load_from_file(FILE *fp){
    int n_records = get_number_records(fp);

    fseek(fp, DISK_PG, SEEK_SET);

    Index *idx = index_create();
    for (int i = 0; i < n_records; i++) index_insert(idx, fp);

    return idx;
}
