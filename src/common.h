#ifndef DEFAULTS_H_
#define DEFAULTS_H_

#include <bc.h>
#include <sys/time.h>
#include <stdint.h>
#include <poll.h>

/* All defined limits below may are used
 * in statically allocated data, dynamic
 * allocation should be used in some cases */
#define DEFAULT_PORT            "4242"
#define DEFAULT_HOST            "0.0.0.0"
#define MAX_GRP_SIZE            256
#define HEARTBEAT_FREQUENCY     5000
#define MAX_PENDING_MSG         1024
#define MAX_LISTEN_QUEUE        16
#define EVENT_QUEUE_CAPACITY    128

struct bc_node
{
	int fd;
	struct sockaddr addr;
	struct timeval last_hb_sent;
	struct timeval last_hb_recv;
};

struct bc_msg_ack
{
	uint64_t id;
	int acks[MAX_GRP_SIZE];
};

struct bc_group
{
	int inp[2]; /* Input pipe */
	int outp[2]; /* Output pipe */

	int size; /* How many nodes are in the group */
	struct pollfd pfds[MAX_GRP_SIZE];
	struct bc_node nodes[MAX_GRP_SIZE];

	struct bc_event equeue[EVENT_QUEUE_CAPACITY];
	int eidx;
	int elen;

	struct bc_msg_ack pending_msg[MAX_PENDING_MSG];
	int pending_msg_len;
};

enum opcode
{
	OP_HEARTBEAT,
	OP_JOIN,
	OP_MSG,
	OP_LEAVE
};


#endif /* DEFAULTS_H_ */
