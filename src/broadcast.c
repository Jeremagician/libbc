#include <bc.h>
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

/* TODO(Jeremy): split this files in smaller shards
 * it's becoming TOOO BIGGG.
 * Maybe try something like:
 * - bc.c containing api function implem
 * - event.c functions handling events in the group
 * - network.c functions handling network things
 * */


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

static struct bc_group ZERO_GROUP;

size_t bc_group_struct_size(void)
{
	return sizeof(struct bc_group);
}

/* get port (IPv4 or IPv6) */
static in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);
    return (((struct sockaddr_in6*)sa)->sin6_port);
}

/* return the sockaddr_in depending on current
 * family used (IPv4, v6) */
static void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

static char *straddr(struct sockaddr *sa)
{
	static char s[INET6_ADDRSTRLEN];
	inet_ntop(sa->sa_family, get_in_addr(sa), s, sizeof s);
	return s;
}

/* starts server on host port
 * Return values:
 * 0 on success
 * < 0 on getaddrinfo error
 * 1 on system error
 *
 * On success fd value is a valid file descriptor */
static int start_server(int *fd, char *host, char *port, struct sockaddr *addr)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, rc;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0; /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	rc = getaddrinfo(host, port, &hints, &result);

	if (rc != 0)
	{
		/* We return the getaddrinfo error as a negative integer */
		return -rc;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (sfd == -1)
		{
			/* Could not create the socket for this address,
			 * try the next one */
			continue;
		}

		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;

		close(sfd);
	}

	if (rp == NULL)
		return 1;

	*addr = *rp->ai_addr;

	freeaddrinfo(result);

	/* We set non blocking socket to use polling */
	if (ioctl(sfd, FIONBIO, (char*)&rc) < 0)
		return 1;

	if (listen(sfd, MAX_LISTEN_QUEUE) != 0)
		return 1;

	*fd = sfd;

	return 0;
}

static int connect_server(int *fd, char *host, char *port, struct sockaddr *addr)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, rc;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	rc = getaddrinfo(host, port, &hints, &result);

	if (rc != 0)
	{
		/* We return the getaddrinfo error as a negative integer */
		return -rc;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (sfd == -1)
		{
			/* Could not create the socket for this address,
			 * try the next one */
			continue;
		}

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;

		close(sfd);
	}

	if (rp == NULL)
		return 1;

	*addr = *rp->ai_addr;

	freeaddrinfo(result);

	/* We set non blocking socket to use polling */
	if (ioctl(sfd, FIONBIO, (char*)&rc) < 0)
		return 1;

	*fd = sfd;

	return 0;
}

/* Note: On error handling and resource cleaning
 * Upon error we need to clean resources to avoid
 * any kind of leaks.  We use goto mechanism to
 * clean resources in an unstacking fashion.
 */
#define GOTO_FAIL(id) do {serrno = errno; goto fail_##id;} while(0)

/* Bootstrap the broadcast group
 * if target_host and target_port are NULL, create an group
 * with only youself as a member.
 *
 * Return semantic is same as bc_init and bc_join
 *
 * We use this function to avoid redundancy such as
 * group join have similar semantic than init
 */
static int bootstrap(char *target_host, char *target_port,
					 char *self_host, char *self_port,
					 struct bc_group *grp, int chan[2])
{
	int ret, serrno, err = 0;

	*grp = ZERO_GROUP;

	self_host = self_host == NULL ? DEFAULT_HOST : self_host;
	self_port = self_port == NULL ? DEFAULT_PORT : self_port;

	ret = start_server(&grp->nodes[0].fd, self_host,
					   self_port, &grp->nodes[0].addr);
	if (ret != 0)
		return ret;

	/* Below operation are error system
	 * we set err here because it's returned in case of error */
	err = 1;

	/* We set up channels using unix pipe */
	if (pipe(grp->inp) != 0)
		GOTO_FAIL(inp);

	if (pipe(grp->outp) != 0)
		GOTO_FAIL(outp);

	chan[1] = grp->inp[1];
	chan[0] = grp->outp[0];

	grp->size = 1;
	grp->pfds[0].fd = grp->nodes[0].fd;
	grp->pfds[0].events = POLLIN;

	if (target_host != NULL)
	{
		target_port = target_port == NULL ? DEFAULT_PORT : target_port;
		grp->size++;

		/* Try to join the node from the group */
		if ((ret = connect_server(&grp->nodes[1].fd, target_host, target_port,
								  &grp->nodes[1].addr)) != 0)
		{
			err = ret;
			GOTO_FAIL(connect);
		}
	}

	printf("Started server on %s:%i\n",
		   straddr(&grp->nodes[0].addr),
		   ntohs(get_in_port(&grp->nodes[0].addr)));

	return 0;

fail_connect:
	close(grp->outp[0]);
	close(grp->outp[1]);
fail_outp:
	close(grp->inp[0]);
	close(grp->inp[1]);
fail_inp:
	close(grp->nodes[0].fd);

	/* We retore errno in case of fail from close calls
	 * serrno is set by teh GOTO_FAIL macro */
	errno = serrno;
	return err;
}

int bc_init(char *self_host, char *self_port,
			struct bc_group *grp, int chan[2])
{
	assert(grp != NULL);
	assert(chan != NULL);
    return bootstrap(NULL, NULL, self_host, self_port, grp, chan);
}

int bc_join(char *target_host, char *target_port,
			char *self_host, char *self_port,
			struct bc_group *grp, int chan[2])
{
	assert(target_host != NULL && *target_host);
	assert(grp != NULL);
	assert(chan != NULL);
	return bootstrap(target_host, target_port, self_host, self_port, grp, chan);
}


void bc_leave(struct bc_group *grp)
{
	assert(grp != NULL);
	while (grp->size --> 0)
	{
		printf("Closed %s:%i\n", straddr(&grp->nodes[grp->size].addr),
			   ntohs(get_in_port(&grp->nodes[grp->size].addr)));
		close(grp->nodes[grp->size].fd);
	}
}


/* Push an event to the grp event queue
 * Return values:
 * NULL if the queue is full
 * pointer to the pushed event otherwise */
static struct bc_event *push_event(struct bc_group *grp, int type,
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

/* pop the event on top of the event queue in the ev parameter.
 * Return values:
 * -1 if the queue is empty
 * 0 otherwise */
static int pop_event(struct bc_group *grp, struct bc_event *ev)
{
	if (grp->elen <= 0)
		return -1;

	*ev = grp->equeue[grp->eidx];
	grp->elen--;
	grp->eidx = (grp->eidx + 1) % EVENT_QUEUE_CAPACITY;
	return 0;
}

static int accept_client(struct bc_group *grp)
{
	int ret;
	struct sockaddr addr;
	socklen_t alen = sizeof(struct sockaddr);

	if (grp->size >= MAX_GRP_SIZE)
	{
		/* Group is full ! */
		return -1;
	}

	ret = accept(grp->nodes[0].fd, &addr, &alen);

	if (ret < 0)
	{
		/* This in fact should not happen
		 * find an elegant way to handle the case.
		 * Maybe just returns ? */
		return -1;
	}

	grp->nodes[grp->size].fd = ret;
	grp->nodes[grp->size].addr = addr;

	return grp->size++;
}

static void handle_connects(struct bc_group *grp)
{
	int rc;

	/* Using goto here allow us to avoid code redundancy */
	goto hdl_con;

	while (rc != -1)
	{
		push_event(grp, BC_JOIN, &grp->nodes[rc].addr);
		printf("%s:%i joined.\n",
			   straddr(&grp->nodes[rc].addr),
			   ntohs(get_in_port(&grp->nodes[rc].addr)));


	hdl_con:
		rc = accept_client(grp);
	}
}

static void handle_poll(struct bc_group *grp)
{
	int i;
	for (i = 0; i < grp->size; i++)
	{
		if (grp->pfds[i].revents & POLLIN)
		{
			/* In this case we have clients trying to connect */
			if (grp->pfds[i].fd == grp->nodes[0].fd)
				handle_connects(grp);
			/* We receive data from one other node */
			else
				handle_receive(grp);
		}
	}
}

int bc_poll(struct bc_group *grp, struct bc_event *ev, int timeout)
{
	int ret;
	assert(grp != NULL);
	assert(ev != NULL);

	ret = poll(grp->pfds, grp->size, 0);

	if (ret > 0)
		handle_poll(grp);
	else if (ret == -1)
		return -1;

	if (pop_event(grp, ev) == 0)
		return 1;

	/* If we are here, this mean that no event occured on the file descriptors
	 * and the event queue is empty, we now poll with used provided timeout */

	ret = poll(grp->pfds, grp->size, timeout);

	if (ret > 0)
		handle_poll(grp);
	else
		return ret;

	/* A new event may have pushed in the last poll_handle
	 * we try to pop */
	if (pop_event(grp, ev) == 0)
		return 1;

	return 0;
}

int bc_send(struct bc_group *grp);

void bc_update(struct bc_group *grp);
