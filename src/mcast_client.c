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

/* (taken from glibc 2.17, slightly modified */
static int _setsourcefilter (int s, uint32_t interface, const struct sockaddr *group,
		socklen_t grouplen, uint32_t fmode, uint32_t numsrc,
		const struct sockaddr_storage *slist)
{
	/* We have to create an struct ip_msfilter object which we can pass
	   to the kernel.  */
	size_t needed = GROUP_FILTER_SIZE (numsrc);

	struct group_filter *gf;
	gf = (struct group_filter *) malloc (needed);
	if (gf == NULL)
		return -1;

	gf->gf_interface = interface;
	memcpy (&gf->gf_group, group, grouplen);
	gf->gf_fmode = fmode;
	gf->gf_numsrc = numsrc;
	memcpy (gf->gf_slist, slist, numsrc * sizeof (struct sockaddr_storage));

	int sol;
	switch (group->sa_family) {
		case AF_INET:
			sol = SOL_IP;
			break;
		case AF_INET6:
			sol = SOL_IPV6;
			break;
		default: return -1;
	}

	int result = setsockopt (s, sol, MCAST_MSFILTER, gf, needed);

	int save_errno = errno;
	free (gf);
	errno = save_errno;

	return result;
}

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
	
	int res = _setsourcefilter(socket, ifindex, 
			(struct sockaddr*) &group, sizeof(group), 
			filter_mode, src_list->count, sources);

	if (res == -1) {
		perror("setsourcefilter()");
	}

	free(sources);
	return res;
}
