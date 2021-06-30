
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef SCREEN_H
#define SCREEN_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "config.h"
#include "data.h"
#include "node.h"
#include "glcd_widget.h"

/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           MACRO'S
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           FUNCTION PROTOTYPES
************************************************************************************************************************
*/

void screen_clear(void);
void screen_force_update(void);
void screen_set_hide_non_assigned_actuators(uint8_t hide);
void screen_set_control_mode_header(uint8_t toggle);
void screen_group_foots(uint8_t toggle);
void screen_encoder(control_t *control, uint8_t encoder);
void screen_encoder_container(uint8_t current_encoder_page);
void screen_page_index(uint8_t current, uint8_t available);
void screen_tittle(const void *data, uint8_t update, int8_t pb_ss);
void screen_footer(uint8_t foot_id, const char *name, const char *value, int16_t property);
void screen_tool(uint8_t tool, uint8_t display_id);
void screen_bank_list(bp_list_t *list, const char *name);
void screen_pbss_list(const char *title, bp_list_t *list, uint8_t pb_ss_toggle, int8_t hold_item_index, 
					  const char *hold_item_label);
void screen_system_menu(menu_item_t *item);
void screen_tool_control_page(node_t *node);
void screen_toggle_tuner(float frequency, char *note, int8_t cents);
void screen_image(uint8_t display, const uint8_t *image);
void screen_shift_overlay(int8_t prev_mode, int16_t *item_ids);
void screen_menu_page(node_t *node);
void screen_control_overlay(control_t *control);
void screen_msg_overlay(char *message);
void screen_popup(system_popup_t *popup_data);
void screen_update_tuner(float frequency, char *note, int8_t cents);
void print_tripple_menu_items(menu_item_t *item_child, uint8_t knob, uint8_t tool_mode);
void screen_text_box(uint8_t display, uint8_t x, uint8_t y, const char *text);

/*
************************************************************************************************************************
*           CONFIGURATION ERRORS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           END HEADER
************************************************************************************************************************
*/

#endif
