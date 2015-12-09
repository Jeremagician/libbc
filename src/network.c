#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>

#include "common.h"
#include "network.h"

/* get port (IPv4 or IPv6) */
in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return (((struct sockaddr_in*)sa)->sin_port);
    return (((struct sockaddr_in6*)sa)->sin6_port);
}

/* return the sockaddr_in depending on current
 * family used (IPv4, v6) */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char *straddr(struct sockaddr *sa)
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
int start_server(int *fd, char *host, char *port, struct sockaddr *addr)
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

int connect_server(int *fd, char *host, char *port, struct sockaddr *addr)
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
