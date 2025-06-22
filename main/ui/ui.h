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

/*********************
 *      DEFINES
 *********************/

/*********************
 *      ASSETS
 *********************/
LV_IMAGE_DECLARE(img_unknown);
LV_IMAGE_DECLARE(img_no_photo);

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void create_ui(void);
void update_ui_keypress(char key);

#endif  // _UI_H_
