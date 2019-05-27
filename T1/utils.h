#ifndef UTILS_H
#define UTILS_H

#define DISK_PG 32000

/* Macros de debug */
#define PRINT_FILE_ON_SCREEN 1
#define PRINT_LIST 0
#define PRINT_RECORDS 0
#define BRIEF_REPORT 0
#define PRINT_NOT_MOVED 0
#define PRINT_ORDERED_LIST 0
#define PRINT_INDEX 0
#define CHOOSE_OUTPUT_NAME 0
#define PRINTING_MATCH_NAMES 0

typedef void (*FuncaoEscritaDado)(FILE *, void *, int, int);
typedef void (*FuncaoEscritaIndice)(FILE *, void *);
typedef void (*FuncaoPadraoVoid)(void *);
typedef int (*FuncaoComparacao)(void *, void *);
typedef int (*FuncaoItemValido)(void *);

int get_number_records(FILE *index);

#endif
