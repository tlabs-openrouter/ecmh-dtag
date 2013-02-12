#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <net/if.h>

#include "mcast_client.h"

//   struct sockaddr_in6 {
//          u_char           sin6_len;      /* length of this structure */
//          u_char           sin6_family;   /* AF_INET6                 */
//          u_int16m_t       sin6_port;     /* Transport layer port #   */
//          u_int32m_t       sin6_flowinfo; /* IPv6 flow information    */
//          struct in6_addr  sin6_addr;     /* IPv6 address             */
//   };

/*
int setsourcefilter(int s, uint32_t interface,
                         struct sockaddr *group, socklen_t grouplen,
                         uint32_t fmode, uint_t numsrc,
                         struct sockaddr_storage *slist);
*/

static void set_ipv6(char* address, struct sockaddr_in6*target) {
	target->sin6_family=AF_INET6;

	inet_pton(AF_INET6, address, &target->sin6_addr);
}

int main(int argc, char *argv[]) {
	int s = mcast_socket_create6(0);

	printf("s=%i\n", s);
	struct sockaddr_in6 ga = { 0 };
	set_ipv6("ff3e::114", &ga);

	struct sockaddr_storage sources[2];
	set_ipv6("2001:db8::a", (struct sockaddr_in6*) &sources[0]);
	set_ipv6("2001:db8:3::1", (struct sockaddr_in6*) &sources[1]);
	
	unsigned int ifindex = if_nametoindex("net0");
	printf("ifindex for net0=%u\n", ifindex);

	int res = mcast_join_group(s, ifindex, (struct sockaddr*) &ga, sizeof(ga));
	if (res == -1) {
		perror("mcast_join_group()");
		return 1;
	}
	
	res = setsourcefilter(s, ifindex, 
			(struct sockaddr*) &ga, sizeof(ga), 
			MCAST_INCLUDE, 2, sources);

	if (res == -1) {
		perror("setsourcefilter()");
		return 1;
	}

	while(1) {
		printf(".");
		sleep(1);
	}

	return 0;
}

