#include <malloc.h>
#include <string.h>

#include "msrc.h"

#ifdef DEBUG
#include <syslog.h>
#include "common.h"
#endif

struct msrc* msrc_create(const struct in6_addr *addr, int timer_value) {
	struct msrc *src = malloc(sizeof(*src));
	if (src) {
		memcpy(&src->addr, addr, sizeof(*addr));
		src->timer = timer_value;
	}

	return src;
}

void msrc_free(struct msrc *src) {
	free(src);
}

struct msrc *msrc_getcopy(const struct msrc *src) {
	struct msrc *copy = malloc(sizeof(*copy));
	if (copy) {
		memcpy(copy, src, sizeof(*src));
	}

	return copy;
}

int msrc_cmp(const struct msrc *a, const struct msrc *b) {
	return IN6_ARE_ADDR_EQUAL(&a->addr, &b->addr);
}

#ifdef DEBUG
void print_sources(struct list *sources) {
	struct listnode     *ln;
	struct msrc *src;
	int level = LOG_DEBUG;
	dolog(LOG_DEBUG, "{\n");
	LIST_LOOP(sources, src, ln){
		log_ip6addr(level, &src->addr);
	}
	dolog(level, "}\n");
}
#endif
