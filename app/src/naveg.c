
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "naveg.h"
#include "node.h"
#include "config.h"
#include "screen.h"
#include "utils.h"
#include "ledz.h"
#include "hardware.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "actuator.h"
#include "protocol.h"
#include "mode_control.h"
#include "mode_navigation.h"
#include "mode_tools.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <float.h>

//reset actuator queue
void reset_queue(void);

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define DIALOG_MAX_SEM_COUNT   1

#define PAGE_DIR_DOWN       0
#define PAGE_DIR_UP         1
#define PAGE_DIR_INIT       2

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
#define BANKS_LIST 0

/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static uint8_t g_ui_connected;
static xSemaphoreHandle g_dialog_sem;
static uint8_t dialog_active = 0;

// only enabled after "boot" command received
bool g_should_wait_for_webgui = false;
bool g_device_booted = false;

static void (*g_update_cb)(void *data, int event);
static void *g_update_data;

static uint8_t g_device_mode, g_prev_device_mode, g_prev_shift_device_mode;

uint8_t g_initialized = 0;
uint8_t g_lock_release = 0;

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

    // initialize update variables
    g_update_cb = NULL;
    g_update_data = NULL;

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

void naveg_turn_off_leds(void)
{
    uint8_t i; 
    for (i = 0; i < LEDS_COUNT; i++)
    {
        set_ledz_trigger_by_color_id(hardware_leds(i), WHITE, 0, 0, 0, 0);
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
            CM_set_screen();
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
    }
}

void naveg_enc_hold(uint8_t encoder)
{
    (void) encoder;
    //action not used in MVP
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

        case MODE_SHIFT:
            //do system callbacks, hardcodec for now
            switch (encoder)
            {
                case 0:
                    system_inp_1_volume_cb(TM_get_menu_item_by_ID(INP_1_GAIN_ID), MENU_EV_DOWN);
                break;
                case 1:
                    system_inp_2_volume_cb(TM_get_menu_item_by_ID(INP_2_GAIN_ID), MENU_EV_DOWN);
                break;
                case 2:
                    system_outp_1_volume_cb(TM_get_menu_item_by_ID(OUTP_1_GAIN_ID), MENU_EV_DOWN);
                    system_outp_2_volume_cb(TM_get_menu_item_by_ID(OUTP_2_GAIN_ID), MENU_EV_DOWN);
                break;
            }
        break;

        case MODE_BUILDER:
            //not defined yet
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

        case MODE_SHIFT:
            //do system callbacks, hardcodec for nwo
            switch (encoder)
            {
                case 0:
                    system_inp_1_volume_cb(TM_get_menu_item_by_ID(INP_1_GAIN_ID), MENU_EV_UP);
                break;
                case 1:
                    system_inp_2_volume_cb(TM_get_menu_item_by_ID(INP_2_GAIN_ID), MENU_EV_UP);
                break;
                case 2:
                    system_outp_1_volume_cb(TM_get_menu_item_by_ID(OUTP_1_GAIN_ID), MENU_EV_UP);
                    system_outp_2_volume_cb(TM_get_menu_item_by_ID(OUTP_2_GAIN_ID), MENU_EV_UP);
                break;
            }
        break;

        case MODE_BUILDER:
            //not defined yet
        break;
    }
}

void naveg_foot_change(uint8_t foot, uint8_t pressed)
{
    if (!g_initialized) return;

    // checks the foot id
    if (foot >= FOOTSWITCHES_COUNT) return;

    switch(g_device_mode)
    {
        case MODE_CONTROL:
            //footswitch used for pages
            if (foot == 2)
            {
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
            if (!pressed)
                return;
            
            if (foot == 2)
            {
                //change pb <-> ss
                NM_toggle_pb_ss();
            }
            else 
            {
                NM_change_pbss(foot);
            }
            
        break;

        case MODE_TOOL_FOOT:
        case MODE_TOOL_MENU:
            //todo trigger tool changes (bypass, taptempo)
        break;

        case MODE_BUILDER:
            //not defined yet
        break;
    }
}

void naveg_foot_double_press(uint8_t foot)
{
    //navigation mode
    if (foot == 1)
    {
        switch(g_device_mode)
        {
            case MODE_CONTROL:
                //enter navigation mode
                if (g_ui_connected) return;

                g_prev_device_mode = MODE_CONTROL;
                g_device_mode = MODE_NAVIGATION;
                NM_print_screen();
            break;

            case MODE_NAVIGATION:;
                //enter control mode
                g_device_mode = MODE_CONTROL;
                g_prev_device_mode = MODE_NAVIGATION;
                CM_set_screen();
            break;

            case MODE_TOOL_FOOT:
                if (g_ui_connected) return;

                TM_turn_off_tuner();
                g_prev_device_mode = MODE_TOOL_FOOT;
                //we enter the prev mode
                g_device_mode = MODE_NAVIGATION;
                NM_print_screen();
            break;

            case MODE_TOOL_MENU:
                if (g_ui_connected) return;

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
        } 
    }

    //tool mode
    else if (foot == 2)
    {
        if ((g_device_mode == MODE_CONTROL) || (g_device_mode == MODE_NAVIGATION)|| (g_device_mode == MODE_TOOL_MENU))
        {
            g_prev_device_mode = g_device_mode;
            g_device_mode = MODE_TOOL_FOOT;
            TM_launch_tool(TOOL_TUNER);
        }
        else if (g_device_mode == MODE_TOOL_FOOT)
        {
            TM_turn_off_tuner();
            if (g_prev_device_mode == MODE_CONTROL)
            {
                g_device_mode = MODE_CONTROL;
                CM_set_screen();
            }
            else if (g_prev_device_mode == MODE_TOOL_MENU)
            {
                g_device_mode = MODE_TOOL_MENU;
                TM_launch_tool(TOOL_MENU);
            }
            else 
            {
                if (g_ui_connected) return;
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
    if (!g_initialized) return;

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
                        TM_launch_tool(TOOL_MENU);
                    }
                    else 
                    {
                        //exit the shift menu, return to opperational mode
                        switch(g_prev_shift_device_mode)
                        {
                            case MODE_CONTROL:
                                g_device_mode = MODE_CONTROL;
                                CM_set_screen();
                            break;

                            case MODE_NAVIGATION:
                                g_device_mode = MODE_NAVIGATION;
                                NM_print_screen();
                            break;

                            case MODE_TOOL_FOOT:
                                g_device_mode = MODE_TOOL_FOOT;
                                TM_launch_tool(TOOL_TUNER);
                            break;

                            case MODE_TOOL_MENU:
                                g_device_mode = MODE_TOOL_MENU;
                                TM_launch_tool(TOOL_MENU);
                            break;

                            case MODE_BUILDER:
                                //not defined yet
                            break;
                        }
                    }
                break;

                //enter builder mode
                case 1:
                break;

                //send save pb command
                case 2:
                    ui_comm_webgui_send(CMD_PEDALBOARD_SAVE, strlen(CMD_PEDALBOARD_SAVE));
                    ui_comm_webgui_wait_response();

                    //also give quick overlay
                break;
            }
        break;
    }
}

void naveg_button_released(uint8_t button)
{
    (void) button;
    if (!g_initialized) return;

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
    }
}

//used for the shift button
void naveg_shift_pressed()
{
    //enter shift mode
    //save to return
    g_prev_shift_device_mode = g_device_mode;

    //turn of tuner if applicable
    if (g_prev_shift_device_mode == MODE_TOOL_FOOT)
        TM_turn_off_tuner();

    //toggle shift
    g_device_mode = MODE_SHIFT;

    //make sure all values are up to date
    //system_inp_1_volume_cb(TM_get_menu_item_by_ID(INP_1_GAIN_ID), MENU_EV_NONE);
    //system_inp_2_volume_cb(TM_get_menu_item_by_ID(INP_2_GAIN_ID), MENU_EV_NONE);
    //system_outp_1_volume_cb(TM_get_menu_item_by_ID(OUTP_1_GAIN_ID), MENU_EV_NONE);
    //system_outp_2_volume_cb(TM_get_menu_item_by_ID(OUTP_2_GAIN_ID), MENU_EV_NONE);

    screen_shift_overlay(g_prev_shift_device_mode);
}

void naveg_shift_releaed()
{
    //already entered some other mode
    if (g_device_mode != MODE_SHIFT)
        return;

    //exit the shift menu, return to opperational mode
    switch(g_prev_shift_device_mode)
    {
        case MODE_CONTROL:
            g_device_mode = MODE_CONTROL;
            CM_set_screen();
        break;

        case MODE_NAVIGATION:
            g_device_mode = MODE_NAVIGATION;
            NM_print_screen();
        break;

        case MODE_TOOL_FOOT:
            g_device_mode = MODE_TOOL_FOOT;
            TM_launch_tool(TOOL_TUNER);
        break;

        case MODE_TOOL_MENU:
            g_device_mode = MODE_TOOL_MENU;
            TM_launch_tool(TOOL_MENU);
        break;

        case MODE_BUILDER:
            //not defined yet
        break;
    }
}

uint8_t naveg_dialog_status(void)
{
    return dialog_active;
}

uint8_t naveg_dialog(const char *msg)
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
        item->desc = &desc;
        item->name = NULL;
        dummy_menu = node_create(item);
    }

    //display_disable_all_tools(DISPLAY_LEFT);
    //tool_on(DISPLAY_TOOL_SYSTEM, DISPLAY_LEFT);
    //tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, DISPLAY_RIGHT);

    //g_current_menu = dummy_menu;
    //g_current_item = dummy_menu->data;

    //screen_system_menu(g_current_item);

    dialog_active = 1;

    if (xSemaphoreTake(g_dialog_sem, portMAX_DELAY) == pdTRUE)
    {
        dialog_active = 0;
        //display_disable_all_tools(DISPLAY_LEFT);
        //display_disable_all_tools(DISPLAY_RIGHT);

        g_update_cb = NULL;
        g_update_data = NULL;

        //g_current_main_menu = g_menu;
        //g_current_main_item = g_menu->first_child->data;
        //reset_menu_hover(g_menu);

        screen_clear();

        //return g_current_item->data.hover;
        return -1;
    }
    //we can never get here, portMAX_DELAY means wait indefinatly I'm adding this to remove a compiler warning
    else
    {
        //ERROR
        return -1; 
    }
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
            CM_set_screen();
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
    }
}