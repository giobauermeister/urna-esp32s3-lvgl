#include "web_api.h"
#include "esp_http_server.h"
#include "handlers/candidate/api_candidate.h"
#include "handlers/party/api_party.h"
#include "handlers/role/api_role.h"
#include "handlers/voting_result/api_voting_result.h"

void register_all_routes(httpd_handle_t server) {
    httpd_register_uri_handler(server, &uri_get_candidate);
    httpd_register_uri_handler(server, &uri_get_candidate_by_id);
    httpd_register_uri_handler(server, &uri_post_candidate);
    httpd_register_uri_handler(server, &uri_put_candidate);
    httpd_register_uri_handler(server, &uri_delete_candidate);
    httpd_register_uri_handler(server, &uri_get_party);
    httpd_register_uri_handler(server, &uri_get_party_by_id);
    httpd_register_uri_handler(server, &uri_post_party);
    httpd_register_uri_handler(server, &uri_put_party);
    httpd_register_uri_handler(server, &uri_delete_party);
    httpd_register_uri_handler(server, &uri_get_role);
    httpd_register_uri_handler(server, &uri_get_role_by_id);
    httpd_register_uri_handler(server, &uri_post_role);
    httpd_register_uri_handler(server, &uri_put_role);
    httpd_register_uri_handler(server, &uri_delete_role);
    httpd_register_uri_handler(server, &uri_get_voting_result);
}

httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16;

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        register_all_routes(server);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

void stop_webserver(httpd_handle_t server)
{
    if (server) httpd_stop(server);
}


