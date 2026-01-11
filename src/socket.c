#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>

int socket_create_server(int *fd, struct sockaddr_in *addr) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("[SOCKET] failed to create a socket");

        return sock;
    }

    *fd = sock;

    int bind_res = bind(sock, (struct sockaddr*) addr, sizeof(struct sockaddr_in));
    if (bind_res == -1) {
        perror("[SOCKET] failed to bind an address");

        return bind_res;
    }

    return 0;
}

int socket_server_listen(int *fd) {
    int result = listen(*fd, 10);
    if (result == -1) {
        perror("[SOCKET] failed to listen a port");

        return result;
    }

    return 0;
}

int socket_server_accept(int fd, struct sockaddr_in *addr, int *addr_len) {
    return accept(fd, (struct sockaddr *) addr, addr_len);
}

int socket_server_recv(int client_fd, char *buffer, int buffer_len, int flags) {
    return recv(client_fd, buffer, buffer_len, flags);
}

int socket_server_send(int client_fd, char *buffer, int buffer_len) {
    int sent = 0;
    while (sent < buffer_len) {
        int result = send(client_fd, buffer + sent, buffer_len - sent,
                          MSG_NOSIGNAL);

        if (result == -1) {
            if (errno == EINTR) continue;
            if (errno == EPIPE || errno == ECONNRESET) {
                return -1;
            }

            perror("[SOCKET] failed to send data");
            return -1;
        }

        sent += result;
    }
    
    return 0;
}

int socket_close(int client_fd) {
    return close(client_fd);
}