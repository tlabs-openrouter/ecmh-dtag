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

int mrec_add_source(struct membership_record *mrec, const struct in6_addr *mca);

void mdb_subscribe(struct list *memb_db, struct in6_addr *mca, int mode, struct in6_addr *source);
void log_ip6addr(int log_level, const struct in6_addr *addr);
void mdb_print(struct list *memb_db);
int mdb_deepcopy(struct list *old, struct list **new);

#endif /* __MSHIP_DB_H */
