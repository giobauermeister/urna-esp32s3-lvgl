/**
 * @file ui.h
 *
 */

#ifndef _UI_H_
#define _UI_H_

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "candidate/candidate.h"

/*********************
 *      DEFINES
 *********************/

/*********************
 *      ASSETS
 *********************/
LV_IMAGE_DECLARE(img_unknown);
LV_IMAGE_DECLARE(img_no_photo);

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    int id;
    char name[32];   // Safe array for local storage
    int n_digits;
} ui_role_static_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void create_ui(void);
void update_ui_keypress(char key);

/**********************
 * GLOBAL VARIABLES
 **********************/
extern ui_role_static_t voting_sequence[MAX_ROLES];
extern int total_roles;
extern int current_role_index;

#endif  // _UI_H_
