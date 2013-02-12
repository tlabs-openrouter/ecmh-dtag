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

	struct in6_addr g1 = { };
	inet_pton(AF_INET6, "ff3e::114", &g1);

	struct list *s1 = list_new();
	struct list *s2 = list_new();
	struct list *s3 = list_new();
	struct list *s4 = list_new();

	struct in6_addr sa = { };
	inet_pton(AF_INET6, "2001:db8::a", &sa);
	struct in6_addr sb = { };
	inet_pton(AF_INET6, "2001:db8::b", &sb);
	struct in6_addr sc = { };
	inet_pton(AF_INET6, "2001:db8::c", &sc);
	struct in6_addr sd = { };
	inet_pton(AF_INET6, "2001:db8::d", &sd);
	struct in6_addr se = { };
	inet_pton(AF_INET6, "2001:db8::e", &se);
	struct in6_addr sf = { };
	inet_pton(AF_INET6, "2001:db8::f", &sf);

	listnode_add(s1, (void*)&sa);
	listnode_add(s1, (void*)&sb);
	listnode_add(s1, (void*)&sc);
	listnode_add(s1, (void*)&sd);
	
	listnode_add(s2, (void*)&sb);
	listnode_add(s2, (void*)&sc);
	listnode_add(s2, (void*)&sd);
	listnode_add(s2, (void*)&se);
	
	listnode_add(s3, (void*)&sd);
	listnode_add(s3, (void*)&se);
	listnode_add(s3, (void*)&sf);

	mdb_subscribe(mdb, &g1, 2, s1);
	mdb_subscribe(mdb, &g1, 2, s2);
	mdb_subscribe(mdb, &g1, 1, s3);
	mdb_print(mdb);

	mdb_subscribe(mdb, &g1, 2, s4);
	mdb_print(mdb);

	return 0;
}
