#ifndef NETWORK_H_
#define NETWORK_H_

/* Utility functions to get port and display
 * address from a sockaddr struct */
in_port_t get_in_port(struct sockaddr *sa);
void *get_in_addr(struct sockaddr *sa);
char *straddr(struct sockaddr *sa);

/* Start or connect to a server using getaddrinfo
 * Return values
 * < 0, negative value of the getaddrinfo error
 *      use gai_strerror to retrieve the error message
 *   0, Function is successful, fd is set to a valid file descriptor
 *      and addr contains the address of the server
 * > 0, A system error occured, use strerror and errno to get the message
 */
int start_server(int *fd, char *host, char *port, struct sockaddr *addr);
int connect_server(int *fd, char *host, char *port, struct sockaddr *addr);

#endif /* NETWORK_H_ */
