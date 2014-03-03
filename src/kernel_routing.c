#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/mroute6.h>
#include <string.h>

#include "kernel_routing.h"
#include "ecmh.h"
#include "linklist.h"

int mrouter6_add_route(int mrouter_s6, int vif, struct in6_addr *im6_src, struct in6_addr *im6_dst);
int mrouter6_del_route(int mrouter_s6, int vif, struct in6_addr *im6_src, struct in6_addr *im6_dst);
static int save_route(int vif, struct in6_addr *im6_src, struct in6_addr *im6_dst);
static int have_route(int vif, struct in6_addr *im6_src, struct in6_addr *im6_dst);
static int check_route(unsigned int ifindex, struct in6_addr *src, struct in6_addr *grp);

static struct list *routes;

int mrouter_init() {
    int mrouter_s6;

    routes = list_new();
    if (!routes) return -1;

    mrouter_s6 = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);

    int val = 1;
    int ret = setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_INIT, (void*)&val, sizeof(val));
    if (ret==-1) {
        list_free (routes);
        return -1;
    }

    return mrouter_s6;
}

int mrouter6_addmif(int mrouter_s6, int mif_index, int mif_flags, int pif_index) {
    struct mif6ctl mc;
    memset(&mc, 0, sizeof(mc));
    /* Assign all mif6ctl fields as appropriate */
    mc.mif6c_mifi = mif_index;
    mc.mif6c_flags = mif_flags;
    mc.mif6c_pifi = pif_index;
    mc.vifc_rate_limit = 0;
    mc.vifc_threshold = 1;

    return setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_ADD_MIF, (void *)&mc, sizeof(mc));
}

int mrouter6_delmif(int mrouter_s6, int mif_index) {
    mifi_t mifi = mif_index;
    return setsockopt(mrouter_s6, IPPROTO_IPV6, MRT6_DEL_MIF, (void *)&mifi, sizeof(mifi));
}


int mrouter6_handlesocket(int fd) {
    char buf[1024];
    struct msghdr msg;
    struct iovec   iov[1];
    struct mrt6msg *m;

    memset(&msg, '\0', sizeof(msg));
    memset(&iov, '\0', sizeof(iov));

    iov[0].iov_base = buf;
    iov[0].iov_len  = sizeof(buf);
    msg.msg_iov     = iov;
    msg.msg_iovlen  = 1;
    dolog(LOG_WARNING, "A\n");
    int res = recvmsg(fd, &msg, 0);
    dolog(LOG_WARNING, "B\n");
    struct mld_hdr* hdr = (struct mld_hdr*)msg.msg_iov->iov_base;
    m = (struct mrt6msg *)buf;

    if (hdr->mld_type == 0) {
        
        if ( /* ignore non-multicast, node- or link-local stuff */
                !IN6_IS_ADDR_MULTICAST(&m->im6_dst) ||
                IN6_IS_ADDR_MC_NODELOCAL(&m->im6_dst) ||
                IN6_IS_ADDR_MC_LINKLOCAL(&m->im6_dst)
           ) return 0;

        char src[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &m->im6_src, src, INET6_ADDRSTRLEN);

        char dst[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &m->im6_dst, dst, INET6_ADDRSTRLEN);

        int wanted_by_downstream = (m->im6_mif == 0) ? check_route(g_conf->downstream_id,&m->im6_src,&m->im6_dst) : 0;
        dolog(LOG_WARNING, "got upcall: type=%i mif=%i src=%s dst=%s check=%i\n", m->im6_msgtype, m->im6_mif, src, dst, wanted_by_downstream);
        if (wanted_by_downstream) {
            log_grp(LOG_INFO, "Adding route", &m->im6_src, &m->im6_dst);
            res = mrouter6_add_route(fd, 0, &m->im6_src, &m->im6_dst);
            if (!res) {
                if (!have_route(0, &m->im6_src, &m->im6_dst)) {
                    if (!save_route(0, &m->im6_src, &m->im6_dst)) {
                        dolog(LOG_DEBUG, "Couldn't save route.\n");
                    }
                }
            }

            if (res) {
                dolog(LOG_WARNING, "Adding route failed with setsockopt() result=%i\n", res);
            }
        }
    }
    return 0;
}

/***********************************************************************
 * check whether ifindex wants grp from src
 */
static int check_route(unsigned int ifindex, struct in6_addr *src, struct in6_addr *grp) {
    struct groupnode	*groupn;
    struct intnode		*interface;
    struct grpintnode	*grpintn;
    struct subscrnode	*subscrn;
    struct listnode		*ln, *ln2, *ln3;

    uint32_t now = 0;

    LIST_LOOP(g_conf->groups, groupn, ln) {
        dolog(LOG_DEBUG, "%s %u\n", __FILE__, __LINE__);
        if (!IN6_ARE_ADDR_EQUAL(grp, &groupn->mca))
            continue;

        LIST_LOOP(groupn->interfaces, grpintn, ln2)
        {
            dolog(LOG_DEBUG, "%s %u\n", __FILE__, __LINE__);
            if (grpintn->ifindex != ifindex)
                continue;
            if (grpintn->filter_mode == MLD2_MODE_IS_INCLUDE) {
                if (!list_isempty(grpintn->includes)) {
                    if (list_hasmember(grpintn->includes, src)) {
                        dolog(LOG_DEBUG, "%s %u\n", __FILE__, __LINE__);
                        return 1;
                    }
                } else {
                    return 0;
                }
            } else {
                now = time(NULL);
                if (grpintn->filter_timer <= now) {
                    dolog(LOG_DEBUG, "%s %u\n", __FILE__, __LINE__);
                    return 0;
                }

                if ((!list_isempty(grpintn->excludes)) && (list_hasmember(grpintn->excludes, src))) {
                    dolog(LOG_DEBUG, "%s %u\n", __FILE__, __LINE__);
                    return 0;
                }
                
                dolog(LOG_DEBUG, "%s %u\n", __FILE__, __LINE__);
                return 1;
            }
        }
    }

    return 0;
}

static int save_route(int vif, struct in6_addr *im6_src, struct in6_addr *im6_dst) {
    struct kernel_route6 *route = malloc(sizeof(struct kernel_route6));
    if (!route)
        return -1;

    route->src = *im6_src;
    route->grp = *im6_dst;
    route->parent = vif;

    return listnode_add(routes, (void*) route) != NULL;
}

static int have_route(int vif, struct in6_addr *im6_src, struct in6_addr *im6_dst) {
    struct kernel_route6*   route;
    struct listnode         *ln;

    LIST_LOOP(routes, route, ln) {
        if (
                IN6_ARE_ADDR_EQUAL(&route->src, im6_src) &&
                IN6_ARE_ADDR_EQUAL(&route->grp, im6_dst) &&
                route->parent==vif
           ) 
            return 1;
    }
    return 0;
}
int expire_routes(int fd, int downstream) {
    (void)downstream;

    struct kernel_route6*   route;
    struct listnode         *ln, *tmp;

    dolog(LOG_DEBUG, "Expiring routes...\n");

    LIST_LOOP(routes, route, ln) {
        log_grp(LOG_DEBUG, "Checking route", &route->src, &route->grp);

        if (!check_route(g_conf->downstream_id, &route->src, &route->grp)) {
            int res = mrouter6_del_route(fd, 0, &route->src, &route->grp);
            log_grp(LOG_INFO, "Deleting route", &route->src, &route->grp);
            tmp = ln;
            ln = ln->next;
            //free(tmp->data);
            list_delete_node(routes, tmp);
            if (!ln) break;
        }
    }
    return 0;
}

static int mrouter6_changeroute(int mrouter_s6, int vif, struct in6_addr *im6_src, struct in6_addr *im6_dst, int mode) {
    struct mf6cctl mc;
    memset(&mc, 0, sizeof(mc));

    mc.mf6cc_origin.sin6_family = AF_INET6;
    mc.mf6cc_origin.sin6_addr = *im6_src;

    mc.mf6cc_mcastgrp.sin6_family = AF_INET6;
    mc.mf6cc_mcastgrp.sin6_addr = *im6_dst;

    mc.mf6cc_parent = vif;

    IF_SET(1, &mc.mf6cc_ifset);

    return setsockopt(mrouter_s6, IPPROTO_IPV6, mode, (void*)&mc, sizeof(mc));
}

int mrouter6_add_route(int mrouter_s6, int vif, struct in6_addr *im6_src, struct in6_addr *im6_dst) {
    return mrouter6_changeroute(mrouter_s6, vif, im6_src, im6_dst, MRT6_ADD_MFC);
}

int mrouter6_del_route(int mrouter_s6, int vif, struct in6_addr *im6_src, struct in6_addr *im6_dst) {
    return mrouter6_changeroute(mrouter_s6, vif, im6_src, im6_dst, MRT6_DEL_MFC);
}

