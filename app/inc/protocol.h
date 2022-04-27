
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "mod-protocol.h"
#include "ui_comm.h"


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

// defines the function to send responses to sender
#define SEND_TO_SENDER(id,msg,len)      (id == SYSTEM_SERIAL) ? sys_comm_send(msg,NULL) : ui_comm_webgui_send(msg,len)

/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

// This struct is used on callbacks argument
typedef struct PROTO_T {
    char **list;
    uint32_t list_count;
    char *response;
    uint32_t response_size;
} proto_t;

// This struct must be used to pass a message to protocol parser
typedef struct MSG_T {
    int sender_id;
    char *data;
    uint32_t data_size;
} msg_t;


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
void protocol_init(void);
void protocol_parse(msg_t *msg);
void protocol_add_command(const char *command, void (*callback)(uint8_t serial_id, proto_t *proto));
void protocol_response(const char *response, proto_t *proto);
void protocol_remove_commands(void);

void cb_ping(uint8_t serial_id, proto_t *proto);
void cb_say(uint8_t serial_id, proto_t *proto);
void cb_led(uint8_t serial_id, proto_t *proto);
void cb_disp_brightness(uint8_t serial_id, proto_t *proto);
void cb_change_assigned_led_blink(uint8_t serial_id, proto_t *proto);
void cb_change_assigned_led_brightness(uint8_t serial_id, proto_t *proto);
void cb_change_assigment_name(uint8_t serial_id, proto_t *proto);
void cb_change_assigment_value(uint8_t serial_id, proto_t *proto);
void cb_change_assigment_unit(uint8_t serial_id, proto_t *proto);
void cb_change_widget_indicator(uint8_t serial_id, proto_t *proto);
void cb_launch_popup(uint8_t serial_id, proto_t *proto);
void cb_glcd_text(uint8_t serial_id, proto_t *proto);
void cb_glcd_dialog(uint8_t serial_id, proto_t *proto);
void cb_glcd_draw(uint8_t serial_id, proto_t *proto);
void cb_gui_connection(uint8_t serial_id, proto_t *proto);
void cb_control_add(uint8_t serial_id, proto_t *proto);
void cb_control_rm(uint8_t serial_id, proto_t *proto);
void cb_control_set(uint8_t serial_id, proto_t *proto);
void cb_control_get(uint8_t serial_id, proto_t *proto);
void cb_initial_state(uint8_t serial_id, proto_t *proto);
void cb_tuner(uint8_t serial_id, proto_t *proto);
void cb_resp(uint8_t serial_id, proto_t *proto);
void cb_restore(uint8_t serial_id, proto_t *proto);
void cb_boot(uint8_t serial_id, proto_t *proto);
void cb_set_selftest_control_skip(uint8_t serial_id, proto_t *proto);
void cb_menu_item_changed(uint8_t serial_id, proto_t *proto);
void cb_pedalboard_clear(uint8_t serial_id, proto_t *proto);
void cb_pedalboard_name(uint8_t serial_id, proto_t *proto);
void cb_snapshot_name(uint8_t serial_id, proto_t *proto);
void cb_pages_available(uint8_t serial_id, proto_t *proto);
void cb_clear_eeprom(uint8_t serial_id, proto_t *proto);
void cb_set_pb_gain(uint8_t serial_id, proto_t *proto);
void cb_pedalboard_change(uint8_t serial_id, proto_t *proto);

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
