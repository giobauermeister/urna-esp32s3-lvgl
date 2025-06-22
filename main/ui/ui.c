/**
 * @file ui.c
 *
 */

 /*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "ui/ui.h"
#include "candidate/candidate.h"
#include "sound/sound.h"
#include <time.h>

/*********************
 *      DEFINES
 *********************/
#define NUM_RECTANGLES 4

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void set_screen_background_white(lv_obj_t *screen);
static void anim_border_opacity_cb(void *rect_obj, int32_t opacity);
static void create_row_rectangles(lv_obj_t *parent, int16_t n_rect);
static void blink_rectangle(uint8_t rect_id, bool run);
static void store_vote_async_cb(void * vote_ptr);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/
// Declare an array to hold the rectangle objects
lv_obj_t *rectangles[NUM_RECTANGLES];
lv_obj_t *lb_rect_input_numbers[NUM_RECTANGLES];

lv_obj_t *lb_candidate_role;
lv_obj_t *lb_cadidate_name;
lv_obj_t *lb_candidate_party_name;

lv_obj_t *ui_bottom_line;
lv_obj_t *ui_lb_press_key;
lv_obj_t *ui_lb_confirm;
lv_obj_t *ui_lb_restart;
lv_obj_t *ui_img_profile;

int lb_rect_input_number = 0;
bool vote_state = false;
char vote_number[5] = "";

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void update_ui_keypress(char key)
{
    LV_LOG_USER("Received key: %c", key);

    if(vote_state == false && (key == 'A' || key == 'C' || key == 'D' || key == '*' || key == '#')) {
        return;
    }

    if(vote_state == true)
    {
        if(key == 'B') // CORRIGE key pressed
        {
            vote_number[0] = '\0';

            for (size_t i = 0; i < 4; i++)
            {
                lv_label_set_text(lb_rect_input_numbers[i], "");
                blink_rectangle(i, false);
            }
            lb_rect_input_number = 0;
            vote_state = false;
            blink_rectangle(lb_rect_input_number, true);

            lv_obj_add_flag(ui_img_profile, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(lb_cadidate_name, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(lb_candidate_party_name, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_bottom_line, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_lb_press_key, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_lb_confirm, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_lb_restart, LV_OBJ_FLAG_HIDDEN);

            return;
        }

        if(key == 'A') // CONFIRMA key pressed
        {
            LV_LOG_USER("Confirm vote for number: %s", vote_number);
            
            ui_vote_store_t *vote = lv_malloc(sizeof(ui_vote_store_t));
            if (!vote) {
                LV_LOG_USER("Failed to allocate memory for vote");
                return;
            }

            vote->number = lv_strdup(vote_number);  // make a copy of the number string
            vote->timestamp = time(NULL);

            if (!vote->number) {
                LV_LOG_USER("Failed to allocate memory for vote number");
                lv_free(vote);
                return;
            }

            lv_async_call(store_vote_async_cb, vote);
            lv_async_call(play_urna_sound_short, NULL);
        }
    }

    if(key == 'B') // CORRIGE key pressed
    {
        vote_number[0] = '\0';
        
        for (size_t i = 0; i < 4; i++)
        {
            lv_label_set_text(lb_rect_input_numbers[i], "");
            blink_rectangle(i, false);
        }
        lb_rect_input_number = 0;
        vote_state = false;
        blink_rectangle(lb_rect_input_number, true);

        lv_obj_add_flag(ui_img_profile, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lb_cadidate_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lb_candidate_party_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_bottom_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_lb_press_key, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_lb_confirm, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_lb_restart, LV_OBJ_FLAG_HIDDEN);

        return;
    }

    if(lb_rect_input_number < 3)
    {
        vote_number[lb_rect_input_number] = key;
        lv_label_set_text_fmt(lb_rect_input_numbers[lb_rect_input_number], "%c", key);
        lb_rect_input_number++;
        blink_rectangle(lb_rect_input_number, true);
        blink_rectangle(lb_rect_input_number-1, false);
        return;
    }

    if(lb_rect_input_number == 3 && vote_state == false)
    {
        vote_number[lb_rect_input_number] = key;
        lv_label_set_text_fmt(lb_rect_input_numbers[lb_rect_input_number], "%c", key);
        blink_rectangle(lb_rect_input_number, false);
        vote_state = true;

        LV_LOG_USER("Search for candidate with number: %s", vote_number);

        ui_candidate_t found_candidate;
        esp_err_t result = search_candidate(vote_number, &found_candidate);
        char file_path[64];

        if (result == ESP_OK) {
            LV_LOG_USER("Candidate found: %s (ID: %d)", found_candidate.name, found_candidate.id);
            // Use the found_candidate structure
            lv_label_set_text(lb_cadidate_name, found_candidate.name);
            lv_label_set_text(lb_candidate_party_name, found_candidate.party_name);
            snprintf(file_path, sizeof(file_path), "S:/%s.bin", found_candidate.number);
            lv_image_set_scale(ui_img_profile, 256);
            lv_image_set_src(ui_img_profile, file_path);
            free_candidate(&found_candidate);  // Free memory after use
        } else {
            LV_LOG_USER("Candidate not found");
            lv_label_set_text(lb_cadidate_name, "Nao encontrado");
            lv_label_set_text(lb_candidate_party_name, " ");
            lv_image_set_scale(ui_img_profile, 512);
            lv_image_set_src(ui_img_profile, &img_unknown);
        }

        lv_obj_remove_flag(ui_img_profile, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(lb_cadidate_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(lb_candidate_party_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_bottom_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_lb_press_key, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_lb_confirm, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_lb_restart, LV_OBJ_FLAG_HIDDEN);

        return;
    }
}

// Create UI elements (buttons, labels, etc.)
void create_ui(void) 
{

    lv_obj_t *screen = lv_screen_active();

    // Set screen background to white
    set_screen_background_white(screen);

    // Create a static label
    lv_obj_t *ui_lb_your_vote_for = lv_label_create(screen);
    lv_label_set_text(ui_lb_your_vote_for, "SEU VOTO PARA");
    lv_obj_set_style_text_font(ui_lb_your_vote_for, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(ui_lb_your_vote_for, LV_ALIGN_TOP_LEFT, 10, 25);

    lb_candidate_role = lv_label_create(screen);
    lv_label_set_text(lb_candidate_role, "Vereador");
    lv_obj_set_style_text_font(lb_candidate_role, &lv_font_montserrat_46, LV_PART_MAIN);
    lv_obj_align(lb_candidate_role, LV_ALIGN_TOP_LEFT, 120, 90);

    lv_obj_t *ui_lb_number = lv_label_create(screen);
    lv_label_set_text(ui_lb_number, "Numero:");
    lv_obj_set_style_text_font(ui_lb_number, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(ui_lb_number, LV_ALIGN_LEFT_MID, 5, 0);

    lv_obj_t *ui_lb_name = lv_label_create(screen);
    lv_label_set_text(ui_lb_name, "Nome:");
    lv_obj_set_style_text_font(ui_lb_name, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(ui_lb_name, LV_ALIGN_TOP_LEFT, 5, 315);

    lb_cadidate_name = lv_label_create(screen);
    lv_label_set_text(lb_cadidate_name, "Giovanni B Santana");
    lv_obj_set_style_text_font(lb_cadidate_name, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_obj_align_to(lb_cadidate_name, ui_lb_name, LV_ALIGN_OUT_RIGHT_MID, 45, 0);
    lv_obj_add_flag(lb_cadidate_name, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *ui_lb_party = lv_label_create(screen);
    lv_label_set_text(ui_lb_party, "Partido:");
    lv_obj_set_style_text_font(ui_lb_party, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(ui_lb_party, LV_ALIGN_TOP_LEFT, 5, 355);

    lb_candidate_party_name = lv_label_create(screen);
    lv_label_set_text(lb_candidate_party_name, "Exemplo");
    lv_obj_set_style_text_font(lb_candidate_party_name, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_obj_align_to(lb_candidate_party_name, ui_lb_party, LV_ALIGN_OUT_RIGHT_MID, 30, 0);
    lv_obj_add_flag(lb_candidate_party_name, LV_OBJ_FLAG_HIDDEN);

    create_row_rectangles(screen, NUM_RECTANGLES);

    blink_rectangle(0, true);
    blink_rectangle(1, false);
    blink_rectangle(2, false);
    blink_rectangle(3, false);

    // Create line at the bottom
    ui_bottom_line = lv_obj_create(screen);
    lv_obj_set_size(ui_bottom_line, 800, 3);
    lv_obj_set_pos(ui_bottom_line, 0, 410);
    lv_obj_set_style_border_color(ui_bottom_line, lv_color_black(), LV_PART_MAIN);
    lv_obj_add_flag(ui_bottom_line, LV_OBJ_FLAG_HIDDEN);

    ui_lb_press_key = lv_label_create(screen);
    lv_label_set_text(ui_lb_press_key, "Aperte a tecla:");
    lv_obj_set_style_text_font(ui_lb_press_key, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(ui_lb_press_key, LV_ALIGN_TOP_LEFT, 5, 415);
    lv_obj_add_flag(ui_lb_press_key, LV_OBJ_FLAG_HIDDEN);

    ui_lb_confirm = lv_label_create(screen);
    lv_label_set_text(ui_lb_confirm, "A para CONFIRMAR este voto");
    lv_obj_set_style_text_font(ui_lb_confirm, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(ui_lb_confirm, LV_ALIGN_TOP_LEFT, 45, 435);
    lv_obj_add_flag(ui_lb_confirm, LV_OBJ_FLAG_HIDDEN);

    ui_lb_restart = lv_label_create(screen);
    lv_label_set_text(ui_lb_restart, "B para REINICIAR este voto");
    lv_obj_set_style_text_font(ui_lb_restart, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(ui_lb_restart, LV_ALIGN_TOP_LEFT, 45, 455);
    lv_obj_add_flag(ui_lb_restart, LV_OBJ_FLAG_HIDDEN);

    ui_img_profile = lv_image_create(screen);
    lv_obj_set_size(ui_img_profile, 240, 300);
    lv_obj_align(ui_img_profile, LV_ALIGN_RIGHT_MID, -80, 0);
    lv_obj_add_flag(ui_img_profile, LV_OBJ_FLAG_HIDDEN);

}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void set_screen_background_white(lv_obj_t *screen)
{
    static lv_style_t screen_style;
    lv_style_init(&screen_style);

    // Set background color to white
    lv_style_set_bg_color(&screen_style, lv_color_white());
    lv_style_set_bg_opa(&screen_style, LV_OPA_COVER);

    // Apply style to screen
    lv_obj_add_style(screen, &screen_style, 0);
}

// Function to update the border opacity during the animation
static void anim_border_opacity_cb(void *rect_obj, int32_t opacity)
{
    lv_obj_t *rect = (lv_obj_t *)rect_obj;
    lv_obj_set_style_border_opa(rect, opacity, 0);
}

static void create_row_rectangles(lv_obj_t *parent, int16_t n_rect)
{
    int16_t rect_width = 70;
    int16_t rect_height = 95;
    int16_t spacing = 10;
    lv_obj_t *previous_rect = NULL;

    // Create style for rectangles
    static lv_style_t rect_style;
    lv_style_init(&rect_style);
    lv_style_set_border_color(&rect_style, lv_color_black()); // Set border color to black
    lv_style_set_border_width(&rect_style, 3);                // Border width of 4 pixels
    lv_style_set_radius(&rect_style, 3);                      // Set border radius to 5
    lv_style_set_border_opa(&rect_style, LV_OPA_COVER);       // Set border opacity to fully opaque
    lv_style_set_bg_opa(&rect_style, LV_OPA_TRANSP);          // Set background opacity to transparent

    for (uint8_t i = 0; i < n_rect; i++) {
        rectangles[i] = lv_obj_create(parent);
        lb_rect_input_numbers[i] = lv_label_create(parent);

        lv_obj_set_size(rectangles[i], rect_width, rect_height);

        if(i == 0) {
            lv_obj_align(rectangles[i], LV_ALIGN_LEFT_MID, 110, 0); // Position on the screen
        } else {
            lv_obj_align_to(rectangles[i], previous_rect, LV_ALIGN_OUT_RIGHT_MID, spacing, 0);
        }

        lv_obj_add_style(rectangles[i], &rect_style, 0);

        lv_obj_set_style_text_font(lb_rect_input_numbers[i], &lv_font_montserrat_48, LV_PART_MAIN);
        lv_obj_align_to(lb_rect_input_numbers[i], rectangles[i], LV_ALIGN_LEFT_MID, 0, 0);
        lv_label_set_text(lb_rect_input_numbers[i], "");
        //lv_label_set_text_fmt(lb_rect_input_numbers[i], "%d", i);

        // Update the previous_rect to the current one
        previous_rect = rectangles[i];
    }
}

static void blink_rectangle(uint8_t rect_id, bool run)
{
    if(run == true) {
        lv_anim_t rect_anim;
        lv_anim_init(&rect_anim);
        lv_anim_set_var(&rect_anim, rectangles[rect_id]);
        lv_anim_set_values(&rect_anim, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_duration(&rect_anim, 600);
        lv_anim_set_playback_duration(&rect_anim, 400);
        lv_anim_set_repeat_count(&rect_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&rect_anim, lv_anim_path_linear);
        lv_anim_set_exec_cb(&rect_anim, anim_border_opacity_cb);
        lv_anim_start(&rect_anim);
    } else {
        lv_anim_delete(rectangles[rect_id], NULL);
        lv_obj_set_style_border_opa(rectangles[rect_id], LV_OPA_COVER, LV_PART_MAIN);
    }
}

static void store_vote_async_cb(void * vote_ptr)
{
    ui_vote_store_t *vote = (ui_vote_store_t *)vote_ptr;
    store_vote(*vote);  // reuse your function
    lv_free(vote);         // free after done
}