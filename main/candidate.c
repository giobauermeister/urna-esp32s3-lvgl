#include "candidate.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>

static const char* TAG_ADD_CANDIDATE = "Candidate[ADD]";
static const char* TAG_DEL_CANDIDATE = "Candidate[DEL]";
static const char* SEARCH_CANDIDATE = "Candidate[SEARCH]";
static const char* TAG_ADD_PARTY = "Party[ADD]";
static const char* TAG_DEL_PARTY = "Party[DEL]";
static const char* TAG_CHECK_PARTY = "Party[CHECK]";
static const char* TAG_ADD_ROLE = "Role[ADD]";
static const char* TAG_DEL_ROLE = "Role[DEL]";
static const char* TAG_CHECK_ROLE = "Role[CHECK]";
static const char* TAG_DEL_FILE = "File[DEL]";

esp_err_t add_candidate(Candidate new_candidate)
{
    ESP_LOGI(TAG_ADD_CANDIDATE, "Add candidate");

    if(check_role_exists(new_candidate.role_id) != ESP_OK) {
        ESP_LOGE(TAG_ADD_CANDIDATE, "Role with id %d does not exist, cannot add candidate", new_candidate.role_id);
        return ESP_FAIL;
    }

    if(check_party_exists(new_candidate.party_id) != ESP_OK) {
        ESP_LOGE(TAG_ADD_CANDIDATE, "Party with id %d does not exist, cannot add candidate", new_candidate.party_id);
        return ESP_FAIL;
    }

    int current_id = 1;

    FILE *f = fopen("/sd/candidates.jdb", "r+");
    if(f != NULL) {
        // File exists, read and find last registered id
        char line[256];
        cJSON *candidate_obj = NULL;
        while(fgets(line, sizeof(line), f)) {
            candidate_obj = cJSON_Parse(line);
            if(candidate_obj) {
                // Try to get the "id" field and ensure it's a valid number
                cJSON *id_item = cJSON_GetObjectItem(candidate_obj, "id");
                if(cJSON_IsNumber(id_item)) {
                    int id = id_item->valueint;
                    if(id > current_id) {
                        current_id = id;
                    }
                } else {
                    ESP_LOGE(TAG_ADD_CANDIDATE, "Invalid or missing 'id' field, skipping line");
                }                
                cJSON_Delete(candidate_obj);
            }
        }
        // Keep id 1 if file exists but no candidate object in file
        if(current_id == 1 && (!candidate_obj)) current_id = 0;
        current_id++;
    } else {
        // File does not exist, create new file and use id = 1
        f = fopen("/sd/candidates.jdb", "w");
        if(f == NULL) {
            ESP_LOGE(TAG_ADD_CANDIDATE, "Failed to open file candidates.jdb for writing");
            return ESP_FAIL;
        }
    }

    cJSON *candidate_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(candidate_obj, "id", current_id);
    cJSON_AddStringToObject(candidate_obj, "name", new_candidate.name);
    cJSON_AddStringToObject(candidate_obj, "number", new_candidate.number);
    cJSON_AddNumberToObject(candidate_obj, "role", new_candidate.role_id);
    cJSON_AddNumberToObject(candidate_obj, "party_id", new_candidate.party_id);

    char* rendered_json = cJSON_PrintUnformatted(candidate_obj);
    ESP_LOGI(TAG_ADD_CANDIDATE, "%s", rendered_json);

    int write_ret = fprintf(f, "%s\n", rendered_json);

    cJSON_Delete(candidate_obj);
    free(rendered_json);
    fclose(f);

    if(write_ret < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t del_candidate_by_id(int candidate_id)
{
    ESP_LOGI(TAG_DEL_CANDIDATE, "Delete candidate by id");

    FILE *f = fopen("/sd/candidates.jdb", "r");
    if(f == NULL) {
        ESP_LOGE(TAG_DEL_CANDIDATE, "Could not open file candidates.jdb for reading");
        return ESP_FAIL;
    }

    FILE *f_temp = fopen("/sd/temp.jdb", "w");
    if(f_temp == NULL) {
        ESP_LOGE(TAG_DEL_CANDIDATE, "Could not open file temp.jdb for reading");
        fclose(f);
        return ESP_FAIL;
    }

    char line[256];
    bool candidate_found = false;

    while(fgets(line, sizeof(line), f)) {
        cJSON *candidate_obj = cJSON_Parse(line);
        if(candidate_obj == NULL) {
            ESP_LOGE(TAG_DEL_CANDIDATE, "Failed to parse JSON line");
            continue;
        }

        // Get candidate's ID from JSON object
        cJSON *id_item = cJSON_GetObjectItem(candidate_obj, "id");
        if (!cJSON_IsNumber(id_item)) {
            ESP_LOGE(TAG_DEL_CANDIDATE, "Invalid or missing 'id' field");
            cJSON_Delete(candidate_obj);
            continue;  // Skip lines without a valid ID
        }

        int id = id_item->valueint;
        if(id == candidate_id) {
            ESP_LOGI(TAG_DEL_CANDIDATE, "Candidate with id %d found and deleted", candidate_id);
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
        ESP_LOGE(TAG_DEL_CANDIDATE, "Candidate with id %d not found", candidate_id);
        remove("/sd/temp.jdb");  // Delete the temporary file since no changes were made
        return ESP_ERR_NOT_FOUND;
    }

    // Replace the original file with the updated temporary file
    remove("/sd/candidates.jdb");  // Delete the original file
    rename("/sd/temp.jdb", "/sd/candidates.jdb");  // Rename temp file to original file

    ESP_LOGI(TAG_DEL_CANDIDATE, "Candidate with id %d deleted successfully", candidate_id);

    return ESP_OK;
}

esp_err_t search_candidate(const char* candidate_number, Candidate* found_candidate) {
    ESP_LOGI(SEARCH_CANDIDATE, "Check if candidate exists: %s", candidate_number);

    memset(found_candidate, 0, sizeof(Candidate));

    FILE *f = fopen("/sd/candidates.jdb", "r");
    if(f == NULL) {
        ESP_LOGE(SEARCH_CANDIDATE, "Could not open file candidates.jdb for reading");
        return ESP_FAIL;
    }

    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        cJSON *candidate_obj = cJSON_Parse(line);
        if(candidate_obj) {
            cJSON *number_item = cJSON_GetObjectItem(candidate_obj, "number");
            if(cJSON_IsString(number_item) && strcmp(number_item->valuestring, candidate_number) == 0) {
                // Candidate is found, fill structure
                found_candidate->id = cJSON_GetObjectItem(candidate_obj, "id")->valueint;
                found_candidate->name = strdup(cJSON_GetObjectItem(candidate_obj, "name")->valuestring);
                found_candidate->number = strdup(cJSON_GetObjectItem(candidate_obj, "number")->valuestring);
                found_candidate->role_id = cJSON_GetObjectItem(candidate_obj, "role")->valueint;
                found_candidate->party_id = cJSON_GetObjectItem(candidate_obj, "party_id")->valueint;
                // found_candidate->photo_path = strdup(cJSON_GetObjectItem(candidate_obj, "photo_path")->valuestring);

                cJSON_Delete(candidate_obj);
                fclose(f);
                return ESP_OK;
            }
            cJSON_Delete(candidate_obj);
        } else {
            ESP_LOGE(SEARCH_CANDIDATE, "Failed to parse JSON line");
        }
    }
    
    fclose(f);
    return ESP_ERR_NOT_FOUND;  // Candidate not found
}

// Helper function to clear candidate data (free memory)
void clear_candidate(Candidate* candidate) {
    if (candidate->name) free(candidate->name);
    if (candidate->number) free(candidate->number);
    if (candidate->role_name) free(candidate->role_name);
    if (candidate->party_name) free(candidate->party_name);
    if (candidate->photo_path) free(candidate->photo_path);
    memset(candidate, 0, sizeof(Candidate));
}

esp_err_t add_party(Party new_party)
{
    ESP_LOGI(TAG_ADD_PARTY, "Add party");

    int current_id = 1;

    FILE *f = fopen("/sd/parties.jdb", "r+");
    if(f != NULL) {
        //File exists, read and find last registered id
        char line[256];
        cJSON *party_obj = NULL;
        while(fgets(line, sizeof(line), f)) {
            party_obj = cJSON_Parse(line);
            if(party_obj) {
                // Try to get the "id" field and ensure it's a valid number
                cJSON *id_item = cJSON_GetObjectItem(party_obj, "id");
                if(cJSON_IsNumber(id_item)) {
                    int id = id_item->valueint;
                    if(id > current_id) {
                        current_id = id;
                    }
                } else {
                    ESP_LOGE(TAG_ADD_PARTY, "Invalid or missing 'id' field, skipping line");
                }                
                cJSON_Delete(party_obj);
            }
        }
        // Keep id 1 if file exists but no party object in file
        if(current_id == 1 && (!party_obj)) current_id = 0;
        current_id++;
    } else {
        // File does not exist, create new file and use id = 1
        f = fopen("/sd/parties.jdb", "w");
        if(f == NULL) {
            ESP_LOGE(TAG_ADD_PARTY, "Failed to open file parties.jdb for writing");
            return ESP_FAIL;
        }
    }

    cJSON *party_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(party_obj, "id", current_id);
    cJSON_AddStringToObject(party_obj, "name", new_party.name);

    char* rendered_json = cJSON_PrintUnformatted(party_obj);
    ESP_LOGI(TAG_ADD_PARTY, "%s", rendered_json);

    int write_ret = fprintf(f, "%s\n", rendered_json);

    cJSON_Delete(party_obj);
    free(rendered_json);
    fclose(f);

    if(write_ret < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t del_party_by_id(int party_id)
{
    ESP_LOGI(TAG_DEL_PARTY, "Delete party by id");
    
    FILE *f = fopen("/sd/parties.jdb", "r");
    if(f == NULL) {
        ESP_LOGE(TAG_DEL_PARTY, "Could not open file parties.jdb for reading");
        return ESP_FAIL;
    }

    FILE *f_temp = fopen("/sd/temp.jdb", "w");
    if(f_temp == NULL) {
        ESP_LOGE(TAG_DEL_PARTY, "Could not open file temp.jdb for reading");
        fclose(f);
        return ESP_FAIL;
    }

    char line[256];
    bool party_found = false;

    while(fgets(line, sizeof(line), f)) {
        cJSON *party_obj = cJSON_Parse(line);
        if(party_obj == NULL) {
            ESP_LOGE(TAG_DEL_PARTY, "Failed to parse JSON line");
            continue;
        }

        // Get party's ID from JSON object
        cJSON *id_item = cJSON_GetObjectItem(party_obj, "id");
        if(!cJSON_IsNumber(id_item)) {
            ESP_LOGE(TAG_DEL_PARTY, "Invalid or missing 'id' field");
            cJSON_Delete(party_obj);
            continue; // Skip lines without a valid ID
        }

        int id = id_item->valueint;
        if(id == party_id) {
            ESP_LOGI(TAG_DEL_PARTY, "Party with id %d found and deleted", party_id);
            party_found = true;
            // Do not write this line to the temp file, effectively deleting it
        } else {
            // Write the line to the temp file if it doesn't match the candidate ID
            fprintf(f_temp, "%s", line);
        }

        cJSON_Delete(party_obj);
    }

    fclose(f);
    fclose(f_temp);

    if(!party_found) {
        ESP_LOGE(TAG_DEL_PARTY, "Party with id %d not found", party_id);
        remove("/sd/temp.jdb"); // Delete the temporary file since no changes were made
        return ESP_ERR_NOT_FOUND;
    }

    // Replace the original file with the updated temporary file
    remove("/sd/parties.jdb");
    rename("/sd/temp.jdb", "/sd/parties.jdb");

    ESP_LOGI(TAG_DEL_PARTY, "Party with id %d deleted successfully", party_id);

    return ESP_OK;
}

esp_err_t add_role(Role new_role)
{
    ESP_LOGI(TAG_ADD_ROLE, "Add role");

    int current_id = 1;

    FILE *f = fopen("/sd/roles.jdb", "r+");
    if(f != NULL) {
        //File exists, read and find last registered id
        char line[256];
        cJSON *role_obj = NULL;
        while(fgets(line, sizeof(line), f)) {
            role_obj = cJSON_Parse(line);
            if(role_obj) {
                // Try to get the "id" field and ensure it's a valid number
                cJSON *id_item = cJSON_GetObjectItem(role_obj, "id");
                if(cJSON_IsNumber(id_item)) {
                    int id = id_item->valueint;
                    if(id > current_id) {
                        current_id = id;
                    }
                } else {
                    ESP_LOGE(TAG_ADD_CANDIDATE, "Invalid or missing 'id' field, skipping line");
                }
                cJSON_Delete(role_obj);
            }
        }
        // Keep id 1 if file exists but no role object in file
        if(current_id == 1 && (!role_obj)) current_id = 0;
        current_id++;
    } else {
        // File does not exist, create new file and use id = 1
        f = fopen("/sd/roles.jdb", "w");
        if(f == NULL) {
            ESP_LOGE(TAG_ADD_ROLE, "Failed to open file parties.jdb for writing");
            return ESP_FAIL;
        }
    }

    cJSON *role_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(role_obj, "id", current_id);
    cJSON_AddStringToObject(role_obj, "name", new_role.name);

    char* rendered_json = cJSON_PrintUnformatted(role_obj);
    ESP_LOGI(TAG_ADD_ROLE, "%s", rendered_json);

    int write_ret = fprintf(f, "%s\n", rendered_json);

    cJSON_Delete(role_obj);
    free(rendered_json);
    fclose(f);

    if(write_ret < 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t del_role_by_id(int role_id)
{
    ESP_LOGI(TAG_DEL_ROLE, "Delete role by id");
    
    FILE *f = fopen("/sd/roles.jdb", "r");
    if(f == NULL) {
        ESP_LOGE(TAG_DEL_ROLE, "Could not open file roles.jdb for reading");
        return ESP_FAIL;
    }

    FILE *f_temp = fopen("/sd/temp.jdb", "w");
    if(f_temp == NULL) {
        ESP_LOGE(TAG_DEL_ROLE, "Could not open file temp.jdb for reading");
        fclose(f);
        return ESP_FAIL;
    }

    char line[256];
    bool role_found = false;

    while(fgets(line, sizeof(line), f)) {
        cJSON *role_obj = cJSON_Parse(line);
        if(role_obj == NULL) {
            ESP_LOGE(TAG_DEL_ROLE, "Failed to parse JSON line");
            continue;
        }

        // Get role's ID from JSON object
        cJSON *id_item = cJSON_GetObjectItem(role_obj, "id");
        if(!cJSON_IsNumber(id_item)) {
            ESP_LOGE(TAG_DEL_ROLE, "Invalid or missing 'id' field");
            cJSON_Delete(role_obj);
            continue; // Skip lines without a valid ID
        }

        int id = id_item->valueint;
        if(id == role_id) {
            ESP_LOGI(TAG_DEL_ROLE, "Role with id %d found and deleted", role_id);
            role_found = true;
            // Do not write this line to the temp file, effectively deleting it
        } else {
            // Write the line to the temp file if it doesn't match the candidate ID
            fprintf(f_temp, "%s", line);
        }

        cJSON_Delete(role_obj);
    }

    fclose(f);
    fclose(f_temp);

    if(!role_found) {
        ESP_LOGE(TAG_DEL_ROLE, "Role with id %d not found", role_id);
        remove("/sd/temp.jdb"); // Delete the temporary file since no changes were made
        return ESP_ERR_NOT_FOUND;
    }

    // Replace the original file with the updated temporary file
    remove("/sd/roles.jdb");
    rename("/sd/temp.jdb", "/sd/roles.jdb");

    ESP_LOGI(TAG_DEL_ROLE, "Role with id %d deleted successfully", role_id);

    return ESP_OK;
}

esp_err_t check_party_exists(int party_id)
{
    ESP_LOGI(TAG_CHECK_PARTY, "Check if party with %d exists", party_id);

    FILE *f = fopen("/sd/parties.jdb", "r");
    if(f == NULL) {
        ESP_LOGE(TAG_CHECK_PARTY, "Could not open file parties.jdb for reading");
        return ESP_FAIL;
    }

    char line[256];
    while(fgets(line, sizeof(line), f)) {
        cJSON *party_obj = cJSON_Parse(line);
        if(party_obj == NULL) {
            ESP_LOGE(TAG_CHECK_PARTY, "Failed to parse JSON line");
            continue;
        }

        int id = cJSON_GetObjectItem(party_obj, "id")->valueint;
        if(id == party_id) {
            ESP_LOGI(TAG_CHECK_PARTY, "Party with id %d found", party_id);
            cJSON_Delete(party_obj);
            fclose(f);
            return ESP_OK;
        }

        cJSON_Delete(party_obj);
    }

    fclose(f);
    ESP_LOGE(TAG_CHECK_PARTY, "Party with id %d not found", party_id);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t check_role_exists(int role_id)
{
    ESP_LOGI(TAG_CHECK_ROLE, "Check if role with id %d exists", role_id);

    FILE *f = fopen("/sd/roles.jdb", "r");
    if(f == NULL) {
        ESP_LOGE(TAG_CHECK_ROLE, "Could not open file roles.jdb for reading");
        return ESP_FAIL;
    }

    char line[256];
    while(fgets(line, sizeof(line), f)) {
        cJSON *role_obj = cJSON_Parse(line);
        if(role_obj == NULL) {
            ESP_LOGE(TAG_CHECK_ROLE, "Failed to parse JSON line");
            continue;
        }

        int id = cJSON_GetObjectItem(role_obj, "id")->valueint;
        if(id == role_id) {
            ESP_LOGI(TAG_CHECK_ROLE, "Role with id %d found", role_id);
            cJSON_Delete(role_obj);
            fclose(f);
            return ESP_OK;
        }

        cJSON_Delete(role_obj);
    }

    fclose(f);
    ESP_LOGE(TAG_CHECK_ROLE, "Role with id %d not found", role_id);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t del_file_content(const char *file_path)
{
    ESP_LOGI(TAG_DEL_FILE, "Clear file %s", file_path);

    FILE *f = fopen(file_path, "w");
    if(f == NULL) {
        ESP_LOGE(TAG_DEL_FILE, "Failed to clear file %s", file_path);
        return ESP_FAIL;
    }

    fclose(f);
    ESP_LOGI(TAG_DEL_FILE, "File %s cleared successfully.", file_path);
    return ESP_OK;
}