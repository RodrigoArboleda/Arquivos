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

void entry_free(IndexEntry *e){
    if (e == NULL) return;
    free(e);
}

void index_free(Index *idx){
    if (idx == NULL) return;
    for (int i = 0; i < idx->n_entries; i++) entry_free(idx->list[i]);
    free(idx->list);
    free(idx);
}

void print_entry(IndexEntry *e){
    if (e == NULL) puts("REGISTRO NULO");
    if (e->name[0] == '\0') printf("%06d : NOME NULO\n", (int)e->offset);
    else printf("%06d : %s\n", (int)e->offset, e->name);
}

Index *index_load_from_file(FILE *fp){
    int n_records = get_number_records(fp);

    fseek(fp, DISK_PG, SEEK_SET);

    Index *idx = index_create();
    for (int i = 0; i < n_records; i++) index_insert(idx, fp);
    if (PRINT_INDEX) for (int i = 0; i < n_records; i++) print_entry(idx->list[i]);

    return idx;
}

void grava_index(Index* index_ram,FILE* fi){
    fseek(fi, 1, SEEK_SET);
    fwrite(&(index_ram->n_entries), sizeof(int), 1 ,fi);
    
    for (int i = 0; i < 32000 - 5; ++i)
    {
        char lixo = '@';
        fwrite(&lixo, sizeof(char), 1, fi);
    }

    for (int i = 0; i < index_ram->n_entries; ++i)
    {
        fwrite(index_ram->list[i]->name, sizeof(char), 120, fi);
        fwrite(&(index_ram->list[i]->offset), sizeof(long long int), 1, fi);
    }
}

IndexEntry* cria_item(FILE *fp){
    IndexEntry *e = (IndexEntry *)malloc(sizeof(IndexEntry));
    if (e == NULL) return NULL;

    e->offset = ftell(fp);
    fseek(fp, 39, SEEK_CUR);
    int tamanho;
    char tag;
    fread(&tamanho, sizeof(int), 1 , fp);
    fread(&tag, sizeof(char), 1 , fp);

    if (tag != 'n')
    {
    	free(e);
    	return NULL;
    }

    fread(e->name, sizeof(char), tamanho - 1, fp);

    for (int i = tamanho-1; i < 120; ++i)
    {
    	e->name[i] = '@';
    }

    return e;
}

void insere_index_ordenado(Index *i, FILE *fp){
    if (i == NULL) return;
    
    IndexEntry* item_novo = cria_item(fp);
    if(item_novo == NULL) return;

    if (i->n_entries + 1 == i->max_size){
        i->max_size += 200;
        i->list = realloc(i->list, i->max_size*sizeof(IndexEntry *));
    }
    
    i->list[ i->n_entries++ ] = item_novo;
	
    int trocou = 1;

    //busca local de insercao
    for (int j = i->n_entries - 1; j > 0 && trocou == 1; --j)
    {
    	trocou = 0;
    	if (strcmp(i->list[j]->name, i->list[j - 1]->name) < 0)
    	{
    		IndexEntry * trocador;
    		trocador = 	i->list[j];
    		i->list[j] = i->list[j - 1];
    		i->list[j - 1] = trocador;	
    		trocou = 1;
    	}

    	if (strcmp(i->list[j]->name, i->list[j - 1]->name) == 0 && i->list[j]->offset < i->list[j - 1]->offset)
    	{
    		IndexEntry * trocador;
    		trocador = 	i->list[j];
    		i->list[j] = i->list[j - 1];
    		i->list[j - 1] = trocador;	
    		trocou = 1;
    	}
    }
}

