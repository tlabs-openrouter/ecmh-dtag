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
	grpintn->filter_mode = 1;
	grpintn->ifindex = interface->ifindex;

	/* Setup the list */
	grpintn->includes = list_new();
	grpintn->includes->del = (void(*)(void *))msrc_free;
	grpintn->includes->cmp = (int(*)(const void *, const void*)) msrc_cmp;
	grpintn->includes->copy	= (void*(*)(const void *)) msrc_getcopy;
	
	grpintn->excludes = list_new();
	grpintn->excludes->del = (void(*)(void *))msrc_free;
	grpintn->excludes->cmp = (int(*)(const void *, const void*)) msrc_cmp;
	grpintn->excludes->copy	= (void*(*)(const void *)) msrc_getcopy;

	/* All okay */
	return grpintn;
}

void grpint_destroy(struct grpintnode *grpintn)
{
	if (!grpintn) return;

	list_delete(grpintn->includes);
	list_delete(grpintn->excludes);

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
struct subscrnode *grpint_refresh(struct grpintnode *grpintn, const struct in6_addr *ipv6, int mode)
{
	return NULL;
}

