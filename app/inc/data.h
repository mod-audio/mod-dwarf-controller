
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef DATA_H
#define DATA_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/
typedef enum {
    MENU_ROOT, 
    MENU_MAIN, 
    MENU_TOGGLE, 
    MENU_LIST, 
    MENU_BAR, 
    MENU_NONE, 
    MENU_OK, 
    MENU_CONFIRM, 
    MENU_CONFIRM2, 
    MENU_TOOL, 
    MENU_FOOT, 
    MENU_CLICK_LIST
} menu_types_t;

enum {MENU_EV_ENTER, MENU_EV_UP, MENU_EV_DOWN, MENU_EV_NONE};

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

typedef struct SCALE_POINT_T {
    char *label;
    float value;
} scale_point_t;

typedef struct CONTROL_T {
    uint8_t hw_id;
    char *label, *unit, *value_string;
    uint16_t properties;
    float value, minimum, maximum;
    int32_t step, steps;
    uint8_t scale_points_count, scale_points_flag;
    scale_point_t **scale_points;
    int16_t scale_point_index;
    uint8_t scroll_dir;
    uint8_t lock_led_actions;
    float screen_indicator_widget_val;
} control_t;

typedef struct LIST_CLONE_T {
    int8_t hw_id;
    int32_t step;
    uint8_t scale_points_count, scale_point_index;
    scale_point_t **scale_points;
} list_clone_t;

typedef struct BP_LIST_T {
    char **names, **uids;
    uint16_t *selected_pb_uids;
    uint8_t selected_count;
    int32_t hover, selected;
    uint16_t page_min, page_max, menu_max;
    uint8_t list_mode;
} bp_list_t;

typedef struct BANK_CONFIG_T {
    uint8_t hw_id;
    uint8_t function;
} bank_config_t;

typedef struct MENU_DESC_T {
    const char *name;
    menu_types_t type;
    int16_t id, parent_id;
    void (*action_cb) (void *data, int event);
    uint8_t need_update;
} menu_desc_t;

typedef struct MENU_DATA_T {
    char **list;
    uint8_t list_count;
    uint8_t selected, hover;
    uint8_t popup_active;
    char *popup_header;
    char *popup_content;

    // FIXME: need to be improved, not all menu items should have this vars (wasting memory)
    float min, max, value, step;
    char *unit_text;
} menu_data_t;

typedef struct MENU_ITEM_T {
    char *name;
    menu_desc_t *desc;
    menu_data_t data;
} menu_item_t;

typedef struct MENU_POPUP_T {
    int16_t menu_id;
    char *popup_header;
    char *popup_content;
} menu_popup_t;

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

control_t * data_parse_control(char **data);
void data_free_control(control_t *control);
bp_list_t *data_parse_banks_list(char **list_data, uint32_t list_count);
void data_free_banks_list(bp_list_t *bp_list);
bp_list_t *data_parse_pedalboards_list(char **list_data, uint32_t list_count);
void data_free_pedalboards_list(bp_list_t *bp_list);


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
