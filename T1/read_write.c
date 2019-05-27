#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_write.h"
#include "escrevernatela.h"
#include "list.h"
#include "utils.h"
#include "index.h"

/* Victor Giovannoni 10786159 */
/* OBS: Em alguns comentários, quando digo que a função não altera o fp,
   quero dizer que não altera a posição do fp que foi passada como parâmetro */

#define TRASH '@'
#define INVALID -1337
#define MAX_SIZE 200
#define TAG_SIZE 41

typedef struct record_ Record;

struct record_{
    char nomeServidor[MAX_SIZE];
    char cargoServidor[MAX_SIZE];
    char telefoneServidor[15];
    double salarioServidor;
    int idServidor;
    long long byte_offset;
};

Record *record_create(int id, double salario, char *telefone, char *nome, char *cargo, long long byte_offset){
    Record *r = (Record *)malloc(sizeof(Record));

    r->idServidor = id;
    r->salarioServidor = salario;
    r->byte_offset = byte_offset;

    memcpy(r->telefoneServidor, telefone, sizeof(r->telefoneServidor));
    memcpy(r->nomeServidor, nome, sizeof(r->nomeServidor));
    memcpy(r->cargoServidor, cargo, sizeof(r->cargoServidor));

    return r;
}

void record_free(Record *r){
    if (r == NULL) return;
    free(r);
}

int ceiling(double x){
    if (x == (int)x) return (int)x; // pega arredondamento pq cima de x
    return (int)x + 1;
}

void read_tags(FILE *fp, char tags[][TAG_SIZE]){
    fseek(fp, 9L, SEEK_SET);

    fread(tags[0], sizeof(char), TAG_SIZE, fp);
    fread(tags[1], sizeof(char), TAG_SIZE, fp);
    fread(tags[2], sizeof(char), TAG_SIZE, fp);
    fread(tags[3], sizeof(char), TAG_SIZE, fp);
    fread(tags[4], sizeof(char), TAG_SIZE, fp);
}

/* atualiza o byte de segurança */
void set_safety_byte(FILE *fp, char new_val){
    fseek(fp, 0L, SEEK_SET);
    fwrite(&new_val, sizeof(char), 1, fp);
}

/* retorna o byte de segurança sem alterar o fp */
char safety_byte(FILE *fp){
    long start_pos = ftell(fp);
    char byte;
    fseek(fp, 0L, SEEK_SET);
    fread(&byte, sizeof(char), 1, fp);
    fseek(fp, start_pos, SEEK_SET);   // retorna pra pos. original
    return byte;
}

/* Retorna nulo caso haja algum erro no arquivo, caso contrario retorna fp */
FILE *safe_open_file(char *filename, char *open_mode){
    FILE *fp = fopen(filename, open_mode);
    if (fp == NULL || (open_mode[0] != 'w' && safety_byte(fp) != '1')){
        puts("Falha no processamento do arquivo.");
        if (fp != NULL) fclose(fp);
        return NULL;
    }
    return fp;
}

/* Dado fp no início do registro, diz se ele está removido. Não altera o fp */
int is_removed(FILE *fp){
    char removed;
    fread(&removed, sizeof(char), 1, fp);
    fseek(fp, -1L, SEEK_CUR);
    return removed != '-';
}

/* Dado fp no início do registro,marca-oc omo removido e preenche com lixo.
    Não preenche o campo de próximo offset nem de tamanho. Não altera o fp */
void set_removed(FILE *fp){
    long long start = ftell(fp);

    int size;
    fwrite("*", sizeof(char), 1, fp);
    fread(&size, sizeof(int), 1, fp);
    fseek(fp, sizeof(long long), SEEK_CUR);

    char trash[1000];
    memset(trash, TRASH, sizeof(trash));
    fwrite(trash, sizeof(char), size - sizeof(long long), fp);

    fseek(fp, start, SEEK_SET);
}

/* Impressão do registro p/ debug */
void record_print(Record *r){
    if (r == NULL) return;
    printf("%08d  |  %-14s  |  %-9.2lf  |  %-40s  |  %-30s\n", r->idServidor, r->telefoneServidor, r->salarioServidor, r->cargoServidor, r->nomeServidor);
}

/* dado o fp no começo do registro, ele pula para o próximo começo de registro, pulando registros removidos */
void skip_record(FILE *fp){
    char removed;
    int record_size;

    fread(&removed, sizeof(char), 1, fp);
    do {
        fread(&record_size, sizeof(int), 1, fp);
        fseek(fp, (long)record_size, SEEK_CUR);

        fread(&removed, sizeof(char), 1, fp);
    } while (!feof(fp) && removed != '-');

    if (!feof(fp)) fseek(fp, -1L, SEEK_CUR);   // volta p/ byte de rmeovido, inicio do registro
}

void set_byte_offset(Record *r, long long new_offset){
    if (r == NULL) return;
    r->byte_offset = new_offset;
}

/* Le e retorna registro do arquivo csv apontado por fp */
Record *read_record(FILE *fp){
    int id = INVALID;
    double salario = INVALID;
    char telefone[15], nome[MAX_SIZE], cargo[MAX_SIZE];
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
    return record_create(id, salario, telefone, nome, cargo, -1);
}

/* Preenche o buffer com lixo e depois escreve str por cima */
void replace_trash(char *str, char *buffer, int buffer_size){
    memset(buffer, TRASH, buffer_size*sizeof(char));
    sprintf(buffer, "%s", str);
}

/* Escreve o cabeçalho do arquivo fp com os dados do csv source */
void write_header(FILE *fp, FILE *source){
    long long temp = -1;

    /* le os metadados do csv */
    char tag1[40], tag2[40], tag3[40], tag4[40], tag5[40];

    fscanf(source, "%[^,]", tag1);
    fgetc(source);
    fscanf(source, "%[^,]", tag2);
    fgetc(source);
    fscanf(source, "%[^,]", tag3);
    fgetc(source);
    fscanf(source, "%[^,]", tag4);
    fgetc(source);
    fscanf(source, "%[^\r\n]", tag5);
    fscanf(source, "%*[\r\n]");

    // inicializo a pagina de disco com lixo, depois retorno o ponteiro ao inicio
    char trash[DISK_PG + 1];
    memset(trash, TRASH, sizeof(trash));
    fwrite(trash, sizeof(char), DISK_PG, fp);
    fseek(fp, (long)0, SEEK_SET);

    fwrite("0", sizeof(char), 1, fp);   // escreve o byte de segurança como 0
    fwrite(&temp, sizeof(long long), 1, fp);    // escreve topoLista como -1

    char string[40];

    /* Escreve os metadados lidos do csv */
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

/* Função auxiliar que retorna numero de bytes restantes p/ próxima pg. disco */
int bytes_until_new_page(FILE *fp){
    return DISK_PG - ftell(fp)%DISK_PG;
}

/* Função que dado fp no início de um registro, retorna o tamanho do registro sem alterar o fp */
int get_record_size(FILE *fp){
    int size;

    fseek(fp, 1L, SEEK_CUR);
    fread(&size, sizeof(int), 1, fp);
    fseek(fp, -5L, SEEK_CUR);

    return size;
}

/* Escreve um único registro considerando fp como início.
    is_writing_size determina se vai escrever o tamanho do
    novo registro ou se mantém igual (logicamente removido)
    Altera fp
*/
void write_record(FILE *fp, Record *r, int last_record, int is_writing_size){
    int size;
    if (!is_writing_size) size = get_record_size(fp);
    else size = record_size(r);

    int pages = ceiling(ftell(fp)/(double)DISK_PG);
    int bytes_written = ftell(fp)%DISK_PG;

    if (is_writing_size && (bytes_written + size)/(double)DISK_PG > 1){
        int trash_amount = bytes_until_new_page(fp);

        char trash[DISK_PG];
        memset(trash, TRASH, sizeof(trash));
        fwrite(trash, sizeof(char), trash_amount, fp);

        int prev_rec_size;
        fseek(fp, (long)(last_record + sizeof(char)), SEEK_SET); // volta para tam. do registro anterior
        fread(&prev_rec_size, sizeof(int), 1, fp);
        prev_rec_size += trash_amount;  // atualiza variavel do tamanho do registro anterior

        fseek(fp, -(long)sizeof(int), SEEK_CUR);            // volta p/ reescrever
        fwrite(&prev_rec_size, sizeof(int), 1, fp);         // escreve novo tamanho

        fseek(fp, (long)(pages*DISK_PG), SEEK_SET);             // volta para o começo da nova pagina
    }

    long long temp1 = -1;
    if (is_writing_size) size -= 5;  /* desconsidero os 5 primeiros bytes pra escrever no registro */

    set_byte_offset(r, ftell(fp));

    fwrite("-", sizeof(char), 1, fp);
    fwrite(&size, sizeof(int), 1, fp);
    fwrite(&temp1, sizeof(long long), 1, fp);       // encadeamento
    fwrite(&(r->idServidor), sizeof(int), 1, fp);
    fwrite(&(r->salarioServidor), sizeof(double), 1, fp);

    char telefone[15];
    replace_trash(r->telefoneServidor, telefone, 15);
    fwrite(telefone, sizeof(char), 14, fp);

    int str_size;
    if (r->nomeServidor[0] != '\0'){
        str_size = 2 + strlen(r->nomeServidor);
        fwrite(&str_size, sizeof(int), 1, fp);
        fwrite("n", sizeof(char), 1, fp);
        fwrite(r->nomeServidor, sizeof(char), str_size - 1, fp);
    }

    if (r->cargoServidor[0] != '\0'){
        str_size = 2 + strlen(r->cargoServidor);
        fwrite(&str_size, sizeof(int), 1, fp);
        fwrite("c", sizeof(char), 1, fp);
        fwrite(r->cargoServidor, sizeof(char), str_size - 1, fp);
    }
}

/* Função que itera pelos registros de source_csv e escreve em fp */
void write_file_body(FILE *fp, FILE *source_csv){
    int last_record = -1;
    int current_record = ftell(fp);

    Record *r = read_record(source_csv);
    while (!feof(fp) && r != NULL){
        write_record(fp, r, last_record, 1);
        record_free(r);
        last_record = current_record;

        r = read_record(source_csv);
        current_record = ftell(fp);
    }
}

/* Função principal da funcionalidade 1. File_name é nome do csv que deve ser lido */
void write_file(char *file_name){
    char output_name[30] = {"arquivoTrab1.bin"};
    if (CHOOSE_OUTPUT_NAME) scanf("%s", output_name);

    FILE *fp = fopen(output_name, "wb+");
    if (fp == NULL){
        printf("Falha no carregamento do arquivo.\n");
        exit(0);
    }

    FILE *source_csv = fopen(file_name, "r");
    if (source_csv == NULL){
        printf("Falha no carregamento do arquivo.\n");
        exit(0);
    }

    write_header(fp, source_csv);
    write_file_body(fp, source_csv);
    set_safety_byte(fp, '1');   // atualiza byte de segurança

    fclose(fp);
    fclose(source_csv);

    printf("%s", output_name);
}

/* Função que retorna o registro lido. fp deve estar no byte 'removido' do registro. Altera fp p/ próximo byte 'removido' */
Record *read_record_binary(FILE *fp){

    int size, id;
    char removed, telefone[15];
    double salario;
    long long index;


    fread(&removed, sizeof(char), 1, fp);   // ler byte removido
    fread(&size, sizeof(int), 1, fp);       // le tamanho

    long long start_pos = ftell(fp);

    if (feof(fp)) return NULL;

    if (removed != '*' && removed != '-'){         // pular pra proxima pagina de disco
        int end = fseek(fp, (long)size, SEEK_CUR);
        if (end == -1) return NULL;     // final de arquivo
        return read_record_binary(fp);
    }

    fread(&index, sizeof(long long), 1, fp); // le inicio da lista
    fread(&id, sizeof(int), 1, fp);          // le id do funcionario
    fread(&salario, sizeof(double), 1, fp);  // le salario
    fread(telefone, sizeof(char), 14, fp);   // le telefone
    telefone[14] = '\0';

    int string_size;
    char tag, nome[MAX_SIZE], cargo[MAX_SIZE];
    memset(nome, '\0', sizeof(nome));
    memset(cargo, '\0', sizeof(cargo));

    while (ftell(fp) <= start_pos + size){
        tag = '\0'; // reinicializo caso acabe o arquivo, p/ n ficar em loop infinito

        fread(&string_size, sizeof(int), 1, fp);
        fread(&tag, sizeof(char), 1, fp);
        if (tag == 'n') fread(nome, sizeof(char), string_size - 1, fp);
        else if (tag == 'c') fread(cargo, sizeof(char), string_size - 1, fp);
        else break; // chegou no fim do registro
    }

    fseek(fp, (long)(start_pos + size), SEEK_SET);

    return record_create(id, salario, telefone, nome, cargo, start_pos - 5);
}

/* Função que imprime o registro no formato da funcionalidade 2 */
void record_ugly_print(Record *r){
    if (r == NULL) return;

    printf("%d ", r->idServidor);

    if (r->salarioServidor == -1) printf("         ");
    else printf("%.2lf ", r->salarioServidor);

    printf("%-14s ", r->telefoneServidor);

    if (r->nomeServidor[0] != '\0') printf("%d %s ", (int)strlen(r->nomeServidor), r->nomeServidor);
    if (r->cargoServidor[0] != '\0') printf("%d %s", (int)strlen(r->cargoServidor), r->cargoServidor);
    putchar('\n');
}

/* Função principal da funcionalidade 2 */
void read_binary_file(char *file_name){

    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL || safety_byte(fp) != '1'){
        printf("Falha no processamento do arquivo.\n");
        exit(0);
    }

    fseek(fp, (long)DISK_PG, SEEK_SET); // pula o cabeçalho

    /* percorre o arquivo lendo os registros e imprimindo */
    int i;
    for (i = 0; !feof(fp); i++){
        if (!is_removed(fp)){
            Record *r = read_record_binary(fp);     // retorna NNULL quando acaba o arquivo
            if (r == NULL) break;

            record_ugly_print(r);
            record_free(r);
        }
        else skip_record(fp);
    }
    if (i == 0){
        printf("Registro inexistente.\n");
        exit(0);
    }

    fseek(fp, 0L, SEEK_END);

    printf("Número de páginas de disco acessadas: %d\n", ceiling(ftell(fp)/(double)DISK_PG));
    fclose(fp);
}

/*  tendo fp no inicio do registro, a função busca o valor do id, compara, retorna valor booleano sem alterar fp
    Coloca finished p/ 1 caso encontre a chave
*/
int compare_id(FILE *fp, void *key, char tag, int *finished){
    if (is_removed(fp)) return 0;
    long start_pos = ftell(fp);
    int record_size;

    fseek(fp, 1L, SEEK_CUR);    // Pula para tamanho do registro

    fread(&record_size, sizeof(int), 1, fp);

    int key_id = *((int *)key); // recupero a chave do ponteiro

    fseek(fp, (long)sizeof(long long), SEEK_CUR);   // pula o encadeamento

    int record_id;
    fread(&record_id, sizeof(int), 1, fp);

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
    if (finished != NULL) *finished = record_id == key_id;    // indica se já achou o campo único
    return record_id == key_id;
}

/* tendo fp no inicio do registro, a função busca o valor do salario, compara, retorna valor booleano sem alterar fp */
int compare_salary(FILE *fp, void *key, char tag, int *finished){
    if (is_removed(fp)) return 0;   // verifica se o registro foi removido
    long start_pos = ftell(fp);
    int record_size;

    fseek(fp, 1L, SEEK_CUR);    // Pula para tamanho do registro
    fread(&record_size, sizeof(int), 1, fp);

    double key_salary = *((double *)key); // recupero a chave do ponteiro

    fseek(fp, (long)(sizeof(long long) + sizeof(int)) , SEEK_CUR);   // pula o encadeamento e o id
    double record_salary;
    fread(&record_salary, sizeof(double), 1, fp);

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
    return record_salary == key_salary;
}

/* tendo fp no inicio do registro, a função busca o valor do telefone, compara, retorna valor booleano sem alterar fp */
int compare_phone(FILE *fp, void *key, char tag, int *finished){
    if (is_removed(fp)) return 0;   // verifica se o registro foi removido
    long start_pos = ftell(fp);
    int record_size;

    fseek(fp, 1L, SEEK_CUR);    // Pula para tamanho do registro
    fread(&record_size, sizeof(int), 1, fp);

    char *key_phone = (char *)key; // recupero a chave do ponteiro

    fseek(fp, (long)(sizeof(long long) + sizeof(int) + sizeof(double)), SEEK_CUR);   // pula o encadeamento, id e salario, e byte de removido
    char record_phone[15];
    fread(record_phone, sizeof(char), 14, fp);
    record_phone[14] = '\0';

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
    return !strcmp(record_phone, key_phone);
}

/* tendo fp no inicio do registro, a função busca o valor do nome, compara, retorna valor booleano sem alterar fp */
int compare_name(FILE *fp, void *key, char tag, int *finished){
    if (is_removed(fp)) return 0;   // verifica se o registro foi removido
    long start_pos = ftell(fp);
    int record_size;

    fseek(fp, 1L, SEEK_CUR);    // Pula para tamanho do registro
    fread(&record_size, sizeof(int), 1, fp);

    char *key_name = (char *)key; // recupero a chave do ponteiro

    fseek(fp, (long)(sizeof(long long) + sizeof(int) + sizeof(double) + 14*sizeof(char)), SEEK_CUR);   // pula o encadeamento, id, salario, telefone, e byte de removido

    int field_size = 1;
    char record_tag;
    /* Percorre os campos de tamanho variável */
    do{
        fseek(fp, (long)(field_size - 1), SEEK_CUR);
        fread(&field_size, sizeof(int), 1, fp);
        fread(&record_tag, sizeof(char), 1, fp);
    } while (!feof(fp) && ftell(fp) <= start_pos + record_size && record_tag != tag);

    if (record_tag != tag){   // nao tem nome
        fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
        return key_name[0] == '\0';
    }

    char record_name[300];
    fread(record_name, sizeof(char), field_size - 1, fp);

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial

    return !strcmp(record_name, key_name);
}

/* tendo fp no inicio do registro, a função busca o valor do cargo, compara, retorna valor booleano sem alterar fp */
int compare_job(FILE *fp, void *key, char tag, int *finished){
    if (is_removed(fp)) return 0;   // verifica se o registro foi removido
    long start_pos = ftell(fp);

    char *key_job = (char *)key; // recupero a chave do ponteiro
    fseek(fp, 1L, SEEK_CUR);    // pula byte de removido

    int record_size;
    fread(&record_size, sizeof(int), 1, fp);
    fseek(fp, (long)(sizeof(long long) + sizeof(int) + sizeof(double) + 14*sizeof(char)), SEEK_CUR);   // pula o encadeamento p/ ler tamanho, e byte de removido

    int field_size = 1;
    char record_tag;
    /* Percorre os campos de tamanho variável */
    do{
        fseek(fp, (long)(field_size - 1), SEEK_CUR);
        fread(&field_size, sizeof(int), 1, fp);
        fread(&record_tag, sizeof(char), 1, fp);
    } while (!feof(fp) && ftell(fp) <= start_pos + record_size && record_tag != tag);

    if (record_tag != tag){
        fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
        return key_job[0] == '\0';
    }

    char record_job[300];
    fread(record_job, sizeof(char), field_size - 1, fp);

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
    return !strcmp(record_job, key_job);
}

/* função que imprime no formato da funcionalidade 3 */
void record_print_tags(Record *r, char tags[][TAG_SIZE]){
    printf("%s: %d\n", 1 + tags[0], r->idServidor);

    if (r->salarioServidor != -1) printf("%s: %.2lf\n", 1 + tags[1], r->salarioServidor);
    else printf("%s: valor nao declarado\n", 1 + tags[1]);

    if (r->telefoneServidor[0] != '\0') printf("%s: %14s\n", 1 + tags[2], r->telefoneServidor);
    else printf("%s: valor nao declarado\n", 1 + tags[2]);

    if (r->nomeServidor[0] != '\0') printf("%s: %s\n", 1 + tags[3], r->nomeServidor);
    else printf("%s: valor nao declarado\n", 1 + tags[3]);

    if (r->cargoServidor[0] != '\0') printf("%s: %s\n\n", 1 + tags[4], r->cargoServidor);
    else printf("%s: valor nao declarado\n\n", 1 + tags[4]);
}

/* Essa função busca os registros pela chave key, acessando e comparando o campo determinado por compare_function */
void search_records(FILE *fp, int (*compare_function)(FILE *fp, void *key, char tag, int *finished), void *key, char tag, char tags[][TAG_SIZE]){
    int results = 0;

    int finished = 0;   /* variável que diz se já achou o campo único */
    for (int i = 0; !feof(fp) && !finished; i++){
        if (compare_function(fp, key, tag, &finished)){
            Record *r = read_record_binary(fp);
            record_print_tags(r, tags);
            record_free(r);
            results++;
        }
        else skip_record(fp);
    }

    if (!finished) fseek(fp, 0L, SEEK_END);

    if (results == 0) printf("Registro inexistente.\n");
    else printf("Número de páginas de disco acessadas: %d\n", ceiling(ftell(fp)/(double)DISK_PG));
}

/* Função principal da funcionalidade 3. File_name é o binário a ser lido */
void search_binary_file(char *file_name){
    char field_name[MAX_SIZE];
    scanf("%s ", field_name);


    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL || safety_byte(fp) == '0'){
        printf("Falha no processamento do arquivo.\n");
        exit(0);
    }

    char tags[5][TAG_SIZE];  // leio a tag e o descritor. tag[i][0] = caracter que define a i-ésima tag
    read_tags(fp, tags); /* le os metadados do cabeçalho */

    fseek(fp, (long)(DISK_PG), SEEK_SET); // pula cabeçalho

    if (!strcmp(field_name, "idServidor")){
        int field_value;
        scanf("%d", &field_value);
        search_records(fp, compare_id, &field_value, tags[0][0], tags);
    }
    if (!strcmp(field_name, "salarioServidor")){
        double field_value;
        scanf("%lf", &field_value);
        search_records(fp, compare_salary, &field_value, tags[1][0], tags);
    }
    if (!strcmp(field_name, "telefoneServidor")){
        char field_value[15];
        scanf("%s", field_value);
        search_records(fp, compare_phone, field_value, tags[2][0], tags);
    }
    if (!strcmp(field_name, "nomeServidor")){
        char field_value[MAX_SIZE];
        scanf("%[^\r\n]", field_value); // quase um dia de debug pra achar esse \r

        search_records(fp, compare_name, field_value, tags[3][0], tags);
    }
    if (!strcmp(field_name, "cargoServidor")){
        char field_value[MAX_SIZE];
        scanf("%[^\r\n]", field_value); // quase um dia de debug pra achar esse \r

        search_records(fp, compare_job, field_value, tags[4][0], tags);
    }


    fclose(fp);
}

/* Função que retorna o topo da lista sem alterar o fp */
long long get_list_start(FILE *fp){
    long start_pos = ftell(fp);
    long long top;

    fseek(fp, 1L, SEEK_SET);
    fread(&top, sizeof(long long), 1, fp);
    fseek(fp, start_pos, SEEK_SET);

    return top;
}

/* Atualiza topo lista sem alterar o fp */
void set_list_start(FILE *fp, long long new){
    long start_pos = ftell(fp);
    fseek(fp, 1L, SEEK_SET);
    fwrite(&new, sizeof(long long), 1, fp);
    fseek(fp, start_pos, SEEK_SET);
}

/* Função que, dado fp no início do registro, retorna o byte offset do próximo registro na lista sem alterar o fp */
long long get_next_offset(FILE *fp){
    long long offset;

    fseek(fp, 5L, SEEK_CUR);
    fread(&offset, sizeof(long long), 1, fp);
    fseek(fp, -13L, SEEK_CUR);

    return offset;
}

/* Função que, dado fp no início do registro, altera o byte offset do próximo registro na lista sem alterar o fp */
void set_next_offset(FILE *fp, long long new){
    if (ftell(fp) == 1) fwrite(&new, sizeof(long long), 1, fp);
    else {
        fseek(fp, 5L, SEEK_CUR);
        fwrite(&new, sizeof(long long), 1, fp);
        fseek(fp, -13L, SEEK_CUR);
    }
}

/* Com fp no inicio do registro, imprime-o sem alterar o fp */
void print_record_from_fp(FILE *fp){
    long start_pos = ftell(fp);
    Record *r = read_record_binary(fp);     // retorna NNULL quando acaba o arquivo
    record_print(r);
    record_free(r);
    fseek(fp, start_pos, SEEK_SET);
}

/* Imprime temanhos dos registros da lista sem alterar o fp */
void print_list(FILE *fp){
    long start_pos = ftell(fp);

    unsigned long long next_offset = get_list_start(fp);
    printf("TOPO LISTA: %ld\n", (long)next_offset);
    while (next_offset != -1){
        fseek(fp, next_offset, SEEK_SET);
        next_offset = get_next_offset(fp);
        printf("(%03d) : %07ld -> %07ld\n", get_record_size(fp), ftell(fp), (unsigned long)next_offset);
    }

    fseek(fp, start_pos, SEEK_SET);
}

/* Com o fp no começo do registro, remove-o alterando a lista sem alterar o fp */
void remove_record(FILE *fp){
    if (feof(fp) || is_removed(fp)) return;
    long long start_pos = ftell(fp);

    /* Remove o registro do arquivo e adiciona o fseek dele na lista ordenada */
    set_removed(fp);    // marca como logicamente removido, preenchendo com lixo

    long long removed_offset = ftell(fp);
    int removed_size = get_record_size(fp);

    long long next_offset = get_list_start(fp);
    if (next_offset == -1){                         // lista vazia
        set_list_start(fp, removed_offset);         // atualiza topo lista
        set_next_offset(fp, (long long)-1);         // coloca proximo registro como -1

        fseek(fp, start_pos, SEEK_SET);
        return;
    }

    long long previous_offset;
    int current_size;

    fseek(fp, 1L, SEEK_SET);

    while (next_offset != -1){
        previous_offset = ftell(fp);
        fseek(fp, next_offset, SEEK_SET);

        current_size = get_record_size(fp);
        if (current_size >= removed_size){
            fseek(fp, previous_offset, SEEK_SET);
            set_next_offset(fp, removed_offset);          // anterior aponta p/ renovudo

            fseek(fp, removed_offset, SEEK_SET);
            set_next_offset(fp, next_offset);  // removido aponta p/ o proximo

            fseek(fp, start_pos, SEEK_SET);
            return;
        }
        next_offset = get_next_offset(fp);
    }

    if (next_offset == -1){                     // deve ser inserido no final
        set_next_offset(fp, removed_offset);
        fseek(fp, removed_offset, SEEK_SET);
        set_next_offset(fp, (long long)-1);     // aponta p/ -1 já que é o ultimo
    }

    fseek(fp, start_pos, SEEK_SET);
}

/* Subrotina de busca de acordo com a funcao de comparacao. Chama a funcao passada por parametro quando acha os registros relevantes */
void search_records_routine(FILE *fp, int (*compare_function)(FILE *fp, void *key, char tag, int *finished), void (*what_to_do)(FILE *fp), void *key, char tag){
    fseek(fp, (long)DISK_PG, SEEK_SET);

    int finished = 0;
    long long start_pos;
    for (int i = 0; !feof(fp) && !finished; i++){
        if (compare_function(fp, key, tag, &finished)){
            start_pos = ftell(fp);
            what_to_do(fp);
            fseek(fp, start_pos, SEEK_SET); // volta p/ começo do registro analisado, depois o pula
        }
        skip_record(fp);
    }
}

/* Função principal da funcionalidade 4 */
void remove_records(char *filename){
    int n;
    scanf("%d", &n);

    FILE *fp = fopen(filename, "rb+");
    if (fp == NULL || safety_byte(fp) == '0'){
        printf("Falha no processamento do arquivo.\n");
        exit(0);
    }

    if (PRINT_LIST){
        print_list(fp);
        return;
    }

    set_safety_byte(fp, '0');   // atualiza byte de integridade

    char tags[5][TAG_SIZE];  // leio a tag e o descritor. tag[i][0] = caracter que define a i-ésima tag
    read_tags(fp, tags); /* le os metadados do cabeçalho */

    int id;
    double salario;
    char field_name[20], value[MAX_SIZE];
    for (int i = 0; i < n; i++){
        scanf("%s%*c", field_name);

        if (!strcmp(field_name, "idServidor")){
            scanf("%d", &id);
            search_records_routine(fp, compare_id, remove_record, &id, tags[0][0]);
        }
        if (!strcmp(field_name, "salarioServidor")){
            salario = le_salario(); // funcao que contabiliza valor nulo (está no outro .h)
            search_records_routine(fp, compare_salary, remove_record, &salario, tags[1][0]);
        }
        if (!strcmp(field_name, "telefoneServidor")){
            scan_quote_string(value);
            search_records_routine(fp, compare_phone, remove_record, value, tags[2][0]);
        }
        if (!strcmp(field_name, "nomeServidor")){
            scan_quote_string(value);
            search_records_routine(fp, compare_name, remove_record, value, tags[3][0]);
        }
        if (!strcmp(field_name, "cargoServidor")){
            scan_quote_string(value);
            search_records_routine(fp, compare_job, remove_record, value, tags[4][0]);
        }
    }

    set_safety_byte(fp, '1');
    if (PRINT_FILE_ON_SCREEN) binarioNaTela1(fp);
    fclose(fp);
}

/* Retorna o registro lido de acordo com a especificação, tratando valores nulos corretaemnte */
Record *read_record_stdin(){
    int id;
    double salario;
    char tel[15], nome[MAX_SIZE], cargo[MAX_SIZE];

    scanf("%d", &id);
    salario = le_salario();
    scan_quote_string(tel);
    scan_quote_string(nome);
    scan_quote_string(cargo);

    return record_create(id, salario, tel, nome, cargo, -1);
}

/* Função de debug que imprime o registro em forma de bytes. Não altera o fp */
void print_bytes(FILE *fp){
    long long start = ftell(fp);

    char buffer[400];
    int size = get_record_size(fp);
    fread(buffer, sizeof(char), size + sizeof(char) + sizeof(int), fp);

    for (int i = 0; i < size + 5; i++){
        putchar(buffer[i]);
    }
    putchar('\n');

    fseek(fp, start, SEEK_SET);
}

/* Coloca fp no final do ultimo registro e retorna offset do início do último registro */
long long goto_last_record(FILE *fp){
    fseek(fp, DISK_PG, SEEK_SET);

    long long last_offset;
    while (!feof(fp)){
        last_offset = ftell(fp);
        skip_record(fp);
    }
    return last_offset;
}

/* Insere o registro no arquivo, atualizando a lista encadeada se necessário. Não altera o fp */
void insert_record(FILE *fp, Record *r, long long *last_record_offset){
    long start_pos = ftell(fp);

    fseek(fp, 1L, SEEK_SET);

    long long next_offset = get_list_start(fp);
    long long previous_offset = 1;  // começa offset do topo da lista, caso remova o primeiro

    if (next_offset == -1){                         // lista vazia, insiro no final do arquivo
        fseek(fp, 0L, SEEK_END);
        long long temp = ftell(fp);
        write_record(fp, r, *last_record_offset, 1);
        *last_record_offset = temp;
        fseek(fp, start_pos, SEEK_SET);
        return;
    }

    int insert_size = record_size(r) - 5;
    int current_size;

    while (next_offset != -1){
        previous_offset = ftell(fp);
        fseek(fp, next_offset, SEEK_SET);

        current_size = get_record_size(fp);
        next_offset = get_next_offset(fp);

        if (current_size >= insert_size){   // insere e remove espaço vago da lista
            write_record(fp, r, ftell(fp), 0); // escreve o registro
            fseek(fp, previous_offset, SEEK_SET);
            set_next_offset(fp, next_offset);   // faz atual->anterior apontar p/ atual->prox

            fseek(fp, start_pos, SEEK_SET);
            return;
        }
    }

    if (next_offset == -1){         // escreve no final, já que nao tem espaço disponível
        fseek(fp, 0L, SEEK_END);
        long long temp = ftell(fp);
        write_record(fp, r, *last_record_offset, 1);
        *last_record_offset = temp;
    }
    fseek(fp, start_pos, SEEK_SET);
}

/* Funcao principal da funcionalidade 5 */
void insert_records(char *filename){
    int n;
    scanf("%d", &n);

    FILE *fp = fopen(filename, "rb+");
    if (fp == NULL || safety_byte(fp) == '0'){
        printf("Falha no processamento do arquivo.\n");
        exit(0);
    }

    if (PRINT_LIST){
        print_list(fp);
        return;
    }


    set_safety_byte(fp, '0');   // atualiza byte de integridade

    char tags[5][TAG_SIZE];  // leio a tag e o descritor. tag[i][0] = caracter que define a i-ésima tag
    read_tags(fp, tags);    /* le os metadados do cabeçalho */

    long long last_record_offset = goto_last_record(fp);
    for (int i = 0; i < n; i++){
        Record *r = read_record_stdin();
        insert_record(fp, r, &last_record_offset);
        record_free(r);
    }

    set_safety_byte(fp, '1');
    if (PRINT_FILE_ON_SCREEN) binarioNaTela1(fp);
    fclose(fp);
}

/* Com fp no começo do registro, altera ID sem alterar o fp */
void set_id(FILE *fp, void *new, long long *last_record_offset){
    long long start_pos = ftell(fp);
    if (PRINT_RECORDS) print_record_from_fp(fp);

    fseek(fp, sizeof(char) + sizeof(int) + sizeof(long long), SEEK_CUR);
    fwrite(new, sizeof(int), 1, fp);
    fseek(fp, start_pos, SEEK_SET);

    if (PRINT_RECORDS) print_record_from_fp(fp);
    if (PRINT_RECORDS) putchar('\n');
}

/* Com fp no começo do registro, altera salario sem alterar o fp */
void set_salary(FILE *fp, void *new, long long *last_record_offset){
    long long start_pos = ftell(fp);
    if (PRINT_RECORDS) print_record_from_fp(fp);

    fseek(fp, sizeof(char) + sizeof(int) + sizeof(long long) + sizeof(int), SEEK_CUR);
    fwrite(new, sizeof(double), 1, fp);
    fseek(fp, start_pos, SEEK_SET);

    if (PRINT_RECORDS) print_record_from_fp(fp);
    if (PRINT_RECORDS) putchar('\n');
}

/* Com fp no começo do registro, altera telefone sem alterar o fp */
void set_phone(FILE *fp, void *new, long long *last_record_offset){
    char tel[15];
    replace_trash((char *)new, tel, 15);

    long long start_pos = ftell(fp);
    if (PRINT_RECORDS) print_record_from_fp(fp);

    fseek(fp, sizeof(char) + sizeof(int) + sizeof(long long) + sizeof(int) + sizeof(double), SEEK_CUR);
    fwrite(tel, sizeof(char), 14, fp);
    fseek(fp, start_pos, SEEK_SET);

    if (PRINT_RECORDS) print_record_from_fp(fp);
    if (PRINT_RECORDS) putchar('\n');
}

/* Com fp no começo do registro, altera nome sem alterar o fp. Pode reinserir o registro se necessário */
void set_name(FILE *fp, void *new, long long *last_record_offset){
    char trash[300];
    memset(trash, TRASH, sizeof(trash));

    int size = get_record_size(fp);
    char *new_name = (char *)new;

    long long start_pos = ftell(fp);
    Record *current = read_record_binary(fp);

    if (PRINT_RECORDS) record_print(current);
    strcpy(current->nomeServidor, new_name);
    if (PRINT_RECORDS){
        record_print(current);
        putchar('\n');
    }

    fseek(fp, start_pos, SEEK_SET);

    if (record_size(current) - 5 <= size){
        if (PRINT_NOT_MOVED) print_bytes(fp);
        set_removed(fp);
        if (PRINT_NOT_MOVED) print_bytes(fp);
        write_record(fp, current, ftell(fp), 0);
        fseek(fp, start_pos, SEEK_SET);
        if (PRINT_NOT_MOVED){
            print_bytes(fp);
            putchar('\n');
        }
    } else {
        remove_record(fp);
        insert_record(fp, current, last_record_offset);
    }

    record_free(current);
}

/* Com fp no começo do registro, altera nome sem alterar o fp. Pode reinserir o registro se necessário */
void set_job(FILE *fp, void *new, long long *last_record_offset){
    char trash[300];
    memset(trash, TRASH, sizeof(trash));

    int size = get_record_size(fp);
    char *new_job = (char *)new;

    long long start_pos = ftell(fp);
    Record *current = read_record_binary(fp);

    if (PRINT_RECORDS) record_print(current);
    strcpy(current->cargoServidor, new_job);
    if (PRINT_RECORDS){
        record_print(current);
        putchar('\n');
    }

    fseek(fp, start_pos, SEEK_SET);
    if (record_size(current) - 5 <= size){
        if (PRINT_NOT_MOVED) print_bytes(fp);
        set_removed(fp);
        if (PRINT_NOT_MOVED) print_bytes(fp);
        write_record(fp, current, ftell(fp), 0);
        fseek(fp, start_pos, SEEK_SET);
        if (PRINT_NOT_MOVED){
            print_bytes(fp);
            putchar('\n');
        }
    } else {
        remove_record(fp);
        insert_record(fp, current, last_record_offset);
    }

    record_free(current);
}

/* Busca os campos de acordo com a funcao de comparacao, insere os offsets dos registros numa fila e depois atualiza-os de acordo com a funcao de substituição */
void search_and_update(FILE *fp, int (*compare_function)(FILE *fp, void *key, char tag, int *finished), void *key, void (*update_field)(FILE *fp, void *new_val, long long *last_record_offset), void *new_val, char tag, long long *last_record_offset){
    fseek(fp, (long)DISK_PG, SEEK_SET);

    long long fila_gambs[1000];
    int n_found = 0;

    int finished = 0;
    long long start_pos;
    for (int i = 0; !feof(fp) && !finished; i++){
        if (compare_function(fp, key, tag, &finished)) fila_gambs[n_found++] = ftell(fp);
        skip_record(fp);
    }

    if (PRINT_RECORDS || BRIEF_REPORT) printf("========= %d FOUND =========\n", n_found);
    for (int i = 0; i < n_found; i++){
        fseek(fp, fila_gambs[i], SEEK_SET);
        update_field(fp, new_val, last_record_offset);
    }
    if (BRIEF_REPORT) putchar('\n');
}

/* Faz a leitura do segundo campo e escolhe a funcao de comparação e substituição, mandando p/ atualização */
void choose_second_field(FILE *fp, int (*compare_function)(FILE *fp, void *key, char tag, int *finished), void *key, char tag, long long *last_record_offset){
    int id;
    double salario;
    char field_name[20], value[MAX_SIZE];
    scanf("%s%*c", field_name);

    if (PRINT_RECORDS || BRIEF_REPORT) printf("mudando %s p/ ", field_name);

    if (!strcmp(field_name, "idServidor")){
        scanf("%d", &id);
        if (PRINT_RECORDS || BRIEF_REPORT) printf("%d\n", id);
        search_and_update(fp, compare_function, key, set_id, &id, tag, last_record_offset);
    }
    else if (!strcmp(field_name, "salarioServidor")){
        salario = le_salario(); // funcao que contabiliza valor nulo (está no outro .h)
        if (PRINT_RECORDS || BRIEF_REPORT) printf("%.2lf\n", salario);
        search_and_update(fp, compare_function, key, set_salary, &salario, tag, last_record_offset);
    }
    else if (!strcmp(field_name, "telefoneServidor")){
        scan_quote_string(value);
        if (PRINT_RECORDS || BRIEF_REPORT) printf("%s\n", value);
        search_and_update(fp, compare_function, key, set_phone, value, tag, last_record_offset);
    }
    else if (!strcmp(field_name, "nomeServidor")){
        scan_quote_string(value);
        if (PRINT_RECORDS || BRIEF_REPORT) printf("%s\n", value);
        search_and_update(fp, compare_function, key, set_name, value, tag, last_record_offset);
    }
    else if (!strcmp(field_name, "cargoServidor")){
        scan_quote_string(value);
        if (PRINT_RECORDS || BRIEF_REPORT) printf("%s\n", value);
        search_and_update(fp, compare_function, key, set_job, value, tag, last_record_offset);
    }
}

/* Funcao principal da funcionalidade 6 */
void update_records(char *filename){
    int n;
    scanf("%d", &n);

    FILE *fp = fopen(filename, "rb+");
    if (fp == NULL || safety_byte(fp) == '0'){
        printf("Falha no processamento do arquivo.\n");
        exit(0);
    }

    if (PRINT_LIST){
        print_list(fp);
        return;
    }

    set_safety_byte(fp, '0');   // atualiza byte de integridade

    char tags[5][TAG_SIZE];  // leio a tag e o descritor. tag[i][0] = caracter que define a i-ésima tag
    read_tags(fp, tags); /* le os metadados do cabeçalho */

    long long last_record_offset = goto_last_record(fp);

    int id;
    double salario;
    char field_name[20], value[MAX_SIZE];
    for (int i = 0; i < n; i++){
        scanf("%s%*c", field_name);
        if (PRINT_RECORDS || BRIEF_REPORT) printf("Pesquisando %s ", field_name);

        if (!strcmp(field_name, "idServidor")){
            scanf("%d", &id);
            if (PRINT_RECORDS || BRIEF_REPORT) printf("= %d\n", id);
            choose_second_field(fp, compare_id, &id, tags[0][0], &last_record_offset);
        }
        else if (!strcmp(field_name, "salarioServidor")){
            salario = le_salario(); // funcao que contabiliza valor nulo (está no outro .h)
            if (PRINT_RECORDS || BRIEF_REPORT) printf("= %.2lf\n", salario);
            choose_second_field(fp, compare_salary, &salario, tags[1][0], &last_record_offset);
        }
        else if (!strcmp(field_name, "telefoneServidor")){
            scan_quote_string(value);
            if (PRINT_RECORDS || BRIEF_REPORT) printf("= %s\n", value);
            choose_second_field(fp, compare_phone, value, tags[2][0], &last_record_offset);
        }
        else if (!strcmp(field_name, "nomeServidor")){
            scan_quote_string(value);
            if (PRINT_RECORDS || BRIEF_REPORT) printf("= %s\n", value);
            choose_second_field(fp, compare_name, value, tags[3][0], &last_record_offset);
        }
        else if (!strcmp(field_name, "cargoServidor")){
            scan_quote_string(value);
            if (PRINT_RECORDS || BRIEF_REPORT) printf("= %s\n", value);
            choose_second_field(fp, compare_job, value, tags[4][0], &last_record_offset);
        }
    }

    set_safety_byte(fp, '1');
    if (PRINT_FILE_ON_SCREEN) binarioNaTela1(fp);
    fclose(fp);
}

void write_new_header(FILE *fp, char tags[][TAG_SIZE]){
    fseek(fp, 0L, SEEK_SET);

    long long temp = -1;
    char trash[DISK_PG];
    memset(trash, TRASH, sizeof(trash));

    fwrite("0", sizeof(char), 1, fp);
    fwrite(&temp, sizeof(long long), 1, fp);

    for (int i = 0; i < 5; i++) fwrite(tags[i], sizeof(char), TAG_SIZE, fp);
    fwrite(trash, sizeof(char), bytes_until_new_page(fp), fp);
}

/* Retorna 1 se a > b, 0 se a = b, -1 se a < b */
int has_greater_id(Record *a, Record *b){
    if (a == NULL || b == NULL) return -1;

    if (a->idServidor > b->idServidor) return 1;
    if (a->idServidor < b->idServidor) return -1;
    return 0;
}

/* Insere os registros nao removidos na lista. Altera fp p/ final do arquivo */
void fill_list_records(FILE *fp, List *l){
    fseek(fp, DISK_PG, SEEK_SET);

    Record *r;
    for (int i = 0; !feof(fp); i++){
        if (is_removed(fp)) skip_record(fp);
        r = read_record_binary(fp);
        list_insert(l, r);
    }
}

/* Funcao principal da funcionalidade 7 */
void sort_file(char *filename){
    char filename2[30];
    scanf("%s", filename2);

    FILE *original_file = safe_open_file(filename, "rb");
    if (original_file == NULL) exit(0);

    List *l = list_create((FuncaoComparacao) has_greater_id, (FuncaoPadraoVoid) record_free, (FuncaoPadraoVoid) record_print);

    char tags[5][TAG_SIZE];
    read_tags(original_file, tags);

    fseek(original_file, DISK_PG, SEEK_SET);

    fill_list_records(original_file, l);
    fclose(original_file);

    FILE *output_file = safe_open_file(filename2, "wb+");
    if (output_file == NULL){
        fclose(output_file);
        exit(0);
    }

    write_new_header(output_file, tags);
    list_write_records(l, output_file, (FuncaoEscritaDado) write_record);

    if (PRINT_ORDERED_LIST) list_print(l);
    list_free(l);

    set_safety_byte(output_file, '1');

    if (PRINT_FILE_ON_SCREEN) binarioNaTela1(output_file);

    fclose(output_file);
}

/* Copia o cabeçalho de original p/ output, escrevendo encadeaento como -1. Altera fps p/ proxima pg. disco */
void copy_file_header(FILE *original, FILE *output){
    char header[DISK_PG];
    fread(header, sizeof(char), DISK_PG, original);
    long long temp = -1;

    fwrite("0", sizeof(char), 1, output);
    fwrite(&temp, sizeof(long long), 1, output);
    fwrite(header + sizeof(char) + sizeof(long long), sizeof(char), DISK_PG - sizeof(char) - sizeof(long long), output);

    fseek(original, DISK_PG, SEEK_SET);
    fseek(output, DISK_PG, SEEK_SET);
}

void merge_files_routine(FILE *f1, FILE *f2, FILE *f3){
    copy_file_header(f1, f3);

    long long previous = -1, current = ftell(f3);

    fseek(f1, DISK_PG, SEEK_SET);
    fseek(f2, DISK_PG, SEEK_SET);

    Record *a = read_record_binary(f1);
    Record *b = read_record_binary(f2);

    while (!feof(f1) && !feof(f2)){
        current = ftell(f3);
        if (a->idServidor > b->idServidor) {
            write_record(f3, b, previous, 1);
            record_free(b);
            b = read_record_binary(f2);
        } else if (a->idServidor < b->idServidor){
            write_record(f3, a, previous, 1);
            record_free(a);
            a = read_record_binary(f1);
        } else {
            write_record(f3, a, previous, 1);
            record_free(a);
            record_free(b);
            a = read_record_binary(f1);
            b = read_record_binary(f2);
        }
        previous = current;
    }
    while (!feof(f1)){
        current = ftell(f3);
        write_record(f3, a, previous, 1);
        record_free(a);
        a = read_record_binary(f1);
        previous = current;
    }
    while (!feof(f2)){
        current = ftell(f3);
        write_record(f3, b, previous, 1);
        record_free(b);
        b = read_record_binary(f2);
        previous = current;
    }
}

void intersect_files_routine(FILE *f1, FILE *f2, FILE *f3){
    copy_file_header(f1, f3);

    long long previous = -1, current = ftell(f3);

    fseek(f1, DISK_PG, SEEK_SET);
    fseek(f2, DISK_PG, SEEK_SET);

    Record *a = read_record_binary(f1);
    Record *b = read_record_binary(f2);

    while (!feof(f1) && !feof(f2)){
        current = ftell(f3);
        if (a->idServidor < b->idServidor){
            record_free(a);
            a = read_record_binary(f1);
        } else if (a->idServidor > b->idServidor){
            record_free(b);
            b = read_record_binary(f2);
        } else {
            write_record(f3, a, previous, 1);
            record_free(a);
            record_free(b);
            a = read_record_binary(f1);
            b = read_record_binary(f2);
        }
        previous = current;
    }
    record_free(a);
    record_free(b);
}

/* Funcao principal da funcionalidade 8 */
void merge_files(char *filename){
    char filename2[30];
    char filename3[30];
    scanf("%s %s", filename2, filename3);

    FILE *file1 = safe_open_file(filename, "rb");
    if (file1 == NULL){
        exit(0);
    }

    FILE *file2 = safe_open_file(filename2, "rb");
    if (file2 == NULL){
        fclose(file1);
        exit(0);
    }

    FILE *file3 = safe_open_file(filename3, "wb+");
    if (file3 == NULL){
        fclose(file1); fclose (file2);
        exit(0);
    }

    merge_files_routine(file1, file2, file3);

    fclose(file1);
    fclose(file2);

    set_safety_byte(file3, '1');
    if (PRINT_FILE_ON_SCREEN) binarioNaTela1(file3);
    fclose(file3);
}

/* Funcao printipal da funcionalidade 9 */
void intersect_files(char *filename){
    char filename2[30];
    char filename3[30];
    scanf("%s %s", filename2, filename3);

    FILE *file1 = safe_open_file(filename, "rb");
    if (file1 == NULL){
        exit(0);
    }

    FILE *file2 = safe_open_file(filename2, "rb");
    if (file2 == NULL){
        fclose(file1);
        exit(0);
    }

    FILE *file3 = safe_open_file(filename3, "wb+");
    if (file3 == NULL){
        fclose(file1); fclose (file2);
        exit(0);
    }

    intersect_files_routine(file1, file2, file3);

    fclose(file1);
    fclose(file2);

    set_safety_byte(file3, '1');
    if (PRINT_FILE_ON_SCREEN) binarioNaTela1(file3);
    fclose(file3);
}

void write_index_entry(FILE *fp, Record *r){
    char buffer[120];
    replace_trash(r->nomeServidor, buffer, 120);
    fwrite(buffer, sizeof(char), 120, fp);
    fwrite(&(r->byte_offset), sizeof(long long), 1, fp);
}

int has_name(Record *r){
    if (r == NULL) return 0;
    return r->nomeServidor[0] != '\0';
}

void set_number_records(FILE *fp, int n_records){
    fseek(fp, 1L, SEEK_SET);
    fwrite(&n_records, sizeof(int), 1, fp);
}

void write_index_body(FILE *fp, List *l){
    int n_records = list_write_index(l, fp, (FuncaoEscritaIndice) write_index_entry, (FuncaoItemValido) has_name);
    set_number_records(fp, n_records);
}

void write_index_header(FILE *fp, List *l){
    char trash[DISK_PG];
    memset(trash, TRASH, sizeof(trash));

    fseek(fp, 0L, SEEK_SET);

    fwrite("0", sizeof(char), 1, fp);
    fwrite(trash, sizeof(char), bytes_until_new_page(fp), fp);
}

/* Funcao principal da funcionalidade 10 */
void create_index_file(char *filename){
    char filename2[30];
    scanf("%s", filename2);

    FILE *source = safe_open_file(filename, "rb");
    if (source == NULL) return;

    FILE *index_file = safe_open_file(filename2, "wb+");
    if (index_file == NULL){
        fclose(source);
        exit(0);
    }

    List *l = list_create((FuncaoComparacao) strcmp, (FuncaoPadraoVoid) record_free, (FuncaoPadraoVoid) record_print);
    fill_list_records(source, l);
    fclose(source);

    write_index_header(index_file, l);
    write_index_body(index_file, l);

    if (PRINT_ORDERED_LIST) list_print(l);
    list_free(l);

    set_safety_byte(index_file, '1');
    if (PRINT_FILE_ON_SCREEN) binarioNaTela1(index_file);
    fclose(index_file);
}

/* Retorna numero de registros no arquivo de indice */
int get_number_records(FILE *index){
    long long start = ftell(index);
    int n_records;

    fseek(index, 1L, SEEK_SET);
    fread(&n_records, sizeof(int), 1, index);

    fseek(index, start, SEEK_SET);
    return n_records;
}

/* Retorna o índice da busca, ou -1 se nao foi encontrado */
int binary_search_index(Index *idx, int l, int r, char *key){
    if (l > r) return -1;

    int mid = (l+r)/2;
    char *search_name = idx->list[mid]->name;
    int compare = strcmp(key, search_name);

    if (compare < 0) return binary_search_index(idx, l, mid - 1, key);
    else if (compare > 0) return binary_search_index(idx, mid + 1, r, key);
    return mid;
}

/* Retorna numero de registros encontrados e insere seus offsets no vetor offsets */
int search_index_routine(Index *idx, char *key, long long *offsets){
    int find_index = binary_search_index(idx, 0, idx->n_entries - 1, key);
    if (find_index == -1) return 0;

    /* Acha o primeiro indice que tem o nome igual ao da busca */
    int first_match = find_index;
    char *current_name = idx->list[find_index]->name;
    while (first_match >= 0 && !strcmp(key, current_name)){
        current_name = idx->list[--first_match]->name;
        if (PRINTING_MATCH_NAMES) printf("%05d : %s\n", first_match, current_name);
    }
    if (PRINTING_MATCH_NAMES) putchar('\n');


    /* Percorre os nomes iguais, adicionando na lista de offsets */
    int n_matches = 0;
    int current_index = first_match + 1;
    current_name = idx->list[current_index]->name;

    while (current_index < idx->n_entries && !strcmp(key, current_name)){
        offsets[n_matches++] = idx->list[current_index]->offset;
        current_name = idx->list[++current_index]->name;

        if (PRINTING_MATCH_NAMES) printf("%05d : %s\n", current_index, current_name);
    }
    if (PRINTING_MATCH_NAMES) putchar('\n');

    return n_matches;
}

void search_index(char *filename){
    char filename2[30];
    char name[MAX_SIZE];

    scanf("%s %*s %[^\n]", filename2, name);

    FILE *source = safe_open_file(filename, "rb");
    if (source == NULL) exit(0);

    FILE *index_file = safe_open_file(filename2, "rb");
    if (index_file == NULL){
        fclose(source);
        exit(0);
    }

    char tags[5][TAG_SIZE];
    read_tags(source, tags);

    Index *idx = index_load_from_file(index_file);
    int pgs_indice = ceiling(ftell(index_file)/(double)DISK_PG), pgs_dados = 0;

    long long result_offsets[MAX_SIZE];
    int n_results = search_index_routine(idx, name, result_offsets);

    if (n_results == 0) printf("Registro inexistente.\n");
    else {
        int prev_disk_pg = -1, current_disk_pg;

        for (int i = 0; i < n_results; i++){
            fseek(source, result_offsets[i], SEEK_SET);
            current_disk_pg = ceiling(ftell(source)/(double)DISK_PG);

            if (current_disk_pg != prev_disk_pg) pgs_dados++;   // incrementa pg. disco acessadas
            prev_disk_pg = current_disk_pg;

            Record *r = read_record_binary(source);
            record_print_tags(r, tags);
        }
        printf("Número de páginas de disco para carregar o arquivo de índice: %d\nNúmero de páginas de disco para acessar o arquivo de dados: %d", pgs_indice, n_results);
    }

    fclose(source);
    fclose(index_file);
}
