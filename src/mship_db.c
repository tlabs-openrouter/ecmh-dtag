#include "ecmh.h"

const char *mld_grec_mode(int mode) {
	switch (mode) {
		case 1: return "IS_IN";
		case 2: return "IS_EX";
		case 3: return "TO_IN";
		case 4: return "TO_EX";
		case 5: return "ALLOW";
		case 6: return "BLOCK";
	}
	return "UNKNOWN";
}

int mdb_deepcopy(struct list *old, struct list **new) {
	struct listnode *ln;
	struct listnode *ln2;
	struct membership_record *mrec;
	struct msrc *src;
	struct list *target;

	*new = list_new();
	target = *new;
	target->del = old->del;
	target->cmp = old->cmp;
	target->copy = old->copy;

	LIST_LOOP(old, mrec, ln) {
		struct membership_record *mrec_copy = mrec_create(&mrec->mca, mrec->filter_mode);
		LIST_LOOP(mrec->source_list, src, ln2) {
			mrec_add_source(mrec_copy, src);
		}
		listnode_add(target, (void*)mrec_copy);
	}

	return 0;
}

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

void print_sources(struct list *sources) {
	struct listnode     *ln;
	struct msrc *src;
	dolog(LOG_DEBUG, "...{\n");
	LIST_LOOP(sources, src, ln){
		log_ip6addr(LOG_DEBUG, &src->addr);
	}
	dolog(LOG_DEBUG, "...}\n");
}

/*struct in6_addr *in6_addr_getcopy(const struct in6_addr *src) {
	struct in6_addr *addr = malloc(sizeof(*addr));
	if (addr) {
		memcpy(addr, src, sizeof(*addr));
	}

	return addr;
}

int in6_addr_cmp(const struct in6_addr *a, const struct in6_addr *b) {
	return IN6_ARE_ADDR_EQUAL(a, b);
}
*/
void log_ip6addr(int log_level, const struct in6_addr *addr) {
	char addr1[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, addr, addr1, sizeof(addr1));
	dolog(LOG_DEBUG, "%s %llu\n", addr1, (long unsigned int)addr1);
}

struct membership_record *mrec_create(const struct in6_addr *mca, int mode) {
	struct membership_record *mrec = malloc(sizeof(*mrec));

	if (mrec) {
		memset(mrec, 0, sizeof(*mrec));
		memcpy(&mrec->mca, mca, sizeof(*mca));
		mrec->filter_mode = mode;
		mrec->source_list = list_new();
		mrec->source_list->del = (void(*)(void *))msrc_free;
		mrec->source_list->cmp = (int(*)(const void *, const void*)) msrc_cmp;
		mrec->source_list->copy = (void*(*)(const void *)) msrc_getcopy;
	}

	return mrec;
}

void mrec_destroy(struct membership_record *mrec) {
	dolog(LOG_DEBUG, "Destroying membership record.\n");
	list_delete(mrec->source_list);
	free (mrec);
}

void mrec_print(struct membership_record *mrec) {
	char addr1[INET6_ADDRSTRLEN];
	struct listnode *ln;
	struct msrc *src;

	inet_ntop(AF_INET6, &mrec->mca, addr1, sizeof(addr1));
	dolog(LOG_DEBUG, "(%s, %s, {\n", addr1, mld_grec_mode(mrec->filter_mode));
	
	LIST_LOOP(mrec->source_list, src, ln) {
		inet_ntop(AF_INET6, &src->addr, addr1, sizeof(addr1));
		dolog(LOG_DEBUG, "%s %llu\n", addr1, (long unsigned int)addr1);
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
		if (IN6_ARE_ADDR_EQUAL(mca, &mrec->mca) && ((!mode) || mrec->filter_mode == mode)) {
			return mrec;
		}
	}

	return NULL;
}

int mrec_add_source(struct membership_record *mrec, const struct msrc *src) {
	struct msrc *newsrc;

	if (!mrec) return 0;

	if (list_hasmember(mrec->source_list, src)) {
			dolog(LOG_WARNING, "Address already in list.\n");
			return 0;
	}

	newsrc = msrc_getcopy(src);
	if (newsrc) {
		listnode_add(mrec->source_list, (void*)newsrc);
		return 1;
	} else {
		dolog(LOG_WARNING, "Cannot allocate IPv6 address.\n");
		return 0;
	}
}

int mrec_add_source_addr(struct membership_record *mrec, const struct in6_addr *addr) {
	struct msrc *src = msrc_create(addr, 0);
	if (!mrec_add_source(mrec, src)) {
		msrc_free(src);
		return 0;
	} else {
		return 1;
	}
}

/*struct list *list_deepcopy(struct list *source) {
	struct in6_addr *addr, *addr_copy;
	struct listnode *ln;

	struct list *target = list_new();

	if (target) {
		target->del = (void(*)(void *))mrec_del_source;
		LIST_LOOP(source, addr, ln) {
			addr_copy = in6_addr_getcopy(addr);
			if (addr_copy) {
				listnode_add(target, addr_copy);
			}
		}
	}
	return target;
}*/

void mdb_subscribe(struct list *memb_db, struct in6_addr *mca, int mode, struct list *sources) {
	struct membership_record *mrec;

#ifdef DEBUG
	char mca_str[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, mca, mca_str, sizeof(mca_str));
	dolog(LOG_DEBUG, "Adding subscription %s %s {  } () with sources:\n", mca_str, mld_grec_mode(mode));
	print_sources(sources);
#endif

	sources = list_union(sources, NULL);

	if (mode == 1) {
		mrec = mrec_find(memb_db, mca, 2);
		if (mrec) {
			dolog(LOG_DEBUG, "INCLUDE with sources but already an EXCLUDE record in place.\n");
			struct list *tmp = list_difference(mrec->source_list, sources);
			list_delete(sources);
			list_delete(mrec->source_list);
			mrec->source_list = tmp;
		} else {
			dolog(LOG_DEBUG, "INCLUDE with %u sources. No EXCLUDE in place.\n", sources->count);
			mrec = mrec_find(memb_db, mca, 1);

			if (! mrec) {
				if (sources->count) {
					mrec = mrec_create(mca, mode);
					listnode_add(memb_db, (void*) mrec);
				} else {
					dolog(LOG_DEBUG, "no sources, no group -> no change.\n");
				}
			}
			if (sources->count) {
				struct list *tmp = list_union(mrec->source_list, sources);
				list_delete(mrec->source_list);
				mrec->source_list = tmp;
			}
			list_delete(sources);
		}
	} else {	
		mrec = mrec_find(memb_db, mca, 1);
		if (mrec) {
			dolog(LOG_DEBUG, "EXCLUDE but INCLUDE record in place.\n");
			mrec->filter_mode = 2;
			struct list *tmp = list_difference(mrec->source_list, sources);
			list_delete(sources);
			list_delete(mrec->source_list);
			mrec->source_list = tmp;
		} else {
			dolog(LOG_DEBUG, "EXCLUDE without INCLUDE record in place.\n");
			mrec = mrec_find(memb_db, mca, 2);
			if (! mrec) {
				dolog(LOG_DEBUG, "Creating new EXCLUDE record.\n");
				mrec = mrec_create(mca, mode);
				list_delete(mrec->source_list);
				mrec->source_list = sources;
				listnode_add(memb_db, (void*) mrec);
			} else {
				dolog(LOG_DEBUG, "Creating intersection of old and new EXCLUDE records.\n");
				struct list *tmp = list_intersect(mrec->source_list, sources);
				list_delete(sources);
				list_delete(mrec->source_list);
				mrec->source_list = tmp;
			}
		}
	}  
}

