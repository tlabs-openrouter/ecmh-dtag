#ifndef __MSHIP_DB_H
#define __MSHIP_DB_H 

#include "linklist.h"

/* implementation of RFC4605 membership database */

struct membership_record {
	struct in6_addr mca;
	int filter_mode;
	struct list *source_list;
};

struct membership_record *mrec_create(const struct in6_addr *mca, int mode);
void mrec_destroy(struct membership_record *mrec);
struct membership_record *mrec_find(const struct list *memb_db, const struct in6_addr *mca, int mode);


struct msrc {
	struct in6_addr addr;
	time_t timer;
};

struct msrc* msrc_create(const struct in6_addr *addr, int timer_value);
void msrc_free(struct msrc *src);
struct msrc *msrc_getcopy(const struct msrc *src);
int msrc_cmp(const struct msrc *a, const struct msrc *b);
void print_sources(struct list *sources);

int mrec_add_source(struct membership_record *mrec, const struct msrc *src);
int mrec_add_source_addr(struct membership_record *mrec, const struct in6_addr *addr);

/*struct in6_addr *in6_addr_getcopy(const struct in6_addr *src);
int in6_addr_cmp(const struct in6_addr *a, const struct in6_addr *b);
*/void log_ip6addr(int log_level, const struct in6_addr *addr);

void mdb_subscribe(struct list *memb_db, struct in6_addr *mca, int mode, struct list *sources);
void mdb_print(struct list *memb_db);
int mdb_deepcopy(struct list *old, struct list **new);
void mrec_print(struct membership_record *mrec);
const char *mld_grec_mode(int mode);

#endif /* __MSHIP_DB_H */
