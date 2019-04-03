#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct record_ Record;

struct record_{
    int idServidor;
    double salarioServidor;
    char telefoneServidor[15];
    char nomeServidor[100];
    char cargoServidor[100];
};

Record *record_create(int id, double salario, char *telefone, char *nome, char *cargo){
    Record *r = (Record *)malloc(sizeof(Record));

    r->idServidor = id;
    r->salarioServidor = salario;

    strcpy(r->telefoneServidor, telefone);
    strcpy(r->nomeServidor, nome);
    strcpy(r->cargoServidor, cargo);

    return r;
}

void record_free(Record *r){
    if (r == NULL) return;
    free(r);
}

void record_print(Record *r){
    if (r == NULL) return;
    printf("%07d  |  %-14s  |  %-09.2lf  |  %-40s  |  %-30s\n", r->idServidor, r->telefoneServidor, r->salarioServidor, r->cargoServidor, r->nomeServidor);
}

Record *read_record(FILE *fp){
    int id;
    double salario;
    char telefone[15] = {'\0'}, nome[100] = {'\0'}, cargo[100] = {'\0'};

    fscanf(fp, "%d", &id);
    fscanf(fp, "%*c");
    fscanf(fp, "%[^,]", telefone);
    fscanf(fp, "%*c");
    fscanf(fp, "%lf", &salario);
    fscanf(fp, "%*c");
    fscanf(fp, "%[^,]", cargo);
    fscanf(fp, "%*c");
    fscanf(fp, "%[^\n]", nome);
    fscanf(fp, "%*c");

    return record_create(id, salario, telefone, nome, cargo);
}

void read_file(){
    FILE *fp = fopen("file.csv", "r");

    fscanf(fp, "%*[^\n]");  // Skips first line
    fscanf(fp, "%*[\n]");

    Record *r;
    for (int i = 0; i < 30; i++){
        r = read_record(fp);
        record_print(r);
        record_free(r);
    }

    fclose(fp);
}

int main(){
    read_file();
}
