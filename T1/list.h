#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>

typedef struct node_ Node;
typedef struct list_ List;

struct list_{
	int (*compare_function)(void *, void *);
	void (*free_function)(void *);
	void (*print_function)(void *);
	Node *start;
};

struct node_{
	Node *next;
	void *item;
};

Node *node_create(void *item);

void node_free(Node *n, void (*free_function)(void *item));

List *list_create(int (*compare_function)(void *, void *), void (*free_function)(void *), void (*print_function)(void *));

void list_insert(List *l, void *item);

void list_free(List *l);

void list_print(List *l);

void list_write_records(List *l, FILE *fp, void (*write_function)(FILE *fp, void *item, int prev, int w_size));

int list_write_index(List *l, FILE *fp, void (*write_function)(FILE *fp, void *item), int (*is_valid_item)(void *item));

#endif
