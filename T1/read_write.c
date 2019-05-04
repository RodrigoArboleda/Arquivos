#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_write.h"
#include "escrevernatela.h"

/* Victor Giovannoni 10786159 */
/* OBS: Em alguns comentários, quando digo que a função não altera o fp,
   quero dizer que não altera a posição do fp que foi passada como parâmetro */

#define DISK_PG 32000
#define TRASH '@'
#define INVALID -1337
#define DEBUG 0
#define MAX_SIZE 100

typedef struct record_ Record;

struct record_{
    char nomeServidor[MAX_SIZE];
    char cargoServidor[MAX_SIZE];
    char telefoneServidor[15];
    double salarioServidor;
    int idServidor;
};

Record *record_create(int id, double salario, char *telefone, char *nome, char *cargo){
    Record *r = (Record *)malloc(sizeof(Record));

    r->idServidor = id;
    r->salarioServidor = salario;

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
    return record_create(id, salario, telefone, nome, cargo);
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

/* Escreve um único registro considerando fp como início */
void write_record(FILE *fp, Record *r, int last_record){
    int size = record_size(r);

    int pages = ceiling(ftell(fp)/(double)DISK_PG);
    int bytes_written = ftell(fp)%DISK_PG;

    if ((bytes_written + size)/(double)DISK_PG > 1){
        printf("ESCREVENDO NOVA PAGINA DE DISCO\n");
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
    size -= 5;  /* desconsidero os 5 primeiros bytes pra escrever no registro */

    char removed[2] = "-";

    fwrite(removed, sizeof(char), 1, fp);
    fwrite(&size, sizeof(int), 1, fp);              // tamanho do registro
    fwrite(&temp1, sizeof(long long), 1, fp);       // encadeamento
    fwrite(&(r->idServidor), sizeof(int), 1, fp);
    fwrite(&(r->salarioServidor), sizeof(double), 1, fp);

    char telefone[14];
    replace_trash(r->telefoneServidor, telefone, 14);
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
        write_record(fp, r, last_record);
        record_free(r);
        last_record = current_record;

        r = read_record(source_csv);
        current_record = ftell(fp);
    }
}

/* Função principal da funcionalidade 1. File_name é nome do csv que deve ser lido */
void write_file(char *file_name){
    char output_name[] = {"arquivoTrab1.bin"};

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
    int start_pos = ftell(fp);

    if (feof(fp)) return NULL;

    if (removed != '*' && removed != '-'){         // pular pra proxima pagina de disco
        int end = fseek(fp, (long)size, SEEK_CUR);
        if (end == -1) return NULL;
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

    return record_create(id, salario, telefone, nome, cargo);
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

    fseek(fp, (long)(sizeof(long long) + sizeof(int)) , SEEK_CUR);   // pula o encadeamento e o id, e byte de removido
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
    } while (ftell(fp) <= start_pos + record_size && record_tag != tag);

    if (ftell(fp) > start_pos + record_size){   // nao tem nome
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
    } while (ftell(fp) <= start_pos + record_size && record_tag != tag);

    if (ftell(fp) > start_pos + record_size){
        fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
        return 0;
    }

    char record_job[300];
    fread(record_job, sizeof(char), field_size - 1, fp);

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
    return !strcmp(record_job, key_job);
}

/* função que imprime no formato da funcionalidade 3 */
void record_print_tags(Record *r, char tags[][41]){
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
void search_records(FILE *fp, int (*compare_function)(FILE *fp, void *key, char tag, int *finished), void *key, char tag, char tags[][41]){
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

    char tags[5][41];  // leio a tag e o descritor. tag[i][0] = caracter que define a i-ésima tag

    /* le os metadados do cabeçalho */
    fseek(fp, (long)9, SEEK_SET);
    fread(tags[0], sizeof(char), 41, fp);
    fread(tags[1], sizeof(char), 41, fp);
    fread(tags[2], sizeof(char), 41, fp);
    fread(tags[3], sizeof(char), 41, fp);
    fread(tags[4], sizeof(char), 41, fp);

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

/* Função que dado fp no início de um registro, retorna o tamanho do registro sem alterar o fp */
int get_record_size(FILE *fp){
    int size;

    fseek(fp, 1L, SEEK_CUR);
    fread(&size, sizeof(int), 1, fp);
    fseek(fp, -5L, SEEK_CUR);

    return size;
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
    /* Remove o registro do arquivo e adiciona o fseek dele na lista ordenada */
    set_removed(fp);    // marca como logicamente removido, preenchendo com lixo

    long long removed_offset = ftell(fp);
    int removed_size = get_record_size(fp);

    long long next_offset = get_list_start(fp);
    if (next_offset == -1){                         // lista vazia
        set_list_start(fp, removed_offset);         // atualiza topo lista
        set_next_offset(fp, (long long)-1);         // coloca proximo registro como -1
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
            return;
        }
        next_offset = get_next_offset(fp);
    }

    if (next_offset == -1){                     // deve ser inserido no final
        set_next_offset(fp, removed_offset);
        fseek(fp, removed_offset, SEEK_SET);
        set_next_offset(fp, (long long)-1);     // aponta p/ -1 já que é o ultimo
    }
}

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

void remove_records(char *filename){
    int n;
    scanf("%d", &n);

    FILE *fp = fopen(filename, "rb+");
    if (fp == NULL || safety_byte(fp) == '0'){
        printf("Falha no processamento do arquivo.\n");
        exit(0);
    }

    if (DEBUG){
        print_list(fp);
        return;
    }

    set_safety_byte(fp, '0');   // atualiza byte de integridade

    char tags[5][41];  // leio a tag e o descritor. tag[i][0] = caracter que define a i-ésima tag

    /* le os metadados do cabeçalho */
    fseek(fp, (long)9, SEEK_SET);
    fread(tags[0], sizeof(char), 41, fp);
    fread(tags[1], sizeof(char), 41, fp);
    fread(tags[2], sizeof(char), 41, fp);
    fread(tags[3], sizeof(char), 41, fp);
    fread(tags[4], sizeof(char), 41, fp);

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
    binarioNaTela1(fp);
    fclose(fp);
}

Record *read_record_stdin(){
    int id;
    double salario;
    char tel[15], nome[MAX_SIZE], cargo[MAX_SIZE];

    scanf("%d", &id);
    salario = le_salario();
    scan_quote_string(tel);
    scan_quote_string(nome);
    scan_quote_string(cargo);

    return record_create(id, salario, tel, nome, cargo);
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

void insert_record(FILE *fp, Record *r){
    long start_pos = ftell(fp);

    fseek(fp, 1L, SEEK_SET);

    long long next_offset = get_list_start(fp);
    long long previous_offset = 1;  // começa offset do topo da lista, caso remova o primeiro

    if (next_offset == -1){                         // lista avazia, insiro no final do arquivo
        long long last_offset = goto_last_record(fp);         // coloca fp no final do ulrimo registro e retorna o inicio do reg. p/ last_offset
        write_record(fp, r, last_offset);
    }

    int insert_size = record_size(r);
    int current_size;

    while (next_offset != -1){
        previous_offset = ftell(fp);
        fseek(fp, next_offset, SEEK_SET);

        current_size = get_record_size(fp);
        next_offset = get_next_offset(fp);

        if (current_size >= insert_size){   // insere e remove espaço vago da lista
            write_record(fp, r, ftell(fp)); // escreve o registro
            fseek(fp, previous_offset, SEEK_SET);
            set_next_offset(fp, next_offset);   // faz atual->anterior apontar p/ atual->prox

            fseek(fp, start_pos, SEEK_SET);
            return;
        }
    }

    if (next_offset == -1){         // escreve no final, já que nao tem espaço disponível
        long long last_offset = goto_last_record(fp);         // coloca fp no final do ulrimo registro e retorna o inicio do reg. p/ last_offset
        write_record(fp, r, last_offset);
    }

    fseek(fp, start_pos, SEEK_SET);
}

void insert_records(char *filename){
    int n;
    scanf("%d", &n);

    FILE *fp = fopen(filename, "rb+");
    if (fp == NULL || safety_byte(fp) == '0'){
        printf("Falha no processamento do arquivo.\n");
        exit(0);
    }

    if (DEBUG){
        print_list(fp);
        return;
    }

    // set_safety_byte(fp, '0');   // atualiza byte de integridade

    char tags[5][41];  // leio a tag e o descritor. tag[i][0] = caracter que define a i-ésima tag

    /* le os metadados do cabeçalho */
    fseek(fp, (long)9, SEEK_SET);
    fread(tags[0], sizeof(char), 41, fp);
    fread(tags[1], sizeof(char), 41, fp);
    fread(tags[2], sizeof(char), 41, fp);
    fread(tags[3], sizeof(char), 41, fp);
    fread(tags[4], sizeof(char), 41, fp);

    for (int i = 0; i < n; i++){
        Record *r = read_record_stdin();
        insert_record(fp, r);
        record_free(r);
    }

    set_safety_byte(fp, '1');
    // binarioNaTela1(fp);
    fclose(fp);
}
