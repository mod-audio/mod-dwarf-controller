
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef  GLCD_WIDGET_H
#define  GLCD_WIDGET_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "glcd.h"
#include "fonts.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

typedef enum {ALIGN_LEFT_TOP, ALIGN_LEFT_MIDDLE, ALIGN_LEFT_BOTTOM,
              ALIGN_CENTER_TOP,ALIGN_CENTER_MIDDLE, ALIGN_CENTER_BOTTOM,
              ALIGN_RIGHT_TOP, ALIGN_RIGHT_MIDDLE, ALIGN_RIGHT_BOTTOM,
              ALIGN_NONE_NONE, ALIGN_LEFT_NONE, ALIGN_RIGHT_NONE, ALIGN_CENTER_NONE,
              ALIGN_LCENTER_BOTTOM, ALIGN_RCENTER_BOTTOM, ALIGN_LRIGHT_BOTTOM, ALIGN_RLEFT_BOTTOM} align_t;

typedef enum {TEXT_SINGLE_LINE, TEXT_MULTI_LINES} text_mode_t;

typedef enum {OK_ONLY, OK_CANCEL, CANCEL_ONLY, YES_NO, EMPTY_POPUP} popup_type_t;

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


typedef struct TEXTBOX_T {
    uint8_t x, y, width, height, color;
    align_t align;
    const char *text;
    const uint8_t *font;
    uint8_t top_margin, bottom_margin, right_margin, left_margin;
    text_mode_t mode;
} textbox_t;

typedef struct LISTBOX_T {
    uint8_t x, y, width, height, color, type;
    int16_t hover, selected, count, selected_count, page_min_offset;
    char **list;
    const uint8_t *font, *font_highlight;
    uint8_t line_space, line_top_margin, line_bottom_margin;
    uint8_t text_left_margin;
    uint8_t direction;
    uint16_t *selected_ids;
    uint8_t *list_item_flags;
    const char *name;
} listbox_t;

typedef struct OVERLAY_T {
    uint8_t x, y, width, height, color;
    float value_num;
    const uint8_t *font;
    const char *name, *value;
    uint16_t properties;
} overlay_t;

typedef struct BAR_T {
    uint8_t x, y;
    uint8_t color;
    uint8_t width, height;
    int32_t step, steps;
    const char *value;
} bar_t;

typedef struct MENU_BAR_T {
    uint8_t x, y;
    uint8_t color;
    uint8_t width, height;
    float min, max, value;
} menu_bar_t;

typedef struct TOGGLE_T {
    uint8_t x, y;
    uint8_t color;
    uint8_t width, height;
    int32_t value;
    uint8_t inner_border;
} toggle_t;

typedef struct PEAKMETER_T {
    float value, peak;
} peakmeter_t;

typedef struct TUNER_T {
    float frequency;
    const char *note;
    int16_t cents;
    int16_t ref_freq;
    uint8_t input;
} tuner_t;

typedef struct POPUP_T {
    uint8_t x, y, width, height;
    const uint8_t *font;
    const char *title, *content;
    popup_type_t type;
    uint8_t button_selected;
} popup_t;


/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           MACROS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           FUNCTION PROTOTYPES
************************************************************************************************************************
*/

//widgets
void widget_textbox(glcd_t *display, textbox_t *textbox);
void widget_listbox(glcd_t *display, listbox_t *listbox);
void widget_menu_listbox(glcd_t *display, listbox_t *listbox);
void widget_banks_listbox(glcd_t *display, listbox_t *listbox);
void widget_pb_ss_title(glcd_t *display, listbox_t *listbox, const uint8_t *title_font, uint8_t toggle);
void widget_listbox2(glcd_t *display, listbox_t *listbox);
void widget_listbox_overlay(glcd_t *display, listbox_t *listbox);
void widget_listbox_pedalboard(glcd_t *display, listbox_t *listbox, const uint8_t *title_font, uint8_t toggle);
void widget_listbox_pedalboard_draging(glcd_t *display, listbox_t *listbox, const uint8_t *title_font, 
                                       uint8_t toggle, int8_t hold_item_index, const char *hold_item_label);
void widget_add_pb_button(glcd_t *display, uint8_t x, uint8_t y, uint8_t invert);
void widget_foot_overlay(glcd_t *display, overlay_t *overlay);
void widget_list_value(glcd_t *display, listbox_t *listbox);
void widget_bar_encoder(glcd_t *display, bar_t *bar);
void widget_bar(glcd_t *display, menu_bar_t *bar);
void widget_toggle(glcd_t *display, toggle_t *toggle);
void widget_peakmeter(glcd_t *display, uint8_t pkm_id, peakmeter_t *pkm);
void widget_tuner(glcd_t *display, tuner_t *tuner);
void widget_popup(glcd_t *display, popup_t *popup);

//icons. TODO its more efficient to make bitmaps of some these
void icon_snapshot(glcd_t *display, uint8_t x, uint8_t y);
void icon_pedalboard(glcd_t *display, uint8_t x, uint8_t y);
void icon_overlay(glcd_t *display, uint8_t x, uint8_t y);
void icon_bank(glcd_t *display, uint8_t x, uint8_t y);
void icon_pb_selected(glcd_t *display, uint8_t x, uint8_t y);
void icon_bank_selected(glcd_t *display, uint8_t x, uint8_t y);
void icon_pb_grabbed(glcd_t *display, uint8_t x, uint8_t y, uint8_t flip);
void icon_footswitch_groups(glcd_t *display, uint8_t x, uint8_t y);
void icon_pb_checkbox(glcd_t *display, uint8_t x, uint8_t y, uint8_t selected);
void icon_keyboard_invert(glcd_t *display, uint8_t char_index);

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
