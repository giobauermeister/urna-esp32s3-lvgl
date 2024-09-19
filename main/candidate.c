#include "candidate.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"

static const char* TAG = "CANDIDATE";

esp_err_t add_candidate(Candidate new_candidate)
{
    ESP_LOGI(TAG, "Add candidate");
    int current_id = 1;

    FILE* f = fopen("/sd/candidates.jdb", "r+");
    if(f != NULL) {
        // File exists, read and find last registered id
        char line[256];
        cJSON* candidate_obj = NULL;
        while(fgets(line, sizeof(line), f)) {
            candidate_obj = cJSON_Parse(line);
            if(candidate_obj) {
                int id = cJSON_GetObjectItem(candidate_obj, "id")->valueint;
                if(id > current_id) {
                    current_id = id;
                }
                cJSON_Delete(candidate_obj);
            }
        }
        // Keep id 1 if file exists but no candidate object in file
        if(current_id == 1 && (!candidate_obj)) current_id = 0;
        current_id++;
    } else {
        // File does not exist, create new file and use id = 0
        f = fopen("/sd/candidates.jdb", "w");
        if(f == NULL) {
            ESP_LOGE(TAG, "Failed to open file candidates.json for writing");
            return ESP_FAIL;
        }
    }

    cJSON* candidate_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(candidate_obj, "id", current_id);
    cJSON_AddStringToObject(candidate_obj, "name", new_candidate.name);
    cJSON_AddStringToObject(candidate_obj, "number", new_candidate.number);
    cJSON_AddStringToObject(candidate_obj, "role", new_candidate.role);
    cJSON_AddNumberToObject(candidate_obj, "party_id", new_candidate.party_id);

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

esp_err_t delete_candidate_by_id(int candidate_id)
{
    ESP_LOGI(TAG, "Delete by id");

    FILE* f = fopen("/sd/candidates.jdb", "r");
    if(f == NULL) {
        ESP_LOGE(TAG, "Could not open file candidates.jdb for reading");
        return ESP_FAIL;
    }

    FILE* f_temp = fopen("/sd/temp.jdb", "w");
    if(f_temp == NULL) {
        ESP_LOGE(TAG, "Could not open file temp.jdb for reading");
        fclose(f_temp);
        return ESP_FAIL;
    }

    char line[256];
    bool candidate_found = false;

    while(fgets(line, sizeof(line), f)) {
        cJSON* candidate_obj = cJSON_Parse(line);
        if(candidate_obj == NULL) {
            ESP_LOGE(TAG, "Failed to parse JSON line");
            continue;
        }

        // Get candidate's ID from JSON object
        cJSON* id_item = cJSON_GetObjectItem(candidate_obj, "id");
        if (!cJSON_IsNumber(id_item)) {
            ESP_LOGE(TAG, "Invalid or missing 'id' field");
            cJSON_Delete(candidate_obj);
            continue;  // Skip lines without a valid ID
        }

        int id = id_item->valueint;
        if(id == candidate_id) {
            ESP_LOGI(TAG, "Candidate with id %d found and deleted", candidate_id);
            candidate_found = true;
            // Do not write this line to the temp file, effectively deleting it
        } else {
            // Write the line to the temp file if it doesn't match the candidate ID
            fprintf(f_temp, "%s", line);
        }

        cJSON_Delete(candidate_obj);
    }

    fclose(f);
    fclose(f_temp);

    if(!candidate_found) {
        ESP_LOGE(TAG, "Candidate with id %d not found", candidate_id);
        remove("/sd/temp.jdb");  // Delete the temporary file since no changes were made
        return ESP_ERR_NOT_FOUND;
    }

    // Replace the original file with the updated temporary file
    remove("/sd/candidates.jdb");  // Delete the original file
    rename("/sd/temp.jdb", "/sd/candidates.jdb");  // Rename temp file to original file

    ESP_LOGI(TAG, "Candidate with id %d deleted successfully", candidate_id);

    return ESP_OK;
}

esp_err_t add_party(Party new_party)
{
    ESP_LOGI(TAG, "Add party");

    int current_id = 1001;

    FILE* f = fopen("/sd/parties.jdb", "r+");
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
        f = fopen("/sd/parties.jdb", "w");
        if(f == NULL) {
            ESP_LOGE(TAG, "Failed to open file parties.json for writing");
            return ESP_FAIL;
        }
    }

    cJSON* party_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(party_obj, "id", current_id);
    cJSON_AddStringToObject(party_obj, "name", new_party.name);

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