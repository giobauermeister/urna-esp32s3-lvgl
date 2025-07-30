#include "candidate/candidate.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdbool.h>
#include "ui/ui.h"

static const char* TAG_ADD_CANDIDATE = "Candidate[ADD]";
static const char* TAG_DEL_CANDIDATE = "Candidate[DEL]";
static const char* TAG_SEARCH_CANDIDATE = "Candidate[SEARCH]";
static const char* TAG_GET_ALL_CANDIDATE = "Candidate[GET_ALL]";
static const char* TAG_ADD_PARTY = "Party[ADD]";
static const char* TAG_DEL_PARTY = "Party[DEL]";
static const char* TAG_GET_PARTY = "Party[GET]";
static const char* TAG_CHECK_PARTY = "Party[CHECK]";
static const char* TAG_ADD_ROLE = "Role[ADD]";
static const char* TAG_DEL_ROLE = "Role[DEL]";
static const char* TAG_GET_ROLE = "Role[GET]";
static const char* TAG_GET_N_ROLES = "Role[GET_N]";
static const char* TAG_CHECK_ROLE = "Role[CHECK]";
static const char* TAG_DEL_FILE = "File[DEL]";
static const char* TAG_STORE_VOTE = "Vote[STORE]";

esp_err_t add_candidate(ui_candidate_t new_candidate, int * out_id)
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

    FILE *f = fopen(FILE_CANDIDATES, "r+");
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
        f = fopen(FILE_CANDIDATES, "w");
        if(f == NULL) {
            ESP_LOGE(TAG_ADD_CANDIDATE, "Failed to open file candidates.jsonl for writing");
            return ESP_FAIL;
        }
    }

    cJSON *candidate_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(candidate_obj, "id", current_id);
    cJSON_AddStringToObject(candidate_obj, "name", new_candidate.name);
    cJSON_AddStringToObject(candidate_obj, "number", new_candidate.number);
    cJSON_AddNumberToObject(candidate_obj, "role_id", new_candidate.role_id);
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

    if(out_id) * out_id = current_id;
    return ESP_OK;
}

esp_err_t del_candidate_by_id(int candidate_id)
{
    ESP_LOGI(TAG_DEL_CANDIDATE, "Delete candidate by id");

    FILE *f = fopen(FILE_CANDIDATES, "r");
    if(f == NULL) {
        ESP_LOGE(TAG_DEL_CANDIDATE, "Could not open file %s for reading", FILE_CANDIDATES);
        return ESP_FAIL;
    }

    FILE *f_temp = fopen(FILE_TEMP, "w");
    if(f_temp == NULL) {
        ESP_LOGE(TAG_DEL_CANDIDATE, "Could not open file %s for reading", FILE_TEMP);
        fclose(f);
        return ESP_FAIL;
    }

    char line[256];
    bool flag_candidate_found = false;

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
            flag_candidate_found = true;
            // Do not write this line to the temp file, effectively deleting it
        } else {
            // Write the line to the temp file if it doesn't match the candidate ID
            fprintf(f_temp, "%s", line);
        }

        cJSON_Delete(candidate_obj);
    }

    fclose(f);
    fclose(f_temp);

    if(!flag_candidate_found) {
        ESP_LOGE(TAG_DEL_CANDIDATE, "Candidate with id %d not found", candidate_id);
        remove(FILE_TEMP);  // Delete the temporary file since no changes were made
        return ESP_ERR_NOT_FOUND;
    }

    // Replace the original file with the updated temporary file
    remove(FILE_CANDIDATES);  // Delete the original file
    rename(FILE_TEMP, FILE_CANDIDATES);  // Rename temp file to original file

    ESP_LOGI(TAG_DEL_CANDIDATE, "Candidate with id %d deleted successfully", candidate_id);

    return ESP_OK;
}

esp_err_t search_candidate(const char * candidate_number, ui_candidate_t * found_candidate)
{
    ESP_LOGI(TAG_SEARCH_CANDIDATE, "Search candidate by number: %s", candidate_number);

    memset(found_candidate, 0, sizeof(ui_candidate_t));

    FILE *f = fopen(FILE_CANDIDATES, "r");
    if(f == NULL) {
        ESP_LOGE(TAG_SEARCH_CANDIDATE, "Could not open file %s for reading", FILE_CANDIDATES);
        return ESP_FAIL;
    }

    char line[256];
    ui_party_t found_party;

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
                found_candidate->role_id = cJSON_GetObjectItem(candidate_obj, "role_id")->valueint;
                found_candidate->party_id = cJSON_GetObjectItem(candidate_obj, "party_id")->valueint;

                // Get party by id
                if(get_party_by_id(found_candidate->party_id, &found_party) == ESP_OK) {
                    found_candidate->party_name = found_party.name;
                } else {
                    found_candidate->party_name = strdup("Sem partido");
                }

                cJSON_Delete(candidate_obj);
                fclose(f);
                return ESP_OK;
            }
            cJSON_Delete(candidate_obj);
        } else {
            ESP_LOGE(TAG_SEARCH_CANDIDATE, "Failed to parse JSON line");
        }
    }
    
    fclose(f);
    return ESP_ERR_NOT_FOUND;  // Candidate not found
}

esp_err_t get_all_candidates(cJSON **array_out)
{
    ESP_LOGI(TAG_GET_ALL_CANDIDATE, "Get all candidates");

    FILE *f = fopen(FILE_CANDIDATES, "r");
    if (f == NULL) {
        ESP_LOGE(TAG_GET_ALL_CANDIDATE, "Could not open file %s for reading", FILE_CANDIDATES);
        return ESP_FAIL;
    }

    *array_out = cJSON_CreateArray();  // allocate the output array

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        cJSON *candidate_obj = cJSON_Parse(line);
        if (candidate_obj) {
            // Add a deep copy to the array (ownership is passed to the array)
            cJSON_AddItemToArray(*array_out, candidate_obj);
        } else {
            ESP_LOGW(TAG_GET_ALL_CANDIDATE, "Skipping invalid JSON line");
        }
    }

    fclose(f);
    return ESP_OK;
}

esp_err_t get_candidate_by_id(int candidate_id, ui_candidate_t * found_candidate)
{
    ESP_LOGI("GET_CANDIDATE_BY_ID", "Get candidate by id: %d", candidate_id);

    memset(found_candidate, 0, sizeof(ui_candidate_t));

    FILE *f = fopen(FILE_CANDIDATES, "r");
    if (f == NULL) {
        ESP_LOGE("GET_CANDIDATE_BY_ID", "Could not open file %s for reading", FILE_CANDIDATES);
        return ESP_FAIL;
    }

    char line[256];
    ui_party_t found_party;
    ui_role_t found_role;

    while (fgets(line, sizeof(line), f)) {
        cJSON *candidate_obj = cJSON_Parse(line);
        if (!candidate_obj) {
            ESP_LOGE("GET_CANDIDATE_BY_ID", "Failed to parse JSON line");
            continue;
        }

        cJSON *id_item = cJSON_GetObjectItem(candidate_obj, "id");
        if (cJSON_IsNumber(id_item) && id_item->valueint == candidate_id) {
            // Found match
            found_candidate->id = id_item->valueint;
            found_candidate->name = strdup(cJSON_GetObjectItem(candidate_obj, "name")->valuestring);
            found_candidate->number = strdup(cJSON_GetObjectItem(candidate_obj, "number")->valuestring);
            found_candidate->role_id = cJSON_GetObjectItem(candidate_obj, "role_id")->valueint;
            found_candidate->party_id = cJSON_GetObjectItem(candidate_obj, "party_id")->valueint;

            // Resolve role name
            if (get_role_by_id(found_candidate->role_id, &found_role) == ESP_OK) {
                found_candidate->role_name = found_role.name;
            } else {
                found_candidate->role_name = strdup("No role");
            }

            // Resolve party name
            if (get_party_by_id(found_candidate->party_id, &found_party) == ESP_OK) {
                found_candidate->party_name = found_party.name;
            } else {
                found_candidate->party_name = strdup("No party");
            }

            cJSON_Delete(candidate_obj);
            fclose(f);
            return ESP_OK;
        }

        cJSON_Delete(candidate_obj);
    }

    fclose(f);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t edit_candidate_by_id(const ui_candidate_t *updated_candidate)
{
    ESP_LOGI("EDIT_CANDIDATE", "Edit candidate with id: %d", updated_candidate->id);

    // Validate party and role existence
    if(check_role_exists(updated_candidate->role_id) != ESP_OK) {
        ESP_LOGE("EDIT_CANDIDATE", "Role with id %d does not exist", updated_candidate->role_id);
        return ESP_FAIL;
    }

    if(check_party_exists(updated_candidate->party_id) != ESP_OK) {
        ESP_LOGE("EDIT_CANDIDATE", "Party with id %d does not exist", updated_candidate->party_id);
        return ESP_FAIL;
    }

    FILE *f = fopen(FILE_CANDIDATES, "r");
    if(f == NULL) {
        ESP_LOGE("EDIT_CANDIDATE", "Could not open file %s for reading", FILE_CANDIDATES);
        return ESP_FAIL;
    }

    FILE *f_temp = fopen(FILE_TEMP, "w");
    if(f_temp == NULL) {
        ESP_LOGE("EDIT_CANDIDATE", "Could not open file %s for writing", FILE_TEMP);
        fclose(f);
        return ESP_FAIL;
    }

    char line[256];
    bool found = false;

    while (fgets(line, sizeof(line), f)) {
        cJSON *candidate_obj = cJSON_Parse(line);
        if (!candidate_obj) {
            ESP_LOGW("EDIT_CANDIDATE", "Skipping invalid JSON line");
            continue;
        }

        cJSON *id_item = cJSON_GetObjectItem(candidate_obj, "id");
        if (cJSON_IsNumber(id_item) && id_item->valueint == updated_candidate->id) {
            found = true;
            cJSON_Delete(candidate_obj);

            candidate_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(candidate_obj, "id", updated_candidate->id);
            cJSON_AddStringToObject(candidate_obj, "name", updated_candidate->name);
            cJSON_AddStringToObject(candidate_obj, "number", updated_candidate->number);
            cJSON_AddNumberToObject(candidate_obj, "role_id", updated_candidate->role_id);
            cJSON_AddNumberToObject(candidate_obj, "party_id", updated_candidate->party_id);
        }

        char *rendered = cJSON_PrintUnformatted(candidate_obj);
        if (rendered) {
            fprintf(f_temp, "%s\n", rendered);
            free(rendered);
        } else {
            ESP_LOGE("EDIT_CANDIDATE", "Failed to print JSON line");
        }

        cJSON_Delete(candidate_obj);
    }

    fclose(f);
    fclose(f_temp);

    if(!found) {
        ESP_LOGW("EDIT_CANDIDATE", "Candidate with id %d not found", updated_candidate->id);
        remove(FILE_TEMP);
        return ESP_ERR_NOT_FOUND;
    }

    // Replace original file
    remove(FILE_CANDIDATES);
    rename(FILE_TEMP, FILE_CANDIDATES);
    ESP_LOGI("EDIT_CANDIDATE", "Candidate with id %d updated successfully", updated_candidate->id);
    return ESP_OK;
}

cJSON *candidate_to_json(const ui_candidate_t * candidate)
{
    if (!candidate) return NULL;

    cJSON *obj = cJSON_CreateObject();

    cJSON_AddNumberToObject(obj, "id", candidate->id);
    cJSON_AddStringToObject(obj, "name", candidate->name ? candidate->name : "");
    cJSON_AddStringToObject(obj, "number", candidate->number ? candidate->number : "");
    cJSON_AddNumberToObject(obj, "role_id", candidate->role_id);
    cJSON_AddStringToObject(obj, "role_name", candidate->role_name ? candidate->role_name : "");
    cJSON_AddNumberToObject(obj, "party_id", candidate->party_id);
    cJSON_AddStringToObject(obj, "party_name", candidate->party_name ? candidate->party_name : "");

    return obj;
}

// Helper function to clear candidate data (free memory)
void free_candidate(ui_candidate_t * candidate) {
    if (candidate->name) free(candidate->name);
    if (candidate->number) free(candidate->number);
    if (candidate->role_name) free(candidate->role_name);
    if (candidate->party_name) free(candidate->party_name);
    memset(candidate, 0, sizeof(ui_candidate_t));
}

esp_err_t add_party(ui_party_t new_party, int * out_id)
{
    ESP_LOGI(TAG_ADD_PARTY, "Add party");

    int current_id = 1;

    FILE *f = fopen(FILE_PARTIES, "r+");
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
        f = fopen(FILE_PARTIES, "w");
        if(f == NULL) {
            ESP_LOGE(TAG_ADD_PARTY, "Failed to open file parties.jsonl for writing");
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

    if(out_id) * out_id = current_id;
    return ESP_OK;
}

esp_err_t del_party_by_id(int party_id)
{
    ESP_LOGI(TAG_DEL_PARTY, "Delete party by id");
    
    FILE *f = fopen(FILE_PARTIES, "r");
    if(f == NULL) {
        ESP_LOGE(TAG_DEL_PARTY, "Could not open file %s for reading", FILE_PARTIES);
        return ESP_FAIL;
    }

    FILE *f_temp = fopen(FILE_TEMP, "w");
    if(f_temp == NULL) {
        ESP_LOGE(TAG_DEL_PARTY, "Could not open file %s for reading", FILE_TEMP);
        fclose(f);
        return ESP_FAIL;
    }

    char line[256];
    bool flag_party_found = false;

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
            flag_party_found = true;
            // Do not write this line to the temp file, effectively deleting it
        } else {
            // Write the line to the temp file if it doesn't match the candidate ID
            fprintf(f_temp, "%s", line);
        }

        cJSON_Delete(party_obj);
    }

    fclose(f);
    fclose(f_temp);

    if(!flag_party_found) {
        ESP_LOGE(TAG_DEL_PARTY, "Party with id %d not found", party_id);
        remove(FILE_TEMP); // Delete the temporary file since no changes were made
        return ESP_ERR_NOT_FOUND;
    }

    // Replace the original file with the updated temporary file
    remove(FILE_PARTIES);
    rename(FILE_TEMP, FILE_PARTIES);

    ESP_LOGI(TAG_DEL_PARTY, "Party with id %d deleted successfully", party_id);

    return ESP_OK;
}

esp_err_t get_party_by_id(int party_id, ui_party_t * found_party)
{
    ESP_LOGI(TAG_GET_PARTY, "Get party by id: %d", party_id);

    FILE *f = fopen(FILE_PARTIES, "r");
    if(f == NULL) {
        ESP_LOGE(TAG_GET_PARTY, "Could not open file %s for reading", FILE_PARTIES);
        return ESP_FAIL;
    }

    char line[256];
    cJSON *party_obj = NULL;

    while(fgets(line, sizeof(line), f)) {
        party_obj = cJSON_Parse(line);
        if(party_obj) {
            // Get party's ID from JSON object
            cJSON *id_item = cJSON_GetObjectItem(party_obj, "id");
            if(!cJSON_IsNumber(id_item)) {
                ESP_LOGE(TAG_GET_PARTY, "Invalid or missing 'id' field");
                cJSON_Delete(party_obj);
                continue; // Skip lines without a valid ID
            }

            int id = id_item->valueint;
            if(id == party_id) {
                // Party is found, fill name element
                cJSON *name_item = cJSON_GetObjectItem(party_obj, "name");

                if(!name_item || !cJSON_IsString(name_item)) {
                    ESP_LOGE(TAG_GET_PARTY, "Invalid or missing 'name' field");
                    cJSON_Delete(party_obj);
                    fclose(f);
                    return ESP_ERR_NOT_FOUND;
                }

                ESP_LOGI(TAG_GET_PARTY, "Party with id %d found", party_id);
                found_party->id = party_id;
                found_party->name = strdup(name_item->valuestring);
                cJSON_Delete(party_obj);
                fclose(f);
                return ESP_OK;
            }
        } else {
            ESP_LOGE(TAG_GET_PARTY, "Failed to parse JSON line");
            continue;
        }        
    }

    fclose(f);
    return ESP_ERR_NOT_FOUND; // Party not found
}

esp_err_t get_all_parties(cJSON **array_out)
{
    FILE *f = fopen(FILE_PARTIES, "r");
    if (f == NULL) {
        ESP_LOGE("GET_ALL_PARTIES", "Could not open %s", FILE_PARTIES);
        return ESP_FAIL;
    }

    *array_out = cJSON_CreateArray();
    if (*array_out == NULL) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        cJSON *party_obj = cJSON_Parse(line);
        if (party_obj) {
            cJSON_AddItemToArray(*array_out, party_obj);
        } else {
            ESP_LOGW("GET_ALL_PARTIES", "Skipping invalid JSON line");
        }
    }

    fclose(f);
    return ESP_OK;
}

esp_err_t edit_party_by_id(const ui_party_t * updated_party)
{
    ESP_LOGI("EDIT_PARTY", "Edit party with id: %d", updated_party->id);

    FILE *f = fopen(FILE_PARTIES, "r");
    if (f == NULL) {
        ESP_LOGE("EDIT_PARTY", "Could not open file %s for reading", FILE_PARTIES);
        return ESP_FAIL;
    }

    FILE *f_temp = fopen(FILE_TEMP, "w");
    if (f_temp == NULL) {
        ESP_LOGE("EDIT_PARTY", "Could not open file %s for writing", FILE_TEMP);
        fclose(f);
        return ESP_FAIL;
    }

    char line[256];
    bool found = false;

    while (fgets(line, sizeof(line), f)) {
        cJSON *party_obj = cJSON_Parse(line);
        if (!party_obj) {
            ESP_LOGW("EDIT_PARTY", "Skipping invalid JSON line");
            continue;
        }

        cJSON *id_item = cJSON_GetObjectItem(party_obj, "id");
        if (cJSON_IsNumber(id_item) && id_item->valueint == updated_party->id) {
            found = true;
            cJSON_Delete(party_obj);

            party_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(party_obj, "id", updated_party->id);
            cJSON_AddStringToObject(party_obj, "name", updated_party->name);
        }

        char *rendered = cJSON_PrintUnformatted(party_obj);
        if (rendered) {
            fprintf(f_temp, "%s\n", rendered);
            free(rendered);
        } else {
            ESP_LOGE("EDIT_PARTY", "Failed to print JSON line");
        }

        cJSON_Delete(party_obj);
    }

    fclose(f);
    fclose(f_temp);

    if (!found) {
        ESP_LOGW("EDIT_PARTY", "Party with id %d not found", updated_party->id);
        remove(FILE_TEMP);
        return ESP_ERR_NOT_FOUND;
    }

    remove(FILE_PARTIES);
    rename(FILE_TEMP, FILE_PARTIES);
    ESP_LOGI("EDIT_PARTY", "Party with id %d updated successfully", updated_party->id);
    return ESP_OK;
}

cJSON *party_to_json(const ui_party_t *party)
{
    if (!party) return NULL;

    cJSON *obj = cJSON_CreateObject();

    cJSON_AddNumberToObject(obj, "id", party->id);
    cJSON_AddStringToObject(obj, "name", party->name);

    return obj;
}

esp_err_t add_role(ui_role_t new_role, int * out_id)
{
    ESP_LOGI(TAG_ADD_ROLE, "Add role");

    int current_id = 1;

    FILE *f = fopen(FILE_ROLES, "r+");
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
        f = fopen(FILE_ROLES, "w");
        if(f == NULL) {
            ESP_LOGE(TAG_ADD_ROLE, "Failed to open file parties.jsonl for writing");
            return ESP_FAIL;
        }
    }

    cJSON *role_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(role_obj, "id", current_id);
    cJSON_AddStringToObject(role_obj, "name", new_role.name);
    cJSON_AddNumberToObject(role_obj, "n_digits", new_role.n_digits);

    char* rendered_json = cJSON_PrintUnformatted(role_obj);
    ESP_LOGI(TAG_ADD_ROLE, "%s", rendered_json);

    int write_ret = fprintf(f, "%s\n", rendered_json);

    cJSON_Delete(role_obj);
    free(rendered_json);
    fclose(f);

    if(write_ret < 0) {
        return ESP_FAIL;
    }

    if(out_id) * out_id = current_id;
    return ESP_OK;
}

int get_number_of_roles(void)
{
    int n_roles = 0;

    ESP_LOGI(TAG_GET_N_ROLES, "Get number of roles");

    FILE *f = fopen(FILE_ROLES, "r");
    if(f == NULL) {
        ESP_LOGE(TAG_GET_N_ROLES, "Could not open file %s for reading", FILE_ROLES);
        return -1;
    }

    char line[256];

    while(fgets(line, sizeof(line), f)) {
        cJSON *role_obj = cJSON_Parse(line);
        if(role_obj == NULL) {
            ESP_LOGE(TAG_GET_N_ROLES, "Failed to parse JSON line");
            continue;
        }

        // Get role's ID from JSON object
        cJSON *id_item = cJSON_GetObjectItem(role_obj, "id");
        if(!cJSON_IsNumber(id_item)) {
            ESP_LOGE(TAG_GET_N_ROLES, "Invalid or missing 'id' field");
            cJSON_Delete(role_obj);
            continue; // Skip lines without a valid ID
        } else {
            // found role object with valid id
            n_roles++;
        }
        cJSON_Delete(role_obj);
    }

    fclose(f);
    return n_roles;
}

esp_err_t get_role_by_id(int role_id, ui_role_t * found_role)
{
    ESP_LOGI(TAG_GET_ROLE, "Get role by id: %d", role_id);

    FILE *f = fopen(FILE_ROLES, "r");
    if(f == NULL) {
        ESP_LOGE(TAG_GET_ROLE, "Could not open file %s for reading", FILE_ROLES);
        return ESP_FAIL;
    }

    char line[256];
    cJSON *role_obj = NULL;

    while(fgets(line, sizeof(line), f)) {
        role_obj = cJSON_Parse(line);
        if(role_obj) {
            // Get roles's ID from JSON object
            cJSON *id_item = cJSON_GetObjectItem(role_obj, "id");
            if(!cJSON_IsNumber(id_item)) {
                ESP_LOGE(TAG_GET_ROLE, "Invalid or missing 'id' field");
                cJSON_Delete(role_obj);
                continue; // Skip lines without a valid ID
            }

            int id = id_item->valueint;
            if(id == role_id) {
                // Role is found, fill name element
                cJSON *name_item = cJSON_GetObjectItem(role_obj, "name");
                cJSON *n_digits_item = cJSON_GetObjectItem(role_obj, "n_digits");

                if(!name_item || !cJSON_IsString(name_item)) {
                    ESP_LOGE(TAG_GET_ROLE, "Invalid or missing 'name' field");
                    cJSON_Delete(role_obj);
                    fclose(f);
                    return ESP_ERR_NOT_FOUND;
                }

                if(!n_digits_item || !cJSON_IsNumber(n_digits_item)) {
                    ESP_LOGE(TAG_GET_ROLE, "Invalid or missing 'n_digits' field");
                    cJSON_Delete(role_obj);
                    fclose(f);
                    return ESP_ERR_NOT_FOUND;
                }

                ESP_LOGI(TAG_GET_ROLE, "Role with id %d found", role_id);
                found_role->id = role_id;
                found_role->name = strdup(name_item->valuestring);
                found_role->n_digits = n_digits_item->valueint;
                cJSON_Delete(role_obj);
                fclose(f);
                return ESP_OK;
            }
            cJSON_Delete(role_obj);
        } else {
            ESP_LOGE(TAG_GET_ROLE, "Failed to parse JSON line");
            continue;
        }
    }

    fclose(f);
    return ESP_ERR_NOT_FOUND; // Role not found
}

esp_err_t get_all_roles(cJSON **array_out)
{
    FILE *f = fopen(FILE_ROLES, "r");
    if (f == NULL) {
        ESP_LOGE("GET_ALL_ROLES", "Could not open %s", FILE_ROLES);
        return ESP_FAIL;
    }

    *array_out = cJSON_CreateArray();
    if (*array_out == NULL) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        cJSON *role_obj = cJSON_Parse(line);
        if (role_obj) {
            cJSON_AddItemToArray(*array_out, role_obj);
        } else {
            ESP_LOGW("GET_ALL_ROLES", "Skipping invalid JSON line");
        }
    }

    fclose(f);
    return ESP_OK;
}

esp_err_t edit_role_by_id(const ui_role_t * updated_role)
{
    ESP_LOGI("EDIT_ROLE", "Edit role with id: %d", updated_role->id);

    if (updated_role->n_digits < 2 || updated_role->n_digits > 4) {
        ESP_LOGE("EDIT_ROLE", "Invalid n_digits %d: must be 2, 3, or 4", updated_role->n_digits);
        return ESP_ERR_INVALID_ARG;
    }

    FILE *f = fopen(FILE_ROLES, "r");
    if (f == NULL) {
        ESP_LOGE("EDIT_ROLE", "Could not open file %s for reading", FILE_ROLES);
        return ESP_FAIL;
    }

    FILE *f_temp = fopen(FILE_TEMP, "w");
    if (f_temp == NULL) {
        ESP_LOGE("EDIT_ROLE", "Could not open file %s for writing", FILE_TEMP);
        fclose(f);
        return ESP_FAIL;
    }

    char line[256];
    bool found = false;

    while (fgets(line, sizeof(line), f)) {
        cJSON *role_obj = cJSON_Parse(line);
        if (!role_obj) {
            ESP_LOGW("EDIT_ROLE", "Skipping invalid JSON line");
            continue;
        }

        cJSON *id_item = cJSON_GetObjectItem(role_obj, "id");
        if (cJSON_IsNumber(id_item) && id_item->valueint == updated_role->id) {
            found = true;
            cJSON_Delete(role_obj);

            role_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(role_obj, "id", updated_role->id);
            cJSON_AddStringToObject(role_obj, "name", updated_role->name);
            cJSON_AddNumberToObject(role_obj, "n_digits", updated_role->n_digits);
        }

        char *rendered = cJSON_PrintUnformatted(role_obj);
        if (rendered) {
            fprintf(f_temp, "%s\n", rendered);
            free(rendered);
        } else {
            ESP_LOGE("EDIT_ROLE", "Failed to print JSON line");
        }

        cJSON_Delete(role_obj);
    }

    fclose(f);
    fclose(f_temp);

    if (!found) {
        remove(FILE_TEMP);
        ESP_LOGW("EDIT_ROLE", "Role with id %d not found", updated_role->id);
        return ESP_ERR_NOT_FOUND;
    }

    remove(FILE_ROLES);
    rename(FILE_TEMP, FILE_ROLES);
    ESP_LOGI("EDIT_ROLE", "Role with id %d updated successfully", updated_role->id);

    return ESP_OK;
}

cJSON *role_to_json(const ui_role_t *role)
{
    if (!role) return NULL;

    cJSON *obj = cJSON_CreateObject();

    cJSON_AddNumberToObject(obj, "id", role->id);
    cJSON_AddStringToObject(obj, "name", role->name ? role->name : "");
    cJSON_AddNumberToObject(obj, "n_digits", role->n_digits);

    return obj;
}

esp_err_t del_role_by_id(int role_id)
{
    ESP_LOGI(TAG_DEL_ROLE, "Delete role by id");
    
    FILE *f = fopen(FILE_ROLES, "r");
    if(f == NULL) {
        ESP_LOGE(TAG_DEL_ROLE, "Could not open file %s for reading", FILE_ROLES);
        return ESP_FAIL;
    }

    FILE *f_temp = fopen(FILE_TEMP, "w");
    if(f_temp == NULL) {
        ESP_LOGE(TAG_DEL_ROLE, "Could not open file %s for reading", FILE_TEMP);
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
        remove(FILE_TEMP); // Delete the temporary file since no changes were made
        return ESP_ERR_NOT_FOUND;
    }

    // Replace the original file with the updated temporary file
    remove(FILE_ROLES);
    rename(FILE_TEMP, FILE_ROLES);

    ESP_LOGI(TAG_DEL_ROLE, "Role with id %d deleted successfully", role_id);

    return ESP_OK;
}

esp_err_t check_party_exists(int party_id)
{
    ESP_LOGI(TAG_CHECK_PARTY, "Check if party with %d exists", party_id);

    FILE *f = fopen(FILE_PARTIES, "r");
    if(f == NULL) {
        ESP_LOGE(TAG_CHECK_PARTY, "Could not open file %s for reading", FILE_PARTIES);
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

    FILE *f = fopen(FILE_ROLES, "r");
    if(f == NULL) {
        ESP_LOGE(TAG_CHECK_ROLE, "Could not open file %s for reading", FILE_ROLES);
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

esp_err_t store_vote(ui_vote_store_t vote)
{
    ESP_LOGI(TAG_STORE_VOTE, "Storing vote for number: %s", vote.number);

    FILE *f = fopen(FILE_VOTES, "a");
    if(f == NULL) {
        ESP_LOGE(TAG_STORE_VOTE, "Could not open file %s for reading", FILE_VOTES);
        return ESP_FAIL;
    }

    cJSON *vote_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(vote_obj, "number", vote.number);
    cJSON_AddNumberToObject(vote_obj, "timestamp", (double)vote.timestamp);

    char *rendered_json = cJSON_PrintUnformatted(vote_obj);
    if (!rendered_json) {
        ESP_LOGE(TAG_STORE_VOTE, "Failed to render vote JSON");
        cJSON_Delete(vote_obj);
        fclose(f);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_STORE_VOTE, "Vote JSON: %s", rendered_json);

    int write_ret = fprintf(f, "%s\n", rendered_json);

    if(write_ret < 0) {
        ESP_LOGE(TAG_STORE_VOTE, "Failed to write vote to file");
        return ESP_FAIL;
    }

    fclose(f);
    return ESP_OK;
}

void load_candidate_roles(void)
{
    FILE *f = fopen(FILE_ROLES, "r");
    if (!f) {
        ESP_LOGE("ROLES", "Could not open %s", FILE_ROLES);
        return;
    }

    char line[256];
    total_roles = 0;

    while (fgets(line, sizeof(line), f) && total_roles < MAX_ROLES) {
        cJSON *obj = cJSON_Parse(line);
        if (!obj) continue;

        cJSON *id = cJSON_GetObjectItem(obj, "id");
        cJSON *name = cJSON_GetObjectItem(obj, "name");
        cJSON *n_digits = cJSON_GetObjectItem(obj, "n_digits");

        if (cJSON_IsNumber(id) && cJSON_IsString(name) && cJSON_IsNumber(n_digits)) {
            ESP_LOGI("ROLES", "Loading role #%d (id=%d, name=%s)", total_roles, id->valueint, name->valuestring);
            voting_sequence[total_roles].id = id->valueint;
            strncpy(voting_sequence[total_roles].name, name->valuestring, sizeof(voting_sequence[total_roles].name) - 1);
            voting_sequence[total_roles].n_digits = n_digits->valueint;
            total_roles++;
        }

        cJSON_Delete(obj);
    }

    fclose(f);
}

esp_err_t save_candidate_image_to_sd(int candidate_number, const uint8_t *data, size_t size)
{
    if (candidate_number <= 0 || !data || size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    char filepath[64];
    snprintf(filepath, sizeof(filepath), "/sd/%d.bin", candidate_number);

    FILE *file = fopen(filepath, "wb");
    if (!file) {
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, size, file);
    fclose(file);

    return (written == size) ? ESP_OK : ESP_FAIL;
}
