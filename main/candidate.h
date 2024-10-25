#ifndef CANDIDATE_H
#define CANDIDATE_H

#include "esp_err.h"

#define FILE_CANDIDATES "/sd/candidates.jdb"
#define FILE_PARTIES    "/sd/parties.jdb"
#define FILE_ROLES      "/sd/roles.jdb"
#define FILE_TEMP       "/sd/temp.jdb"

typedef struct {
    int id;
    char* name;
    char* number;
    int role_id;
    char* role_name;
    int party_id;
    char* party_name;
} Candidate;

typedef struct {
    int id;
    char* name;
} Party;

typedef struct {
    int id;
    char* name;
    int n_digits;
} Role;

esp_err_t del_file_content(const char *file_path);
esp_err_t add_candidate(Candidate new_dandidate);
esp_err_t del_candidate_by_id(int candidate_id);
esp_err_t search_candidate(const char* candidate_number, Candidate* found_candidate);
void free_candidate(Candidate* candidate);
esp_err_t add_party(Party new_party);
esp_err_t del_party_by_id(int party_id);
esp_err_t check_party_exists(int party_id);
esp_err_t get_party_by_id(int party_id, Party* found_party);
esp_err_t add_role(Role new_role);
esp_err_t del_role_by_id(int role_id);
esp_err_t check_role_exists(int role_id);

#endif // CANDIDATE_H