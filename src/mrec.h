#ifndef __MREC_H
#define __MREC_H 

#include <netinet/in.h>

#include "linklist.h"

struct msrc;

struct mrec {
	struct in6_addr mca;
	int filter_mode;
	struct list *source_list;
};

struct mrec *mrec_create(const struct in6_addr *mca, int mode);
void mrec_destroy(struct mrec *mrec);
struct mrec *mrec_find(const struct list *memb_db, const struct in6_addr *mca, int mode);

void mrec_print(struct mrec *mrec);

int mrec_add_source(struct mrec *mrec, const struct msrc *src);
int mrec_add_source_addr(struct mrec *mrec, const struct in6_addr *addr);

const char *mld_grec_mode(int mode);

#endif /* __MREC_H */
