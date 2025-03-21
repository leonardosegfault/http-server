#include "protocol.h"

/**
 * Creates a HTTP server.
 * 
 * @param server_fd Server file descriptor destination.
 * @param address Socket's address.
 * @param port Socket's port.
 * @returns 0 for success or the error code.
 */
int server_create(int *server_fd, struct sockaddr_in *info,
                  struct in_addr address, unsigned short port);

/**
 * Starts server's event loop.
 * @param fd Server's file descriptor.
 */
void server_start_event_loop(int fd);
           
/**
 * Creates a listener for HTTP requests.
 * @param handler Event handler.
 */
void server_bind_on_request(void (*handler)
                            (int client_fd, http_request_t *request));

/**
 * Sends a file to the client.
 * @param client_fd Client's file descriptor.
 * @param file File.
 * @returns 0 on success or -1 on error.
 */
int server_send_file(int client_fd, FILE *file);