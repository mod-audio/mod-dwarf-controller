
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

static void (*g_update_cb)(void *data, int event);
static void *g_update_data;

static uint8_t g_device_mode, g_prev_device_mode;

uint8_t g_initialized = 0;

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

    g_initialized = 1;

    vSemaphoreCreateBinary(g_dialog_sem);
    // vSemaphoreCreateBinary is created as available which makes
    // first xSemaphoreTake pass even if semaphore has not been given
    // http://sourceforge.net/p/freertos/discussion/382005/thread/04bfabb9
    xSemaphoreTake(g_dialog_sem, 0);
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

        // reset the banks and pedalboards state after return from ui connection
        //if (g_banks) data_free_banks_list(g_banks);
        //if (g_naveg_pedalboards) data_free_pedalboards_list(g_naveg_pedalboards);
        //g_banks = NULL;
        //g_naveg_pedalboards = NULL;
    }
    
    //if we where in some menu's, we might need to exit them
    switch(g_device_mode)
    {
        case MODE_CONTROL:
            //pass to control mode 
        break;

        case MODE_NAVIGATION:
            //pass to navigation code
        break;

        case MODE_TOOL:
            //pass for tuner controls
        break;

        case MODE_BUILDER:
            //not defined yet
        break;
    }

    /*
    if ((tool_mode_status(DISPLAY_TOOL_NAVIG)) || tool_mode_status(DISPLAY_TOOL_SYSTEM))
    {
        naveg_toggle_tool(DISPLAY_TOOL_SYSTEM, 0);
        node_t *node = g_current_main_menu;

        //sets the pedalboard items back to original
        for (node = node->first_child; node; node = node->next)
        {
            // gets the menu item
            menu_item_t *item = node->data;

            if ((item->desc->id == PEDALBOARD_ID) || (item->desc->id == BANKS_ID))
            {
                item->desc->type = ((item->desc->id == PEDALBOARD_ID) ? MENU_LIST : MENU_NONE);
                g_current_item = item;
            }
        }
    }*/

}

uint8_t naveg_ui_status(void)
{
    return g_ui_connected;
}

void naveg_enc_enter(uint8_t encoder)
{
    (void) encoder;
}

void naveg_enc_hold(uint8_t encoder)
{
    (void) encoder;
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
            //pass to navigation code
            NM_down();
        break;

        case MODE_TOOL:
            //pass for tuner/menu/bypass controls
            TM_down(encoder);
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
            //pass to navigation code
            NM_up();
        break;

        case MODE_TOOL:
            //pass for tuner/menu/bypass controls
            TM_up(encoder);
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
            }
            else 
            {
                NM_change_pbss(foot);
            }
            
        break;

        case MODE_TOOL:

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
                NM_print_screen();
                g_device_mode = MODE_NAVIGATION;
            break;

            case MODE_NAVIGATION:
                //enter control mode
                CM_print_screen();
                g_device_mode = MODE_CONTROL;
            break;

            case MODE_TOOL:
                //we enter the prev mode
                if (g_prev_device_mode == MODE_NAVIGATION)
                {
                    NM_print_screen();
                    g_device_mode = MODE_NAVIGATION;
                }
                else
                {
                    CM_print_screen();
                    g_device_mode = MODE_CONTROL;
                }
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
        if ((g_device_mode == MODE_CONTROL) || (g_device_mode == MODE_NAVIGATION))
        {
            g_prev_device_mode = g_device_mode;
            g_device_mode = MODE_TOOL;
            TM_launch_tool(TOOL_TUNER);
        }
        else if (g_device_mode == MODE_TOOL)
        {
            g_device_mode = g_prev_device_mode;

            if (g_device_mode == MODE_CONTROL)
                CM_print_screen();
            else 
                NM_print_screen();
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

        case MODE_TOOL:
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
                    if (g_prev_device_mode == MODE_CONTROL)
                    {
                        g_device_mode = MODE_TOOL;
                        TM_launch_tool(TOOL_MENU);
                    }
                    else 
                    {
                        g_device_mode = MODE_CONTROL;
                        CM_print_screen();
                    }
                break;

                //enter builder mode
                case 1:
                break;

                //send save pb command
                case 2:
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
            //pass to control mode 
        break;

        case MODE_NAVIGATION:
            //pass to navigation code
        break;

        case MODE_TOOL:
            //pass for tuner controls
        break;

        case MODE_BUILDER:
            //not defined yet
        break;
    }
}

//used for the shift button
void naveg_shift_pressed()
{
    //enter shift mode
    //save to return
    g_prev_device_mode = g_device_mode;

    //toggle shift
    g_device_mode = MODE_SHIFT;

    screen_shift_overlay(g_prev_device_mode);
}

void naveg_shift_releaed()
{
    //already entered some other mode
    if (g_device_mode != MODE_SHIFT)
        return;

    //exit the shift menu, return to opperational mode
    switch(g_prev_device_mode)
    {
        case MODE_CONTROL:
            CM_print_screen();
            g_device_mode = MODE_CONTROL;
        break;

        case MODE_NAVIGATION:
        break;

        case MODE_TOOL:
            TM_launch_tool(TOOL_MENU);
            g_device_mode = MODE_TOOL;
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