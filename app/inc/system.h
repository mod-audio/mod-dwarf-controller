
/*
************************************************************************************************************************
* This library defines the system related functions, inclusive the system menu callbacks
************************************************************************************************************************
*/

#ifndef SYSTEM_H
#define SYSTEM_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "data.h"
#include <stdbool.h>

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

// system menu callbacks
int16_t system_get_shift_item(uint8_t index);
void system_update_menu_value(uint8_t item_ID, uint16_t value);
uint8_t system_get_clock_source(void);
void system_bluetooth_cb(void *arg, int event);
void system_info_cb(void *arg, int event);
void system_upgrade_cb(void *arg, int event);
void system_inp_0_volume_cb(void *arg, int event);
void system_inp_1_volume_cb(void *arg, int event);
void system_inp_2_volume_cb(void *arg, int event);
void system_outp_0_volume_cb(void *arg, int event);
void system_outp_1_volume_cb(void *arg, int event);
void system_outp_2_volume_cb(void *arg, int event);
void system_hp_volume_cb(void *arg, int event);
void system_save_gains_cb(void *arg, int event);
void system_display_brightness_cb(void *arg, int event);
void system_display_contrast_cb(void *arg, int event);
void system_shift_item_cb(void *arg, int event);
void system_default_tool_cb(void *arg, int event);
void system_control_header_cb(void *arg, int event);
void system_tuner_mute_cb(void *arg, int event);
void system_tuner_input_cb(void *arg, int event);
void system_play_cb (void *arg, int event);
void system_taptempo_cb (void *arg, int event);
void system_click_list_cb(void *arg, int event);
void system_midi_src_cb (void *arg, int event);
void system_midi_send_cb (void *arg, int event);
void system_ss_prog_change_cb (void *arg, int event);
void system_pb_prog_change_cb (void *arg, int event);
void system_tempo_cb (void *arg, int event);
void system_bpb_cb (void *arg, int event);
void system_bypass_cb (void *arg, int event);
void system_load_pro_cb(void *arg, int event);
void system_save_pro_cb(void *arg, int event);
void system_hide_actuator_cb(void *arg, int event);
void system_usb_mode_cb(void *arg, int event);
void system_shift_mode_cb(void *arg, int event);
void system_noise_removal_cb(void *arg, int event);

//system plugins
void system_noisegate_channel_cb(void *arg, int event);
void system_noisegate_thres_cb(void *arg, int event);
void system_noisegate_decay_cb(void *arg, int event);
void system_comp_mode_cb(void *arg, int event);
void system_comp_release_cb(void *arg, int event);
void system_comp_pb_vol_cb(void *arg, int event);

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
