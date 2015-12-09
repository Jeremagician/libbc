#ifndef BC_H_
#define BC_H_

#include <sys/socket.h>
#include <sys/time.h>

/* bc_group comes as an opaque type
 * it can be allocated using the bc_group_struct_size function
 * - On the stack using alloca
 * - On the heap using malloc
 *
 * then call bc_init or bc_join to init the structure */
struct bc_group;

/* bc_group_struct_size:
 * Returns struct bc_group size */
size_t bc_group_struct_size(void);

enum bc_event_type
{
	BC_JOIN,
	BC_DELIVER,
	BC_LEAVE,
	BC_DEAD
};

/* Each event identifies the origin
 * with addr, and the local timestamp.
 *
 * If the event is deliver, the user
 * MUST read len bytes from the group
 * communication channel. */

struct bc_event
{
	int type;
	struct sockaddr addr;
	struct timeval localstamp;
	size_t len;
};

/* bc_init:
 * Init a new broadcast group with current node as sole member.
 * Amonst other initial configuration, this will start a server listening
 * on the given target.
 *
 * Target format is the following : <hostname, ip>[:<port, protocol>]
 * If no port is provided, the default port will be used
 *
 * Returns values:
 * 0 on success
 * < 0 if the error is a getaddrinfo error
 * 1 if system error
 *
 * One should use either strerror or gai_strerror to have localized
 * error messages. For gai error, use the absolute value of the returned integer.
 *
 * If successful, the grp structure will be in a valid state
 */
int bc_init(char *self_host, char *self_port,
			struct bc_group *grp, int chan[2]);

/* bc_join:
 * Join an existing broadcast group on hostname and port.
 * This can be done on an abitrary node within the broadcast group.
 * It will start a server listening on the self_host
 *
 * Returns 0 if the operation is successful, -1 otherwise
 * If successful, the grp structure will be in a valid state
 * Returns values:
 * 0 on success
 * < 0 if the error is a getaddrinfo error
 * 1 if system error
 *
 * One should use either strerror or gai_strerror to have localized
 * error messages. For gai error, use the absolute value of the returned integer.
 *
 * If successful, the grp structure will be in a valid state
 */
int bc_join(char *target_host, char *target_port,
			char *self_host, char *self_port,
			struct bc_group *grp, int chan[2]);

/* bc_leave:
 * Leave the given broadcast group, all resources are released
 */
void bc_leave(struct bc_group *grp);

/* bc_poll:
 * Poll the broadcast group for events.
 *
 * Return value:
 * -1 if an error occured (see errno)
 * 0 time limit expires without event caughts
 * 1 if an event has been successful polled in the ev variable
 */
int bc_poll(struct bc_group *grp, struct bc_event *ev, int timeout);

/* bc_send:
 * The current state of the write buffer is sent as one message to the group
 *
 * Return value:
 * 0 if the send is successful
 * -1 if an error occured
 */
int bc_send(struct bc_group *grp);

/* bc_update:
 * Heartbeat handling. This function is automatically used by send and poll
 * but must be used at least once every heartbeat timeout
 */
void bc_update(struct bc_group *grp);

#endif /* BC_H_ */
