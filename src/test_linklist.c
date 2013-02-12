#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "linklist.h"


int main(int argc, char *argv[]) {
	struct list *list = list_new();

	list->cmp = strcmp;
	list->copy = strdup;
	list->del = free;

	listnode_add(list, list->copy("1"));

	return 0;
}
