#include "ecmh.h"
#include "mship_db.h"

struct conf *g_conf;

struct conf gconf;

int main (int argc, char *argv[]) {
	g_conf = &gconf;
	g_conf->verbose = true;
	dolog(LOG_WARNING, "test\n");

	struct list *mdb = list_new();
	mdb->del = (void(*)(void *))mrec_destroy;

	struct in6_addr any = { };
	struct in6_addr g1 = { };
	inet_pton(AF_INET6, "ff3e::114", &g1);
	
	struct in6_addr g2 = { };
	inet_pton(AF_INET6, "ff3e::115", &g2);
	
	struct in6_addr s1 = { };
	inet_pton(AF_INET6, "2001:db8::114", &s1);
	mdb_subscribe(mdb, &g1, 1, &s1);
	mdb_print(mdb);

	inet_pton(AF_INET6, "2001:db8::115", &s1);
	mdb_subscribe(mdb, &g1, 1, &s1);
	mdb_print(mdb);
	
	inet_pton(AF_INET6, "2001:db8::114", &s1);
	mdb_subscribe(mdb, &g1, 2, &s1);
	mdb_print(mdb);
	
	mdb_subscribe(mdb, &g1, 1, &any);
	mdb_print(mdb);
	
	mdb_subscribe(mdb, &g1, 2, &any);
	mdb_print(mdb);
	
	inet_pton(AF_INET6, "2001:db8::114", &s1);
	mdb_subscribe(mdb, &g1, 2, &s1);
	mdb_print(mdb);
	
	inet_pton(AF_INET6, "2001:db8::115", &s1);
	mdb_subscribe(mdb, &g1, 1, &s1);
	mdb_print(mdb);
	
	mdb_subscribe(mdb, &g1, 1, &any);
	mdb_print(mdb);
	
	mdb_subscribe(mdb, &g2, 2, &any);
	mdb_print(mdb);

	dolog(LOG_DEBUG, "XXXXXXXXXXXXXx\n");
	struct list *mdb2;
	mdb_deepcopy(mdb, &mdb2);

	mdb_print(mdb2);

	return 0;
}
