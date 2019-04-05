#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISK_PG 32000
#define TRASH '@'
#define N_RECORDS 5000

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

Node *node_create(Record *r){
    Node *n = (Node *)malloc(sizeof(Node));
    n->next = NULL;
    n->record = r;
    return n;
}

void node_free(Node *n){
    if (n == NULL) return;
    record_free(n->record);
    free(n);
}

Queue *queue_create(){
    Queue *q = (Queue *)malloc(sizeof(Queue));

    q->head = node_create(NULL);
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

/* Preenche o buffer com lixo e depois escreve str por cima */
void replace_trash(char *str, char *buffer, int buffer_size){
    memset(buffer, TRASH, buffer_size*sizeof(char));
    sprintf(buffer, "%s", str);
}

void write_header(FILE *fp){
    if (fp == NULL){
        // ERRO
        return;
    }
    long long temp = -1;

    char trash[DISK_PG + 1];
    memset(trash, '@', sizeof(trash));
    fwrite(trash, sizeof(char), DISK_PG, fp);
    fseek(fp, (long)0, SEEK_SET);               // nova pagina de disco

    fwrite("1", sizeof(char), 1, fp);
    fwrite(&temp, sizeof(long long), 1, fp);

    char string[40];

    replace_trash("numero de identificacao do servidor", string, 40);
    fwrite("i", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash("salario do servidor", string, 40);
    fwrite("s", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash("telefone celular do servidor", string, 40);
    fwrite("t", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash("nome do servidor", string, 40);
    fwrite("n", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash("cargo do servidor", string, 40);
    fwrite("c", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    fseek(fp, (long)DISK_PG, SEEK_SET);    // coloca o fp no final da página de disco
}

void print_binary_file(FILE *fp){
    fseek(fp, 0L, SEEK_SET);

    char temp;
    while (!feof(fp)){
        fread(&temp, sizeof(char), 1, fp);
        printf("%c", temp);
    }
}

void flip_safety_bit(FILE *fp){
    fseek(fp, 0L, SEEK_SET);

    char temp;
    fread(&temp, sizeof(char), 1, fp);
    fseek(fp, 0L, SEEK_SET);

    if (temp == '0'){
        fwrite("1", sizeof(char), 1, fp);
    } else {
        fwrite("0", sizeof(char), 1, fp);
    }
}

Queue *read_file(){
    FILE *input = fopen("file.csv", "r");
    Queue *q = queue_create();

    fscanf(input, "%*[^\n]");  // Skips first line
    fscanf(input, "%*[\n]");

    Record *r;
    int i = 0;
    for (i = 0; i < N_RECORDS; i++){
        r = read_record(input);
        queue_insert(q, r);
    }

    fclose(input);
    return q;
}

void print_queue(Queue *q){
    Node *cur = q->head->next;
    while (cur != NULL){
        record_print(cur->record);
        cur = cur->next;
    }
}

int record_size(Record *r){
    int ans = 1;                // byte removido
    ans += sizeof(int);         // tamanho registro
    ans += sizeof(long long);   // encadeamento lista
    ans += sizeof(r->idServidor);
    ans += sizeof(r->salarioServidor);
    ans += sizeof(r->telefoneServidor) - 1;

    if (r->nomeServidor[0] != '\0'){
        ans += sizeof(int);     // tamanho do campo
        ans += sizeof(char);    // tag
        ans += strlen(r->nomeServidor);
    }
    if (r->cargoServidor[0] != '\0'){
        ans += sizeof(int);     // tamanho do campo
        ans += sizeof(char);    // tag
        ans += strlen(r->cargoServidor);
    }
    return ans;
}

/* cria nova pagina de disco inicializada com lixo e avança o FP para o inicio da nova pg., incrementa pages */
void create_new_disk_page(FILE *fp, int *pages){
    char trash[DISK_PG + 1];
    memset(trash, '@', sizeof(trash));

    fwrite(trash, sizeof(char), DISK_PG, fp);
    // VERIFICAR ERRO AQUI NO ++
    fseek(fp, (long)(++(*pages)*DISK_PG), SEEK_SET);    // posiciona o ptr no começo da proxima pagina
}

void write_record(FILE *fp, Record *r, int *bytes, int *pages){
    int size = record_size(r);
    if ((*bytes + size)/DISK_PG > 1){
        create_new_disk_page(fp, pages);
        *bytes = 0;  // escreve 0 bytes na nova pagina de disco
    }

    *bytes += size;     // incrementa a qt. bytes escritos

    int temp = size - 1;
    long long temp1 = -1;

    fwrite("-", sizeof(char), 1, fp);
    fwrite(&temp, sizeof(int), 1, fp);              // tamanho do registro, -1 pq nao devo contar o '*' ou '-'
    fwrite(&temp1, sizeof(long long), 1, fp);       // encadeamento
    fwrite(&(r->idServidor), sizeof(int), 1, fp);
    fwrite(&(r->salarioServidor), sizeof(double), 1, fp);

    char telefone[14];
    replace_trash(r->telefoneServidor, telefone, 14);
    fwrite(telefone, sizeof(char), 14, fp);

    int str_size;
    if (r->nomeServidor[0] != '\0'){
        str_size = 1 + strlen(r->nomeServidor);
        fwrite(&str_size, sizeof(int), 1, fp);
        fwrite("n", sizeof(char), 1, fp);
        fwrite(r->nomeServidor, sizeof(char), str_size - 1, fp);
    }

    if (r->nomeServidor[0] != '\0'){
        str_size = 1 + strlen(r->cargoServidor);
        fwrite(&str_size, sizeof(int), 1, fp);
        fwrite("c", sizeof(char), 1, fp);
        fwrite(r->cargoServidor, sizeof(char), str_size - 1, fp);
    }
}

void write_file_body(FILE *fp, Queue *q){
    Node *cur = q->head->next;
    int bytes_written = 0;
    int disk_pages = 1;

    while (cur != NULL){
        write_record(fp, cur->record, &bytes_written, &disk_pages);
        cur = cur->next;
    }
}

void write_file(){
    Queue *q = read_file();

    FILE *fp = fopen("test.bin", "wb+");
    write_header(fp);
    write_file_body(fp, q);

    //print_queue(q);
    //print_binary_file(fp);
    queue_free(q);
    fclose(fp);
}

int main(){
    write_file();
}
