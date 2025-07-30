#include "api_role.h"
#include "cJSON.h"
#include "candidate/candidate.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

esp_err_t web_api_get_handler_role(httpd_req_t *req)
{
    cJSON *role_array = NULL;
    if (get_all_roles(&role_array) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to retrieve roles");
        return ESP_FAIL;
    }

    char *json_str = cJSON_PrintUnformatted(role_array);
    cJSON_Delete(role_array);

    if (!json_str) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON encoding error");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    free(json_str);
    return ESP_OK;
}

esp_err_t web_api_get_handler_role_by_id(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *prefix = "/api/role/";
    const char *id_str = uri + strlen(prefix);

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing role ID");
        return ESP_FAIL;
    }

    int id = atoi(id_str);
    ui_role_t role = { .id = id };

    if (get_role_by_id(id, &role) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Role not found");
        return ESP_FAIL;
    }

    cJSON *json = role_to_json(&role);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create JSON");
        return ESP_FAIL;
    }

    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);

    free(role.name);  // free strdup()
    free(json_str);
    return ESP_OK;
}

esp_err_t web_api_post_handler_role(httpd_req_t *req)
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

    content[ret] = '\0';  // Null-terminate input

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *name = cJSON_GetObjectItem(json, "name");
    cJSON *n_digits = cJSON_GetObjectItem(json, "n_digits");

    if (!cJSON_IsString(name) || !cJSON_IsNumber(n_digits)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid fields");
        return ESP_FAIL;
    }

    int digits = n_digits->valueint;
    if (digits < 2 || digits > 4) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "n_digits must be 2, 3, or 4");
        return ESP_FAIL;
    }

    ui_role_t new_role = {
        .id = -1,
        .name = strdup(name->valuestring),
        .n_digits = digits
    };

    if (!new_role.name) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    int assigned_id;
    esp_err_t result = add_role(new_role, &assigned_id);
    cJSON_Delete(json);

    if (result != ESP_OK) {
        free(new_role.name);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to add role");
        return ESP_FAIL;
    }

    // Build and return role JSON
    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "id", assigned_id);
    cJSON_AddStringToObject(res, "name", new_role.name);
    cJSON_AddNumberToObject(res, "n_digits", new_role.n_digits);

    char *res_str = cJSON_PrintUnformatted(res);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, res_str, HTTPD_RESP_USE_STRLEN);

    free(res_str);
    cJSON_Delete(res);
    free(new_role.name);

    return ESP_OK;
}

esp_err_t web_api_put_handler_role(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *prefix = "/api/role/";
    const char *id_str = uri + strlen(prefix);

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing role ID");
        return ESP_FAIL;
    }

    int id = atoi(id_str);
    if (id <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid role ID");
        return ESP_FAIL;
    }

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
    cJSON *n_digits = cJSON_GetObjectItem(json, "n_digits");

    if (!cJSON_IsString(name) || !cJSON_IsNumber(n_digits)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid fields");
        return ESP_FAIL;
    }

    if (n_digits->valueint < 2 || n_digits->valueint > 4) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "n_digits must be 2, 3, or 4");
        return ESP_FAIL;
    }

    // Make a copy of name->valuestring before freeing the JSON
    char *name_str_copy = strdup(name->valuestring);
    int n_digits_val = n_digits->valueint;

    if (!name_str_copy) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    ui_role_t role = {
        .id = id,
        .name = name_str_copy,
        .n_digits = n_digits_val
    };

    esp_err_t result = edit_role_by_id(&role);

    if (result == ESP_ERR_NOT_FOUND) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Role not found");
        return ESP_FAIL;
    } else if (result != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to update role");
        return ESP_FAIL;
    }

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "id", id);
    cJSON_AddStringToObject(res, "name", name_str_copy);
    cJSON_AddNumberToObject(res, "n_digits", n_digits_val);

    char *res_str = cJSON_PrintUnformatted(res);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, res_str, HTTPD_RESP_USE_STRLEN);

    free(res_str);
    free(role.name);
    cJSON_Delete(json);
    cJSON_Delete(res);
    return ESP_OK;
}

esp_err_t web_api_delete_handler_role(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *prefix = "/api/role/";
    const char *id_str = uri + strlen(prefix);

    httpd_resp_set_type(req, "application/json");

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
            "{\"status\":\"error\",\"message\":\"Missing role ID\"}");
        return ESP_FAIL;
    }

    int id = atoi(id_str);
    if (id <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
            "{\"status\":\"error\",\"message\":\"Invalid role ID\"}");
        return ESP_FAIL;
    }

    esp_err_t result = del_role_by_id(id);
    if (result == ESP_OK) {
        char resp[128];
        snprintf(resp, sizeof(resp),
            "{\"status\":\"ok\",\"message\":\"Role with ID %d deleted successfully\"}", id);
        httpd_resp_sendstr(req, resp);
        return ESP_OK;
    } else if (result == ESP_ERR_NOT_FOUND) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND,
            "{\"status\":\"error\",\"message\":\"Role not found\"}");
        return ESP_FAIL;
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
            "{\"status\":\"error\",\"message\":\"Internal server error while deleting role\"}");
        return ESP_FAIL;
    }
}

httpd_uri_t uri_get_role = {
    .uri      = "/api/role",
    .method   = HTTP_GET,
    .handler  = web_api_get_handler_role,
    .user_ctx = NULL
};

httpd_uri_t uri_get_role_by_id = {
    .uri      = "/api/role/*",
    .method   = HTTP_GET,
    .handler  = web_api_get_handler_role_by_id,
    .user_ctx = NULL
};

httpd_uri_t uri_post_role = {
    .uri      = "/api/role",
    .method   = HTTP_POST,
    .handler  = web_api_post_handler_role,
    .user_ctx = NULL
};

httpd_uri_t uri_put_role = {
    .uri      = "/api/role/*",
    .method   = HTTP_PUT,
    .handler  = web_api_put_handler_role,
    .user_ctx = NULL
};

httpd_uri_t uri_delete_role = {
    .uri      = "/api/role/*",
    .method   = HTTP_DELETE,
    .handler  = web_api_delete_handler_role,
    .user_ctx = NULL
};

