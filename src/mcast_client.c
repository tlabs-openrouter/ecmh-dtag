#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

#include "linklist.h"
#include "mcast_client.h"
#include "mrec.h"
#include "msrc.h"

int mcast_socket_create6() {
	int fd = socket(AF_INET6, SOCK_DGRAM, 0);
	if (fd == -1) {
		perror("creating socket");
		return -1;
	}
	
	return fd;
}

int mcast_join_group(int socket, int ifindex, struct sockaddr *group, socklen_t grouplen) {
	int res;
	struct group_req req;
	req.gr_interface = ifindex;
	memcpy(&req.gr_group, group, grouplen);

	res = setsockopt(socket, IPPROTO_IPV6, MCAST_JOIN_GROUP, &req, sizeof(req));
	if (res == -1) {
		perror("setsockopt()");
		return -1;
	}

	return 0;
}

int mcast_set_filter(int socket, int ifindex, struct in6_addr *mca, int filter_mode, struct list *src_list) {
	struct listnode *ln;
	struct msrc *src;
	struct sockaddr_in6 group;
	struct sockaddr_storage *sources;
	
	printf("%s:%u mcast_set_filter() socket=%u ifindex=%u filter_mode=%s sources=%u\n", __FILE__, __LINE__, socket, ifindex, mld_grec_mode(filter_mode), src_list->count);
	if (src_list->count == 0) {
		printf("%s:%u empty source list.\n", __FILE__, __LINE__);
	}
	
	group.sin6_family=AF_INET6;
	memcpy(&group.sin6_addr, mca, sizeof(group.sin6_addr));

	if (! (sources = malloc(sizeof(struct sockaddr_storage)*src_list->count))) {
		perror("allocating sources");
		return 0;
	}

	memset(sources, '\0', sizeof(struct sockaddr_storage)*src_list->count);
	int i = 0;
	LIST_LOOP(src_list, src, ln) {
		printf("%s:%u mcast_set_filter() adding source with timer=%i\n", __FILE__, __LINE__, (unsigned int)(src->timer-time(NULL)));
		struct sockaddr_in6 *source = (struct sockaddr_in6*) &sources[i];
		source->sin6_family = AF_INET6;
		memcpy(&source->sin6_addr, &src->addr, sizeof(source->sin6_addr));
		i++;
	}
	
	int res = setsourcefilter(socket, ifindex, 
			(struct sockaddr*) &group, sizeof(group), 
			filter_mode, src_list->count, sources);

	if (res == -1) {
		perror("setsourcefilter()");
	}

	free(sources);
	return res;
}
