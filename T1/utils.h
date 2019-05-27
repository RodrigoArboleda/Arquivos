#ifndef UTILS_H
#define UTILS_H

#define DISK_PG 32000

typedef void (*FuncaoEscritaDado)(FILE *, void *, int, int);
typedef void (*FuncaoEscritaIndice)(FILE *, void *);
typedef void (*FuncaoPadraoVoid)(void *);
typedef int (*FuncaoComparacao)(void *, void *);
typedef int (*FuncaoItemValido)(void *);

int get_number_records(FILE *index);

#endif
