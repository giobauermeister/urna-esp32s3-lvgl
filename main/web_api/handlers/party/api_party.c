#include "api_party.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

esp_err_t web_api_get_handler_party(httpd_req_t *req)
{
    const char resp[] = "web_api_get_handler_party";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t web_api_get_handler_party_by_id(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *prefix = "/api/party/";
    const char *id_str = uri + strlen(prefix);

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing party ID");
        return ESP_FAIL;
    }

    int id = atoi(id_str);
    char resp[64];
    snprintf(resp, sizeof(resp), "Returning party with ID: %d", id);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t web_api_post_handler_party(httpd_req_t *req)
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
    const char resp[] = "web_api_post_handler_party";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t web_api_put_handler_party(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *prefix = "/api/party/";
    const char *id_str = uri + strlen(prefix);

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing party ID");
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
             "Updating party ID: %d, content: %.60s",
             id, content);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t web_api_delete_handler_party(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *prefix = "/api/party/";
    const char *id_str = uri + strlen(prefix);

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing party ID");
        return ESP_FAIL;
    }

    int id = atoi(id_str);

    char resp[128];
    snprintf(resp, sizeof(resp), "Deleted party with ID: %d", id);
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t uri_get_party = {
    .uri      = "/api/party",
    .method   = HTTP_GET,
    .handler  = web_api_get_handler_party,
    .user_ctx = NULL
};

httpd_uri_t uri_get_party_by_id = {
    .uri      = "/api/party/*",
    .method   = HTTP_GET,
    .handler  = web_api_get_handler_party_by_id,
    .user_ctx = NULL
};

httpd_uri_t uri_post_party = {
    .uri      = "/api/party",
    .method   = HTTP_POST,
    .handler  = web_api_post_handler_party,
    .user_ctx = NULL
};

httpd_uri_t uri_put_party = {
    .uri      = "/api/party/*",
    .method   = HTTP_PUT,
    .handler  = web_api_put_handler_party,
    .user_ctx = NULL
};

httpd_uri_t uri_delete_party = {
    .uri      = "/api/party/*",
    .method   = HTTP_DELETE,
    .handler  = web_api_delete_handler_party,
    .user_ctx = NULL
};

