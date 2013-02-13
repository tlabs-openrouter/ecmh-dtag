#ifndef __MLD2_LOGIC_H
#define __MLD2_LOGIC_H 

struct groupnode;
struct grpintnode;
struct intnode;
struct mrec;

void handle_downstream_subscription(struct intnode *intn, struct mrec *mrec);
bool handle_upstream_subscription(struct intnode *intn);
int mld2_switch_to_include(struct groupnode *groupn, struct grpintnode *grpintn);
int remove_grpintn(struct groupnode *groupn, struct grpintnode *grpintn);

#endif /* __MLD2_LOGIC_H */
