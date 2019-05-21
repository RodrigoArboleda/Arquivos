#ifndef LISTA_H
#define LISTA_H

#include <stdio.h>
#include <stdlib.h>

#define INIT_SIZE 2000

typedef struct node_ Node;
typedef struct list_ List;

struct list_{
	int (*size_funcion)(void *);
	void (*free_function)(void *);
	Node *start;
};

struct node_{
	Node *next;
	void *item;
	int size;
};

Node *node_create(void *item, int (*size_function)(void *item)){
	Node *n = (Node *)malloc(sizeof(Node));
	n->item = item;
	n->next = NULL;
	
	if (item != NULL) n->size = size_function(item);
	else n->size = 0;

	return n;
}

void node_free(Node *n, void (*free_function)(void *item)){
	if (n == NULL) return;

	n->next = NULL;
	free_function(n->item);
	free(n);
}

List *list_create(int (*size_function)(void *), void (*free_function)(void *)){
	List *l = (List *)calloc(INIT_SIZE, sizeof(void *));
	
	l->max_size = INIT_SIZE;
	l->cur_size = 0;
	l->free_function = free_function;
	l->size_function = size_function;
	
	l->start = node_create(NULL, NULL);

	return l;
}

void list_insert(List *l, void *item){
	if (n == NULL || item == NULL) return;

	Node *cur = l->start;
	Node *insert = node_create(item, l->size_function);

	while (cur->next != NULL && insert->size > cur->next->size) cur = cur->next;

	if (cur->next == NULL){
		cur->next = insert;
	} else {
		insert->next = cur->next;
		cur->next = insert;		
	}
}

void list_free(List *l){

}

#endif