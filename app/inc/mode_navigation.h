
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef MODE_NAVIGATION_H
#define MODE_NAVIGATION_H


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

//flag to indicate we are adding banks, not pbs
#define ADD_FULL_BANKS -1

/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

//list modes
enum {BANKS_LIST, PEDALBOARD_LIST, SNAPSHOT_LIST, LIST_POPUP, PB_LIST_BEGINNING_BOX,
	  PB_LIST_BEGINNING_BOX_SELECTED, BANK_LIST_CHECKBOXES, BANK_LIST_CHECKBOXES_ENGAGED,
	  PB_LIST_CHECKBOXES, PB_LIST_CHECKBOXES_ENGAGED};

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

void NM_init(void);
void NM_clear(void);
void NM_initial_state(uint16_t max_menu, uint16_t page_min, uint16_t page_max, char *bank_uid, char *pedalboard_uid, char **pedalboards_list);
void NM_enter(void);
void NM_encoder_hold(uint8_t encoder);
void NM_encoder_released(uint8_t encoder);
uint8_t NM_up(void);
uint8_t NM_down(void);
void NM_set_banks(bp_list_t *bp_list);
bp_list_t *NM_get_banks(void);
void NM_set_pedalboards(bp_list_t *bp_list);
bp_list_t *NM_get_pedalboards(void);
void NM_set_current_list(uint8_t list_type);
uint8_t NM_get_current_list(void);
void NM_toggle_mode(void);
void NM_update_lists(uint8_t list_type);
void NM_print_screen();
void NM_print_prev_screen(void);
void NM_set_leds(void);
void NM_button_pressed(uint8_t button);
void NM_change_pbss(uint8_t next_prev, uint8_t pressed);
void NM_toggle_pb_ss(void);
void NM_load_selected(void);
uint16_t NM_get_current_selected(uint8_t list_type);
void NM_set_last_selected(uint8_t list_type);
void NM_set_selected_index(uint8_t list_type, int16_t index);
uint16_t NM_get_current_hover(uint8_t list_type);
uint16_t NM_get_list_count(uint8_t list_type);
void NM_save_pbss_name(const void *data, uint8_t pb_ss_toggle);
char* NM_get_pbss_name(uint8_t pb_ss_toggle);

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
