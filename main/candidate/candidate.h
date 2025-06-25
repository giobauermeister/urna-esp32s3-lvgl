#ifndef CANDIDATE_H
#define CANDIDATE_H

#include "esp_err.h"
#include <time.h>
#include <stdbool.h>
#include "cJSON.h"

#define FILE_CANDIDATES "/sd/candidates.jsonl"
#define FILE_PARTIES    "/sd/parties.jsonl"
#define FILE_ROLES      "/sd/roles.jsonl"
#define FILE_VOTES      "/sd/votes.jsonl"
#define FILE_TEMP       "/sd/temp.jsonl"

#define MAX_ROLES 10

typedef struct {
    int id;
    char * name;
    char * number;
    int role_id;
    char * role_name;
    int party_id;
    char * party_name;
} ui_candidate_t;

typedef struct {
    int id;
    char * name;
} ui_party_t;

typedef struct {
    int id;
    char * name;
    int n_digits;
} ui_role_t;

typedef struct {
    char * number;
    time_t timestamp;
} ui_vote_store_t;

esp_err_t del_file_content(const char * file_path);
esp_err_t add_candidate(ui_candidate_t new_dandidate);
esp_err_t del_candidate_by_id(int candidate_id);
esp_err_t search_candidate(const char * candidate_number, ui_candidate_t * found_candidate);
esp_err_t get_all_candidates(cJSON **array_out);
esp_err_t get_candidate_by_id(int candidate_id, ui_candidate_t * found_candidate);
cJSON *candidate_to_json(const ui_candidate_t * candidate);
void free_candidate(ui_candidate_t * candidate);
esp_err_t add_party(ui_party_t new_party);
esp_err_t del_party_by_id(int party_id);
esp_err_t check_party_exists(int party_id);
esp_err_t get_party_by_id(int party_id, ui_party_t * found_party);
esp_err_t get_all_parties(cJSON **array_out);
cJSON *party_to_json(const ui_party_t *party);
esp_err_t add_role(ui_role_t new_role);
esp_err_t del_role_by_id(int role_id);
esp_err_t get_role_by_id(int role_id, ui_role_t * found_role);
esp_err_t get_all_roles(cJSON **array_out);
cJSON *role_to_json(const ui_role_t *role);
esp_err_t check_role_exists(int role_id);
int get_number_of_roles(void);
esp_err_t store_vote(ui_vote_store_t vote);
void load_candidate_roles(void);

#endif // CANDIDATE_H