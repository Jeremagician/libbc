#ifndef EVENT_H_
#define EVENT_H_

#include "common.h"

/* Push an event to the grp event queue
 * Return values:
 * NULL if the queue is full
 * pointer to the pushed event otherwise */
struct bc_event *push_event(struct bc_group *grp, int type,
							struct sockaddr *addr);

/* pop the event on top of the event queue in the ev parameter.
 * Return values:
 * -1 if the queue is empty
 * 0 otherwise */
int pop_event(struct bc_group *grp, struct bc_event *ev);

#endif /* EVENT_H_ */
