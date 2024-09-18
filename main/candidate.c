#include "candidate.h"
#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"

static const char* TAG = "CANDIDATE";

esp_err_t add_candidate(const char* name, const char* number, const char* role, int party_id)
{
    ESP_LOGI(TAG, "Add candidate");
    int current_id = 1;

    FILE* f = fopen("/sd/candidates.json", "r+");
    if(f != NULL) {
        // File exists, read and find last registered id
        char line[256];
        while(fgets(line, sizeof(line), f)) {
            cJSON* candidate_obj = cJSON_Parse(line);
            if(candidate_obj) {
                int id = cJSON_GetObjectItem(candidate_obj, "id")->valueint;
                if(id > current_id) {
                    current_id = id;
                }
                cJSON_Delete(candidate_obj);
            }
        }
        current_id++;
    } else {
        // File does not exist, create new file and use id = 0
        f = fopen("/sd/candidates.json", "w");
        if(f == NULL) {
            ESP_LOGE(TAG, "Failed to open file candidates.json for writing");
            return ESP_FAIL;
        }
    }

    cJSON* candidate_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(candidate_obj, "id", current_id);
    cJSON_AddStringToObject(candidate_obj, "name", name);
    cJSON_AddStringToObject(candidate_obj, "number", number);
    cJSON_AddStringToObject(candidate_obj, "role", role);
    cJSON_AddNumberToObject(candidate_obj, "party_id", party_id);

    char* rendered_json = cJSON_PrintUnformatted(candidate_obj);
    ESP_LOGI(TAG, "%s", rendered_json);

    int write_ret = fprintf(f, "%s\n", rendered_json);

    cJSON_Delete(candidate_obj);
    free(rendered_json);
    fclose(f);

    if(write_ret < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t add_party(const char* name)
{
    ESP_LOGI(TAG, "Add party");

    int current_id = 1001;

    FILE* f = fopen("/sd/parties.json", "r+");
    if(f != NULL) {
        //File exists, read and find last registered id
        char line[256];
        while(fgets(line, sizeof(line), f)) {
            cJSON* party_obj = cJSON_Parse(line);
            if(party_obj) {
                int id = cJSON_GetObjectItem(party_obj, "id")->valueint;
                if(id > current_id) {
                    current_id = id;
                }
                cJSON_Delete(party_obj);
            }
        }
        current_id++;
    } else {
        // File does not exist, create new file and use id = 1001
        f = fopen("/sd/parties.json", "w");
        if(f == NULL) {
            ESP_LOGE(TAG, "Failed to open file parties.json for writing");
            return ESP_FAIL;
        }
    }

    cJSON* party_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(party_obj, "id", current_id);
    cJSON_AddStringToObject(party_obj, "name", name);

    char* rendered_json = cJSON_PrintUnformatted(party_obj);
    ESP_LOGI(TAG, "%s", rendered_json);

    int write_ret = fprintf(f, "%s\n", rendered_json);

    cJSON_Delete(party_obj);
    free(rendered_json);
    fclose(f);

    if(write_ret < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;

}