
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

static uint8_t g_force_update_pedalboard = 1;
static uint8_t g_scroll_dir = 1;

// only enabled after "boot" command received
bool g_should_wait_for_webgui = false;


//yolo
static bp_list_t *g_banks, *g_naveg_pedalboards, g_footswitch_pedalboards;
static uint16_t g_bp_state, g_current_pedalboard, g_bp_first, g_pb_footswitches;
static menu_item_t *g_current_item, *g_current_main_item;
static bank_config_t g_bank_functions[BANK_FUNC_COUNT];
static void (*g_update_cb)(void *data, int event);
static void *g_update_data;
static node_t *g_menu, *g_current_menu, *g_current_main_menu;


static control_t *g_controls[ENCODERS_COUNT], *g_foots[FOOTSWITCHES_COUNT];

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
        if (g_banks) data_free_banks_list(g_banks);
        if (g_naveg_pedalboards) data_free_pedalboards_list(g_naveg_pedalboards);
        g_banks = NULL;
        g_naveg_pedalboards = NULL;
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

}

void naveg_enc_hold(uint8_t encoder)
{
    
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
        break;

        case MODE_TOOL:
            //pass for tuner controls
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
        break;

        case MODE_TOOL:
            //pass for tuner controls
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
                    CM_load_next_footswitch_page();
            }
            else 
            {
                //pass to control mode 
                CM_foot_control_change(foot, pressed);
            }
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
        break;

        case MODE_TOOL:
            //pass for tuner controls
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
                        g_device_mode = MODE_TOOL;
                    else 
                        g_device_mode = MODE_CONTROL;
                    
                    TM_launch_tool(TOOL_MENU);
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

/*
void naveg_toggle_tool(uint8_t tool, uint8_t display)
{
    if (!g_initialized) return;
    static uint8_t banks_loaded = 0;
    // clears the display
    screen_clear(display);

    // changes the display to tool mode
    if (1)
    {
        // action to do when the tool is enabled
        switch (tool)
        {
            case DISPLAY_TOOL_NAVIG:
                // initial state to banks/pedalboards navigation
                if (!banks_loaded) request_banks_list(2);
                banks_loaded = 1;
                tool_mode_trigger_tool(DISPLAY_TOOL_SYSTEM_SUBMENU, 0);
                display = 1;
                g_bp_state = BANKS_LIST;
                break;
            case DISPLAY_TOOL_TUNER:
                display_disable_all_tools(display);

                g_protocol_busy = true;
                system_lock_comm_serial(g_protocol_busy);

                // sends the data to GUI
                ui_comm_webgui_send(CMD_TUNER_ON, strlen(CMD_TUNER_ON));

                // waits the pedalboards list be received
                ui_comm_webgui_wait_response();

                g_protocol_busy = false;
                system_lock_comm_serial(g_protocol_busy);

                break;
            case DISPLAY_TOOL_SYSTEM:
                screen_clear(1);
                display_disable_all_tools(DISPLAY_LEFT);
                display_disable_all_tools(DISPLAY_RIGHT);
                tool_mode_trigger_tool(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
        }

        // draws the tool
        tool_on(tool, display);
        screen_tool(tool, display);

        if (tool == DISPLAY_TOOL_SYSTEM)
        {
            //already enter banks menu on display 1
            menu_enter(1);
            g_current_main_menu = g_current_menu;
            g_current_main_item = g_current_item;
            menu_enter(0);
        }
    }
    // changes the display to control mode
    else
    {
        // action to do when the tool is disabled
        switch (tool)
        {
            case DISPLAY_TOOL_SYSTEM:
                display_disable_all_tools(display);
                display_disable_all_tools(1);
                g_update_cb = NULL;
                g_update_data = NULL;
                banks_loaded = 0;
                // force save gains when leave the menu
                system_save_gains_cb(NULL, MENU_EV_ENTER);
                break;
            case DISPLAY_TOOL_NAVIG:
                tool_off(DISPLAY_TOOL_NAVIG);
                tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
                return;
                break;

            case DISPLAY_TOOL_TUNER:

                g_protocol_busy = true;
                system_lock_comm_serial(g_protocol_busy);

                // sends the data to GUI
                ui_comm_webgui_send(CMD_TUNER_OFF, strlen(CMD_TUNER_OFF));

                // waits the pedalboards list be received
                ui_comm_webgui_wait_response();

                g_protocol_busy = false;
                system_lock_comm_serial(g_protocol_busy);

                tool_off(DISPLAY_TOOL_TUNER);

                if (tool_is_on(DISPLAY_TOOL_SYSTEM))
                {
                   tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1); 
                   return;
                } 

                break;
        }

        //clear previous commands in the buffer
        ui_comm_webgui_clear();

        control_t *control = g_controls[display];

        // draws the control
        screen_encoder(display, control);

        //draw the top bar
        display ? screen_ss_name(NULL, 0) : screen_pb_name(NULL, 0);

        //draw the index (do not update values)
        naveg_set_index(0, display, 0, 0);

        // checks the function assigned to foot and update the footer
        if (bank_config_check(display)) bank_config_footer();
        else if (g_foots[display]) foot_control_add(g_foots[display]);
        else screen_footer(display, NULL, NULL, 0);

        if (tool == DISPLAY_TOOL_SYSTEM)
        {
            screen_clear(1);
            control = g_controls[1];
            display = 1; 
     
            // draws the control
            screen_encoder(display, control);

            //draw the top bar
            screen_ss_name(NULL, 0);
    
            //draw the index (do not update values)
            naveg_set_index(0, display, 0, 0);

            // checks the function assigned to foot and update the footer
            if (bank_config_check(display)) bank_config_footer();
            else if (g_foots[display]) foot_control_add(g_foots[display]);
            else screen_footer(display, NULL, NULL, 0);
        }
    }
}
*/
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

    g_current_menu = dummy_menu;
    g_current_item = dummy_menu->data;

    screen_system_menu(g_current_item);

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

        return g_current_item->data.hover;
    }
    //we can never get here, portMAX_DELAY means wait indefinatly I'm adding this to remove a compiler warning
    else
    {
        //ERROR
        return -1; 
    }
}