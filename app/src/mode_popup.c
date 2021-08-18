
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/


#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "mode_navigation.h"
#include "mode_control.h"
#include "mode_popup.h"
#include "config.h"
#include "hardware.h"
#include "protocol.h"
#include "ledz.h"
#include "naveg.h"
#include "screen.h"
#include "ui_comm.h"

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

static system_popup_t g_global_popups[] = {
    SYSTEM_POPUPS
    {0, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL}
};

/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/


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

static uint8_t g_current_popup_id, g_prev_popup_id;
static uint8_t g_keyboard_toggled = 0;
static uint8_t g_keyboard_index = 0;
static char* g_current_name_input;
static uint8_t g_list_update_needed;

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

static void exit_popup(void)
{
    naveg_trigger_popup(-1);
}

//resets the naming widget to only include spaces
void reset_naming_widget_name(void)
{
    strcpy(g_current_name_input, "                  ");
    g_current_name_input[18] = 0;
}

//copy a name to the naming widget, but add spaces till the end of the box
void copy_name_to_naming_widget(char *source_to_copy)
{
    uint8_t source_length = 0;
    source_length = strlen(source_to_copy);

    if (source_length > 17)
        source_length = 17;

    strcpy(g_current_name_input, source_to_copy);

    uint8_t i;
    for (i = source_length-1; i < 18; i++) {
        strcat(g_current_name_input, " ");
    }

    g_current_name_input[18] = 0;
}

//check all spaces at the end of a name, we dont send that to mod-ui
void turnicate_naming_widget_spaces(void)
{
    uint8_t i;
    for (i = 17; i > 0; i--) {
        if (g_current_name_input[i] != ' ')
            break;
    }

    g_current_name_input[i+1] = 0;
}

void retrieve_save_response(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;

    //name does not yet exist
    if (atoi(list[1]) == 0){
        g_list_update_needed = 1;
    }
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void PM_init(void)
{
    //allocate memory for name
    g_current_name_input = (char *) MALLOC((19) * sizeof(char));

    reset_naming_widget_name();
}

void PM_enter(uint8_t encoder)
{
    //check if naming
    if (g_global_popups[g_current_popup_id].has_naming_input) {
        switch (encoder)
        {
            case 0:
            break;

            case 2:
            case 1:
                if (g_keyboard_toggled){
                    g_current_name_input[g_global_popups[g_current_popup_id].cursor_index] = keyboard_index_to_char(g_keyboard_index);
                    if (g_global_popups[g_current_popup_id].cursor_index < 18)
                        g_global_popups[g_current_popup_id].cursor_index++;
                }
                else {
                    g_keyboard_toggled = 1;
                    g_keyboard_index = 0;
                }

                PM_print_screen();
            break;
        }
    }
    //pass to button pressed
    else {
        if ((g_global_popups[g_current_popup_id].button_max == 2) && (g_global_popups[g_current_popup_id].button_value == 1))
            PM_button_pressed(2);
        else
            PM_button_pressed(g_global_popups[g_current_popup_id].button_value);
    }
}

void PM_up(uint8_t encoder)
{
    //check if naming
    if (g_global_popups[g_current_popup_id].has_naming_input) {
        switch (encoder)
        {
            case 0:
                if (g_global_popups[g_current_popup_id].cursor_index > 0)
                    g_global_popups[g_current_popup_id].cursor_index--;

                PM_print_screen();
            break;

            case 1:
                if (!g_keyboard_toggled) {
                    g_keyboard_toggled = 1;
                    g_keyboard_index = 0;
                }
                else if (g_keyboard_index > 0){
                    g_keyboard_index--;
                    g_current_name_input[g_global_popups[g_current_popup_id].cursor_index] = keyboard_index_to_char(g_keyboard_index);
                }
                else {
                    g_keyboard_index = 59;
                    g_global_popups[g_current_popup_id].input_name[g_global_popups[g_current_popup_id].cursor_index] = keyboard_index_to_char(g_keyboard_index);
                }

                PM_print_screen();
            break;

            case 2:
            //TODO WE MIGHT NOT ADD THIS FEATURE
                if (!g_keyboard_toggled) {
                    g_keyboard_toggled = 1;
                }
                else {
                    //check where we are
                    if (g_keyboard_index < 15)
                        g_keyboard_index += 45;
                    else
                        g_keyboard_index -= 15;

                    g_global_popups[g_current_popup_id].input_name[g_global_popups[g_current_popup_id].cursor_index] = keyboard_index_to_char(g_keyboard_index);
                }
                PM_print_screen();
            break;
        }
    }
    else if (g_global_popups[g_current_popup_id].button_value > 0){
        g_global_popups[g_current_popup_id].button_value--;
        PM_print_screen();
    }
}

void PM_down(uint8_t encoder)
{
    //check if naming
    if (g_global_popups[g_current_popup_id].has_naming_input) {
        switch (encoder)
        {
            case 0:
                if (g_global_popups[g_current_popup_id].cursor_index < 18) {
                    g_global_popups[g_current_popup_id].cursor_index++;

                    if (g_global_popups[g_current_popup_id].cursor_index > strlen(g_global_popups[g_current_popup_id].input_name))
                        strcat(g_global_popups[g_current_popup_id].input_name, " ");
                }

                PM_print_screen();
            break;

            case 1:
                if (!g_keyboard_toggled) {
                    g_keyboard_toggled = 1;
                }
                else if (g_keyboard_index < 59) {
                    g_keyboard_index++;
                    g_global_popups[g_current_popup_id].input_name[g_global_popups[g_current_popup_id].cursor_index] = keyboard_index_to_char(g_keyboard_index);
                }
                else {
                    g_keyboard_index = 0;
                    g_global_popups[g_current_popup_id].input_name[g_global_popups[g_current_popup_id].cursor_index] = keyboard_index_to_char(g_keyboard_index);
                }

                PM_print_screen();
            break;

            case 2:
            //TODO WE MIGHT NOT ADD THIS FEATURE
                if (!g_keyboard_toggled) {
                    g_keyboard_toggled = 1;
                }
                else {
                    //check where we are
                    if (g_keyboard_index > 44)
                        g_keyboard_index -= 45;
                    else
                        g_keyboard_index += 15;

                    g_global_popups[g_current_popup_id].input_name[g_global_popups[g_current_popup_id].cursor_index] = keyboard_index_to_char(g_keyboard_index);
                }
                PM_print_screen();
            break;
        }
    }
    else if (g_global_popups[g_current_popup_id].button_value < g_global_popups[g_current_popup_id].button_max-1){
        g_global_popups[g_current_popup_id].button_value++;
        PM_print_screen();
    }
}

void PM_set_state(void)
{
    PM_print_screen();
    PM_set_leds();
}

void PM_print_screen(void)
{
    if (g_keyboard_toggled)
        screen_keyboard(&g_global_popups[g_current_popup_id], g_keyboard_index);
    else
        screen_popup(&g_global_popups[g_current_popup_id]);
}

void PM_set_leds(void)
{
    uint8_t i;
    for (i = 0; i < 6; i++)
    {
        ledz_off(hardware_leds(i), WHITE);
    }

    ledz_t *led;
    led_state_t led_state;
    led_state.fade_ratio = 0;
    led_state.fade_rate = 0;

    switch (g_current_popup_id)
    {
        case POPUP_SAVE_SELECT_ID:
            //pb
            led = hardware_leds(3);
            led_state.color = TRIGGER_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            //ss
            led = hardware_leds(4);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            //cancel
            led_state.color = TOGGLED_COLOR;
            led = hardware_leds(5);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);

            led_state.color = ENUMERATED_COLOR;
            led = hardware_leds(6);
            set_ledz_trigger_by_color_id(led, LED_OFF, led_state);
        break;

        case POPUP_NEW_BANK_ID:
        case POPUP_SAVE_SS_ID:
        case POPUP_SAVE_PB_ID:
            //save
            led = hardware_leds(3);
            led_state.color = TRIGGER_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led_state.color = TOGGLED_COLOR;
            //clear
            led = hardware_leds(4);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            //cancel
            led = hardware_leds(5);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
        break;

        case POPUP_OVERWRITE_SS_ID:
        case POPUP_OVERWRITE_PB_ID:
        case POPUP_DELETE_BANK_ID:
        case POPUP_REMOVE_SS_ID:
        case POPUP_REMOVE_PB_ID:
            //confirm
            led = hardware_leds(3);
            led_state.color = TRIGGER_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            //cancel
            led_state.color = TOGGLED_COLOR;
            led = hardware_leds(5);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
        break;
    }
}

//main state machine for popup logic
void PM_button_pressed(uint8_t button)
{
    //now notify mod-ui
    char buffer[30];
    uint8_t i = 0;

    switch (g_current_popup_id)
    {
        case POPUP_SAVE_SELECT_ID:
            switch(button)
            {
                //save pedalboard
                case 0:
                    PM_launch_popup(POPUP_SAVE_PB_ID);
                break;

                //save snapshot
                case 1:
                    PM_launch_popup(POPUP_SAVE_SS_ID);
                break;

                //cancel
                case 2:
                    naveg_trigger_popup(-1);
                break;
            } 
        break;

        case POPUP_SAVE_SS_ID:
        case POPUP_SAVE_PB_ID:
            switch(button)
            {
                //send save
                case 0:
                    //check if only spaces, if so, ask for cancel
                    if (!strcmp(g_global_popups[g_current_popup_id].input_name , "                  ")){
                        g_keyboard_toggled = 0;
                        g_prev_popup_id = g_current_popup_id;
                        g_current_popup_id = POPUP_EMPTY_NAME_ID;
                        PM_launch_popup(POPUP_EMPTY_NAME_ID);
                        return;
                    }

                    //TODO we should catch the response to see if we have a duplicate name
                    if (g_current_popup_id==POPUP_SAVE_SS_ID)
                        ui_comm_webgui_set_response_cb(retrieve_save_response, NULL);
                    else
                        ui_comm_webgui_set_response_cb(retrieve_save_response, NULL);

                    //remove spaces at the end
                    turnicate_naming_widget_spaces();

                    //send save to webui
                    i = copy_command(buffer, (g_current_popup_id==POPUP_SAVE_SS_ID)?CMD_SNAPSHOT_SAVE_AS:CMD_PEDALBOARD_SAVE_AS);

                    const char *p = g_current_name_input;
                    // copy the pedalboard uid
                    if (!*p)
                        buffer[i++] = '0';
                    else {
                        while (*p) {
                            buffer[i++] = *p;
                            p++;
                        }
                    }
                    buffer[i] = 0;

                    // sends the data to GUI
                    ui_comm_webgui_send(buffer, i);

                    // waits the pedalboards list be received
                    ui_comm_webgui_wait_response();

                    if (g_list_update_needed) {
                        if (g_current_popup_id==POPUP_SAVE_SS_ID) {
                            //set last item as active
                            NM_set_last_selected(SNAPSHOT_LIST);

                            //update list items
                            NM_update_lists(SNAPSHOT_LIST);

                            //save successful message
                            PM_launch_attention_overlay("\n\n\nsnapshot save successful", exit_popup);
                        }
                        else {
                            //set last item as active
                            NM_set_last_selected(PEDALBOARD_LIST);

                            //update list items
                            NM_update_lists(PEDALBOARD_LIST);

                            //save successful message
                            PM_launch_attention_overlay("\n\n\npedalboard save successful", exit_popup);
                        }
                        g_list_update_needed =0;
                    }
                    else {
                        //we need to set the overwrite popup now
                        PM_launch_popup((g_current_popup_id==POPUP_SAVE_SS_ID)?POPUP_OVERWRITE_SS_ID:POPUP_OVERWRITE_PB_ID);
                    }
                break;

                //clear name
                case 1:
                    //clear name
                    g_global_popups[g_current_popup_id].cursor_index = 0;
                    reset_naming_widget_name();
                    PM_print_screen();
                break;

                //cancel, or delete
                case 2:
                    if (g_keyboard_toggled){
                        g_current_name_input[g_global_popups[g_current_popup_id].cursor_index] = keyboard_index_to_char(59);
                        if (g_global_popups[g_current_popup_id].cursor_index > 0)
                            g_global_popups[g_current_popup_id].cursor_index--;

                        PM_print_screen();
                    }
                    else {
                        naveg_trigger_popup(-1);
                        g_keyboard_toggled = 0;
                        g_keyboard_index = 0;
                    }
                break;
            }
        break;

        case POPUP_NEW_BANK_ID:
            switch(button)
            {
                case 0:
                    //check if only spaces, if so, ask for cancel
                    if (!strcmp(g_global_popups[g_current_popup_id].input_name , "                  ")){
                        g_keyboard_toggled = 0;
                        g_prev_popup_id = g_current_popup_id;
                        g_current_popup_id = POPUP_EMPTY_NAME_ID;
                        PM_launch_popup(POPUP_EMPTY_NAME_ID);
                        return;
                    }

                    //TODO we should catch the response to see if we have a duplicate name
                    ui_comm_webgui_set_response_cb(NULL, NULL);

                    //remove spaces at the end
                    turnicate_naming_widget_spaces();

                    //send save to webui
                    i = copy_command(buffer, CMD_BANK_NEW);

                    const char *p = g_current_name_input;
                    // copy the pedalboard uid
                    if (!*p)
                        buffer[i++] = '0';
                    else {
                        while (*p) {
                            buffer[i++] = *p;
                            p++;
                        }
                    }
                    buffer[i] = 0;

                    // sends the data to GUI
                    ui_comm_webgui_send(buffer, i);

                    // waits the pedalboards list be received
                    ui_comm_webgui_wait_response();

                    //TODO move this check to the callback and possibly give an extra popup
                        PM_launch_attention_overlay("\n\n\nbank creation successful", exit_popup);
                break;

                case 1:
                    //clear name
                    g_global_popups[g_current_popup_id].cursor_index = 0;
                    reset_naming_widget_name();
                    PM_print_screen();
                break;

                case 2:
                    if (g_keyboard_toggled){
                        g_current_name_input[g_global_popups[g_current_popup_id].cursor_index] = keyboard_index_to_char(59);
                        if (g_global_popups[g_current_popup_id].cursor_index > 0)
                            g_global_popups[g_current_popup_id].cursor_index--;

                        PM_print_screen();
                    }
                    else {
                        naveg_trigger_popup(-1);
                        g_keyboard_toggled = 0;
                        g_keyboard_index = 0;
                    }
                break;
            }
        break;

        case POPUP_EMPTY_NAME_ID:
            switch(button)
            {
                case 0:
                    PM_launch_popup(g_prev_popup_id);
                break;

                //cancel
                case 2:
                    naveg_trigger_popup(-1);
                break;
            }
        break;

        case POPUP_DELETE_BANK_ID:
        case POPUP_REMOVE_SS_ID:
        case POPUP_REMOVE_PB_ID:
            switch(button)
            {
                case 0:
                    //send delete msg
                    ui_comm_webgui_set_response_cb(NULL, NULL);
                    ui_comm_webgui_clear();

                    switch (g_current_popup_id) {
                        case POPUP_DELETE_BANK_ID:
                            i = copy_command(buffer, CMD_BANK_DELETE);
                            i += int_to_str(NM_get_current_hover(BANKS_LIST), &buffer[i], sizeof(buffer) - i, 0);

                            ui_comm_webgui_send(buffer, i);
                            ui_comm_webgui_wait_response();
                            NM_update_lists(BANKS_LIST);
                        break;
                        case POPUP_REMOVE_SS_ID:
                            i = copy_command(buffer, CMD_SNAPSHOT_DELETE);
                            i += int_to_str(NM_get_current_hover(SNAPSHOT_LIST), &buffer[i], sizeof(buffer) - i, 0);

                            ui_comm_webgui_send(buffer, i);
                            ui_comm_webgui_wait_response();
                            NM_update_lists(SNAPSHOT_LIST);

                            ////we removed the selected item, set the index out of bounds
                            //if (NM_get_current_selected(SNAPSHOT_LIST) == NM_get_current_hover(SNAPSHOT_LIST))
                            //    NM_set_selected_index(SNAPSHOT_LIST, );
                        break;
                        case POPUP_REMOVE_PB_ID:
                            i = copy_command(buffer, CMD_PEDALBOARD_DELETE);
                            i += int_to_str(NM_get_current_selected(BANKS_LIST), &buffer[i], sizeof(buffer) - i, 0);
                            buffer[i++] = ' ';
                            i += int_to_str(NM_get_current_hover(PEDALBOARD_LIST), &buffer[i], sizeof(buffer) - i, 0);

                            ui_comm_webgui_send(buffer, i);
                            ui_comm_webgui_wait_response();
                            NM_update_lists(PEDALBOARD_LIST);

                            //we removed the selected item, set the index out of bounds
                            if (NM_get_current_selected(PEDALBOARD_LIST) == NM_get_current_hover(PEDALBOARD_LIST))
                                NM_set_selected_index(PEDALBOARD_LIST, -1);
                            else
                                NM_set_selected_index(PEDALBOARD_LIST, -2);
                        break;
                    }

                    //delete successful overlay
                    if (g_current_popup_id == POPUP_REMOVE_PB_ID)
                        PM_launch_attention_overlay("\n\n\nremove successful", exit_popup);
                    else
                        PM_launch_attention_overlay("\n\n\ndelete successful", exit_popup);
                break;

                //cancel
                case 2:
                    naveg_trigger_popup(-1);
                break;
            } 
        break;

        case POPUP_OVERWRITE_SS_ID:
        case POPUP_OVERWRITE_PB_ID:
            switch(button)
            {
                case 0:
                    ui_comm_webgui_set_response_cb(NULL, NULL);
                    ui_comm_webgui_clear();

                    //send overwrite msg
                    if (g_current_popup_id == POPUP_OVERWRITE_SS_ID) {
                        i = copy_command(buffer, CMD_SNAPSHOTS_SAVE);
                    }
                    else {
                       i = copy_command(buffer, CMD_PEDALBOARD_SAVE);
                    }

                    ui_comm_webgui_send(buffer, i);

                    //delete successful overlay
                    PM_launch_attention_overlay("\n\n\noverwrite successful", exit_popup);
                break;

                //cancel
                case 2:
                    naveg_trigger_popup(-1);
                break;
            } 
        break;
    }

}

void PM_launch_popup(uint8_t popup_id)
{
    //change current popup id and print it
    g_current_popup_id = popup_id;

    //fetch the needed things
    if (g_global_popups[g_current_popup_id].has_naming_input) {

        g_global_popups[g_current_popup_id].cursor_index = 0;
        g_keyboard_toggled = 0;

        switch (g_current_popup_id) {
            case POPUP_SAVE_PB_ID:
                copy_name_to_naming_widget(NM_get_pbss_name(0));
            break;

            case POPUP_SAVE_SS_ID:
                copy_name_to_naming_widget(NM_get_pbss_name(1));
            break;

            case POPUP_NEW_BANK_ID:
                reset_naming_widget_name();
            break;
        }

        g_global_popups[g_current_popup_id].input_name = g_current_name_input;

        //check cursor
        uint8_t i;
        for (i = 17; i > 0; i--) {
            if (g_current_name_input[i] != ' ') {
                break;
            }
        }
    }
    else
        g_keyboard_toggled = 0;

    PM_set_state();
}

void PM_launch_attention_overlay(char *message, void (*timeout_cb))
{
    screen_msg_overlay(message);

    hardware_set_overlay_timeout(MSG_TIMEOUT, timeout_cb);
}

uint8_t PM_get_current_popup(void)
{
    return g_current_popup_id;
}