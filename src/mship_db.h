#ifndef __MSHIP_DB_H
#define __MSHIP_DB_H 

#include "linklist.h"

/* implementation of RFC4605 membership database */

/*struct in6_addr *in6_addr_getcopy(const struct in6_addr *src);
int in6_addr_cmp(const struct in6_addr *a, const struct in6_addr *b);
*/void log_ip6addr(int log_level, const struct in6_addr *addr);

void mdb_subscribe(struct list *memb_db, struct in6_addr *mca, int mode, struct list *sources);
void mdb_print(struct list *memb_db);
int mdb_deepcopy(struct list *old, struct list **new);
const char *mld_grec_mode(int mode);

#endif /* __MSHIP_DB_H */
