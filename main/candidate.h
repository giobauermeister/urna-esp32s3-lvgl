#ifndef CANDIDATE_H
#define CANDIDATE_H

#include "esp_err.h"

typedef struct 
{
    const char* name;
    const char* number;
    const char* role;
    int party_id;
} Candidate;

typedef struct
{
    const char* name;
} Party;

esp_err_t add_candidate(Candidate new_dandidate);
esp_err_t add_party(Party new_party);

#endif // CANDIDATE_H