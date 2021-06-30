
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
    {NULL, NULL, NULL, NULL, NULL, 0, 0, 0, NULL}
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

uint8_t g_current_popup_id;

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

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void PM_init(void)
{
    //init popup fields
}

void PM_enter(void)
{
    //check if naming
    if (g_global_popups[g_current_popup_id].has_naming_input) {

    }
    //pass to button pressed
    else {
        if ((g_global_popups[g_current_popup_id].button_max == 2) && (g_global_popups[g_current_popup_id].button_value == 1))
            PM_button_pressed(2);
        else
            PM_button_pressed(g_global_popups[g_current_popup_id].button_value);
    }
}

void PM_up(void)
{
    //check if naming
    if (g_global_popups[g_current_popup_id].has_naming_input) {

    }
    else if (g_global_popups[g_current_popup_id].button_value < g_global_popups[g_current_popup_id].button_max){
        g_global_popups[g_current_popup_id].button_value++;
    }
}

void PM_down(void)
{
    //check if naming
    if (g_global_popups[g_current_popup_id].has_naming_input) {

    }
    else if (g_global_popups[g_current_popup_id].button_value > 0){
        g_global_popups[g_current_popup_id].button_value--;
    }
}

void PM_set_state(void)
{
    PM_print_screen();
    PM_set_leds();
}

void PM_print_screen(void)
{
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
            //clear
            led = hardware_leds(4);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            //cancel
            led_state.color = TOGGLED_COLOR;
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
                    //change popup id
                    g_current_popup_id = POPUP_SAVE_PB_ID;

                    //copy current name to name field

                    PM_launch_popup(g_current_popup_id);
                break;

                //save snapshot
                case 1:
                    //change popup id
                    g_current_popup_id = POPUP_SAVE_SS_ID;

                    //copy current name to name field

                    PM_launch_popup(g_current_popup_id);
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
                    //send save to webui

                    //get response back

                    //if already in use, overwrite popup

                    //else 
                        PM_launch_attention_overlay("save sucsesfull", exit_popup);
                break;

                //clear name
                case 1:
                    //clear name
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
                        break;
                        case POPUP_REMOVE_SS_ID:
                            i = copy_command(buffer, CMD_SNAPSHOT_DELETE);
                            i += int_to_str(NM_get_current_hover(SNAPSHOT_LIST), &buffer[i], sizeof(buffer) - i, 0);
                        break;
                        case POPUP_REMOVE_PB_ID:
                            i = copy_command(buffer, CMD_PEDALBOARD_DELETE);
                            i += int_to_str(NM_get_current_hover(PEDALBOARD_LIST), &buffer[i], sizeof(buffer) - i, 0);
                        break;
                    }
                    
                    ui_comm_webgui_send(buffer, i);

                    //delete sucsesfull overlay
                    if (g_current_popup_id == POPUP_REMOVE_PB_ID)
                        PM_launch_attention_overlay("remove sucsesfull", exit_popup);
                    else
                        PM_launch_attention_overlay("delete sucsesfull", exit_popup);
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
                        i += int_to_str(NM_get_current_selected(SNAPSHOT_LIST), &buffer[i], sizeof(buffer) - i, 0);
                    }
                    else {
                       i = copy_command(buffer, CMD_PEDALBOARD_SAVE);
                       i += int_to_str(NM_get_current_selected(PEDALBOARD_LIST), &buffer[i], sizeof(buffer) - i, 0);
                    }

                    ui_comm_webgui_send(buffer, i);

                    //delete sucsesfull overlay
                    PM_launch_attention_overlay("overwrite sucsesfull", exit_popup);
                break;

                //cancel
                case 2:
                    naveg_trigger_popup(-1);
                break;
            } 
        break;

        case POPUP_NEW_BANK_ID:
            switch(button)
            {
                case 0:
                    //send save to webui

                    //get response back

                    //if already in use, overwrite popup

                    //else 
                        PM_launch_attention_overlay("bank creation sucsesfull", exit_popup);
                break;

                case 1:
                    //clear name
                break;

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
    PM_set_state();
}

void PM_launch_attention_overlay(char *message, void (*timeout_cb))
{
    screen_msg_overlay(message);

    hardware_set_overlay_timeout(MSG_TIMEOUT, timeout_cb);
}