#include "ecmh.h"

int mdb_deepcopy(struct list *old, struct list **new) {
	struct listnode *ln;
	struct listnode *ln2;
	struct membership_record *mrec;
	struct in6_addr *addr;
	struct list *target;

	*new = list_new();
	target = *new;
	target->del = (void(*)(void *))mrec_destroy;

	LIST_LOOP(old, mrec, ln) {
		struct membership_record *mrec_copy = mrec_create(&mrec->mca, mrec->filter_mode);
		LIST_LOOP(mrec->source_list, addr, ln2) {
			mrec_add_source(mrec_copy, addr);
		}
		listnode_add(target, (void*)mrec_copy);
	}

	return 0;
}

/* creates a list of changes from a to b and stores them in C */
int mdb_compare(struct list *a, struct list *b, struct list **changes) {
	struct list *target;

	*changes = list_new();
	target = *changes;
}

static void mrec_del_source(struct in6_addr *src) {
	free(src);
}

void log_ip6addr(int log_level, const struct in6_addr *addr) {
	char addr1[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, addr, addr1, sizeof(addr1));
	dolog(log_level, "%s\n", addr1);
}

struct membership_record *mrec_create(const struct in6_addr *mca, int mode) {
	struct membership_record *mrec = malloc(sizeof(struct membership_record));

	if (mrec) {
		memset(mrec, 0, sizeof(struct membership_record));
		memcpy(&mrec->mca, mca, sizeof(*mca));
		mrec->filter_mode = mode;
		mrec->source_list = list_new();
		mrec->source_list->del = (void(*)(void *))mrec_del_source;
	}

	return mrec;
}

void mrec_destroy(struct membership_record *mrec) {
	dolog(LOG_DEBUG, "Destroying membership record.\n");
	list_delete_all_node(mrec->source_list);
	free (mrec);
}

void mrec_print(struct membership_record *mrec) {
	char addr1[INET6_ADDRSTRLEN];
	struct listnode *ln;
	struct in6_addr *addr;

	inet_ntop(AF_INET6, &mrec->mca, addr1, sizeof(addr1));
	dolog(LOG_DEBUG, "(%s, %s, {\n", addr1, mrec->filter_mode==1?"INCLUDE":"EXCLUDE");
	
	LIST_LOOP(mrec->source_list, addr, ln) {
		inet_ntop(AF_INET6, addr, addr1, sizeof(addr1));
		dolog(LOG_DEBUG, "%s\n", addr1);
	}
	
	dolog(LOG_DEBUG, "})\n");
}

void mdb_print(struct list *memb_db) {
	struct listnode *ln;
	struct membership_record *mrec;

	dolog(LOG_DEBUG, "Membership database: {\n");
	LIST_LOOP(memb_db, mrec, ln) {
		mrec_print(mrec);
	}
	dolog(LOG_DEBUG, "}\n");
}

struct membership_record *mrec_find(const struct list *memb_db, const struct in6_addr *mca, int mode) {
	struct listnode *ln;
	struct membership_record *mrec;

	LIST_LOOP(memb_db, mrec, ln) {
		if (IN6_ARE_ADDR_EQUAL(mca, &mrec->mca) && mrec->filter_mode == mode) {
			return mrec;
		}
	}

	return NULL;
}

static bool list_hasmember(const struct list *source_list, const struct in6_addr *src) {
	struct listnode *ln;
	struct in6_addr *addr;

	LIST_LOOP(source_list, addr, ln) {
		if (IN6_ARE_ADDR_EQUAL(src, addr)) {
			return true;
		}
	}

	return false;
}

static int mrec_remove_source(struct membership_record *mrec, const struct in6_addr *src) {
	struct listnode *ln;
	struct in6_addr *addr;
	LIST_LOOP(mrec->source_list, addr, ln) {
		if (IN6_ARE_ADDR_EQUAL(&src, &addr)) {
			listnode_delete(mrec->source_list, addr);
			return 1;
		}
	}

	return 0;
}

int mrec_add_source(struct membership_record *mrec, const struct in6_addr *src) {
	struct in6_addr *addr;

	if (list_hasmember(mrec->source_list, src)) {
			dolog(LOG_WARNING, "Address already in list.\n");
			return 0;
	}
	
	addr = malloc(sizeof(*addr));
	if (addr) {
		memcpy(addr, src, sizeof(*addr));
		listnode_add(mrec->source_list, (void*)addr);
		return 1;
	} else {
		dolog(LOG_WARNING, "Cannot allocate IPv6 address.\n");
		return 0;
	}
}

void mdb_subscribe(struct list *memb_db, struct in6_addr *mca, int mode, struct in6_addr *source) {
	struct in6_addr any = {0};
	struct membership_record *mrec;

	int is_any = IN6_ARE_ADDR_EQUAL(source, &any);
#ifdef DEBUG
	char mca_str[INET6_ADDRSTRLEN];
	char src_str[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, mca, mca_str, sizeof(mca_str));
	inet_ntop(AF_INET6, source, src_str, sizeof(src_str));
	dolog(LOG_DEBUG, "Adding subscription %s %s { %s } (is_any=%i)\n", mca_str, mode==1?"INCLUDE":"EXCLUDE", src_str, is_any);
#endif
	
	if ((mode == 2 && !is_any) || (mode==1 && is_any)) {
		dolog(LOG_DEBUG, "Won't exclude anything.\n");
		return;
	}

	if (mode == 1) { 
		/* INCLUDE */
		mrec = mrec_find(memb_db, mca, 1);
		if (mrec && is_any) {
			dolog(LOG_DEBUG, "INCLUDE{} (exclude whole group) requested, but there's another INCLUDE subscription.\n");
			return;
		}
		mrec = mrec_find(memb_db, mca, 2);
		if (mrec && list_isempty(mrec->source_list)) {
			dolog(LOG_DEBUG, "Source already included by EXCLUDE{}.\n");
			return;
		}
		if (mrec && mrec_remove_source(mrec, source)) {
			dolog(LOG_DEBUG, "Source also in EXCLUDE{}-list. Removing there.\n");
		}
	} else { 
		/* EXCLUDE */
		mrec = mrec_find(memb_db, mca, 1);

		if (mrec && is_any) {
			dolog(LOG_DEBUG, "EXLUDE{} (include whole group) requested. Removing other INCLUDE and EXCLUDE subscriptions.\n");
			listnode_delete(memb_db, mrec);

			mrec = mrec_find(memb_db, mca, 2);
			if (mrec) {
				list_delete_all_node(mrec->source_list);
			} else {
				mrec = mrec_create(mca, mode);
				listnode_add(memb_db, (void*) mrec);
			}
			
			return;
		} 
		
		if (mrec && list_hasmember(mrec->source_list, source)) {
			dolog(LOG_DEBUG, "Source needed my another subscription. Won't exclude.\n");
			return;
		}

		mrec = mrec_find(memb_db, mca, 2);
		if (mrec && list_isempty(mrec->source_list)) {
			dolog(LOG_DEBUG, "EXCLUDE{group} requested, but whole group is included.\n");
			return;
		}
	}

	mrec = mrec_find(memb_db, mca, mode);
	if (! mrec) {
		dolog(LOG_DEBUG, "Creating new membership record.\n");

		if (mode==2) {
			dolog(LOG_WARNING, "Exclusion of previously not included MC group requested.\n");
		}

		if ((mrec = mrec_create(mca, mode))) {
			listnode_add(memb_db, (void*) mrec);
		} else {
			dolog(LOG_WARNING, "Could not create membership record!\n");
			return;
		}
	}
	
/*	if (mode == 2 && IN6_ARE_ADDR_EQUAL(&source, &any)) {
		mrec = mrec_find(memb_db, mca, 1);
		if (mrec) listnode_delete(memb_db, mrec);
	}
*/
	if (!is_any) mrec_add_source(mrec, source);
}

