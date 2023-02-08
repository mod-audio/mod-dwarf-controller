
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef MODE_POPUP_H
#define MODE_POPUP_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

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

#define NAMING_WIDGET_TXT       "\n\n\n\nKnob 1/2 turn  - pick char\nKnob 1/2 click - input char\nKnob  3  turn  - cursor pos"
#define NAMING_WIDGET_BTN1      "save"
#define NAMING_WIDGET_BTN2      "clear"
#define NAMING_WIDGET_BTN3      "cancel"

#define CONFIRM_CANCEL_BTN1     "confirm"
#define CONFIRM_CANCEL_BTN2     "-"
#define CONFIRM_CANCEL_BTN3     "cancel"

#define POPUP_SAVE_SELECT_ID    0
#define POPUP_SAVE_SELECT_MSG   "\n\nWhat would you like to save?"
#define PBTN1_SAVE_SELECT       "PB"
#define PBTN2_SAVE_SELECT       "snapsht"
#define PBTN3_SAVE_SELECT       "cancel"

#define POPUP_SAVE_PB_ID        1
#define POPUP_SAVE_PB_MSG       NAMING_WIDGET_TXT
#define PBTN1_SAVE_PB           NAMING_WIDGET_BTN1
#define PBTN2_SAVE_PB           NAMING_WIDGET_BTN2
#define PBTN3_SAVE_PB           NAMING_WIDGET_BTN3

#define POPUP_SAVE_SS_ID        2
#define POPUP_SAVE_SS_MSG       NAMING_WIDGET_TXT
#define PBTN1_SAVE_SS           NAMING_WIDGET_BTN1
#define PBTN2_SAVE_SS           NAMING_WIDGET_BTN2
#define PBTN3_SAVE_SS           NAMING_WIDGET_BTN3

#define POPUP_REMOVE_PB_ID      3
#define POPUP_REMOVE_PB_MSG     "\n\nare you sure you want\nto remove the selected\npedalboard from this bank?"
#define PBTN1_REMOVE_PB         CONFIRM_CANCEL_BTN1
#define PBTN2_REMOVE_PB         CONFIRM_CANCEL_BTN2
#define PBTN3_REMOVE_PB         CONFIRM_CANCEL_BTN3

#define POPUP_REMOVE_SS_ID      4
#define POPUP_REMOVE_SS_MSG     "\n\nare you sure you want\nto delete the current\nsnapshot?"
#define PBTN1_REMOVE_SS         CONFIRM_CANCEL_BTN1
#define PBTN2_REMOVE_SS         CONFIRM_CANCEL_BTN2
#define PBTN3_REMOVE_SS         CONFIRM_CANCEL_BTN3

#define POPUP_OVERWRITE_PB_ID   5
#define POPUP_OVERWRITE_PB_MSG  "\n\nare you sure you want\nto overwrite the current\npedalboard?"
#define PBTN1_OVERWRITE_PB      CONFIRM_CANCEL_BTN1
#define PBTN2_OVERWRITE_PB      CONFIRM_CANCEL_BTN2
#define PBTN3_OVERWRITE_PB      CONFIRM_CANCEL_BTN3

#define POPUP_OVERWRITE_SS_ID   6
#define POPUP_OVERWRITE_SS_MSG  "\n\nare you sure you want\nto overwrite the current\nsnapshot?"
#define PBTN1_OVERWRITE_SS      CONFIRM_CANCEL_BTN1
#define PBTN2_OVERWRITE_SS      CONFIRM_CANCEL_BTN2
#define PBTN3_OVERWRITE_SS      CONFIRM_CANCEL_BTN3

#define POPUP_NEW_BANK_ID       7
#define POPUP_NEW_BANK_MSG      NAMING_WIDGET_TXT
#define PBTN1_NEW_BANK          NAMING_WIDGET_BTN1
#define PBTN2_NEW_BANK          NAMING_WIDGET_BTN2
#define PBTN3_NEW_BANK          NAMING_WIDGET_BTN3

#define POPUP_DELETE_BANK_ID    8
#define POPUP_DELETE_BANK_MSG   "\n\n\nare you sure you want\nto delete the selected bank?"
#define PBTN1_DELETE_BANK       CONFIRM_CANCEL_BTN1
#define PBTN2_DELETE_BANK       CONFIRM_CANCEL_BTN2
#define PBTN3_DELETE_BANK       CONFIRM_CANCEL_BTN3

#define POPUP_EMPTY_NAME_ID    9
#define POPUP_EMPTY_NAME_MSG   "empty name not allowed"
#define PBTN1_EMPTY_NAME       "ok"
#define PBTN2_EMPTY_NAME       CONFIRM_CANCEL_BTN2
#define PBTN3_EMPTY_NAME       CONFIRM_CANCEL_BTN3

#define POPUP_TRAIL_PB_ID    10
#define POPUP_TRAIL_PB_MSG   "This Pedalboard has a plugin in trail mode, audio will drop out every x seconds"
#define PBTN1_TRAIL_PB       "ok"
#define PBTN2_TRAIL_PB       CONFIRM_CANCEL_BTN2
#define PBTN3_TRAIL_PB       CONFIRM_CANCEL_BTN2

#define DUPLICATE_BANK_ID    11
#define DUPLICATE_BANK_MSG   "A bank with this name already exists. Please choose a \ndifferent name"
#define PBTN1_DUPL_BANK      "ok"
#define PBTN2_DUPL_BANK      CONFIRM_CANCEL_BTN2
#define PBTN3_DUPL_BANK      CONFIRM_CANCEL_BTN3

    /*ID,                     POPUP TITLE,            POPUP TEXT,              POPUP BUTTON TEXT,                           val, cursor, btns, naming widget, txt input*/
#define SYSTEM_POPUPS     \
    {POPUP_SAVE_SELECT_ID   ,"What to save?",         POPUP_SAVE_SELECT_MSG,   PBTN1_SAVE_SELECT,  PBTN2_SAVE_SELECT,  PBTN3_SAVE_SELECT,  0,  0,  3,  0, NULL },  \
    {POPUP_SAVE_PB_ID       ,"Save pedalboard",       POPUP_SAVE_PB_MSG,       PBTN1_SAVE_PB,      PBTN2_SAVE_PB,      PBTN3_SAVE_PB,      0,  0,  3,  1, NULL },  \
    {POPUP_SAVE_SS_ID       ,"Save snapshot",         POPUP_SAVE_SS_MSG,       PBTN1_SAVE_SS,      PBTN2_SAVE_SS,      PBTN3_SAVE_SS,      0,  0,  3,  1, NULL },  \
    {POPUP_REMOVE_PB_ID     ,"Remove pedalboard",     POPUP_REMOVE_PB_MSG,     PBTN1_REMOVE_PB,    PBTN2_REMOVE_PB,    PBTN3_REMOVE_PB,    0,  0,  2,  0, NULL },  \
    {POPUP_REMOVE_SS_ID     ,"Remove snapshot",       POPUP_REMOVE_SS_MSG,     PBTN1_REMOVE_SS,    PBTN2_REMOVE_SS,    PBTN3_REMOVE_SS,    0,  0,  2,  0, NULL },  \
    {POPUP_OVERWRITE_PB_ID  ,"overwrite pedalboard",  POPUP_OVERWRITE_PB_MSG,  PBTN1_OVERWRITE_PB, PBTN2_OVERWRITE_PB, PBTN3_OVERWRITE_PB, 0,  0,  2,  0, NULL },  \
    {POPUP_OVERWRITE_SS_ID  ,"overwrite snapshot",    POPUP_OVERWRITE_SS_MSG,  PBTN1_OVERWRITE_SS, PBTN2_OVERWRITE_SS, PBTN3_OVERWRITE_SS, 0,  0,  2,  0, NULL },  \
    {POPUP_NEW_BANK_ID      ,"Create bank",           POPUP_NEW_BANK_MSG,      PBTN1_NEW_BANK,     PBTN2_NEW_BANK,     PBTN3_NEW_BANK,     0,  0,  3,  1, NULL },  \
    {POPUP_DELETE_BANK_ID   ,"Delete bank",           POPUP_DELETE_BANK_MSG,   PBTN1_DELETE_BANK,  PBTN2_DELETE_BANK,  PBTN3_DELETE_BANK,  0,  0,  2,  0, NULL },  \
    {POPUP_EMPTY_NAME_ID    ,"Invalid input",         POPUP_EMPTY_NAME_MSG,    PBTN1_EMPTY_NAME,   PBTN2_EMPTY_NAME,   PBTN3_EMPTY_NAME,   0,  0,  2,  0, NULL },  \
    {POPUP_TRAIL_PB_ID      ,"Trail Plugin",          POPUP_TRAIL_PB_MSG,      PBTN1_TRAIL_PB,     PBTN2_TRAIL_PB,     PBTN3_TRAIL_PB,     0,  0,  1,  0, NULL },  \
    {DUPLICATE_BANK_ID      ,"Bank Already exists",   DUPLICATE_BANK_MSG,      PBTN1_DUPL_BANK,    PBTN2_DUPL_BANK,    PBTN3_DUPL_BANK,    0,  0,  2,  0, NULL },  \
/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

typedef struct SYSTEM_POPUP_T {
    uint8_t id;
    const char *title;
    const char *popup_text;
    const char *btn1_txt, *btn2_txt, *btn3_txt;
    uint8_t cursor_index;
    int8_t button_value;
    const uint8_t button_max, has_naming_input;
    char *input_name;
} system_popup_t;

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

void PM_init(void);
void PM_enter(uint8_t encoder);
void PM_up(uint8_t encoder);
void PM_down(uint8_t encoder);
void PM_set_state(void);
void PM_print_screen(void);
void PM_set_leds(void);
void PM_button_pressed(uint8_t button);
void PM_launch_popup(uint8_t popup_id);
void PM_launch_attention_overlay(const char *message, void (*timeout_cb));
uint8_t PM_get_current_popup(void);

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
