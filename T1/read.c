#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISK_PG 32000
#define TRASH '@'

typedef struct record_ Record;
typedef struct queue_ Queue;
typedef struct node_ Node;

struct record_{
    int idServidor;
    double salarioServidor;
    char telefoneServidor[15];
    char nomeServidor[100];
    char cargoServidor[100];
};

struct node_{
    Node *next;
    Record *record;
};

struct queue_{
    Node *head;
    Node *end;
};

Node *node_create(){
    Node *n = (Node *)malloc(szieof(Node));
    n->next = NULL;
    n->record = NULL;
    return n;
}

void node_free(Node *n){
    if (n == NULL) return;
    record_free(n->record);
    free(n);
}

Queue *queue_create(){
    Queue *q = (Queue *)malloc(sizeof(Queue));

    q->head = node_create();
    q->end = q->head;

    return q;
}

void recursive_destruction(Node *cur){
    if (cur == NULL) return;
    node_free(cur);
}

void queue_free(Queue *q){
    if (q == NULL) return;
    recursive_destruction(q->head);
    free(q);
}

void queue_insert(Queue *q, Record *r){
    if (q == NULL || r == NULL) return;
    q->end->next = node_create(r);
    q->end = q->end->next;
}

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

void replace_trash(char *str, char *buffer){
    memset(buffer, TRASH, 40*sizeof(char));
    sprintf(buffer, "%s", str);
}

void write_header(FILE *fp){
    if (fp == NULL){
        // ERRO
        return;
    }
    long temp = -1;

    char trash[DISK_PG + 1];
    memset(trash, '@', sizeof(trash));

    fwrite(trash, sizeof(char), DISK_PG, fp);
    fseek(fp, 0L, SEEK_SET);

    fwrite("1", sizeof(char), 1, fp);
    fwrite(&temp, sizeof(long), 1, fp);

    char string[40];

    replace_trash("numero de identificacao do servidor", string);
    fwrite("i", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash("salario do servidor", string);
    fwrite("s", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash("telefone celular do servidor", string);
    fwrite("t", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash("nome do servidor", string);
    fwrite("n", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash("cargo do servidor", string);
    fwrite("c", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    fseek(fp, (long)DISK_PG, SEEK_SET);    // coloca o fp no final da página de disco
}

void print_binary_file(FILE *fp){
    fseek(fp, 0L, SEEK_SET);
    while (!feof(fp)){
        fwrite(fp, sizeof(char), 1, stdout);
    }
}

void flip_safety_bit(FILE *fp){
    fseek(fp, 0L, SEEK_SET);

    char temp;
    fread(fp, sizeof(char), 1, &temp);
    fseek(fp, 0L, SEEK_SET);

    if (temp == '0'){
        fwrite("1", sizeof(char), 1, fp);
    } else {
        fwrite("0", sizeof(char), 1, fp);
    }
}

Queue *read_file(){
    FILE *input = fopen("file.csv", "r");

    fscanf(input, "%*[^\n]");  // Skips first line
    fscanf(input, "%*[\n]");

    Record *r;
    for (int i = 0; i < 30; i++){
        r = read_record(input);
        record_print(r);
        record_free(r);
    }

    flip_safety_bit(output);
    fclose(input);
}

int main(){
    Queue *q = read_file();

    FILE *fp = fopen("test.bin", "wb");
    write_header(fp);
    //print_binary_file(fp);

    queue_free(q);
    fclose(fp);
}
