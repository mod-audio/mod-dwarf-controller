
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
void system_lock_comm_serial(bool busy);
void system_update_menu_value(uint8_t item_ID, uint16_t value);
uint8_t system_get_current_profile(void);
void system_true_bypass_cb(void *arg, int event);
void system_pedalboard_cb(void *arg, int event);
void system_bluetooth_cb(void *arg, int event);
void system_services_cb(void *arg, int event);
void system_restart_jack_cb(void *arg, int event);
void system_restart_host_cb(void *arg, int event);
void system_restart_ui_cb(void *arg, int event);
void system_restart_bluez_cb(void *arg, int event);
void system_versions_cb(void *arg, int event);
void system_release_cb(void *arg, int event);
void system_device_cb(void *arg, int event);
void system_tag_cb(void *arg, int event);
void system_upgrade_cb(void *arg, int event);
void system_volume_cb(void *arg, int event);
void system_save_gains_cb(void *arg, int event);
void system_banks_cb(void *arg, int event);
void system_display_cb(void *arg, int event);
void system_sl_in_cb (void *arg, int event);
void system_sl_out_cb (void *arg, int event);
void system_hp_bypass_cb (void *arg, int event);
void system_tuner_cb (void *arg, int event);
void system_play_cb (void *arg, int event);
void system_quick_bypass_cb (void *arg, int event);
void system_midi_src_cb (void *arg, int event);
void system_midi_send_cb (void *arg, int event);
void system_ss_prog_change_cb (void *arg, int event);
void system_pb_prog_change_cb (void *arg, int event);
void system_tempo_cb (void *arg, int event);
void system_bpb_cb (void *arg, int event);
void system_bypass_cb (void *arg, int event);
void system_load_pro_cb(void *arg, int event);
void system_save_pro_cb(void *arg, int event);
void system_qbp_channel_cb (void *arg, int event);


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
