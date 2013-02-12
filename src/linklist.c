/* Generic linked list routine.
 * Copyright (C) 1997, 2000 Kunihiro Ishiguro
 * modded for ecmh by Jeroen Massar
 */

#include "linklist.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>

/* Allocate new list. */
struct list *list_new()
{
	struct list *new;

	new = malloc(sizeof(struct list));
	if (!new) return NULL;
	memset(new, 0, sizeof(struct list));
	return new;
}

/* Free list. */
void list_free(struct list *l)
{
	if (l) free(l);
}

/* Allocate new listnode.  Internal use only. */
static struct listnode *listnode_new(void)
{
	struct listnode *node;

	node = malloc(sizeof(struct listnode));
	if (!node) return NULL;
	memset(node, 0, sizeof(struct listnode));
	return node;
}

/* Free listnode. */
static void listnode_free(struct listnode *node)
{
	free(node);
}


/* checks if elem is part of list. uses list->cmp when defined, 
 * ptr equality otherwise.
 *
 * returns ptr of element in list if cmp!=0 (or element==needle when cmp==NULL),
 *         NULL otherwise
 */
void *list_hasmember(const struct list *list, const void* needle) {
	struct listnode *ln;
	void* element;

	if (list->cmp) {
		LIST_LOOP(list, element, ln) {
			if (list->cmp(element, needle)) {
				return element;
			}
		}
	} else {
		LIST_LOOP(list, element, ln) {
			if (element == needle) {
				return element;
			}
		}
	}

	return NULL;
}


/* returns the intersection between a and b as a new list. */
/* uses copy and cmp if defined, ptr addresses otherwise. */
/* result will use a->del, a->cmp and a->copy. */
struct list *list_intersect(const struct list *a, const struct list *b) {
	struct list *result;
	void *element;
	struct listnode *ln;

	result = list_new();
	result->del = (void(*)(void *))a->del;
	result->copy = (void*(*)(const void *))a->copy;
	result->cmp = (int(*)(const void *, const void*))a->cmp;
	
	LIST_LOOP(a, element, ln) {
		if (list_hasmember(b, element)) {
			element = a->copy(element);
			listnode_add(result, element);
		}
	}

	LIST_LOOP(b, element, ln) {
		if (list_hasmember(a, element) && ! list_hasmember(result, element)) {
			element = a->copy(element);
			listnode_add(result, element);
		}
	}

	return result;
}

struct list *list_union(const struct list *a, const struct list *b) {
	void *elem;
	struct listnode *ln;

	struct list *result = list_new();
	result->del = a->del;
	result->cmp = a->cmp;
	result->copy = a->copy;

	LIST_LOOP(a, elem, ln) {
		if (a->copy) elem = a->copy(elem);
		listnode_add(result, elem);
	}

	if (b) {
		LIST_LOOP(b, elem, ln) {
			if (!list_hasmember(result, elem)) {
				if (a->copy) elem = a->copy(elem);
				listnode_add(result, elem);
			}
		}
	}

	return result;
}

struct list * list_remove_all(struct list *a, const struct list *b) {
	void *element;
	struct listnode *ln;
	
	LIST_LOOP(b, element, ln) {
		if ((element = list_hasmember(a, element))) {
			listnode_delete(a, element);
		}
	}

	return a;
}

struct list * list_add_all(struct list *a, const struct list *b) {
	void *element;
	struct listnode *ln;
	
	LIST_LOOP(b, element, ln) {
		if (! (element = list_hasmember(a, element))) {
			listnode_add(a, element);
		}
	}

	return a;
}

struct list * list_difference(const struct list *a, const struct list *b) {
	void *element;
	struct listnode *ln;
	
	struct list *result = list_new();
	result->del = a->del;
	result->cmp = a->cmp;
	result->copy = a->copy;

	LIST_LOOP(a, element, ln) {
		if (!list_hasmember(b, element)) {
			element = a->copy(element);
			listnode_add(result, element);
		}
	}

	return result;
}

/* Add new data to the list. */
void listnode_add(struct list *list, void *val)
{
	struct listnode *node;

	node = listnode_new();
	if (!node) return;

	node->prev = list->tail;
	node->data = val;

	if (list->head == NULL) list->head = node;
	else list->tail->next = node;
	list->tail = node;

	if (list->count < 0) list->count = 0;
	list->count++;
}

/* Delete specific date pointer from the list. */
void listnode_delete(struct list *list, void *val)
{
	struct listnode *node;

	for (node = list->head; node; node = node->next)
	{
		if (node->data != val) continue;

		if (node->prev) node->prev->next = node->next;
		else list->head = node->next;
		if (node->next) node->next->prev = node->prev;
		else list->tail = node->prev;
		list->count--;
		listnode_free(node);
		return;
	}
}

/* Delete all listnode from the list. */
void list_delete_all_node(struct list *list)
{
	struct listnode *node;
	struct listnode *next;

	for (node = list->head; node; node = next)
	{
		next = node->next;
		if (list->del) (*list->del)(node->data);
		listnode_free(node);
	}
	list->head = list->tail = NULL;
	list->count = 0;
}

/* Delete all listnode then free list itself. */
void list_delete(struct list *list)
{
	struct listnode *node;
	struct listnode *next;

	for (node = list->head; node; node = next)
	{
		next = node->next;
		if (list->del) (*list->del)(node->data);
		listnode_free(node);
	}
	list_free (list);
}

/* Delete the node from list.  For ospfd and ospf6d. */
void list_delete_node(struct list *list, struct listnode *node)
{
	if (node->prev) node->prev->next = node->next;
	else list->head = node->next;
	if (node->next) node->next->prev = node->prev;
	else list->tail = node->prev;
	list->count--;
	listnode_free(node);
}

/* Move the node to the front - for int_find() - jeroen */
void list_movefront_node(struct list *list, struct listnode *node)
{
	/* Don't do a thing when it is already there */
	if (list->head == node) return;

	/* Delete it from the list's current position */
	if (node->prev) node->prev->next = node->next;
	else list->head = node->next;
	if (node->next) node->next->prev = node->prev;
	else list->tail = node->prev;

	/* Insert it at the front */
	if (list->head)
	{
		node->prev = list->head->prev;
		list->head->prev = node;
	}
	node->next = list->head;
	list->head = node;
}

