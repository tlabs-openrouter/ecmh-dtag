#ifndef __MSRC_H
#define __MSRC_H 

#include <netinet/in.h>
#include <time.h>

#include "linklist.h"

struct msrc {
	struct in6_addr addr;
	time_t timer;
};

struct msrc* msrc_create(const struct in6_addr *addr, int timer_value);
void msrc_free(struct msrc *src);
struct msrc *msrc_getcopy(const struct msrc *src);
int msrc_cmp(const struct msrc *a, const struct msrc *b);
void print_sources(struct list *sources);

#endif /* __MSRC_H */
