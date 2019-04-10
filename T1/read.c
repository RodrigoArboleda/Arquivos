#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Victor Giovannoni 10786159 */

#define DISK_PG 32000
#define TRASH '@'
#define INVALID -1337

typedef struct record_ Record;
typedef struct queue_ Queue;
typedef struct node_ Node;

struct record_{
    char nomeServidor[100];
    char cargoServidor[100];
    char telefoneServidor[15];
    double salarioServidor;
    int idServidor;
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
    printf("%08d  |  %-14s  |  %-9.2lf  |  %-40s  |  %-30s\n", r->idServidor, r->telefoneServidor, r->salarioServidor, r->cargoServidor, r->nomeServidor);
}

Record *read_record(FILE *fp){
    int id = INVALID;
    double salario = INVALID;
    char telefone[15], nome[100], cargo[100];
    memset(telefone, '\0', sizeof(telefone));
    memset(nome, '\0', sizeof(nome));
    memset(cargo, '\0', sizeof(cargo));

    fscanf(fp, "%d", &id);
    fscanf(fp, "%*c");
    fscanf(fp, "%lf", &salario);
    fscanf(fp, "%*c");
    fscanf(fp, "%[^,]", telefone);
    fscanf(fp, "%*c");
    fscanf(fp, "%[^,]", nome);
    fscanf(fp, "%*c");
    fscanf(fp, "%[^\r\n]", cargo);
    fscanf(fp, "%*[\r\n]");


    if (id == INVALID || salario == INVALID) return NULL;
    return record_create(id, salario, telefone, nome, cargo);
}

/* Preenche o buffer com lixo e depois escreve str por cima */
void replace_trash(char *str, char *buffer, int buffer_size){
    memset(buffer, TRASH, buffer_size*sizeof(char));
    sprintf(buffer, "%s", str);
}

void write_header(FILE *fp, char *filename){
    if (fp == NULL){
        // ERRO
        return;
    }
    long long temp = -1;

    FILE *source = fopen(filename, "r");

    char tag1[40], tag2[40], tag3[40], tag4[40], tag5[40];
    fscanf(source, "%[^,],%[^,],%[^,],%[^,],%[^\n]\n", tag1, tag2, tag3, tag4, tag5);


    // inicializo a pagina de disco com lixo, depois retorno o ponteiro ao inivio
    char trash[DISK_PG + 1];
    memset(trash, TRASH, sizeof(trash));
    fwrite(trash, sizeof(char), DISK_PG, fp);
    fseek(fp, (long)0, SEEK_SET);

    fwrite("1", sizeof(char), 1, fp);
    fwrite(&temp, sizeof(long long), 1, fp);

    char string[40];

    replace_trash(tag1, string, 40);
    fwrite("i", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash(tag2, string, 40);
    fwrite("s", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash(tag3, string, 40);
    fwrite("t", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash(tag4, string, 40);
    fwrite("n", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    replace_trash(tag5, string, 40);
    fwrite("c", sizeof(char), 1, fp);
    fwrite(string, sizeof(char), 40, fp);

    fseek(fp, (long)DISK_PG, SEEK_SET);    // coloca o fp no final da página de disco

    fclose(source);
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

Queue *read_csv_file(char *file_name){
    FILE *input = fopen(file_name, "r");
    if (input == NULL){
        printf("Falha no carregamento do arquivo.");
        exit(0);
    }
    Queue *q = queue_create();

    fscanf(input, "%*[^\n]");  // pula a primeira linha do csv
    fgetc(input);

    Record *r = read_record(input);   // read record retorna NULL se acabou o arquivo
    while (r != NULL){
        queue_insert(q, r);
        r = read_record(input);
    }

    fclose(input);
    return q;
}

void print_queue(Queue *q){
    Node *cur = q->head->next;
    for (int i = 0; cur != NULL; i++){
        record_print(cur->record);
        cur = cur->next;
    }
}

/* Retorna o tamanho do registro completo, incluindo os dois primeiros campos */
int record_size(Record *r){
    int ans = 1;
    ans += sizeof(int);
    ans += sizeof(long long);   // encadeamento lista
    ans += sizeof(r->idServidor);
    ans += sizeof(r->salarioServidor);
    ans += sizeof(r->telefoneServidor) - 1;

    if (r->nomeServidor[0] != '\0'){
        ans += sizeof(int);     // tamanho do campo
        ans += sizeof(char);    // tag
        ans += 1 + strlen(r->nomeServidor);
    }
    if (r->cargoServidor[0] != '\0'){
        ans += sizeof(int);     // tamanho do campo
        ans += sizeof(char);    // tag
        ans += 1 + strlen(r->cargoServidor);
    }
    return ans;
}

int bytes_until_new_page(FILE *fp){
    return DISK_PG - ftell(fp)%DISK_PG;
}

void write_record(FILE *fp, Record *r, int *bytes, int *pages, int last_record){
    int size = record_size(r);
    if ((*bytes + size)/(double)DISK_PG > 1){
        int trash_amount = bytes_until_new_page(fp);

        char trash[DISK_PG];
        memset(trash, TRASH, sizeof(trash));
        fwrite(trash, sizeof(char),  trash_amount ,fp);

        int prev_rec_size;
        fseek(fp, (long)(last_record + sizeof(char)), SEEK_SET); // volta para tam. do registro anterior
        fread(&prev_rec_size, sizeof(int), 1, fp);
        prev_rec_size += trash_amount;

        fseek(fp, -(long)sizeof(int), SEEK_CUR);            // volta p/ reescrever
        fwrite(&prev_rec_size, sizeof(int), 1, fp);         // atualiza tamanho anterior

        fseek(fp, (long)(*pages*DISK_PG), SEEK_SET);             // volta para o começo da nova pagina
        (*pages)++;
        *bytes = 0;  // escreve 0 bytes na nova pagina de disco
    }

    *bytes += size;

    long long temp1 = -1;
    size -= 5;  /* desconsidero os 5 primeiros bytes pra escrever no registro */

    fwrite("-", sizeof(char), 1, fp);
    fwrite(&size, sizeof(int), 1, fp);              // tamanho do registro, -1 pq nao devo contar o '*' ou '-'
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
        fwrite(r->nomeServidor, sizeof(char), str_size, fp);
    }

    if (r->cargoServidor[0] != '\0'){
        str_size = 1 + strlen(r->cargoServidor);
        fwrite(&str_size, sizeof(int), 1, fp);
        fwrite("c", sizeof(char), 1, fp);
        fwrite(r->cargoServidor, sizeof(char), str_size, fp);
    }
}

void write_file_body(FILE *fp, Queue *q){
    Node *cur = q->head->next;
    int bytes_written = 0;
    int disk_pages = 2;

    int last_record = -1;
    int current_record = ftell(fp);
    while (cur != NULL){
        write_record(fp, cur->record, &bytes_written, &disk_pages, last_record);
        last_record = current_record;
        current_record = ftell(fp);
        cur = cur->next;
    }
}

void write_file(char *file_name){
    Queue *q = read_csv_file(file_name);

    FILE *fp = fopen("arquivoTrab1.bin", "wb+");
    if (fp == NULL){
        printf("Falha no carregamento do arquivo.");
        exit(0);
    }

    write_header(fp, file_name);
    write_file_body(fp, q);
    queue_free(q);
    fclose(fp);
}

int ceiling(double x){
    if (x == (int)x) return (int)x;
    return (int)x + 1;
}

Record *read_record_binary(FILE *fp, int *paginas){
    int size, id;
    char removed, telefone[15];
    double salario;
    long long index;


    fread(&removed, sizeof(char), 1, fp);   // ler byte removido
    fread(&size, sizeof(int), 1, fp);       // le tamanho
    int start_pos = ftell(fp);

    if (removed == '*'){
        fseek(fp, (long)(size), SEEK_CUR);
        return NULL;
    } else if (removed != '-'){         // pular pra proxima pagina de disco
        (*paginas)++;
        long current_pos = ftell(fp);
        int pagina_atual = 1 + ceiling(current_pos/(double)DISK_PG);

        int end = fseek(fp, (long)pagina_atual*DISK_PG, SEEK_SET);
        if (end == -1) return (Record *)0x01;
        return NULL;
    }


    fread(&index, sizeof(long long), 1, fp); // le inicio da lista
    fread(&id, sizeof(int), 1, fp);          // le id do funcionario
    fread(&salario, sizeof(double), 1, fp);  // le salario
    fread(telefone, sizeof(char), 14, fp);   // le telefone
    telefone[14] = '\0';

    int string_size;
    char tag, nome[100], cargo[100];
    memset(nome, '\0', sizeof(nome));
    memset(cargo, '\0', sizeof(cargo));

    while (ftell(fp) != start_pos + size){
        fread(&string_size, sizeof(int), 1, fp);
        fread(&tag, sizeof(char), 1, fp);
        if (tag == 'n') fread(nome, sizeof(char), string_size, fp);
        else if (tag == 'c') fread(cargo, sizeof(char), string_size, fp);
        else break; // chegou no fim do registro
    }

    return record_create(id, salario, telefone, nome, cargo);
}

int safety_byte(FILE *fp){
    int byte;
    fseek(fp, 0L, SEEK_SET);
    fread(&byte, sizeof(int), 1, fp);
    return byte;
}

void record_ugly_print(Record *r){
    if (r == NULL) return;

    printf("%d ", r->idServidor);
    printf("%.2lf ", r->salarioServidor);
    printf("%-14s ", r->telefoneServidor);

    if (r->nomeServidor[0] != '\0') printf("%d %s ", strlen(r->nomeServidor), r->nomeServidor);
    if (r->cargoServidor[0] != '\0') printf("%d %s", strlen(r->cargoServidor), r->cargoServidor);
    putchar('\n');

}

void read_binary_file(char *file_name){

    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL || safety_byte(fp) == 0){
        printf("Falha no carregamento do arquivo.");
        exit(0);
    }

    // char tag1[41], tag2[41], tag3[41], tag4[41], tag5[41];  // leio a tag e o descritor. tag[0] = caracter que a define
    // fseek(fp, (long)9, SEEK_SET);
    // fread(tag1, sizeof(char), 40, fp);
    // fread(tag2, sizeof(char), 40, fp);
    // fread(tag3, sizeof(char), 40, fp);
    // fread(tag4, sizeof(char), 40, fp);
    // fread(tag5, sizeof(char), 40, fp);

    fseek(fp, (long)DISK_PG, SEEK_SET); // pula o cabeçalho

    int paginas = 2, i;
    for (i = 0;  ; i++){
        Record *r = read_record_binary(fp, &paginas); // retorna 0x01 quando acaba o arquivo
        if (r == (Record *)0x01) break;

        record_ugly_print(r);
        record_free(r);
    }
    if (i == 0){
        printf("Registro inexistente.");
        exit(0);
    }

    printf("Numero de paginas de disco acessadas: %d\n", ceiling(ftell(fp)/(double)DISK_PG));
    fclose(fp);
}

int main(){
   //write_file("file.csv");
   read_binary_file("arquivoTrab1.bin");
}
