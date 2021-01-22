
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <string.h>
#include <stdlib.h>

#include "protocol.h"
#include "utils.h"
#include "naveg.h"
#include "ledz.h"
#include "hardware.h"
#include "glcd.h"
#include "utils.h"
#include "screen.h"
#include "cli.h"
#include "ui_comm.h"
#include "sys_comm.h"
#include "mod-protocol.h"
#include "mode_control.h"
#include "mode_navigation.h"
#include "mode_tools.h"

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define NOT_FOUND           (-1)
#define MANY_ARGUMENTS      (-2)
#define FEW_ARGUMENTS       (-3)
#define INVALID_ARGUMENT    (-4)


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

const char *g_error_messages[] = {
    RESP_ERR_COMMAND_NOT_FOUND,
    RESP_ERR_MANY_ARGUMENTS,
    RESP_ERR_FEW_ARGUMENTS,
    RESP_ERR_INVALID_ARGUMENT
};


/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/

typedef struct CMD_T {
    char* command;
    char** list;
    uint32_t count;
    void (*callback)(uint8_t serial_id, proto_t *proto);
} cmd_t;


/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static unsigned int g_command_count = 0;
static cmd_t g_commands[COMMAND_COUNT_DUO];


/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL CONFIGURATION ERRORS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           LOCAL FUNCTIONS
************************************************************************************************************************
*/

static int is_wildcard(const char *str)
{
    if (str && strchr(str, '%') != NULL) return 1;
    return 0;
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void protocol_parse(msg_t *msg)
{
    uint32_t i, j;
    int32_t index = NOT_FOUND;
    proto_t proto;

    proto.list = strarr_split(msg->data, ' ');
    proto.list_count = strarr_length(proto.list);
    proto.response = NULL;

    // TODO: check invalid argumets (wildcards)

    if (proto.list_count == 0) 
    {
        FREE(proto.list);
        return;
    }

    unsigned int match, variable_arguments = 0;

    // loop all registered commands
    for (i = 0; i < g_command_count; i++)
    {
        match = 0;
        index = NOT_FOUND;

        // checks received protocol
        for (j = 0; j < proto.list_count && j < g_commands[i].count; j++)
        {
            if (strcmp(g_commands[i].list[j], proto.list[j]) == 0)
            {
                match++;
            }
            else if (match > 0)
            {
                if (is_wildcard(g_commands[i].list[j])) match++;
                else if (strcmp(g_commands[i].list[j], "...") == 0)
                {
                    match++;
                    variable_arguments = 1;
                }
            }
        }

        if (match > 0)
        {
            // checks if the last argument is ...
            if (j < g_commands[i].count)
            {
                if (strcmp(g_commands[i].list[j], "...") == 0) variable_arguments = 1;
            }

            // few arguments
            if (proto.list_count < (g_commands[i].count - variable_arguments))
            {
                index = FEW_ARGUMENTS;
            }

            // many arguments
            else if (proto.list_count > g_commands[i].count && !variable_arguments)
            {
                index = MANY_ARGUMENTS;
            }

            // arguments match
            else if (match == proto.list_count || variable_arguments)
            {
                index = i;
            }

            break;
        }
    }

    // Protocol OK
    if (index >= 0)
    {
        if (g_commands[index].callback)
        {
            g_commands[index].callback( msg->sender_id, &proto);

            if (proto.response)
            {
                SEND_TO_SENDER(msg->sender_id, proto.response, proto.response_size);

                // The free is not more necessary because response is not more allocated dynamically
                // uncomment bellow line if this change in the future
                // FREE(proto.response);
            }
        }
    }
    // Protocol error
    else
    {
        //SEND_TO_SENDER(msg->sender_id, g_error_messages[-index-1], strlen(g_error_messages[-index-1]));
        SEND_TO_SENDER(msg->sender_id, msg->data, strlen(msg->data));
    }

    FREE(proto.list);
}


void protocol_add_command(const char *command, void (*callback)(uint8_t serial_id, proto_t *proto))
{
    if (g_command_count >= COMMAND_COUNT_DUO) while (1);

    char *cmd = str_duplicate(command);
    g_commands[g_command_count].command = cmd;
    g_commands[g_command_count].list = strarr_split(cmd, ' ');
    g_commands[g_command_count].count = strarr_length(g_commands[g_command_count].list);
    g_commands[g_command_count].callback = callback;
    g_command_count++;
}

void protocol_response(const char *response, proto_t *proto)
{
    static char response_buffer[32];

    proto->response = response_buffer;

    proto->response_size = strlen(response);
    if (proto->response_size >= sizeof(response_buffer))
        proto->response_size = sizeof(response_buffer) - 1;

    strncpy(response_buffer, response, sizeof(response_buffer)-1);
    response_buffer[proto->response_size] = 0;
}

void protocol_send_response(const char *response, const uint8_t value ,proto_t *proto)
{
    char buffer[20];
    uint8_t i = 0;
    memset(buffer, 0, sizeof buffer);

    i = copy_command(buffer, response); 
    
    // insert the value on buffer
    i += int_to_str(value, &buffer[i], sizeof(buffer) - i, 0);

    protocol_response(&buffer[0], proto);
}

void protocol_remove_commands(void)
{
    unsigned int i;

    for (i = 0; i < g_command_count; i++)
    {
        FREE(g_commands[i].command);
        FREE(g_commands[i].list);
    }
}

//initialize all protocol commands
void protocol_init(void)
{
    // protocol definitions
    protocol_add_command(CMD_PING, cb_ping);
    protocol_add_command(CMD_SAY, cb_say);
    protocol_add_command(CMD_LED, cb_led);
    protocol_add_command(CMD_GLCD_TEXT, cb_glcd_text);
    protocol_add_command(CMD_GLCD_DIALOG, cb_glcd_dialog);
    protocol_add_command(CMD_GLCD_DRAW, cb_glcd_draw);
    protocol_add_command(CMD_GUI_CONNECTED, cb_gui_connection);
    protocol_add_command(CMD_GUI_DISCONNECTED, cb_gui_connection);
    protocol_add_command(CMD_CONTROL_ADD, cb_control_add);
    protocol_add_command(CMD_CONTROL_REMOVE, cb_control_rm);
    protocol_add_command(CMD_CONTROL_SET, cb_control_set);
    protocol_add_command(CMD_CONTROL_GET, cb_control_get);
    protocol_add_command(CMD_INITIAL_STATE, cb_initial_state);
    protocol_add_command(CMD_DUO_BANK_CONFIG, cb_bank_config);
    protocol_add_command(CMD_TUNER, cb_tuner);
    protocol_add_command(CMD_RESPONSE, cb_resp);
    protocol_add_command(CMD_RESTORE, cb_restore);
    protocol_add_command(CMD_DUO_BOOT, cb_boot);
    protocol_add_command(CMD_MENU_ITEM_CHANGE, cb_menu_item_changed);
    protocol_add_command(CMD_PEDALBOARD_CLEAR, cb_pedalboard_clear);
    protocol_add_command(CMD_PEDALBOARD_NAME_SET, cb_pedalboard_name);
    protocol_add_command(CMD_SNAPSHOT_NAME_SET, cb_snapshot_name);
    protocol_add_command(CMD_DWARF_PAGES_AVAILABLE, cb_pages_available);
    protocol_add_command(CMD_SELFTEST_SKIP_CONTROL_ENABLE, cb_set_selftest_control_skip);
}

/*
************************************************************************************************************************
*           PROTOCOL CALLBACKS
************************************************************************************************************************
*/

void cb_ping(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    g_ui_communication_started = 1;
    g_protocol_busy = false;
    protocol_send_response(CMD_RESPONSE, 0, proto);
}

void cb_say(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    protocol_response(proto->list[1], proto);
}

void cb_led(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    ledz_t *led = hardware_leds(atoi(proto->list[1]));

    uint8_t value[3] = {atoi(proto->list[2]), atoi(proto->list[3]), atoi(proto->list[4])}; 
    ledz_set_color(MAX_COLOR_ID,value);

    if (proto->list_count == 7)
    {
        led->led_state.color = MAX_COLOR_ID;
        ledz_set_state(led, LED_ON);
    }
    else
    {
        led->led_state.color = MAX_COLOR_ID;
        led->led_state.time_on = atoi(proto->list[5]);
        led->led_state.time_off = atoi(proto->list[6]);
        led->led_state.amount_of_blinks = LED_BLINK_INFINIT;
        ledz_set_state(led, LED_BLINK);
    }

    protocol_send_response(CMD_RESPONSE, 0, proto);
}

void cb_glcd_text(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    uint8_t glcd_id, x, y;
    glcd_id = atoi(proto->list[1]);
    x = atoi(proto->list[2]);
    y = atoi(proto->list[3]);

    if (glcd_id >= GLCD_COUNT) return;

    screen_text_box(glcd_id, x, y, proto->list[4]);
    protocol_send_response(CMD_RESPONSE, 0, proto);
}

void cb_glcd_dialog(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    uint8_t val = naveg_dialog(proto->list[1]);
    protocol_send_response(CMD_RESPONSE, val, proto);
}

void cb_glcd_draw(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    uint8_t glcd_id, x, y;
    glcd_id = atoi(proto->list[1]);
    x = atoi(proto->list[2]);
    y = atoi(proto->list[3]);

    if (glcd_id >= GLCD_COUNT) return;

    uint8_t msg_buffer[WEBGUI_COMM_RX_BUFF_SIZE];

    str_to_hex(proto->list[4], msg_buffer, WEBGUI_COMM_RX_BUFF_SIZE);
    glcd_draw_image(hardware_glcds(glcd_id), x, y, msg_buffer, GLCD_BLACK);

    protocol_send_response(CMD_RESPONSE, 0, proto);
}

void cb_gui_connection(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    //lock actuators
    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);
    //clear the buffer so we dont send any messages
    ui_comm_webgui_clear();

    if (strcmp(proto->list[0], CMD_GUI_CONNECTED) == 0)
        naveg_ui_connection(UI_CONNECTED);
    else
        naveg_ui_connection(UI_DISCONNECTED);

    //we are done supposedly closing the menu, we can unlock the actuators
    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);

    protocol_send_response(CMD_RESPONSE, 0, proto);
}

void cb_control_add(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    //lock actuators
    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    control_t *control = data_parse_control(proto->list);

    CM_add_control(control, 1);

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
    protocol_send_response(CMD_RESPONSE, 0, proto);
}

void cb_control_rm(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    g_ui_communication_started = 1;
    
    CM_remove_control(atoi(proto->list[1]));

    uint8_t i;
    for (i = 2; i < TOTAL_ACTUATORS + 1; i++)
    {
        if ((proto->list[i]) != NULL)
        {
            CM_remove_control(atoi(proto->list[i]));
        }
        else break;
    }
    
    protocol_send_response(CMD_RESPONSE, 0, proto);
}

void cb_control_set(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    //lock actuators
    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    if (hardware_get_overlay_counter())
        g_actuator_display_lock = 1;

    CM_set_control(atoi(proto->list[1]), atof(proto->list[2]));

    g_actuator_display_lock = 0;

    protocol_send_response(CMD_RESPONSE, 0, proto);

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

void cb_control_get(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    float value;
    value = CM_get_control_value(atoi(proto->list[1]));

    char resp[32];
    protocol_send_response(CMD_RESPONSE, 0, proto);

    float_to_str(value, &resp[strlen(resp)], 8, 3);
    protocol_response(resp, proto);
}

void cb_initial_state(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    g_ui_communication_started = 1;
    NM_initial_state(atoi(proto->list[1]), atoi(proto->list[2]), atoi(proto->list[3]), proto->list[4], proto->list[5], &(proto->list[6]));
    protocol_send_response(CMD_RESPONSE, 0, proto);
}

//MDW TODO, NOT USED PLEASE REMOVE
void cb_bank_config(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);
/*
    bank_config_t bank_func;
    bank_func.hw_id = atoi(proto->list[1]);
    bank_func.function = atoi(proto->list[2]);
    naveg_bank_config(&bank_func);*/
    protocol_send_response(CMD_RESPONSE, 0, proto);

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

void cb_tuner(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    screen_update_tuner(atof(proto->list[1]), proto->list[2], atoi(proto->list[3]));
    protocol_send_response(CMD_RESPONSE, 0, proto);
}

void cb_resp(uint8_t serial_id, proto_t *proto)
{
    if (serial_id == SYSTEM_SERIAL)
        sys_comm_response_cb(proto->list);
    else
        ui_comm_webgui_response_cb(proto->list);
}

void cb_restore(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    //clear screen
    screen_clear();

    cli_restore(RESTORE_INIT);
    protocol_send_response(CMD_RESPONSE, 0, proto);
}

//MDW TODO, see whats needed here
void cb_boot(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    g_self_test_mode = false;
    g_self_test_cancel_button = false;

    g_should_wait_for_webgui = true;

    //set the tuner mute state 
    //system_update_menu_value(MENU_ID_TUNER_MUTE, atoi(proto->list[1]));

    //set the current user profile 
    //system_update_menu_value(MENU_ID_CURRENT_PROFILE, atoi(proto->list[2]));

    protocol_send_response(CMD_RESPONSE, 0, proto);

    //system is live, we can ask gains
    cli_command(NULL, CLI_DISCARD_RESPONSE);

    g_device_booted = true; 

    //after boot we are ready to print the control vieuw
    CM_set_state();
}

void cb_set_selftest_control_skip(uint8_t serial_id, proto_t *proto)
{
    (void) serial_id;

    //lock actuators and clear tx buffer
    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);
    ui_comm_webgui_clear_tx_buffer();

    g_self_test_cancel_button = true; 

    protocol_send_response(CMD_RESPONSE, 0, proto);

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

void cb_menu_item_changed(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    system_update_menu_value(atoi(proto->list[1]), atoi(proto->list[2]));
    
    uint8_t i;
    for (i = 3; i < ((MENU_ID_TOP+1) * 2); i+=2)
    {
        if (atoi(proto->list[i]) != 0)
        {
            system_update_menu_value(atoi(proto->list[i]), atoi(proto->list[i+1]));
        }
        else break;
    }

    protocol_send_response(CMD_RESPONSE, 0, proto);

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

void cb_pedalboard_clear(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    if (naveg_get_current_mode() == MODE_CONTROL)
        CM_reset_encoder_page();

    //clear controls
    uint8_t i;
    for (i = 0; i < TOTAL_ACTUATORS; i++)
    {
        CM_remove_control(i);
    }

    protocol_send_response(CMD_RESPONSE, 0, proto);

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

void cb_pedalboard_name(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    //lock actuators
    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    screen_tittle(&proto->list[1], 1, 0);

    //stop the loading leds from fading
    //this is done here as its one of the last messages from MOD-UI
    if (naveg_get_current_mode() == MODE_NAVIGATION)
    {
        NM_set_leds();
    }

    protocol_send_response(CMD_RESPONSE, 0, proto);

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

void cb_snapshot_name(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    //lock actuators
    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    screen_tittle(&proto->list[1], 1, 1);

    protocol_send_response(CMD_RESPONSE, 0, proto);

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

void cb_pages_available(uint8_t serial_id, proto_t *proto)
{
    if (serial_id != WEBGUI_SERIAL)
        return;

    //lock actuators
    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    uint8_t pages_toggles[8] = {};
    pages_toggles[0] = atoi(proto->list[1]);
    pages_toggles[1] = atoi(proto->list[2]);
    pages_toggles[2] = atoi(proto->list[3]);
    pages_toggles[3] = atoi(proto->list[4]);
    pages_toggles[4] = atoi(proto->list[5]);
    pages_toggles[5] = atoi(proto->list[6]);
    pages_toggles[6] = atoi(proto->list[7]);
    pages_toggles[7] = atoi(proto->list[8]);

    CM_set_pages_available(pages_toggles);

    protocol_send_response(CMD_RESPONSE, 0, proto);

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}