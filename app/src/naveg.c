
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "naveg.h"
#include "config.h"
#include "screen.h"
#include "ledz.h"
#include "hardware.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "protocol.h"
#include "mode_control.h"
#include "mode_navigation.h"
#include "mode_tools.h"
#include <stdlib.h>
#include <string.h>

//reset actuator queue
void reset_queue(void);

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

static uint8_t g_ui_connected;
static xSemaphoreHandle g_dialog_sem;
static uint8_t g_dialog_active = 0;

// only enabled after "boot" command received
bool g_popup_active = false;
uint8_t g_encoders_pressed[ENCODERS_COUNT] = {};

static uint8_t g_device_mode, g_prev_device_mode, g_prev_shift_device_mode;

uint8_t g_initialized = 0;
uint8_t g_lock_release[FOOTSWITCHES_COUNT] = {};
uint8_t g_shift_latching = 1;

int16_t g_shift_item_ids[3];

// only disabled after "boot" command received
bool g_self_test_mode = true;
bool g_self_test_cancel_button = false;

bool shift_mode_active = false;

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

void enter_shift_menu(void)
{
    if (g_device_mode == MODE_SELFTEST)
    {
        if (g_dialog_active)
            return;
        
        char buffer[30];
        uint8_t i;

        //skip control action
        if (g_self_test_cancel_button)
        {
            i = copy_command(buffer, CMD_SELFTEST_SKIP_CONTROL);
        }
        else 
        {
            i = copy_command(buffer, CMD_SELFTEST_BUTTON_CLICKED);

            // insert the hw_id on buffer
            i += int_to_str(6, &buffer[i], sizeof(buffer) - i, 0);
        }

        ui_comm_webgui_clear_tx_buffer();

        // send the data to GUI
        ui_comm_webgui_send(buffer, i);

        return;
    }

    if (g_popup_active || !g_device_booted) return;

    hardware_set_overlay_timeout(0, NULL);

    //enter shift mode
    //save to return
    g_prev_shift_device_mode = g_device_mode;

    //turn of tuner if applicable
    if (g_prev_shift_device_mode == MODE_TOOL_FOOT)
        TM_turn_off_tuner();

    //toggle shift
    g_device_mode = MODE_SHIFT;

    //print shift screen
    screen_shift_overlay(g_prev_shift_device_mode, &g_shift_item_ids[0]);

    //toggle the LED's
    ledz_t *led = hardware_leds(3);
    led_state_t led_state;
    if (g_prev_shift_device_mode != MODE_TOOL_MENU)
        led_state.color = ENUMERATED_COLOR;
    else
        led_state.color = TOGGLED_COLOR;

    set_ledz_trigger_by_color_id(led, LED_ON, led_state);
    led = hardware_leds(5);
    led_state.color = ENUMERATED_COLOR;
    set_ledz_trigger_by_color_id(led, LED_ON, led_state);
    led = hardware_leds(4);
    set_ledz_trigger_by_color_id(led, LED_OFF, led_state);

    if (!g_self_test_mode)
        ledz_on(hardware_leds(6), WHITE);
}

void exit_shift_menu(void)
{
    if (g_popup_active || !g_device_booted) return;

    hardware_set_overlay_timeout(0, NULL);

    if (!g_self_test_mode)
        ledz_off(hardware_leds(6), WHITE);

    //already entered some other mode
    if (g_device_mode != MODE_SHIFT)
        return;

    //always a chance we changed gains, send save
    system_save_gains_cb(NULL, MENU_EV_ENTER);

    //exit the shift menu, return to opperational mode
    switch(g_prev_shift_device_mode)
    {
        case MODE_CONTROL:
            g_device_mode = MODE_CONTROL;
            CM_set_state();
        break;

        case MODE_NAVIGATION:
            g_device_mode = MODE_NAVIGATION;
            NM_print_screen();
        break;

        case MODE_TOOL_FOOT:
            g_device_mode = MODE_TOOL_FOOT;
            TM_launch_tool(-1);
        break;

        case MODE_TOOL_MENU:
            g_device_mode = MODE_TOOL_MENU;
            TM_launch_tool(TOOL_MENU);
        break;

        case MODE_BUILDER:
            //not defined yet
        break;

        case MODE_SELFTEST:
            //not used
        break;
    }
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

uint8_t naveg_get_current_mode(void)
{
    return g_device_mode;
}

void naveg_init(void)
{
    //init control_mode
    CM_init();

    //init navigation mode
    NM_init();

    //init tool mode
    TM_init();

    g_device_mode = MODE_CONTROL;
    g_prev_device_mode = MODE_CONTROL;
    g_prev_shift_device_mode = MODE_CONTROL;

    g_initialized = 1;

    vSemaphoreCreateBinary(g_dialog_sem);
    // vSemaphoreCreateBinary is created as available which makes
    // first xSemaphoreTake pass even if semaphore has not been given
    // http://sourceforge.net/p/freertos/discussion/382005/thread/04bfabb9
    xSemaphoreTake(g_dialog_sem, 0);
}

void naveg_update_shift_item_ids(void)
{
    uint8_t i;
    uint8_t read_buffer = 0;

    for (i = 0; i < 3; i++)
    {
        //do system callbacks
        EEPROM_Read(0, SHIFT_ITEM_ADRESS+i, &read_buffer, MODE_8_BIT, 1);
        g_shift_item_ids[i] = system_get_shift_item(read_buffer);
    }
}

void naveg_update_shift_item_values(void)
{
    uint8_t i;
    for (i = 0; i < 3; i++)
    {
        menu_item_t *shift_item = TM_get_menu_item_by_ID(g_shift_item_ids[i]);
        shift_item->desc->action_cb(shift_item, MENU_EV_NONE);
    }
}

void naveg_update_single_shift_item(uint8_t shift_item_id, int16_t item_id)
{
    g_shift_item_ids[shift_item_id] = item_id;
    menu_item_t *shift_item = TM_get_menu_item_by_ID(g_shift_item_ids[shift_item_id]);
    shift_item->desc->action_cb(shift_item, MENU_EV_NONE);
}

void naveg_turn_off_leds(void)
{
    uint8_t i;
    ledz_t *led; 
    for (i = 0; i < LEDS_COUNT; i++)
    {
        led = hardware_leds(i);
        led_state_t led_state = {
            .color = WHITE,
        };
        set_ledz_trigger_by_color_id(led, LED_OFF, led_state);
    }
}

void naveg_ui_connection(uint8_t status)
{
    if (!g_initialized) return;

    if (status == UI_CONNECTED)
    {
        g_ui_connected = 1;
    }
    else
    {
        g_ui_connected = 0;

        NM_clear();
    }
    
    //if we where in some menu's, we might need to exit them
    switch(g_device_mode)
    {
        case MODE_CONTROL:
            //no action needed
        break;

        case MODE_NAVIGATION:
            //enter control mode
            g_device_mode = MODE_CONTROL;
            CM_set_state();
        break;

        case MODE_TOOL_FOOT:
        case MODE_TOOL_MENU:
            //no action needed
        break;

        case MODE_BUILDER:
            //not defined yet
        break;
    }
}

uint8_t naveg_ui_status(void)
{
    return g_ui_connected;
}

void naveg_enc_enter(uint8_t encoder)
{
    if (!g_initialized) return;

    // checks the foot id
    if (encoder >= ENCODERS_COUNT) return;

    g_encoders_pressed[encoder] = 1;

    switch(g_device_mode)
    {
        case MODE_CONTROL:
            CM_toggle_control(encoder);
        break;

        case MODE_NAVIGATION:
            if (encoder == 0) NM_enter();
        break;

        case MODE_TOOL_FOOT:
        case MODE_TOOL_MENU:
            TM_encoder_click(encoder);
        break;

        case MODE_BUILDER:
            //not defined yet
        break;

        case MODE_SELFTEST:
            if (g_dialog_active)
            {
                TM_encoder_click(0);
            }
            else
            {
                char buffer[30];
                uint8_t i;

                i = copy_command(buffer, CMD_SELFTEST_ENCODER_CLICKED);

                // insert the hw_id on buffer
                i += int_to_str(encoder, &buffer[i], sizeof(buffer) - i, 0);

                ui_comm_webgui_clear_tx_buffer();

                // send the data to GUI
                ui_comm_webgui_send(buffer, i);
            }
        break;
    }
}

void naveg_enc_released(uint8_t encoder)
{
    if (!g_initialized) return;

    // checks the foot id
    if (encoder >= ENCODERS_COUNT) return;


    if (g_device_mode != MODE_NAVIGATION)
        g_encoders_pressed[encoder] = 0;
    else
        NM_encoder_released(encoder);
}

void naveg_enc_hold(uint8_t encoder)
{
    if (!g_initialized) return;

    // checks the foot id
    if (encoder >= ENCODERS_COUNT) return;

    if (g_device_mode != MODE_NAVIGATION)
        g_encoders_pressed[encoder] = 1;
    else
        NM_encoder_hold(encoder);

    if (g_self_test_mode)
        naveg_enc_enter(encoder);
}

void naveg_enc_down(uint8_t encoder)
{
    if (!g_initialized) return;

    // checks the foot id
    if (encoder >= ENCODERS_COUNT) return;

    switch(g_device_mode)
    {
        case MODE_CONTROL:
            //pass to control mode 
            CM_inc_control(encoder);
        break;

        case MODE_NAVIGATION:
            if (encoder == 0)
            {
                //pass to navigation code
                NM_down();
            }
        break;

        case MODE_TOOL_FOOT:
        case MODE_TOOL_MENU:
            //pass for tuner/menu/bypass controls
            TM_down(encoder);
        break;

        case MODE_SHIFT:;
            //do system callbacks
            uint8_t read_buffer = 0;
            switch (encoder)
            {
                case 0:
                    EEPROM_Read(0, SHIFT_ITEM_ADRESS, &read_buffer, MODE_8_BIT, 1);
                break;
                case 1:
                    EEPROM_Read(0, SHIFT_ITEM_ADRESS+1, &read_buffer, MODE_8_BIT, 1);
                break;
                case 2:
                    EEPROM_Read(0, SHIFT_ITEM_ADRESS+2, &read_buffer, MODE_8_BIT, 1);
                break;
            }
            
            menu_item_t *item = TM_get_menu_item_by_ID(system_get_shift_item(read_buffer));

            if (item->desc->type == MENU_BAR)
            {
                if (g_encoders_pressed[encoder])
                    item->data.step = 10;
                else
                    item->data.step = 1;
            }

            item->desc->action_cb(item, MENU_EV_UP);

            screen_shift_overlay(-1, NULL);
        break;

        case MODE_BUILDER:
            //not defined yet
        break;

        case MODE_SELFTEST:
            if (g_dialog_active)
            {
                TM_down(0);
            }
            else
            {
                char buffer[30];
                uint8_t i;

                i = copy_command(buffer, CMD_SELFTEST_ENCODER_RIGHT);

                // insert the hw_id on buffer
                i += int_to_str(encoder, &buffer[i], sizeof(buffer) - i, 0);

                ui_comm_webgui_clear_tx_buffer();

                // send the data to GUI
                ui_comm_webgui_send(buffer, i);
            }
        break;
    }
}

void naveg_enc_up(uint8_t encoder)
{
    if (!g_initialized) return;

    // checks the foot id
    if (encoder >= ENCODERS_COUNT) return;

    switch(g_device_mode)
    {
        case MODE_CONTROL:
            //pass to control mode 
            CM_dec_control(encoder);
        break;

        case MODE_NAVIGATION:
            if (encoder == 0)
            {
                //pass to navigation code
                NM_up();
            }
        break;

        case MODE_TOOL_FOOT:
        case MODE_TOOL_MENU:
            //pass for tuner/menu/bypass controls
            TM_up(encoder);
        break;

        case MODE_SHIFT:;
            //do system callbacks
            uint8_t read_buffer = 0;
            switch (encoder)
            {
                case 0:
                    EEPROM_Read(0, SHIFT_ITEM_ADRESS, &read_buffer, MODE_8_BIT, 1);
                break;
                case 1:
                    EEPROM_Read(0, SHIFT_ITEM_ADRESS+1, &read_buffer, MODE_8_BIT, 1);
                break;
                case 2:
                    EEPROM_Read(0, SHIFT_ITEM_ADRESS+2, &read_buffer, MODE_8_BIT, 1);
                break;
            }
            
            menu_item_t *item = TM_get_menu_item_by_ID(system_get_shift_item(read_buffer));

            if (item->desc->type == MENU_BAR)
            {
                if (g_encoders_pressed[encoder])
                    item->data.step = 10;
                else
                    item->data.step = 1;
            }

            item->desc->action_cb(item, MENU_EV_DOWN);

            screen_shift_overlay(-1, NULL);
        break;

        case MODE_BUILDER:
            //not defined yet
        break;

        case MODE_SELFTEST:
            if (g_dialog_active)
            {
                TM_up(0);
            }
            else
            {
                char buffer[30];
                uint8_t i;

                i = copy_command(buffer, CMD_SELFTEST_ENCODER_LEFT);

                // insert the hw_id on buffer
                i += int_to_str(encoder, &buffer[i], sizeof(buffer) - i, 0);

                ui_comm_webgui_clear_tx_buffer();

                // send the data to GUI
                ui_comm_webgui_send(buffer, i);
            }
        break;
    }
}

void naveg_foot_change(uint8_t foot, uint8_t pressed)
{
    if (!g_initialized || g_popup_active) return;

    // checks the foot id
    if (foot >= FOOTSWITCHES_COUNT) return;

    if (g_lock_release[foot] && !pressed)
    {
        g_lock_release[foot] = 0;
        return;
    }

    switch(g_device_mode)
    {
        case MODE_CONTROL:
            //footswitch used for pages
            if (foot == 2)
            {
                hardware_set_overlay_timeout(0, NULL);

                if (pressed)
                    CM_load_next_page();
            }
            else 
            {
                //pass to control mode 
                CM_foot_control_change(foot, pressed);
            }
        break;

        case MODE_NAVIGATION:
            //no release action
            if (pressed)
                hardware_set_overlay_timeout(0, NULL);

            if ((foot == 2) && pressed)
            {
                //change pb <-> ss
                NM_toggle_pb_ss();
            }
            else 
            {
                NM_change_pbss(foot, pressed);
            }
        break;

        case MODE_TOOL_FOOT:
            if (!pressed)
            {
                if ((foot == 1) && (TM_check_tool_status() == TOOL_TUNER))
                    TM_foot_change(foot, pressed);

                return;
            }

            hardware_set_overlay_timeout(0, NULL);

            if (foot < 2)
            {
                TM_foot_change(foot, pressed);
            }
            else if (foot == 2)
            {
                TM_tool_up();
            }
        break;

        case MODE_TOOL_MENU:
            //no foots used
        break;

        case MODE_BUILDER:
            //not defined yet
        break;

        case MODE_SELFTEST:
            //if in selftest mode, we just send if we are working or not
            if (!g_dialog_active && pressed)
            {
                char buffer[30];
                uint8_t i;
                i = copy_command(buffer, CMD_SELFTEST_BUTTON_CLICKED);

                // insert the hw_id on buffer
                i += int_to_str(foot, &buffer[i], sizeof(buffer) - i, 0);

                ui_comm_webgui_clear_tx_buffer();

                // send the data to GUI
                ui_comm_webgui_send(buffer, i);
            }
        break;
    }
}

void naveg_foot_double_press(uint8_t foot)
{
    if ((g_popup_active) || (g_device_mode == MODE_SELFTEST)) return;

    hardware_set_overlay_timeout(0, NULL);

    //lock foots when double press, we dont want a release action here
    g_lock_release[foot] = 1;
    //we always use foot 0 for double press, a bit dirty I know
    //TODO make pretty
    g_lock_release[0] = 1;

    //navigation mode
    if (foot == 1)
    {
        //reset nav mode if nececary
        if (NM_get_current_list() == BANKS_LIST)
            NM_set_current_list(PEDALBOARD_LIST);

        switch(g_device_mode)
        {
            case MODE_CONTROL:
                //enter navigation mode
                if (g_ui_connected)
                {
                    give_attention_popup("\nPlease disconnect   the Web-ui to enter navigation mode", CM_print_screen);
                    return;
                }

                g_prev_device_mode = MODE_CONTROL;
                g_device_mode = MODE_NAVIGATION;
                NM_print_screen();
            break;

            case MODE_NAVIGATION:;
                //enter control mode
                naveg_turn_off_leds();

                g_device_mode = MODE_CONTROL;
                g_prev_device_mode = MODE_NAVIGATION;

                CM_set_state();
            break;

            case MODE_TOOL_FOOT:
                if (g_ui_connected)
                {
                    give_attention_popup("\nPlease disconnect   the Web-ui to enter navigation mode", TM_print_tool);
                    return;
                }

                g_prev_device_mode = MODE_TOOL_FOOT;
                //we enter the prev mode
                g_device_mode = MODE_NAVIGATION;
                NM_print_screen();

                TM_turn_off_tuner();
            break;

            case MODE_TOOL_MENU:
                if (g_ui_connected)
                {
                    give_attention_popup("\nPlease disconnect   the Web-ui to enter navigation mode", TM_print_tool);
                    return;
                }

                g_prev_device_mode = MODE_TOOL_MENU;
                //we enter the prev mode
                g_device_mode = MODE_NAVIGATION;
                NM_print_screen();
            break;

            case MODE_BUILDER:
                //not defined yet
            break;

            case MODE_SHIFT:
                //block action
                return;
            break;

            case MODE_SELFTEST:
                //not used
            break;
        } 
    }

    //tool mode
    else if (foot == 2)
    {
        if ((g_device_mode == MODE_CONTROL) || (g_device_mode == MODE_NAVIGATION)|| (g_device_mode == MODE_TOOL_MENU))
        {
            g_prev_device_mode = g_device_mode;
            g_device_mode = MODE_TOOL_FOOT;
            TM_launch_tool(TOOL_FOOT);
        }
        else if (g_device_mode == MODE_TOOL_FOOT)
        {
            TM_turn_off_tuner();
            if (g_prev_device_mode == MODE_CONTROL)
            {
                g_device_mode = MODE_CONTROL;
                CM_set_state();
            }
            else if (g_prev_device_mode == MODE_TOOL_MENU)
            {
                g_device_mode = MODE_TOOL_MENU;
                TM_launch_tool(TOOL_MENU);
            }
            else 
            {
                if (g_ui_connected) return;

				//reset nav mode if nececary
      			if (NM_get_current_list() == BANKS_LIST)
      			    NM_set_current_list(PEDALBOARD_LIST);
                
                g_device_mode = MODE_NAVIGATION;
                NM_print_screen();
            }
            g_prev_device_mode = MODE_TOOL_FOOT;
        }
    }

    //we dont have others atm
    return;
}

//used fot the 3 encoder buttons
void naveg_button_pressed(uint8_t button)
{
    hardware_set_overlay_timeout(0, NULL);

    if (!g_initialized) return;

    if (g_popup_active)
    {
        //do same actions as encoders would do, but with enter
        //left = down + enter
        //right = up + enter
        if (button == 0)
        {
            naveg_enc_up(g_popup_encoder);
            naveg_enc_enter(g_popup_encoder);
        }
        else if (button == 2)
        {
            naveg_enc_down(g_popup_encoder);
            naveg_enc_enter(g_popup_encoder);
        }

        return;
    }

    switch(g_device_mode)
    {
        case MODE_CONTROL:
            //pass to control mode 
            CM_load_next_encoder_page(button);
        break;

        case MODE_NAVIGATION:
            //pass to navigation code
            NM_button_pressed(button);
        break;

        case MODE_TOOL_FOOT:
        case MODE_TOOL_MENU:
            //pass for tool controls
            TM_enter(button);
        break;

        case MODE_BUILDER:
            //not defined yet
        break;

        case MODE_SHIFT:
            switch(button)
            {
                //enter menu
                case 0:
                    if (g_prev_shift_device_mode != MODE_TOOL_MENU)
                    {
                        g_device_mode = MODE_TOOL_MENU;
                        shift_mode_active = false;
                        TM_launch_tool(TOOL_MENU);
                    }
                    else 
                    {
                        shift_mode_active = false;
                        //exit the shift menu, return to opperational mode
                        switch(g_prev_shift_device_mode)
                        {
                            case MODE_CONTROL:
                                g_device_mode = MODE_CONTROL;
                                CM_set_state();
                            break;

                            case MODE_NAVIGATION:
                                g_device_mode = MODE_NAVIGATION;
                                NM_print_screen();
                            break;

                            case MODE_TOOL_FOOT:
                                g_device_mode = MODE_TOOL_FOOT;
                                TM_launch_tool(-1);
                            break;

                            case MODE_TOOL_MENU:
                                g_device_mode = MODE_CONTROL;
                                CM_set_state();
                            break;

                            case MODE_BUILDER:
                                //not defined yet
                            break;
                        }
                    }
                    
                    if (!g_self_test_mode)
                        ledz_off(hardware_leds(6), WHITE);
                break;

                //enter builder mode
                case 1:
                break;

                //send save pb command
                case 2:
                    ui_comm_webgui_send(CMD_PEDALBOARD_SAVE, strlen(CMD_PEDALBOARD_SAVE));
                    ui_comm_webgui_wait_response();

                    //also give quick overlay
                    give_attention_popup(PEDALBOARD_SAVED_TXT, naveg_print_shift_screen);
                break;
            }
        break;

        case MODE_SELFTEST:
            //if in selftest mode, we just send if we are working or not
            if (!g_dialog_active)
            {
                char buffer[30];
                uint8_t i;
                i = copy_command(buffer, CMD_SELFTEST_BUTTON_CLICKED);

                // insert the hw_id on buffer
                i += int_to_str(button + FOOTSWITCHES_COUNT, &buffer[i], sizeof(buffer) - i, 0);

                ui_comm_webgui_clear_tx_buffer();

                // send the data to GUI
                ui_comm_webgui_send(buffer, i);

                return;
            }
        break;
    }
}

void naveg_button_released(uint8_t button)
{
    (void) button;
    if (!g_initialized || g_popup_active) return;

    switch(g_device_mode)
    {
        case MODE_CONTROL:
            //not used
        break;

        case MODE_NAVIGATION:
            //not used
        break;

        case MODE_TOOL_FOOT:
        case MODE_TOOL_MENU:
            //not used
        break;

        case MODE_BUILDER:
            //not used
        break;

        case MODE_SELFTEST:
            //not used
        break;
    }
}

//used for the shift button
void naveg_shift_pressed()
{
    if (g_shift_latching)
    {
        if (shift_mode_active)
        {
            shift_mode_active = false;
            exit_shift_menu();
        }
        else
        {
            shift_mode_active = true;
            enter_shift_menu();
        }
    }
    else
        enter_shift_menu();
}

void naveg_shift_releaed()
{
    if (!g_shift_latching)
        exit_shift_menu();
}

uint8_t naveg_dialog_status(void)
{
    return g_dialog_active;
}

uint8_t naveg_dialog(char *msg)
{
    static node_t *dummy_menu = NULL;
    static menu_desc_t desc = {NULL, MENU_CONFIRM2, DIALOG_ID, DIALOG_ID, NULL, 0};
    
    if (!dummy_menu)
    {
        menu_item_t *item;
        item = (menu_item_t *) MALLOC(sizeof(menu_item_t));
        item->data.hover = 0;
        item->data.selected = 0xFF;
        item->data.list_count = 2;
        item->data.list = NULL;
        item->data.popup_content = msg;
        item->data.popup_header = "selftest";
        item->data.popup_active = 1;
        item->desc = &desc;
        item->name = NULL;
        dummy_menu = node_create(item);
    }

    menu_item_t *dummy_item = dummy_menu->data;
    TM_set_dummy_menu_item(dummy_menu);

    g_device_booted = true;
    g_device_mode = MODE_SELFTEST;

    g_dialog_active = 1;

    if (xSemaphoreTake(g_dialog_sem, portMAX_DELAY) == pdTRUE)
    {
        g_dialog_active = 0;

        TM_reset_menu();

        screen_clear();

        return dummy_item->data.hover;
    }
    //we can never get here, portMAX_DELAY means wait indefinatly I'm adding this to remove a compiler warning
    else
    {
        //ERROR
        return -1; 
    }
}

void naveg_release_dialog_semaphore(void)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(g_dialog_sem, &xHigherPriorityTaskWoken); 
}

void naveg_trigger_mode_change(uint8_t mode)
{
    //save to return
    g_prev_device_mode = g_device_mode;

    //toggle shift
    g_device_mode = mode;

    //exit the shift menu, return to opperational mode
    switch(g_device_mode)
    {
        case MODE_CONTROL:
            CM_set_state();
        break;

        case MODE_NAVIGATION:
            NM_print_screen();
        break;

        case MODE_TOOL_MENU:
        case MODE_TOOL_FOOT:
            TM_print_tool();
        break;

        case MODE_BUILDER:
            //not defined yet
        break;

        case MODE_SELFTEST:
            //not used
        break;
    }
}

void naveg_print_shift_screen(void)
{
    screen_shift_overlay(g_prev_shift_device_mode, NULL);
}

void naveg_set_shift_mode(uint8_t mode)
{
    g_shift_latching = mode;
}