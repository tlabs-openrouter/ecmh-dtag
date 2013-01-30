/**************************************
 ecmh - Easy Cast du Multi Hub
 by Jeroen Massar <jeroen@unfix.org>
***************************************
 $Author: fuzzel $
 $Id: grpint.c,v 1.7 2005/02/09 17:58:06 fuzzel Exp $
 $Date: 2005/02/09 17:58:06 $
**************************************/

#include "ecmh.h"

struct grpintnode *grpint_create(const struct intnode *interface)
{
	struct grpintnode *grpintn = malloc(sizeof(*grpintn));

	if (!grpintn) return NULL;

	/* Fill her in */
	grpintn->ifindex = interface->ifindex;

	/* Setup the list */
	grpintn->subscriptions = list_new();
	grpintn->subscriptions->del = (void(*)(void *))subscr_destroy;

	/* All okay */
	return grpintn;
}

void grpint_destroy(struct grpintnode *grpintn)
{
	if (!grpintn) return;

	/* Empty the subscriber list */
	list_delete_all_node(grpintn->subscriptions);

	/* Free the node */
	free(grpintn);
}

struct grpintnode *grpint_find(const struct list *list, const struct intnode *interface)
{
	struct grpintnode	*grpintn;
	struct listnode		*ln;

	LIST_LOOP(list, grpintn, ln)
	{
		if (grpintn->ifindex == interface->ifindex) return grpintn;
	}
	return NULL;
}

/*
 * grpintn	= The GroupInterface node
 * ipv6		= Source IPv6 address
 *		  ff3x::/96  : The source IPv6 address that wants to (not) receive this S<->G channel
 * mode		= MLD2_MODE_IS_INCLUDE/MLD2_MODE_IS_EXCLUDE
 */
struct subscrnode *grpint_refresh(struct grpintnode *grpintn, const struct in6_addr *from, int mode, const struct in6_addr *ipv6)
{
	struct subscrnode *subscrn;
	bool isnew = false;

	/* Find our beloved group */
	subscrn = subscr_find(grpintn->subscriptions, from, ipv6);
	if (!subscrn)
	{
		/* Create the subscr node */
		subscrn = subscr_create(from, mode, ipv6);
		isnew = true;
	}

	/* Exclude all ? -> Unsubscribe */
	if (	mode == MLD2_MODE_IS_EXCLUDE &&
		IN6_IS_ADDR_UNSPECIFIED(ipv6))
	{
		/*
		 * Don't refresh the subscription,
		 * causing it to be removed later on
		 * because of a timeout
		 */
		dolog(LOG_DEBUG, "Not refreshing subscription.\n");
		subscrn->refreshtime = 0;
		return subscrn;
	}

	/* Add the group to the list */
	if (subscrn)
	{
		if (isnew) listnode_add(grpintn->subscriptions, (void *)subscrn);
	} else {
		return NULL;
	}

	/* Mode still the same? */
	if (mode != subscrn->mode)
	{
		dolog(LOG_DEBUG, "grpint_refresh() - Mode changed from %u to %u\n", subscrn->mode, mode);
		subscrn->mode = mode;
	}

	/* Refresh it */
	subscrn->refreshtime = time(NULL);

	dolog(LOG_DEBUG, "Downstream subscriptions changed:\n");
	subscr_print(grpintn->subscriptions);

	/* All Okay */
	return subscrn;
}

