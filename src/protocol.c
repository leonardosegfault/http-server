#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "socket.h"
#include "protocol.h"

#define MAX_URI_LENGTH (int) 0x400

typedef enum {
    STATE_METHOD = 0,
    STATE_URI = 1,
    STATE_VERSION = 2,
    STATE_HEADERS = 3
} STATE;

int http_get_method(char *data) {
    if (!strcmp(data, "OPTIONS")) {
        return M_OPTIONS;
    } else if (!strcmp(data, "GET")) {
        return M_GET;
    } else if (!strcmp(data, "HEAD")) {
        return M_HEAD;
    } else if (!strcmp(data, "POST")) {
        return M_POST;
    } else if (!strcmp(data, "PUT")) {
        return M_PUT;
    } else if (!strcmp(data, "DELETE")) {
        return M_DELETE;
    } else if (!strcmp(data, "TRACE")) {
        return M_TRACE;
    } else if (!strcmp(data, "CONNECT")) {
        return M_CONNECT;
    }

    return -1;
}

int http_parse(char *data, int data_len, http_request_t *dest) {
    STATE state = STATE_METHOD;
    http_request_t request = {0};

    for (int i = 0; i < data_len; i++) {
        char c = data[i];
        if (c == '\r' && data[i + 1] == '\n') {
            i += 2;
            continue;
        }

        switch (state) {
            case STATE_METHOD:
                int method;
                char m_buf[8] = {0};

                for (int j = 0; j < sizeof(m_buf); j++) {
                    if (!(c >= 'A' && c <= 'Z'))
                        break;

                    strncat(m_buf, &c, 1);
                    c = data[++i];
                }

                state = STATE_URI;
                method = http_get_method(m_buf);
                if (method == -1)
                    return HTTP_INVALID_METHOD;

                break;
            
            case STATE_URI:
                memset(request.uri, 0, sizeof(request.uri));

                for (int j = 0; j < MAX_URI_LENGTH; j++) {
                    if (c == ' ' || c == '\r' || c == '\n')
                        break;

                    if (j == MAX_URI_LENGTH - 1)
                        return HTTP_INVALID_URI;

                    strncat(request.uri, &c, 1);
                    c = data[++i];
                }

                state = STATE_VERSION;

                break;

            case STATE_VERSION:
                if (strncmp(data + i, "HTTP/", 5) != 0)
                    break;

                i += 4;
                memset(request.version, 0, sizeof(request.version));

                for (int j = 0; j < sizeof(request.version); j++) {
                    c = data[++i];

                    if (c == '.' || c > '0' && c < '9') { 
                        strncat(request.version, &c, 1);
                    } else {
                        state = STATE_HEADERS;
                        break;
                    }
                }

            case STATE_HEADERS:
                http_header_t *header =
                    &request.headers[request.header_count++];
                int reading_content = 0;
                while (c != '\r') {
                    c = data[i++];
                    if (c == '\n')
                        continue;
                    if (c == ':' && !reading_content) {
                        c = data[i++];
                        reading_content = 1;
                        continue;
                    }

                    if (!reading_content) {
                        strncat(header->name, &c, 1);
                    } else {
                        strncat(header->content, &c, 1);
                    }
                }

                break;
        }
    }

    *dest = request;

    return 0;
}

void http_create_response(char *dest, http_response_t res, int no_body) {
    strcat(dest, "HTTP/1.1 ");

    char status[4] = {0};
    sprintf(status, "%d", res.status_code);
    strcat(dest, status);
    strcat(dest, " ");

    strcat(dest, res.status_text);
    strcat(dest, "\r\n");

    for (int i = 0; i < res.header_count; i++) {
        http_header_t header = res.headers[i];
        strcat(dest, header.name);
        strcat(dest, ":");
        strcat(dest, header.content);
        strcat(dest, "\r\n");
    }

    char clen[10] = {0};
    sprintf(clen, "%d", res.body_len);
    strcat(dest, "Content-Length: ");
    strcat(dest, clen);
    strcat(dest, "\n\n");

    if (no_body != 1) {
        strcat(dest, res.body);
        strcat(dest, "\r\n");
    }
}

void http_add_header(http_response_t *dest, char *name, char *content) {
    int i = dest->header_count++;
    strcat(dest->headers[i].name, name);
    strcat(dest->headers[i].content, content);
}