/**************************************
 ecmh - Easy Cast du Multi Hub
 by Jeroen Massar <jeroen@unfix.org>
***************************************
 $Author: fuzzel $
 $Id: grpint.h,v 1.5 2005/02/09 17:58:06 fuzzel Exp $
 $Date: 2005/02/09 17:58:06 $
**************************************/

/* The node used to hold the interfaces which a group joined */
struct grpintnode
{
	unsigned int ifindex;				/* The interface */
	struct list	 *includes;				/* include list */
	struct list	 *excludes;				/* exclude list */
	struct list	 *requested;			/* requested list */
	unsigned int filter_mode;			/* rfc3810 Router Filter Mode */
	unsigned int filter_timer;			/* rfc3810 Filter Timer */
};

struct grpintnode *grpint_create(const struct intnode *interface);
void grpint_destroy(struct grpintnode *grpintn);
struct grpintnode *grpint_find(const struct list *list, const struct intnode *interface);
struct subscrnode *grpint_refresh(struct grpintnode *grpintn, const struct in6_addr *ipv6, int mode);

