#include "api_voting_result.h"

/* Our URI handler function to be called during GET /uri request */
esp_err_t web_api_get_handler_voting_result(httpd_req_t *req)
{
    /* Send a simple response */
    const char resp[] = "web_api_get_handler_voting_result";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get_voting_result = {
    .uri      = "/api/voting_result",
    .method   = HTTP_GET,
    .handler  = web_api_get_handler_voting_result,
    .user_ctx = NULL
};

