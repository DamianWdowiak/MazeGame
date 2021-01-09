#include <stdlib.h>
#include <stdio.h>
#include "linked_list.h"

struct linked_list_t* ll_create() {
	struct linked_list_t* list = (struct linked_list_t*)calloc(1, sizeof(struct linked_list_t));
	if (!list)
		return NULL;
	list->head = NULL;
	list->tail = NULL;
	return list;
}

int ll_push_back(struct linked_list_t* ll, int value,int x,int y) {
	if (!ll)
		return 1;

	struct node_t* node = (struct node_t*)calloc(1, sizeof(struct node_t));
	if (!node) {
		return 2;
	}
	node->data = value;
	node->x = x;
	node->y = y;
	if (!ll->head && !ll->tail) {
		ll->head = node;
		ll->tail = node;
	}
	else if (ll->tail) {
		ll->tail->next = node;
		ll->tail = node;
	}
	else {
		free(node);
		return 1;
	}
	return 0;
}

int ll_pop_front(struct linked_list_t* ll, int* err_code) {
	if (!ll || !ll->head) {
		if (err_code)
			* err_code = 1;
		return -1;
	}
	if (err_code)
		* err_code = 0;
	if (ll->head == ll->tail) {
		int temp = ll->tail->data;
		free(ll->head);
		ll->head = NULL;
		ll->tail = NULL;
		return temp;
	}
	if (ll->head) {
		int temp = ll->head->data;
		struct node_t* node = ll->head;
		ll->head = ll->head->next;
		free(node);
		return temp;
	}
	if (err_code)
		* err_code = 1;
	return 0;
}
int ll_pop_back(struct linked_list_t* ll, int* err_code) {
	if (!ll || !ll->head) {
		if (err_code)
			* err_code = 1;
		return -1;
	}
	if (err_code)
		* err_code = 0;
	if (ll->head == ll->tail) {
		int temp = ll->tail->data;
		free(ll->head);
		ll->head = NULL;
		ll->tail = NULL;
		return temp;
	}
	if (ll->tail) {
		int temp = ll->tail->data;
		struct node_t* node = ll->tail;
		struct node_t* temp2 = ll->head;
		while (temp2->next != ll->tail) {
			temp2 = temp2->next;
		}
		ll->tail = temp2;
		ll->tail->next = NULL;
		free(node);
		return temp;
	}
	if (err_code)
		* err_code = 1;
	return 0;
}

int ll_remove(struct linked_list_t* ll, unsigned int index, int* err_code) {
	if (!ll || !ll->head) {
		if (err_code)
			* err_code = 1;
		return -1;
	}
	if (!index)
		return ll_pop_front(ll, err_code);
	struct node_t* node = ll->head;
	for (unsigned int i = 0; i < index; ++i) {
		node = node->next;
		if (node) {
			if (i == index - 1 && !node->next)
				return ll_pop_back(ll, err_code);
		}
		if (!node) {
			if (err_code)
				* err_code = 1;
			return -1;
		}
	}
	if (err_code)
		* err_code = 0;
	int temp = node->data;
	struct node_t* temp2 = ll->head;
	while (temp2->next != node) {
		temp2 = temp2->next;
	}
	temp2->next = node->next;
	free(node);
	return temp;
}

void ll_clear(struct linked_list_t* ll) {
	if (ll) {
		struct node_t* node = ll->head;
		while (node) {
			struct node_t* temp = node->next;
			free(node);
			node = temp;
		}
		ll->head = NULL;
		ll->tail = NULL;
	}
}