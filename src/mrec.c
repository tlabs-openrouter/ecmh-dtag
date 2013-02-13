#include <malloc.h>
#include <string.h>

/* for the log constants */
#include <syslog.h>
#include <arpa/inet.h>
#include "common.h"

#include "mld2.h"
#include "mrec.h"
#include "msrc.h"

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

struct mrec *mrec_create(const struct in6_addr *mca, int mode) {
	struct mrec *mrec = malloc(sizeof(*mrec));

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

void mrec_destroy(struct mrec *mrec) {
	dolog(LOG_DEBUG, "Destroying membership record.\n");
	list_delete(mrec->source_list);
	free (mrec);
}

void mrec_print(struct mrec *mrec) {
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

struct mrec *mrec_find(const struct list *memb_db, const struct in6_addr *mca, int mode) {
	struct listnode *ln;
	struct mrec *mrec;

	LIST_LOOP(memb_db, mrec, ln) {
		if (IN6_ARE_ADDR_EQUAL(mca, &mrec->mca) && ((!mode) || mrec->filter_mode == mode)) {
			return mrec;
		}
	}

	return NULL;
}

int mrec_add_source(struct mrec *mrec, const struct msrc *src) {
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

int mrec_add_source_addr(struct mrec *mrec, const struct in6_addr *addr) {
	struct msrc *src = msrc_create(addr, 0);
	if (!mrec_add_source(mrec, src)) {
		msrc_free(src);
		return 0;
	} else {
		return 1;
	}
}

