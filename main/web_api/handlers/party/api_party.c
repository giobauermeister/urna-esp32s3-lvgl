#include "api_party.h"
#include "cJSON.h"
#include "candidate/candidate.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

esp_err_t web_api_get_handler_party(httpd_req_t *req)
{
    cJSON *party_array = NULL;
    if (get_all_parties(&party_array) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to retrieve parties");
        return ESP_FAIL;
    }

    char *json_str = cJSON_PrintUnformatted(party_array);
    cJSON_Delete(party_array);

    if (!json_str) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON encoding error");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    free(json_str);
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
    ui_party_t party;

    if (get_party_by_id(id, &party) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Party not found");
        return ESP_FAIL;
    }

    cJSON *json = party_to_json(&party);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create JSON");
        return ESP_FAIL;
    }

    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);

    free(party.name);  // free strdup()
    free(json_str);
    return ESP_OK;
}

esp_err_t web_api_post_handler_party(httpd_req_t *req)
{
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    content[ret] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *name = cJSON_GetObjectItem(json, "name");
    if (!cJSON_IsString(name)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid 'name'");
        return ESP_FAIL;
    }

    ui_party_t new_party = {
        .id = -1,
        .name = strdup(name->valuestring)
    };

    if (!new_party.name) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    int assigned_id;
    esp_err_t result = add_party(new_party, &assigned_id);
    cJSON_Delete(json);

    if (result != ESP_OK) {
        free(new_party.name);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to add party");
        return ESP_FAIL;
    }

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "id", assigned_id);
    cJSON_AddStringToObject(res, "name", new_party.name);
    char *res_str = cJSON_PrintUnformatted(res);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, res_str, HTTPD_RESP_USE_STRLEN);

    free(res_str);
    cJSON_Delete(res);
    free(new_party.name);

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
    if (id <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid party ID");
        return ESP_FAIL;
    }

    // Read body content
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1); // room for '\0'
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    content[ret] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *name = cJSON_GetObjectItem(json, "name");
    if (!cJSON_IsString(name)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid 'name' field");
        return ESP_FAIL;
    }

    ui_party_t updated_party = {
        .id = id,
        .name = strdup(name->valuestring)
    };

    if (!updated_party.name) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    esp_err_t result = edit_party_by_id(&updated_party);

    if (result == ESP_ERR_NOT_FOUND) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Party not found");
        return ESP_FAIL;
    } else if (result != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to update party");
        return ESP_FAIL;
    }

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "id", id);
    cJSON_AddStringToObject(res, "name", updated_party.name);

    char *res_str = cJSON_PrintUnformatted(res);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, res_str, HTTPD_RESP_USE_STRLEN);

    // Free after response is sent
    free(res_str);
    cJSON_Delete(res);
    free(updated_party.name);
    cJSON_Delete(json);

    return ESP_OK;
}

esp_err_t web_api_delete_handler_party(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *prefix = "/api/party/";
    const char *id_str = uri + strlen(prefix);

    httpd_resp_set_type(req, "application/json");

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
            "{\"status\":\"error\",\"message\":\"Missing party ID\"}");
        return ESP_FAIL;
    }

    int id = atoi(id_str);
    if (id <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
            "{\"status\":\"error\",\"message\":\"Invalid party ID\"}");
        return ESP_FAIL;
    }

    esp_err_t result = del_party_by_id(id);
    if (result == ESP_OK) {
        char resp[128];
        snprintf(resp, sizeof(resp),
            "{\"status\":\"ok\",\"message\":\"Party with ID %d deleted successfully\"}", id);
        httpd_resp_sendstr(req, resp);
        return ESP_OK;
    } else if (result == ESP_ERR_NOT_FOUND) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND,
            "{\"status\":\"error\",\"message\":\"Party not found\"}");
        return ESP_FAIL;
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
            "{\"status\":\"error\",\"message\":\"Internal server error while deleting party\"}");
        return ESP_FAIL;
    }
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

