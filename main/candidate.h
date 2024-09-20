#ifndef CANDIDATE_H
#define CANDIDATE_H

#include "esp_err.h"

typedef struct {
    int id;
    const char* name;
    const char* number;
    int role_id;
    int party_id;
} Candidate;

typedef struct {
    int id;
    const char* name;
} Party;

typedef struct {
    int id;
    const char* name;
} Role;

esp_err_t del_file_content(const char *file_path);
esp_err_t add_candidate(Candidate new_dandidate);
esp_err_t del_candidate_by_id(int candidate_id);
esp_err_t add_party(Party new_party);
esp_err_t del_party_by_id(int party_id);
esp_err_t check_party_exists(int party_id);
esp_err_t add_role(Role new_role);
esp_err_t del_role_by_id(int role_id);
esp_err_t check_role_exists(int role_id);

#endif // CANDIDATE_H