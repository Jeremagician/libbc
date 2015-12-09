#include "event.h"

struct bc_event *push_event(struct bc_group *grp, int type,
								  struct sockaddr *addr)
{
	int idx;
	struct bc_event *ev;

	if (grp->elen >= EVENT_QUEUE_CAPACITY)
		return NULL;

	idx = (grp->eidx + grp->elen) % EVENT_QUEUE_CAPACITY;
	grp->elen++;

	ev = &grp->equeue[idx];
	ev->type = type;
	ev->addr = *addr;
	gettimeofday(&ev->localstamp, NULL);

	return ev;
}

int pop_event(struct bc_group *grp, struct bc_event *ev)
{
	if (grp->elen <= 0)
		return -1;

	*ev = grp->equeue[grp->eidx];
	grp->elen--;
	grp->eidx = (grp->eidx + 1) % EVENT_QUEUE_CAPACITY;
	return 0;
}
