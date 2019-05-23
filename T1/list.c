#include "list.h"

Node *node_create(void *item, int (*get_sort_param)(void *item)){
	Node *n = (Node *)malloc(sizeof(Node));
	n->item = item;
	n->next = NULL;
	
	if (item != NULL) n->sort_param = get_sort_param(item);
	else n->sort_param = 0;

	return n;
}

void node_free(Node *n, void (*free_function)(void *item)){
	if (n == NULL) return;

	n->next = NULL;
	free_function(n->item);
	free(n);
}

List *list_create(int (*get_sort_param)(void *), void (*free_function)(void *), void (*print_function)(void *)){
	List *l = (List *)malloc(sizeof(List));
	
	l->free_function = free_function;
	l->get_sort_param = get_sort_param;
	l->print_function = print_function;

	l->start = node_create(NULL, NULL);

	return l;
}

void list_insert(List *l, void *item){
	if (l == NULL || item == NULL) return;

	Node *cur = l->start;
	Node *insert = node_create(item, l->get_sort_param);

	while (cur->next != NULL && insert->sort_param > cur->next->sort_param) cur = cur->next;

	if (cur->next == NULL){
		cur->next = insert;
	} else {
		insert->next = cur->next;
		cur->next = insert;		
	}
}

void recursive_destruction(Node *n, void (*free_function)(void *)){
	if (n == NULL) return;
	recursive_destruction(n->next, free_function);
	node_free(n, free_function);
}

void list_free(List *l){
	recursive_destruction(l->start, l->free_function);
	free(l);
}

void list_print(List *l){
	if (l == NULL) return;

	Node *cur = l->start->next;
	while (cur != NULL){
		l->print_function(cur->item);
		cur = cur->next;
	}
}

void list_write_records(List *l, FILE *fp, void (*write_function)(FILE *fp, void *item, int prev, int w_size)){
	if (l == NULL || fp == NULL) return;

	Node *cur = l->start->next;
	long long prev = -1, current = ftell(fp);

	while (cur != NULL){
		current = ftell(fp);
		write_function(fp, cur->item, prev, 1);
	
		prev = current;
		cur = cur->next;
	}
}