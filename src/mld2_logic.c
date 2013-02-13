#include "ecmh.h"

#include "mcast_client.h"

static void set_source_timers(struct list *sources, const struct list *which_list, unsigned int timer_value) {
	struct msrc *source;
	struct listnode *ln;

	LIST_LOOP(sources, source, ln) {
		if (which_list && ! list_hasmember(which_list, source)) 
			continue;
		source->timer = timer_value;
	}
}

int remove_grpintn(struct groupnode *groupn, struct grpintnode *grpintn) {
	listnode_delete(groupn->interfaces, grpintn);
	if (list_isempty(groupn->interfaces)) {
		dolog(LOG_DEBUG, "%s %u Deleting group.\n", __FILE__, __LINE__);
		listnode_delete(g_conf->groups, groupn);
		return 1;
	}
	return 0;
}


void handle_downstream_subscription(struct intnode *intn, struct mrec *mrec) {
	struct grpintnode *grpintn;
	time_t now;
	bool isnew = false;

	grpintn = groupint_get(&mrec->mca, intn, &isnew);
	if (!grpintn) {
		goto out;
	}

	dolog(LOG_DEBUG, "handle_downstream_subscription() oldmode=%s newmode=%s isnew=%u\n", mld_grec_mode(grpintn->filter_mode), mld_grec_mode(mrec->filter_mode), isnew);

	if (grpintn->filter_mode == MLD2_MODE_IS_INCLUDE) {
		if (mrec->filter_mode == MLD2_MODE_IS_INCLUDE || mrec->filter_mode == MLD2_ALLOW_NEW_SOURCES || mrec->filter_mode == MLD2_CHANGE_TO_INCLUDE) {
//			dolog(LOG_DEBUG, "Adding new sources to the includes list.\n");
			struct list *tmp = list_union(grpintn->includes, mrec->source_list);
			
			/* (B)=MALI */
			set_source_timers(tmp, mrec->source_list, time(NULL)+MALI);
			
			if (mrec->filter_mode == MLD2_CHANGE_TO_INCLUDE) {
				/* Q(MA,A-B) */
				struct list *AminusB = list_difference(grpintn->includes, mrec->source_list);
				mld_send_mquery(intn, &mrec->mca, AminusB, false);
				set_source_timers(tmp, AminusB, time(NULL)+LLQI);
				list_delete(AminusB);
			}
			list_delete(grpintn->includes);
			grpintn->includes = tmp;			

		} else if (mrec->filter_mode == MLD2_MODE_IS_EXCLUDE || mrec->filter_mode == MLD2_CHANGE_TO_EXCLUDE) {
			dolog(LOG_DEBUG, "Switching to EXCLUDE mode.\n");
			struct list *AjoinB = list_intersect(grpintn->includes, mrec->source_list);
			struct list *BminusA = list_difference(mrec->source_list, grpintn->includes);
			struct list *AminusB = list_difference(grpintn->includes, mrec->source_list);

			list_delete(grpintn->includes);
			list_delete(grpintn->excludes);

			/* Delete (A-B) */
			grpintn->includes = list_difference(AjoinB, AminusB);
			grpintn->excludes = list_difference(BminusA, AminusB);
			
			/* (B-A)=0 */
			set_source_timers(grpintn->includes, BminusA, 0);

			/* Filter Timer=MALI */
			now = time(NULL);
			grpintn->filter_timer = now+MALI;

			if (mrec->filter_mode == MLD2_CHANGE_TO_EXCLUDE) {
				/* Send Q(MA,A*B) */
				mld_send_mquery(intn, &mrec->mca, AjoinB, false);
				set_source_timers(grpintn->includes, AjoinB, now+LLQI);
			}
			
			list_delete(AjoinB);
			list_delete(BminusA);
			list_delete(AminusB);
			
			grpintn->filter_mode = MLD2_MODE_IS_EXCLUDE;

		} else if (mrec->filter_mode == MLD2_BLOCK_OLD_SOURCES) {
			dolog(LOG_DEBUG, "INCLUDE (A) + BLOCK (B) -> INCLUDE (A) + Send Q(MA,A*B)\n");

			/* Q(MA,A*B) */
			struct list *AintersectB = list_intersect(grpintn->includes, mrec->source_list);
			mld_send_mquery(intn, &mrec->mca, AintersectB, false);
			set_source_timers(grpintn->includes, AintersectB, time(NULL)+LLQI);
			list_delete(AintersectB);
		}
	} else if (grpintn->filter_mode == MLD2_MODE_IS_EXCLUDE) {
		if (mrec->filter_mode == MLD2_MODE_IS_INCLUDE || mrec->filter_mode == MLD2_ALLOW_NEW_SOURCES || mrec->filter_mode == MLD2_CHANGE_TO_INCLUDE) {
			dolog(LOG_DEBUG, "...\n");
			struct list *XplusA = list_union(grpintn->includes, mrec->source_list);
			struct list *YminusA = list_intersect(grpintn->excludes, mrec->source_list);

			/* (A)=MALI */
			set_source_timers(XplusA, NULL, time(NULL)+MALI);

			if (mrec->filter_mode == MLD2_CHANGE_TO_INCLUDE) {
				/* Q(MA,X-A) */
				struct list *XminusA = list_difference(grpintn->includes, mrec->source_list);
				mld_send_mquery(intn, &mrec->mca, XminusA, false);
				set_source_timers(XplusA, XminusA, time(NULL)+LLQI);
				list_delete(XminusA);
				/* Q(MA) */
				mld_send_query(intn, &mrec->mca, NULL, false);
				grpintn->filter_timer = time(NULL)+LLQI;
			}
			list_delete(grpintn->includes);
			list_delete(grpintn->excludes);
			grpintn->includes = XplusA;
			grpintn->excludes = YminusA;

		} else if (mrec->filter_mode == MLD2_MODE_IS_EXCLUDE || mrec->filter_mode == MLD2_CHANGE_TO_EXCLUDE) {
			dolog(LOG_DEBUG, "EXCLUDE && (MLD2_MODE_IS_EXCLUDE || MLD2_CHANGE_TO_EXCLUDE))\n");
			struct list *AminusY = list_difference(mrec->source_list, grpintn->excludes);
			struct list *YintersectA = list_intersect(grpintn->excludes, mrec->source_list);
			struct list *AminXminY = list_difference(AminusY, grpintn->includes);

			/* Delete (X-A) */
			struct list *XminusA = list_difference(grpintn->includes, mrec->source_list);
			list_remove_all(grpintn->includes, XminusA);
			list_remove_all(grpintn->excludes, XminusA);

			/* Delete (Y-A) */
			struct list *YminusA = list_difference(grpintn->excludes, mrec->source_list);
			list_remove_all(grpintn->includes, YminusA);
			list_remove_all(grpintn->excludes, YminusA);

			if (mrec->filter_mode == MLD2_MODE_IS_EXCLUDE) {
				/* (A-X-Y)=MALI */
				set_source_timers(grpintn->includes, AminXminY, time(NULL)+MALI);
				set_source_timers(grpintn->excludes, AminXminY, time(NULL)+MALI);
			} else if (mrec->filter_mode == MLD2_CHANGE_TO_EXCLUDE) {
				/* (A-X-Y)=Filter Timer*/
				set_source_timers(grpintn->includes, AminXminY, grpintn->filter_timer);
				set_source_timers(grpintn->excludes, AminXminY, grpintn->filter_timer);
				/* Q(MA,A-Y) */
				mld_send_mquery(intn, &mrec->mca, AminusY, false);
				set_source_timers(grpintn->includes, AminusY, time(NULL)+LLQI);
				set_source_timers(grpintn->excludes, AminusY, time(NULL)+LLQI);
			}
			/* Filter Timer=MALI */
			grpintn->filter_timer = time(NULL)+MALI;
			
			list_delete(YminusA);
			list_delete(AminusY);
			list_delete(YintersectA);
			list_delete(XminusA);

		} else if (mrec->filter_mode == MLD2_BLOCK_OLD_SOURCES) {
			dolog(LOG_DEBUG, "EXCLUDE (X,Y) + BLOCK (A) -> EXCLUDE (X+(A-Y),Y)\n");
			struct list *AminusY = list_difference(mrec->source_list, grpintn->excludes);
			struct list *AminXminY = list_difference(AminusY, grpintn->includes);
			list_add_all(grpintn->includes, AminusY);

			/* (A-X-Y) = Filter Timer */
			set_source_timers(grpintn->includes, AminXminY, grpintn->filter_timer);

			/* Q(MA,A-Y) */
			mld_send_mquery(intn, &mrec->mca, AminusY, false);
			set_source_timers(grpintn->includes, AminusY, time(NULL)+LLQI);
			set_source_timers(grpintn->excludes, AminusY, time(NULL)+LLQI);

			list_delete(AminusY);
			list_delete(AminXminY);
		}
	}

	dolog(LOG_DEBUG, "New grpintstate: mode=%s timer=%u\n", grpintn->filter_mode==1?"INCLUDE":"EXCLUDE", grpintn->filter_timer);
	dolog(LOG_DEBUG, "New includes:\n");
	print_sources(grpintn->includes);
	dolog(LOG_DEBUG, "New excludes:\n");
	print_sources(grpintn->excludes);

	return;

out:
		mrec_destroy(mrec);
}

bool handle_upstream_subscription(struct intnode *intn) {

	struct groupnode	*groupn;
	struct grpintnode	*grpintn;
	struct msrc			*src;
	struct list			*sources;
	struct listnode		*ln, *ln2, *ln3;

	int ifindex = g_conf->upstream_id;
	int upstream_socket = 0;
	int old_upstream_socket = 0;

	dolog(LOG_DEBUG, "%s:%u handle_upstream_subscription() on %s (socket=%i)\n", __FILE__, __LINE__, intn->name, intn->upstream_socket);
	
	old_upstream_socket = intn->upstream_socket;
	intn->upstream_socket = 0;
	
	LIST_LOOP(g_conf->groups, groupn, ln)
	{
		LIST_LOOP(groupn->interfaces, grpintn, ln2) {
			struct in6_addr *mca = &groupn->mca;
			if (grpintn->ifindex != intn->ifindex) continue;
			sources = grpintn->filter_mode==MLD2_MODE_IS_INCLUDE ? grpintn->includes : grpintn->excludes;

			dolog(LOG_DEBUG, "mode==%i\n", grpintn->filter_mode);

			if (!sources->count && grpintn->filter_mode==MLD2_MODE_IS_INCLUDE) {
				dolog(LOG_DEBUG, "mode==INCLUDE & empty filter on multicast group: \n");
				log_ip6addr(LOG_DEBUG, mca);
				continue;
			}

			dolog(LOG_DEBUG, "Setting filter on interface Nr. %i and multicast group: \n", ifindex);
			log_ip6addr(LOG_DEBUG, mca);

			if (! upstream_socket ) {
				upstream_socket = mcast_socket_create6();
				dolog(LOG_DEBUG, "Created new upstream socket %i for %s (old_upstream_socket=%i).\n", upstream_socket, intn->name, old_upstream_socket);

				intn->upstream_socket = upstream_socket;
			}

			struct sockaddr_in6 group;
			group.sin6_family = AF_INET6;
			memcpy(&group.sin6_addr, mca, sizeof(*mca));

			mcast_join_group(upstream_socket, ifindex, (struct sockaddr *)&group, sizeof(group));
			mcast_set_filter(upstream_socket, ifindex, mca, grpintn->filter_mode, sources);
		}
	}

	if (old_upstream_socket) {
		dolog(LOG_DEBUG, "%s:%u closing upstream socket %i for %s.\n", __FILE__, __LINE__, old_upstream_socket, intn->name);
		close(old_upstream_socket);
	}

	dolog(LOG_DEBUG, "%s:%u handle_upstream_subscription() exit.\n", __FILE__, __LINE__);
	return false;
}


int mld2_switch_to_include(struct groupnode *groupn, struct grpintnode *grpintn) {
	dolog(LOG_DEBUG, "%s %u grpintn switching to INCLUDE mode.\n", __FILE__, __LINE__);
	
	grpintn->filter_mode=MLD2_MODE_IS_INCLUDE;
	handle_upstream_subscription(int_find(grpintn->ifindex));

	if (list_isempty(grpintn->includes)) {
		return remove_grpintn(groupn, grpintn);
	}

	return 0;
}

