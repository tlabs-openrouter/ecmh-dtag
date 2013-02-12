#ifndef __MCAST_CLIENT_H
#define __MCAST_CLIENT_H 

//#include "linklist.h"

struct list;
struct in6_addr;
struct sockaddr;

int mcast_socket_create6(void);
int mcast_join_group(int socket, int ifindex, struct sockaddr *group, socklen_t grouplen);
int mcast_set_filter(int socket, int ifindex, struct in6_addr *mca, int filter_mode, struct list *src_list);

#endif /* __MCAST_CLIENT_H */
