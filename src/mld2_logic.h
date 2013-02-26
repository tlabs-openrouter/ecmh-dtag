#ifndef __MLD2_LOGIC_H
#define __MLD2_LOGIC_H 

struct groupnode;
struct grpintnode;
struct intnode;
struct mrec;
struct msrc;

void handle_downstream_subscription(struct intnode *intn, struct mrec *mrec);
bool handle_upstream_subscription(struct intnode *intn);
int mld2_switch_to_include(struct groupnode *groupn, struct grpintnode *grpintn);
int remove_grpintn(struct groupnode *groupn, struct grpintnode *grpintn);

int expire_source(struct groupnode *groupn, struct grpintnode *grpintn, struct msrc *src, time_t now);
void expire_sources(void);

#endif /* __MLD2_LOGIC_H */
