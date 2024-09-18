#ifndef CANDIDATE_H
#define CANDIDATE_H

#include "esp_err.h"

esp_err_t add_candidate(const char* name, const char* number, const char* role, int party_id);
esp_err_t add_party(const char* name);

#endif // CANDIDATE_H