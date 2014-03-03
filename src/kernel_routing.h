int mrouter_init(void);
int mrouter6_addmif(int mrouter_s6, int mif_index, int mif_flags, int pif_index);
int mrouter6_delmif(int mrouter_s6, int mif_index);
int mrouter6_handlesocket(int fd);

int expire_routes(int fd, int downstream);

struct kernel_route6 {
    struct in6_addr src;
    struct in6_addr grp;
    int parent;
    /*struct if_set ifset;
    int refcount;*/
};

