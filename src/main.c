#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "socket.h"

void send_not_found_error(int client_fd) {
    char *message = "<h1>Invalid path</h1><p>The requested resource was not found.</p>";
    http_response_t response = {0};
    response.body = message;
    response.body_len = strlen(message);
    response.status_code = 404;
    response.status_text = "Not Found";

    http_add_header(&response, "Content-Type", "text/html");

    char response_buffer[256] = {0};
    http_create_response(response_buffer, response, 0);

    int response_length = strlen(response_buffer);
    socket_server_send(client_fd, response_buffer, response_length);
    socket_close(client_fd);
}

void on_request(int client_fd, http_request_t *request) {
    char path[100] = "./public";
    strcat(path, request->uri);

    if (strcmp(request->uri, "/") == 0)
        strcat(path, "/index.html");

    int size = 0;
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        send_not_found_error(client_fd);
        return;
    }

    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    if (size == -1) {
        send_not_found_error(client_fd);
        return;
    }

    http_response_t response = {0};
    response.body = "";
    response.body_len = size;
    response.status_code = 200;
    response.status_text = "OK";

    char *ext = strrchr(path, '.');
    if (strcmp(ext, ".html") == 0)
        http_add_header(&response, "Content-Type", "text/html");
    else if (strcmp(ext, ".txt") == 0)
        http_add_header(&response, "Content-Type", "text/plain");
    else
        http_add_header(&response, "Content-Type", "application/octet-stream");

    char response_buffer[256] = {0};
    http_create_response(response_buffer, response, 1);

    int response_length = strlen(response_buffer);
    socket_server_send(client_fd, response_buffer, response_length);
    server_send_file(client_fd, file);
    socket_close(client_fd);
    fclose(file);
}

int main() {
    int server;
    
    struct sockaddr_in server_info;
    struct in_addr server_address = {};
    int server_created = server_create(&server, &server_info, server_address,
                                       8080);

    if (server_created != 0)
        return 1;

    int ip = ntohl(server_info.sin_addr.s_addr);
    printf("[SERVER] listening to http://%d.%d.%d.%d:%d\n",
        ip & 0xFF,
        (ip >> 12) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 24) & 0xFF,
        ntohs(server_info.sin_port));

    server_bind_on_request(on_request);
    server_start_event_loop(server);

    return 0;
}