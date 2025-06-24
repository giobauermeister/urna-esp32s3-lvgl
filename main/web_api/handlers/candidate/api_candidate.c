#include "api_candidate.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Our URI handler function to be called during GET /uri request */
esp_err_t web_api_get_handler_candidate(httpd_req_t *req)
{
    const char *resp_all = "web_api_get_handler_candidate";
    httpd_resp_send(req, resp_all, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t web_api_get_handler_candidate_by_id(httpd_req_t *req)
{
    const char *uri = req->uri;             // e.g., "/api/candidate/123"
    const char *prefix = "/api/candidate/"; // same as uri_get_candidate_by_id.uri
    const char *id_str = uri + strlen(prefix); // point past the prefix

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing candidate ID");
        return ESP_FAIL;
    }

    int id = atoi(id_str);
    char resp[64];
    snprintf(resp, sizeof(resp), "Returning candidate with ID: %d", id);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* Our URI handler function to be called during POST /uri request */
esp_err_t web_api_post_handler_candidate(httpd_req_t *req)
{
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    /* Send a simple response */
    const char resp[] = "web_api_post_handler_candidate";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t web_api_put_handler_candidate(httpd_req_t *req)
{
    const char *uri = req->uri;                   // "/api/candidate/123"
    const char *prefix = "/api/candidate/";
    const char *id_str = uri + strlen(prefix);    // "123"

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing candidate ID");
        return ESP_FAIL;
    }

    int id = atoi(id_str);

    // Read body content
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    // Optionally null-terminate the received string (if it's text-based JSON)
    content[recv_size] = '\0';

    char resp[128];
    snprintf(resp, sizeof(resp),
             "Updating candidate with ID: %d, content: %.60s",
             id, content);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t web_api_delete_handler_candidate(httpd_req_t *req)
{
    const char *uri = req->uri;                   // "/api/candidate/123"
    const char *prefix = "/api/candidate/";
    const char *id_str = uri + strlen(prefix);    // "123"

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing candidate ID");
        return ESP_FAIL;
    }

    int id = atoi(id_str);

    char resp[128];
    snprintf(resp, sizeof(resp), "Deleted candidate with ID: %d", id);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t uri_get_candidate = {
    .uri      = "/api/candidate",
    .method   = HTTP_GET,
    .handler  = web_api_get_handler_candidate,
    .user_ctx = NULL
};

httpd_uri_t uri_get_candidate_by_id = {
    .uri      = "/api/candidate/*",
    .method   = HTTP_GET,
    .handler  = web_api_get_handler_candidate_by_id,
    .user_ctx = NULL
};

httpd_uri_t uri_post_candidate = {
    .uri      = "/api/candidate",
    .method   = HTTP_POST,
    .handler  = web_api_post_handler_candidate,
    .user_ctx = NULL
};

httpd_uri_t uri_put_candidate = {
    .uri      = "/api/candidate/*",
    .method   = HTTP_PUT,
    .handler  = web_api_put_handler_candidate,
    .user_ctx = NULL
};

httpd_uri_t uri_delete_candidate = {
    .uri      = "/api/candidate/*",
    .method   = HTTP_DELETE,
    .handler  = web_api_delete_handler_candidate,
    .user_ctx = NULL
};

