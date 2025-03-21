enum HTTP_METHOD {
    M_OPTIONS = 0,
    M_GET = 1,
    M_HEAD = 2,
    M_POST = 3,
    M_PUT = 4,
    M_DELETE = 5,
    M_TRACE = 6,
    M_CONNECT = 7
};

enum HTTP_PARSE_ERRORS {
    HTTP_INVALID_METHOD = 1,
    HTTP_INVALID_URI = 2
};

typedef struct HttpHeader {
    char name[32];
    char content[256];
} http_header_t;

typedef struct HttpRequest {
    enum HTTP_METHOD method;
    char uri[1024];
    char version[8];

    unsigned int header_count;
    http_header_t headers[16];
} http_request_t;

typedef struct HttpResponse {
    unsigned int status_code;
    char *status_text;

    char *body;
    unsigned int body_len;

    unsigned int header_count;
    http_header_t headers[16];
} http_response_t;

/**
 * Parses a HTTP request.
 * @param data Data pointer.
 * @param data_len Data length.
 * @param dest HTTP request structure.
 * @returns 0 if success or the error code.
 */
int http_parse(char *data, int data_len, http_request_t *dest);

/**
 * Encodes a HTTP response.
 * @param dest String destination.
 * @param res HTTP response.
 * @param no_body Removes the final body line. Used when a separate buffer is
 *                going to be sent.
 */
void http_create_response(char *dest, http_response_t res, int no_body);

/**
 * Adds a header to the HTTP response.
 * @param dest Response object.
 * @param name Header name.
 * @param content Header content.
 */
void http_add_header(http_response_t *dest, char *name, char *content);