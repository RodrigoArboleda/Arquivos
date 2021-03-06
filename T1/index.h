#ifndef INDEX_H
#define INDEX_H

typedef struct index_entry_ IndexEntry;
typedef struct index_ Index;

struct index_entry_{
    char name[120];
    long long offset;
};

struct index_{
    IndexEntry **list;  // vetor de IndexEntries
    int max_size;       // tamanho maximo (aumenta caso tenha mais registros)
    int n_entries;      // numero de registros atualmente
};

Index *index_create();

Index *index_load_from_file(FILE *fp);

IndexEntry *entry_create(char name[120], long long offset);

IndexEntry *entry_create_fp(FILE *fp);

void index_insert(Index *i, FILE *fp);

void index_free(Index *idx);

void index_search_by_name(Index *idx, char *key);

/*
Fucao:Grava no arquivo fi o index OBS: n grava o cabecalho so muda o quanto tem
Parametro:
			index_ram: ponteiro por index
			fi: ponteiro pro arquivo
*/
void grava_index(Index* index_ram, FILE* fi);

/*
Fucao: ordena um index
Parametro:
			index_ram: ponteiro por index
*/
void insere_index_ordenado(Index *i, FILE *fp);

#endif
