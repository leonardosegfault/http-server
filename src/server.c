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
#include "queue.h"

#define RECV_LENGTH (int) 0x40000 // 256 kb
#define QUEUE_LENGTH 100

typedef struct ProcessingClient {
    int sent;
    int received;
    struct epoll_event* event;
} processing_client_t;

static int queue = -1;
static linked_list_t client_list = {0};
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

static int handle_client_event(processing_client_t* client) {
    struct epoll_event ev = *client->event;

    int total_recv = 0;
    char buffer[RECV_LENGTH] = {0};
    while (1) {
        int nbytes = socket_server_recv(ev.data.fd, buffer,
                                        sizeof(buffer) - total_recv,
                                        MSG_DONTWAIT);

        if (nbytes == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("[SERVER] failed to recv data");
                socket_close(ev.data.fd);

                return 1;
            }
        }
        
        total_recv += nbytes;
        if (total_recv == sizeof(buffer)) {
            return 1;
        }
    }

    http_request_t request;
    http_response_t response = {0};
    int parse_err = http_parse(buffer, total_recv, &request);
    if (parse_err != 0) {
        char message[100] = {0};
        char response_buffer[256] = {0};
        int response_buffer_len = 0;

        if (parse_err == HTTP_INVALID_METHOD) {
            strcat(message, "The requested method is not valid");
        } else if (parse_err == HTTP_INVALID_URI) {
            strcat(message, "The requested URI is not valid");
        } else {
            strcat(message, "Unknown error");
        }

        response.body = message;
        response.body_len = sizeof(message);
        response.status_code = 400;
        response.status_text = "Bad Request";

        http_add_header(&response, "Content-Type", "text/plain");
        http_create_response(response_buffer, response, 0);

        response_buffer_len = strlen(response_buffer);
        socket_server_send(ev.data.fd, response_buffer, response_buffer_len);
        socket_close(ev.data.fd);
    } else {
        (*http_request_handler)(ev.data.fd, &request);
    }

    return 0;
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

        struct epoll_event ev;
        for (int i = 0; i < amount; i++) {
            ev = events[i];
            if (ev.data.fd == fd) {
                handle_server_event(fd, ev);
            } else {
                processing_client_t* pclient = malloc(sizeof(processing_client_t));
                pclient->sent = 0;
                pclient->received = 0;

                struct epoll_event* ev_data = malloc(sizeof(struct epoll_event));
                *ev_data = ev;
                pclient->event = ev_data;

                linked_list_item_t* client_data = ll_new_item(pclient);
                ll_insert(&client_list, client_data);
            }
        }

        linked_list_item_t* curr_client = client_list.first;
        while (curr_client != NULL) {
            processing_client_t* pclient = curr_client->value;
            if (pclient == NULL) {
                curr_client = curr_client->next;
                ll_remove(&client_list, curr_client);
                continue;
            }

            int client_res = handle_client_event(pclient);
            if (client_res == 0) {
                linked_list_item_t* next = curr_client->next;
                // TODO: validate if the client data was fully sent
                free(pclient->event);
                free(pclient);
                ll_remove(&client_list, curr_client);
                curr_client = next;
            }
        }
    }
}

void server_bind_on_request(void (*handler) (int, http_request_t*)) {
    http_request_handler = handler;
}

int server_send_file(int client_fd, FILE *file) {
    int nbytes = -1;
    while (nbytes != 0) {
        char buffer[1024];
        nbytes = fread(buffer, sizeof(char), sizeof(buffer), file);
        if (nbytes < 1) {
            return nbytes;
        }

        socket_server_send(client_fd, buffer, nbytes);
    }

    return 0;
}