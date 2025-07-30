#include "api_candidate.h"
#include "cJSON.h"
#include "candidate/candidate.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

esp_err_t web_api_get_handler_candidate(httpd_req_t *req)
{
    cJSON *candidates_array = NULL;
    if (get_all_candidates(&candidates_array) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read candidates");
        return ESP_FAIL;
    }

    char *json_str = cJSON_PrintUnformatted(candidates_array);
    cJSON_Delete(candidates_array);

    if (!json_str) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON encoding error");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);
    free(json_str);
    return ESP_OK;
}

esp_err_t web_api_get_handler_candidate_by_id(httpd_req_t *req)
{
    const char *uri = req->uri;             // e.g., "/api/candidate/123"
    const char *prefix = "/api/candidate/"; // match your route
    const char *id_str = uri + strlen(prefix); // get the "123" part

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing candidate ID");
        return ESP_FAIL;
    }

    int id = atoi(id_str);
    if (id <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid candidate ID");
        return ESP_FAIL;
    }

    ui_candidate_t candidate;
    memset(&candidate, 0, sizeof(candidate));

    if (get_candidate_by_id(id, &candidate) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Candidate not found");
        return ESP_FAIL;
    }

    cJSON *json = candidate_to_json(&candidate);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON encoding failed");
        return ESP_FAIL;
    }

    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    if (!json_str) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON print failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);

    // Clean up memory
    free(json_str);
    free(candidate.name);
    free(candidate.number);
    free(candidate.party_name);

    return ESP_OK;
}

esp_err_t web_api_post_handler_candidate(httpd_req_t *req)
{
    char content[256];
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
    cJSON *number = cJSON_GetObjectItem(json, "number");
    cJSON *role_id = cJSON_GetObjectItem(json, "role_id");
    cJSON *party_id = cJSON_GetObjectItem(json, "party_id");

    if (!cJSON_IsString(name) || !cJSON_IsString(number) ||
        !cJSON_IsNumber(role_id) || !cJSON_IsNumber(party_id)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid fields");
        return ESP_FAIL;
    }

    ui_candidate_t candidate = {
        .id = 0,
        .name = strdup(name->valuestring),
        .number = strdup(number->valuestring),
        .role_id = role_id->valueint,
        .party_id = party_id->valueint,
        .role_name = NULL,
        .party_name = NULL
    };

    if (!candidate.name || !candidate.number) {
        free(candidate.name);
        free(candidate.number);
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    int assigned_id;
    if (add_candidate(candidate, &assigned_id) != ESP_OK) {
        free(candidate.name);
        free(candidate.number);
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to add candidate");
        return ESP_FAIL;
    }

    // Use the returned ID
    candidate.id = assigned_id;

    // Build JSON response
    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "id", candidate.id);
    cJSON_AddStringToObject(res, "name", candidate.name);
    cJSON_AddStringToObject(res, "number", candidate.number);
    cJSON_AddNumberToObject(res, "role_id", candidate.role_id);
    cJSON_AddNumberToObject(res, "party_id", candidate.party_id);

    char *res_str = cJSON_PrintUnformatted(res);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, res_str, HTTPD_RESP_USE_STRLEN);

    free(res_str);
    cJSON_Delete(res);
    cJSON_Delete(json);
    free(candidate.name);
    free(candidate.number);

    return ESP_OK;
}

esp_err_t web_api_put_handler_candidate(httpd_req_t *req)
{
    const char *uri = req->uri;                   // "/api/candidate/123"
    const char *prefix = "/api/candidate/";
    const char *id_str = uri + strlen(prefix);    // "123"

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "{\"status\":\"error\",\"message\":\"Missing candidate ID\"}");
        return ESP_FAIL;
    }

    int id = atoi(id_str);

    // Read body content
    char content[512];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    content[recv_size] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "{\"status\":\"error\",\"message\":\"Invalid JSON body\"}");
        return ESP_FAIL;
    }

    ui_candidate_t candidate;
    memset(&candidate, 0, sizeof(ui_candidate_t));
    candidate.id = id;

    cJSON *name = cJSON_GetObjectItem(json, "name");
    cJSON *number = cJSON_GetObjectItem(json, "number");
    cJSON *role_id = cJSON_GetObjectItem(json, "role_id");
    cJSON *party_id = cJSON_GetObjectItem(json, "party_id");

    if (!cJSON_IsString(name) || !cJSON_IsString(number) || !cJSON_IsNumber(role_id) || !cJSON_IsNumber(party_id)) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "{\"status\":\"error\",\"message\":\"Missing or invalid candidate fields\"}");
        return ESP_FAIL;
    }

    candidate.name = name->valuestring;
    candidate.number = number->valuestring;
    candidate.role_id = role_id->valueint;
    candidate.party_id = party_id->valueint;

    if (edit_candidate_by_id(&candidate) != ESP_OK) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "{\"status\":\"error\",\"message\":\"Failed to update candidate\"}");
        return ESP_FAIL;
    }

    ui_candidate_t updated_candidate;
    if (get_candidate_by_id(id, &updated_candidate) != ESP_OK) {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "{\"status\":\"error\",\"message\":\"Updated candidate not found\"}");
        return ESP_FAIL;
    }

    cJSON *res_json = candidate_to_json(&updated_candidate);
    char *res_str = cJSON_PrintUnformatted(res_json);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, res_str, HTTPD_RESP_USE_STRLEN);

    free(res_str);
    cJSON_Delete(res_json);
    cJSON_Delete(json);

    return ESP_OK;
}

esp_err_t web_api_delete_handler_candidate(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *prefix = "/api/candidate/";
    const char *id_str = uri + strlen(prefix);

    httpd_resp_set_type(req, "application/json");

    if (*id_str == '\0') {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
            "{\"status\":\"error\",\"message\":\"Missing candidate ID\"}");
        return ESP_FAIL;
    }

    int id = atoi(id_str);
    if (id <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
            "{\"status\":\"error\",\"message\":\"Invalid candidate ID\"}");
        return ESP_FAIL;
    }

    esp_err_t result = del_candidate_by_id(id);
    if (result == ESP_OK) {
        char resp[128];
        snprintf(resp, sizeof(resp),
            "{\"status\":\"ok\",\"message\":\"Candidate with ID %d deleted successfully\"}", id);
        httpd_resp_sendstr(req, resp);
        return ESP_OK;
    } else if (result == ESP_ERR_NOT_FOUND) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND,
            "{\"status\":\"error\",\"message\":\"Candidate not found\"}");
        return ESP_FAIL;
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
            "{\"status\":\"error\",\"message\":\"Internal server error while deleting candidate\"}");
        return ESP_FAIL;
    }
}

esp_err_t web_api_post_handler_candidate_image(httpd_req_t *req)
{
    const char *prefix = "/api/candidate/image/";
    const char *number_str = req->uri + strlen(prefix);

    int candidate_number = atoi(number_str);
    if (candidate_number <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid candidate number");
        return ESP_FAIL;
    }

    size_t total_size = req->content_len;
    uint8_t *data = malloc(total_size);
    if (!data) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    size_t received = 0;
    while (received < total_size) {
        int r = httpd_req_recv(req, (char *)data + received, total_size - received);
        if (r <= 0) {
            free(data);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
            return ESP_FAIL;
        }
        received += r;
    }

    esp_err_t result = save_candidate_image_to_sd(candidate_number, data, total_size);
    free(data);

    if (result != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save .bin file");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\",\"message\":\"File saved successfully\"}");
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

httpd_uri_t uri_post_candidate_image = {
    .uri = "/api/candidate/image/*",
    .method = HTTP_POST,
    .handler = web_api_post_handler_candidate_image,
    .user_ctx = NULL
};

