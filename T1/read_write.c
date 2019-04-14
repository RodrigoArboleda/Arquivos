#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "read_write.h"

/* Victor Giovannoni 10786159 */

#define DISK_PG 32000
#define TRASH '@'
#define INVALID -1337

typedef struct record_ Record;

struct record_{
    char nomeServidor[100];
    char cargoServidor[100];
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

/* Impressão do registro p/ debug */
void record_print(Record *r){
    if (r == NULL) return;
    printf("%08d  |  %-14s  |  %-9.2lf  |  %-40s  |  %-30s\n", r->idServidor, r->telefoneServidor, r->salarioServidor, r->cargoServidor, r->nomeServidor);
}

/* Le e retorna registro do arquivo csv apontado por fp */
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

/* Escreve um único registro  */
void write_record(FILE *fp, Record *r, int last_record){
    int size = record_size(r);

    int pages = ceiling(ftell(fp)/(double)DISK_PG);
    int bytes_written = ftell(fp)%DISK_PG;

    if ((bytes_written + size)/(double)DISK_PG > 1){
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

    fwrite("-", sizeof(char), 1, fp);
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

    if (removed == '*'){
        fseek(fp, (long)(size), SEEK_CUR);
        return read_record_binary(fp);
    } else if (removed != '-'){         // pular pra proxima pagina de disco
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
    char tag, nome[100], cargo[100];
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

    if (r->telefoneServidor[0] == '\0')
        for (int i = 0; i < 15; i++) putchar(' ');
    else printf("%-14s ", r->telefoneServidor);

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
        Record *r = read_record_binary(fp);     // retorna NNULL quando acaba o arquivo
        if (r == NULL) break;

        record_ugly_print(r);
        record_free(r);
    }
    if (i == 0){
        printf("Registro inexistente.\n");
        exit(0);
    }

    fseek(fp, 0L, SEEK_END);

    printf("Número de páginas de disco acessadas: %d\n", ceiling(ftell(fp)/(double)DISK_PG));
    fclose(fp);
}

/*  tendo fp no o tamanho do registro, a função busca o valor do id, compara, retorna valor booleano e volta fp p/ inicio do registro
    Coloca finished p/ 1 caso encontre a chave
*/
int compare_id(FILE *fp, void *key, char tag, int *finished){
    long start_pos = ftell(fp);
    int record_size;
    fread(&record_size, sizeof(int), 1, fp);

    int key_id = *((int *)key); // recupero a chave do ponteiro

    fseek(fp, (long)sizeof(long long), SEEK_CUR);   // pula o encadeamento
    // IDEIA: PROGRAMA ANALISAR DEPENDENCIA DE FUNÇÕES COM GRAFOS (.h)
    int record_id;
    fread(&record_id, sizeof(int), 1, fp);

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
    *finished = record_id == key_id;    // indica se já achou o campo único
    return record_id == key_id;
}

/* tendo fp no o tamanho do registro, a função busca o valor do salario, compara, retorna valor booleano e volta fp p/ inicio do registro */
int compare_salary(FILE *fp, void *key, char tag, int *finished){
    long start_pos = ftell(fp);
    int record_size;
    fread(&record_size, sizeof(int), 1, fp);

    double key_salary = *((double *)key); // recupero a chave do ponteiro

    fseek(fp, (long)(sizeof(long long) + sizeof(int)) , SEEK_CUR);   // pula o encadeamento e o id
    // IDEIA: PROGRAMA ANALISAR DEPENDENCIA DE FUNÇÕES COM GRAFOS (.h)
    double record_salary;
    fread(&record_salary, sizeof(double), 1, fp);

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
    return record_salary == key_salary;
}

/* tendo fp no o tamanho do registro, a função busca o valor do telefone, compara, retorna valor booleano e volta fp p/ inicio do registro */
int compare_phone(FILE *fp, void *key, char tag, int *finished){
    long start_pos = ftell(fp);
    int record_size;
    fread(&record_size, sizeof(int), 1, fp);

    char *key_phone = (char *)key; // recupero a chave do ponteiro

    fseek(fp, (long)(sizeof(long long) + sizeof(int) + sizeof(double)), SEEK_CUR);   // pula o encadeamento, id e salario
    // IDEIA: PROGRAMA ANALISAR DEPENDENCIA DE FUNÇÕES COM GRAFOS (.h)
    char record_phone[15];
    fread(record_phone, sizeof(char), 14, fp);
    record_phone[14] = '\0';

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial
    return !strcmp(record_phone, key_phone);
}

/* tendo fp no o tamanho do registro, a função busca o valor do nome, compara, retorna valor booleano e volta fp p/ inicio do registro */
int compare_name(FILE *fp, void *key, char tag, int *finished){
    long start_pos = ftell(fp);
    int record_size;
    fread(&record_size, sizeof(int), 1, fp);

    char *key_name = (char *)key; // recupero a chave do ponteiro

    fseek(fp, (long)(sizeof(long long) + sizeof(int) + sizeof(double) + 14*sizeof(char)), SEEK_CUR);   // pula o encadeamento, id, salario, telefone

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

    char record_name[300];
    fread(record_name, sizeof(char), field_size - 1, fp);

    fseek(fp, start_pos, SEEK_SET); // retorna p/ posicao inicial

    return !strcmp(record_name, key_name);
}

/* tendo fp no o tamanho do registro, a função busca o valor do cargo, compara, retorna valor booleano e volta fp p/ inicio do registro */
int compare_job(FILE *fp, void *key, char tag, int *finished){
    long start_pos = ftell(fp);

    char *key_job = (char *)key; // recupero a chave do ponteiro

    int record_size;
    fread(&record_size, sizeof(int), 1, fp);
    fseek(fp, (long)(sizeof(long long) + sizeof(int) + sizeof(double) + 14*sizeof(char)), SEEK_CUR);   // pula o encadeamento p/ ler tamanho

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

/* dado o fp no tamanho do registro, ele pula para o próximo tamanho de registro */
void skip_record(FILE *fp){
    char removed = '*';
    while (!feof(fp) && removed == '*'){
        int record_size;
        fread(&record_size, sizeof(int), 1, fp);
        fseek(fp, (long)record_size, SEEK_CUR);

        fread(&removed, sizeof(char), 1, fp);
    }
}

/* função que imprime no formato da funcionalidade 3 */
void record_print_tags(Record *r, char *t1, char *t2, char *t3, char *t4, char *t5){
    printf("%s: %d\n", t1, r->idServidor);

    if (r->salarioServidor != -1) printf("%s: %.2lf\n", t2, r->salarioServidor);
    else printf("%s: valor nao declarado\n", t2);

    if (r->telefoneServidor[0] != '\0') printf("%s: %14s\n", t3, r->telefoneServidor);
    else printf("%s: valor nao declarado\n", t3);

    if (r->nomeServidor[0] != '\0') printf("%s: %s\n", t4, r->nomeServidor);
    else printf("%s: valor nao declarado\n", t4);

    if (r->cargoServidor[0] != '\0') printf("%s: %s\n\n", t5, r->cargoServidor);
    else printf("%s: valor nao declarado\n\n", t5);

}

/* Essa função busca os registros pela chave key, acessando e comparando o campo determinado por compare_function */
void search_records(FILE *fp, int (*compare_function)(FILE *fp, void *key, char tag, int *finished), void *key, char tag, char *t1, char *t2, char *t3, char *t4, char *t5){
    int results = 0;

    int finished = 0;   /* variável que diz se já achou o campo único */
    for (int i = 0; !feof(fp) && !finished; i++){
        if (compare_function(fp, key, tag, &finished)){
            fseek(fp, (long)-1, SEEK_CUR);  // volta p/ começo do reg
            Record *r = read_record_binary(fp);
            fseek(fp, (long)1, SEEK_CUR);   // coloca fp no tamanho do registro

            record_print_tags(r, t1, t2, t3, t4, t5);
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
    char field_name[100];
    scanf("%s ", field_name);


    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL || safety_byte(fp) == '0'){
        printf("Falha no processamento do arquivo.\n");
        exit(0);
    }

    char tag1[41], tag2[41], tag3[41], tag4[41], tag5[41];  // leio a tag e o descritor. tag[0] = caracter que a define

    /* le os metadados do cabeçalho */
    fseek(fp, (long)9, SEEK_SET);
    fread(tag1, sizeof(char), 41, fp);
    fread(tag2, sizeof(char), 41, fp);
    fread(tag3, sizeof(char), 41, fp);
    fread(tag4, sizeof(char), 41, fp);
    fread(tag5, sizeof(char), 41, fp);

    fseek(fp, (long)(DISK_PG + 1), SEEK_SET); // pula cabeçalho

    if (!strcmp(field_name, "idServidor")){
        int field_value;
        scanf("%d", &field_value);
        search_records(fp, compare_id, &field_value, tag1[0], tag1 + 1, tag2 + 1, tag3 + 1, tag4 + 1, tag5 + 1);
    }
    if (!strcmp(field_name, "salarioServidor")){
        double field_value;
        scanf("%lf", &field_value);
        search_records(fp, compare_salary, &field_value, tag2[0], tag1 + 1, tag2 + 1, tag3 + 1, tag4 + 1, tag5 + 1);
    }
    if (!strcmp(field_name, "telefoneServidor")){
        char field_value[15];
        scanf("%s", field_value);
        search_records(fp, compare_phone, field_value, tag3[0], tag1 + 1, tag2 + 1, tag3 + 1, tag4 + 1, tag5 + 1);
    }
    if (!strcmp(field_name, "nomeServidor")){
        char field_value[100];
        scanf("%[^\r\n]", field_value); // quase um dia de debug pra achar esse \r

        search_records(fp, compare_name, field_value, tag4[0], tag1 + 1, tag2 + 1, tag3 + 1, tag4 + 1, tag5 + 1);
    }
    if (!strcmp(field_name, "cargoServidor")){
        char field_value[100];
        scanf("%[^\r\n]", field_value); // quase um dia de debug pra achar esse \r

        search_records(fp, compare_job, field_value, tag5[0], tag1 + 1, tag2 + 1, tag3 + 1, tag4 + 1, tag5 + 1);
    }


    fclose(fp);
}
