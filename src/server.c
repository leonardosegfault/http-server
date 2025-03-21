#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "socket.h"
#include "server.h"

#define RECV_LENGTH (int) 0x40000 // 256 kb
#define QUEUE_LENGTH 10

static int queue = -1;
static void (*http_request_handler) (int, http_request_t*);

static void handle_server_event(int server_fd, struct epoll_event ev) {
    while (1) {
        struct sockaddr_in addr;
        int addrlen = sizeof(addr);
        int client_fd = socket_server_accept(server_fd, &addr, &addrlen);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            perror("[SERVER] failed to accept client");
            return;
        }

        struct epoll_event c_ev;
        c_ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        c_ev.data.fd = client_fd;

        int ctl_res = epoll_ctl(queue, EPOLL_CTL_ADD, client_fd, &c_ev);
        if (ctl_res != 0) {
            printf("ctl error : %d %d\n", ctl_res, EBUSY);
        }
    }
}

static void handle_client_event(struct epoll_event ev) {
    int done = 0;
    int bytes_available = RECV_LENGTH;
    char *buffer = malloc(bytes_available);
    memset(buffer, 0, bytes_available);

    int total_recv = 0;
    while (done == 0) {
        char temp_buf[RECV_LENGTH];
        int result = socket_server_recv(ev.data.fd, temp_buf, sizeof(temp_buf),
                                        MSG_DONTWAIT);

        if (result == 0) {
            done = 1;
            break;
        } else if (result == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // TODO: some better non-blocking
                done = 1;
                break;
            } else {
                printf("%d\n", errno);
                perror("[SERVER] failed to recv data");

                free(buffer);
                socket_close(ev.data.fd);
                // TODO: better error handling
            }
        }

        total_recv += result;
        bytes_available -= result;

        if (bytes_available <= 0) {
            buffer = realloc(buffer, total_recv + result);
            if (buffer == NULL) {
                printf("[SERVER] failed to realloc buffer");
                exit(1);
            }

            memset(buffer + total_recv, 0, result);
            bytes_available += result;
        }

        strncat(buffer, temp_buf, bytes_available);
    }

    if (!done)
        return;

    http_request_t request;
    http_response_t response = {0};
    int parse_err = http_parse(buffer, total_recv, &request);
    if (parse_err != 0) {
        char message[100] = {0};
        char response_buffer[256] = {0};
        int response_buffer_len = 0;

        if (parse_err == HTTP_INVALID_METHOD)
            strcat(message, "<h1>Invalid request</h1><p>The requested method is not valid.</p>");
        else if (parse_err == HTTP_INVALID_URI)
            strcat(message, "<h1>Invalid request</h1><p>The requested URI is not valid.</p>");
        else
            strcat(message, "<h1>Invalid request</h1><p>The could not be satisfied.</p>");

        response.body = message;
        response.body_len = sizeof(message);
        response.status_code = 400;
        response.status_text = "Bad Request";

        http_add_header(&response, "Content-Type", "text/html");
        http_create_response(response_buffer, response, 0);

        response_buffer_len = strlen(response_buffer);
        socket_server_send(ev.data.fd, response_buffer, response_buffer_len);
        socket_close(ev.data.fd);
    } else {
        (*http_request_handler)(ev.data.fd, &request);
    }

    free(buffer);
};

int server_create(int *server, struct sockaddr_in *arg, struct in_addr address,
                unsigned short port) {
    if (queue == -1)
        queue = epoll_create1(0);

    arg->sin_family = AF_INET;
    arg->sin_addr = address;
    arg->sin_port = htons(port);

    int server_created = socket_create_server(server, arg);
    if (server_created != 0)
        return server_created;

    int listening = socket_server_listen(server);
    if (listening != 0)
        return listening;

    int flags = fcntl(*server, F_GETFL);
    if (fcntl(*server, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("[SERVER] failed to set server's fd to non-blocking");
        return -1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = *server;

    int ctl_res = epoll_ctl(queue, EPOLL_CTL_ADD, *server, &ev);
    if (ctl_res != 0) {
        perror("[SERVER] failed to add the server to epoll queue");

        return -1;
    }

    return 0;
}

void server_start_event_loop(int fd) {
    printf("[SERVER] event loop started.\n");

    while (1) {
        struct epoll_event events[QUEUE_LENGTH];
        int amount = epoll_wait(queue, events, QUEUE_LENGTH, -1);
        if (amount == -1) {
            perror("[SERVER] failed to wait epoll");
            break;
        }

        for (int i = 0; i < amount; i++) {
            if (events[i].data.fd == fd)
                handle_server_event(fd, events[i]);
            else
                handle_client_event(events[i]);
        }
    }
}

void server_bind_on_request(void (*handler) (int, http_request_t*)) {
    http_request_handler = handler;
}

int server_send_file(int client_fd, FILE *file) {
    int read = -1;
    while (read != 0) {
        char buffer[1024];
        read = fread(buffer, sizeof(char), sizeof(buffer), file);
        if (read == -1)
            return -1;

        socket_server_send(client_fd, buffer, read);
    }

    return 0;
}