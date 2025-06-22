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
#define MAX_DIGITS 4

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
static ui_role_static_t * get_current_role(void);
static bool go_to_next_role(void);
static void restart_voting(void);
static void show_voting_end_screen_and_restart(void);
static void restart_voting_cb(lv_timer_t * timer);

/**********************
 *  STATIC VARIABLES
 **********************/
static bool candidate_found = false;

/**********************
 *  GLOBAL VARIABLES
 **********************/
// Declare an array to hold the rectangle objects
lv_obj_t *rectangles[MAX_DIGITS];
lv_obj_t *lb_rect_input_numbers[MAX_DIGITS];

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
char vote_number[MAX_DIGITS + 1] = "";
int total_rectangles = 0;

ui_role_static_t voting_sequence[MAX_ROLES];
int total_roles = 0;
int current_role_index = 0;

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
            candidate_found = false;

            lv_memset(vote_number, 0, sizeof(vote_number));

            for (size_t i = 0; i < total_rectangles; i++)
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
            if (!candidate_found) {
                LV_LOG_USER("Cannot confirm vote: candidate not found");
                return;
            }

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

            if (go_to_next_role()) {
                candidate_found = false;
                lv_async_call(play_urna_sound_short, NULL);
                // Prepare for next voting stage

                // 1. Reset vote state and number
                lv_memset(vote_number, 0, sizeof(vote_number));
                lb_rect_input_number = 0;
                vote_state = false;

                // 2. Destroy old rectangles (optional if you recreate them in-place)
                for (size_t i = 0; i < total_rectangles; i++) {
                    lv_obj_delete(rectangles[i]);
                    lv_obj_delete(lb_rect_input_numbers[i]);
                }

                // 3. Get new role info and recreate rectangles
                ui_role_static_t *role = get_current_role();
                if (role) {
                    total_rectangles = role->n_digits;
                    create_row_rectangles(lv_screen_active(), total_rectangles);

                    // 4. Blink the first rectangle
                    blink_rectangle(0, true);
                    for (int i = 1; i < total_rectangles; i++) {
                        blink_rectangle(i, false);
                    }

                    // 5. Update role name label
                    lv_label_set_text(lb_candidate_role, role->name);
                }

                // 6. Hide result UI elements again
                lv_obj_add_flag(ui_img_profile, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(lb_cadidate_name, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(lb_candidate_party_name, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_bottom_line, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_lb_press_key, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_lb_confirm, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(ui_lb_restart, LV_OBJ_FLAG_HIDDEN);

                return;
            } else {
                LV_LOG_USER("All roles completed. End of voting.");
                lv_async_call(play_urna_sound_long, NULL);
                show_voting_end_screen_and_restart();
            }
        }
    }

    if(key == 'B') // CORRIGE key pressed
    {
        lv_memset(vote_number, 0, sizeof(vote_number));
        
        for (size_t i = 0; i < total_rectangles; i++)
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

    if(lb_rect_input_number < total_rectangles - 1)
    {
        vote_number[lb_rect_input_number] = key;
        lv_label_set_text_fmt(lb_rect_input_numbers[lb_rect_input_number], "%c", key);
        lb_rect_input_number++;
        blink_rectangle(lb_rect_input_number, true);
        blink_rectangle(lb_rect_input_number-1, false);
        return;
    }

    if(lb_rect_input_number == total_rectangles - 1 && vote_state == false)
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
            candidate_found = true;
            // Use the found_candidate structure
            lv_label_set_text(lb_cadidate_name, found_candidate.name);
            lv_label_set_text(lb_candidate_party_name, found_candidate.party_name);
            snprintf(file_path, sizeof(file_path), "S:/%s.bin", found_candidate.number);
            lv_image_set_scale(ui_img_profile, 256);
            lv_image_set_src(ui_img_profile, file_path);
            free_candidate(&found_candidate);  // Free memory after use
            lv_obj_remove_flag(ui_lb_confirm, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(ui_lb_restart, LV_OBJ_FLAG_HIDDEN);
        } else {
            LV_LOG_USER("Candidate not found");
            candidate_found = false;
            lv_label_set_text(lb_cadidate_name, "Nao encontrado");
            lv_label_set_text(lb_candidate_party_name, " ");
            lv_image_set_scale(ui_img_profile, 512);
            lv_image_set_src(ui_img_profile, &img_unknown);
            lv_obj_remove_flag(ui_lb_restart, LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_remove_flag(ui_img_profile, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(lb_cadidate_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(lb_candidate_party_name, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_bottom_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(ui_lb_press_key, LV_OBJ_FLAG_HIDDEN);

        return;
    }
}

// Create UI elements (buttons, labels, etc.)
void create_ui(void) 
{

    lv_obj_t *screen = lv_screen_active();

    // Set screen background to white
    set_screen_background_white(screen);

    ui_role_static_t *role = get_current_role();
    if (role) {
        LV_LOG_USER("Now voting for %s (%d digits)", role->name, role->n_digits);
    }

    // Create a static label
    lv_obj_t *ui_lb_your_vote_for = lv_label_create(screen);
    lv_label_set_text(ui_lb_your_vote_for, "SEU VOTO PARA");
    lv_obj_set_style_text_font(ui_lb_your_vote_for, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(ui_lb_your_vote_for, LV_ALIGN_TOP_LEFT, 10, 25);

    lb_candidate_role = lv_label_create(screen);
    if (role) {
        lv_label_set_text(lb_candidate_role, role->name);
    } else {
        lv_label_set_text(lb_candidate_role, "???");
    }
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

    if (role) {
        total_rectangles = role->n_digits;
        create_row_rectangles(screen, total_rectangles);

        for (int i = 0; i < total_rectangles; i++) {
            blink_rectangle(i, i == 0);  // only first rectangle blinks
        }
    }

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
    store_vote(*vote);
    lv_free(vote);
}

static ui_role_static_t * get_current_role(void)
{
    if (current_role_index < total_roles) {
        return &voting_sequence[current_role_index];
    }
    return NULL;
}

static bool go_to_next_role(void)
{
    if (current_role_index + 1 < total_roles) {
        current_role_index++;
        return true;
    }
    return false;  // no more roles, voting done
}

static void restart_voting(void)
{
    LV_LOG_USER("Restarting voting");
    current_role_index = 0;
    lb_rect_input_number = 0;
    vote_state = false;
    lv_memset(vote_number, 0, sizeof(vote_number));

    // Destroy current screen
    lv_obj_clean(lv_screen_active());

    // Recreate UI from scratch
    create_ui();

    LV_LOG_USER("vote_number: %s", vote_number);
}

static void show_voting_end_screen_and_restart(void)
{
    // Clear everything
    lv_obj_clean(lv_screen_active());

    lv_obj_t *screen = lv_screen_active();

    lv_obj_t *lb_fim = lv_label_create(screen);
    lv_label_set_text(lb_fim, "FIM");
    lv_obj_set_style_text_font(lb_fim, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_center(lb_fim);

    // Schedule restart after 3 seconds
    lv_timer_t *restart_timer = lv_timer_create_basic();
    lv_timer_set_cb(restart_timer, restart_voting_cb);
    lv_timer_set_period(restart_timer, 3000);
    lv_timer_set_repeat_count(restart_timer, 1);
}

static void restart_voting_cb(lv_timer_t * timer)
{
    lv_timer_delete(timer);
    restart_voting();
}