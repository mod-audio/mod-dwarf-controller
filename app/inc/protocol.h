
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
#define SEND_TO_SENDER(id,msg,len)      ui_comm_webgui_send(msg,len)

// special "flag" to indicate banks control (last available bitmask value for a 16bit integer)
#define FLAG_CONTROL_BANKS 0x4000

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
void protocol_add_command(const char *command, void (*callback)(proto_t *proto));
void protocol_response(const char *response, proto_t *proto);
void protocol_remove_commands(void);

void cb_ping(proto_t *proto);
void cb_say(proto_t *proto);
void cb_led(proto_t *proto);
void cb_glcd_text(proto_t *proto);
void cb_glcd_dialog(proto_t *proto);
void cb_glcd_draw(proto_t *proto);
void cb_gui_connection(proto_t *proto);
void cb_control_add(proto_t *proto);
void cb_control_rm(proto_t *proto);
void cb_control_set(proto_t *proto);
void cb_control_get(proto_t *proto);
void cb_control_set_index(proto_t *proto);
void cb_initial_state(proto_t *proto);
void cb_bank_config(proto_t *proto);
void cb_tuner(proto_t *proto);
void cb_resp(proto_t *proto);
void cb_restore(proto_t *proto);
void cb_boot(proto_t *proto);
void cb_menu_item_changed(proto_t *proto);
void cb_pedalboard_clear(proto_t *proto);
void cb_pedalboard_name(proto_t *proto);
void cb_snapshot_name(proto_t *proto);

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
