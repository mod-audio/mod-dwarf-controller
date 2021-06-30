
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

#define NAMING_WIDGET_TXT       "Knob 1 turn - select posistion\nKnob 1 click - toggle keyboard\nButton 2 - clear name"
#define NAMING_WIDGET_BTN1      "save"
#define NAMING_WIDGET_BTN2      "clear"
#define NAMING_WIDGET_BTN3      "cancel"

#define CONFIRM_CANCEL_BTN1     "confirm"
#define CONFIRM_CANCEL_BTN2     "-"
#define CONFIRM_CANCEL_BTN3     "cancel"

#define POPUP_SAVE_SELECT_ID    0
#define POPUP_SAVE_SELECT_MSG   "\n\nWhat whould you like to save?"
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
#define POPUP_REMOVE_PB_MSG     "are you sure you wish\nto delete the current\npedalboard from this bank?"
#define PBTN1_REMOVE_PB         CONFIRM_CANCEL_BTN1
#define PBTN2_REMOVE_PB         CONFIRM_CANCEL_BTN2
#define PBTN3_REMOVE_PB         CONFIRM_CANCEL_BTN3

#define POPUP_REMOVE_SS_ID      4
#define POPUP_REMOVE_SS_MSG     "are you sure you wish\nto delete the current\nsnapshot?"
#define PBTN1_REMOVE_SS         CONFIRM_CANCEL_BTN1
#define PBTN2_REMOVE_SS         CONFIRM_CANCEL_BTN2
#define PBTN3_REMOVE_SS         CONFIRM_CANCEL_BTN3

#define POPUP_OVERWRITE_PB_ID   5
#define POPUP_OVERWRITE_PB_MSG  "are you sure you wish\nto overwrite the current\npedalboard?"
#define PBTN1_OVERWRITE_PB      CONFIRM_CANCEL_BTN1
#define PBTN2_OVERWRITE_PB      CONFIRM_CANCEL_BTN2
#define PBTN3_OVERWRITE_PB      CONFIRM_CANCEL_BTN3

#define POPUP_OVERWRITE_SS_ID   6
#define POPUP_OVERWRITE_SS_MSG  "are you sure you wish\nto overwrite the current\nnapshot?"
#define PBTN1_OVERWRITE_SS      CONFIRM_CANCEL_BTN1
#define PBTN2_OVERWRITE_SS      CONFIRM_CANCEL_BTN2
#define PBTN3_OVERWRITE_SS      CONFIRM_CANCEL_BTN3

#define POPUP_NEW_BANK_ID       7
#define POPUP_NEW_BANK_MSG      NAMING_WIDGET_TXT
#define PBTN1_DELETE_BANK       CONFIRM_CANCEL_BTN1
#define PBTN2_DELETE_BANK       CONFIRM_CANCEL_BTN2
#define PBTN3_DELETE_BANK       CONFIRM_CANCEL_BTN3

#define POPUP_DELETE_BANK_ID    8
#define POPUP_DELETE_BANK_MSG   "are you sure you wish\nto delete this bank?"
#define PBTN1_NEW_BANK          NAMING_WIDGET_BTN1
#define PBTN2_NEW_BANK          NAMING_WIDGET_BTN2
#define PBTN3_NEW_BANK          NAMING_WIDGET_BTN3

    /*POPUP TITLE,            POPUP TEXT,              POPUP BUTTON TEXT,   val, btns, naming widget, txt input*/ 
#define SYSTEM_POPUPS     \
    {"What to save?",         POPUP_SAVE_SELECT_MSG,   PBTN1_SAVE_SELECT,  PBTN2_SAVE_SELECT,  PBTN3_SAVE_SELECT,  0,  3,  0, NULL },  \
    {"Save pedalboard",       POPUP_SAVE_PB_MSG,       PBTN1_SAVE_PB,      PBTN2_SAVE_PB,      PBTN3_SAVE_PB,      0,  3,  1, NULL },  \
    {"Save snapshot",         POPUP_SAVE_SS_MSG,       PBTN1_SAVE_SS,      PBTN2_SAVE_SS,      PBTN3_SAVE_SS,      0,  3,  1, NULL },  \
    {"Remove pedalboard",     POPUP_REMOVE_PB_MSG,     PBTN1_REMOVE_PB,    PBTN2_REMOVE_PB,    PBTN3_REMOVE_PB,    0,  2,  0, NULL },  \
    {"Remove snapshot",       POPUP_REMOVE_SS_MSG,     PBTN1_REMOVE_SS,    PBTN2_REMOVE_SS,    PBTN3_REMOVE_SS,    0,  2,  0, NULL },  \
    {"overwrite pedalboard",  POPUP_OVERWRITE_PB_MSG,  PBTN1_OVERWRITE_PB, PBTN2_OVERWRITE_PB, PBTN3_OVERWRITE_PB, 0,  2,  0, NULL },  \
    {"overwrite snapshot",    POPUP_OVERWRITE_SS_MSG,  PBTN1_OVERWRITE_SS, PBTN2_OVERWRITE_SS, PBTN3_OVERWRITE_SS, 0,  2,  0, NULL },  \
    {"Create new bank",       POPUP_NEW_BANK_MSG,      PBTN1_NEW_BANK,     PBTN2_NEW_BANK,     PBTN3_NEW_BANK,     0,  3,  1, NULL },  \
    {"Delete bank",           POPUP_DELETE_BANK_MSG,   PBTN1_DELETE_BANK,  PBTN2_DELETE_BANK,  PBTN3_DELETE_BANK,  0,  2,  0, NULL },  \

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

void PM_init(void);
void PM_enter(void);
void PM_up(void);
void PM_down(void);
void PM_set_state(void);
void PM_print_screen(void);
void PM_set_leds(void);
void PM_button_pressed(uint8_t button);
void PM_launch_popup(uint8_t popup_id);
void PM_launch_attention_overlay(char *message, void (*timeout_cb));

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
