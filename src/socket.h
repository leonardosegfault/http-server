#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/ip.h>
#include <netinet/in.h>

/**
 * Creates a TCP server.
 * @param fd Server file descriptor.
 * @param addr Server socket info.
 * @returns 0 on success or the error code.
*/
int socket_create_server(int *fd, struct sockaddr_in *addr);

/**
 * Binds and listens to the server address.
 * @param fd Server file descriptor.
 * @returns 0 on success or the error code.
*/
int socket_server_listen(int *fd);

/**
 * Accepts a connection.
 * @param fd Server file descriptor.
 * @param addr Client address destination.
 * @param addr_len Client address length.
 * @returns 0 on success or the error code.
*/
int socket_server_accept(int fd, struct sockaddr_in *addr, int *addr_len);

/**
 * Receives server data.
 * @param client_fd Client file descriptor.
 * @param buffer Buffer to be received.
 * @param buffer_len Buffer length.
 * @param flags Socket flags.
 * @returns 0 on success or the error code.
*/
int socket_server_recv(int client_fd, char *buffer, int buffer_len, int flags);

/**
 * Sends data to the client.
 * @param client_fd Client file descriptor.
 * @param buffer Buffer to be sent.
 * @param buffer_len Buffer length.
 * @returns 0 on success or -1 on error.
*/
int socket_server_send(int client_fd, char *buffer, int buffer_len);

/**
 * Closes the client's connection.
 * @param client_fd Client file descriptor.
 * @returns 0 on success or -1 on error.
*/
int socket_close(int client_fd);