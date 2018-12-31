#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <netinet/in.h>    /* Internet domain header, for struct sockaddr_in */
#include "hcq_server.h"

/*
 * Initialize a server address associated with the given port.
 */
struct sockaddr_in *init_server_addr(int port);
/*
 * Create and set up a socket for a server to listen on.
 */
int set_up_server_socket(struct sockaddr_in *self, int num_queue);
/* Accept a connection. Note that a new file descriptor is created for
 * communication with the client. The initial socket descriptor is used
 * to accept connections, but the new socket is used to communicate.
 * Return the new client's file descriptor or -1 on error.
 */
int accept_connection(int listenfd);
#endif
