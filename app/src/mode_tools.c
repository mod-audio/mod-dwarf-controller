
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

#include "config.h"
#include "hardware.h"
#include "serial.h"
#include "protocol.h"
#include "glcd.h"
#include "ledz.h"
#include "actuator.h"
#include "data.h"
#include "naveg.h"
#include "screen.h"
#include "cli.h"
#include "node.h"
#include "ui_comm.h"
#include "sys_comm.h"
#include "images.h"

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

enum {TOOL_OFF, TOOL_ON};

#define MAX_CHARS_MENU_NAME     (128/4)
#define MAX_TOOLS               5

/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

static menu_desc_t g_menu_desc[] = {
    SYSTEM_MENU
    {NULL, 0, -1, -1, NULL, 0}
};

static const menu_popup_t g_menu_popups[] = {
    POPUP_CONTENT
    {-1, NULL, NULL}
};

/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/

struct TOOL_T {
    uint8_t state, display;
} g_tool[MAX_TOOLS];

/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/

#define MAP(x, Omin, Omax, Nmin, Nmax)      ( x - Omin ) * (Nmax -  Nmin)  / (Omax - Omin) + Nmin;

/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static node_t *g_menu, *g_current_menu;
static menu_item_t *g_current_item;
static uint8_t g_max_items_list;
static void (*g_update_cb)(void *data, int event);
static void *g_update_data;


static xSemaphoreHandle g_dialog_sem;
static uint8_t dialog_active = 0;
static uint16_t g_bp_state = 0;
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

static void tool_on(uint8_t tool)
{
    g_tool[tool].state = TOOL_ON;
}

static void tool_off(uint8_t tool)
{
    g_tool[tool].state = TOOL_OFF;
}

static int tool_is_on(uint8_t tool)
{
    return g_tool[tool].state;
}

static void display_disable_all_tools(uint8_t display)
{
    int i;

    if (tool_is_on(DISPLAY_TOOL_TUNER))
    {
        g_protocol_busy = true;
        system_lock_comm_serial(g_protocol_busy);

        // sends the data to GUI
        ui_comm_webgui_send(CMD_TUNER_OFF, strlen(CMD_TUNER_OFF));

        // waits the pedalboards list be received
        ui_comm_webgui_wait_response();

        g_protocol_busy = false;
        system_lock_comm_serial(g_protocol_busy);
    }
    
    for (i = 0; i < MAX_TOOLS; i++)
    {
        if (g_tool[i].display == display)
            g_tool[i].state = TOOL_OFF;
    }
}

static void menu_enter(uint8_t display_id)
{
    uint8_t i;
    node_t *node = g_current_menu;
    menu_item_t *item = g_current_item;

    //we are either in 2 types of menu, the root list, or in a main item that holds 3 ellements
    if (item->desc->type == MENU_ROOT)
    {
        // locates the clicked item
        node = g_current_menu->first_child;
        for (i = 0; i < item->data.hover; i++) node = node->next;

        // gets the menu item
        item = node->data;
        g_current_item = node->data;
    }
    else if (item->desc->type == MENU_LIST)
    {/*
        // locates the clicked item
        node = display_id ? g_current_menu->first_child : g_current_main_menu->first_child;
        for (i = 0; i < item->data.hover; i++) node = node->next;

        // gets the menu item
        item = node->data;
        // checks if is 'back to previous'
        if (item->desc->type == MENU_RETURN)
        {
            if (item->desc->action_cb)
                item->desc->action_cb(item, MENU_EV_ENTER);

            node = node->parent->parent;
            item = node->data;
        }

        //extra check if we need to switch back to non-ui connected mode on the current-pb and banks menu
        else if (((item->desc->id == BANKS_ID) || (item->desc->id == PEDALBOARD_ID)) && (!naveg_ui_status()))
        {
            item->desc->type = ((item->desc->id == PEDALBOARD_ID) ? MENU_LIST : MENU_NONE);
        }

        // updates the current item
        if ((item->desc->type != MENU_TOGGLE) && (item->desc->type != MENU_NONE)) g_current_item = node->data;
        // updates this specific toggle items (toggle items with pop-ups)
        if (item->desc->parent_id == PROFILES_ID) g_current_item = node->data;
    */
    }
    else if (item->desc->type == MENU_CONFIRM  || item->desc->type == MENU_OK)
    {
        /*
        // calls the action callback
        if ((item->desc->type != MENU_OK) && (item->desc->type != MENU_CANCEL) && (item->desc->action_cb))
            item->desc->action_cb(item, MENU_EV_ENTER);

        if ((item->desc->id == PEDALBOARD_ID) || (item->desc->id == BANKS_ID))
        {
            if (naveg_ui_status())
            {
                //change menu type to menu_ok to display pop-up
                item->desc->type = MENU_MESSAGE;
                g_current_item = item;
            }
            else
            {
                //reset the menu type to its original state
                item->desc->type = ((item->desc->id == PEDALBOARD_ID) ? MENU_LIST : MENU_NONE);
                g_current_item = item;
            }
        }
        else
        {
            // gets the menu item
            item = display_id ? g_current_menu->data : g_current_main_menu->data;
            g_current_item = item;
        }*/
    }

    // checks the selected item
    if (item->desc->type == MENU_ROOT)
    {
        // changes the current menu
        g_current_menu = node;

        // initialize the counter
        item->data.list_count = 0;

        // adds the menu lines
        for (node = node->first_child; node; node = node->next)
        {
            menu_item_t *item_child = node->data;

            item->data.list[item->data.list_count++] = item_child->name;
        }

        // calls the action callback
        if ((item->desc->action_cb)) item->desc->action_cb(item, MENU_EV_ENTER);
    }
    /*else if (item->desc->type == MENU_CONFIRM ||item->desc->type == MENU_OK)
    {
        if (item->desc->type == MENU_OK)
        {
            //if bleutooth activate right away
            if (item->desc->id == BLUETOOTH_ID+1)
            {
                item->desc->action_cb(item, MENU_EV_ENTER);
            }
            
            // highlights the default button
            item->data.hover = 0;

            // defines the buttons count
            item->data.list_count = 1;
        }
        else
        {
            // highlights the default button
            item->data.hover = 1;

            // defines the buttons count
            item->data.list_count = 2;
        }

        // default popup content value
        item->data.popup_content = NULL;
        // locates the popup menu
        i = 0;
        while (g_menu_popups[i].popup_content)
        {
            if (item->desc->id == g_menu_popups[i].menu_id)
            {
                item->data.popup_content = g_menu_popups[i].popup_content;
                item->data.popup_header = g_menu_popups[i].popup_header;

                break;
            }
            i++;
        }
    }
    else if (item->desc->type == MENU_TOGGLE)
    {
        // calls the action callback
        if (item->desc->action_cb) item->desc->action_cb(item, MENU_EV_ENTER);
    }
    else if (item->desc->type == MENU_OK)
    {
        // highlights the default button
        item->data.hover = 0;

        // defines the buttons count
        item->data.list_count = 1;

        // default popup content value
        item->data.popup_content = NULL;

        // locates the popup menu
        i = 0;
        while (g_menu_popups[i].popup_content)
        {
            if (item->desc->id == g_menu_popups[i].menu_id)
            {
                item->data.popup_content = g_menu_popups[i].popup_content;
                break;
            }
            i++;
        }
    }
    else if (item->desc->type == MENU_NONE)
    {
        // calls the action callback
        if (item->desc->action_cb)
            item->desc->action_cb(item, MENU_EV_ENTER);

        if (display_id)
        {
            item = g_current_menu->data;
            g_current_item = item;
        }
    }
    else if (item->desc->type == MENU_BAR)
    {
        if (display_id)
        {
            static uint8_t toggle = 0;
            if (toggle == 0)
            {
                toggle = 1;
                // calls the action callback
                if ((item->desc->action_cb) && (item->desc->type != MENU_SET))
                    item->desc->action_cb(item, MENU_EV_ENTER);
            }
            else
            {
                toggle = 0;
                if (item->desc->type == MENU_VOL)
                    system_save_gains_cb(item, MENU_EV_ENTER);
                else
                    item->desc->action_cb(item, MENU_EV_ENTER);
                //resets the menu node
                item = g_current_menu->data;
                g_current_item = item;
            }
        }
    }
*/
    if (item->desc->parent_id == DEVICE_ID && item->desc->action_cb)
        item->desc->action_cb(item, MENU_EV_ENTER);

    if (tool_is_on(DISPLAY_TOOL_SYSTEM))
    {
        screen_system_menu(item);
        g_update_cb = NULL;
        g_update_data = NULL;
        if (item->desc->need_update)
        {
            g_update_cb = item->desc->action_cb;
            g_update_data = item;
        }
    }

    if (item->desc->type == MENU_CONFIRM2)
    {
        dialog_active = 0;
        portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(g_dialog_sem, &xHigherPriorityTaskWoken);
    }
}

static void menu_up(uint8_t display_id)
{
    menu_item_t *item = g_current_item;

        if (item->data.hover > 0)
            item->data.hover--;

    if (item->desc->action_cb)
        item->desc->action_cb(item, MENU_EV_UP);

    screen_system_menu(item);
}


static void menu_down(uint8_t display_id)
{
    menu_item_t *item = g_current_item;

        if (item->data.hover < (item->data.list_count - 1))
            item->data.hover++;

    if (item->desc->action_cb)
        item->desc->action_cb(item, MENU_EV_DOWN);

    screen_system_menu(item);
}


static void tuner_enter(void)
{
    static uint8_t input = 1;

    char buffer[128];
    uint32_t i = copy_command(buffer, CMD_TUNER_INPUT);

    // toggle the input
    input = (input == 1 ? 2 : 1);

    // inserts the input number
    i += int_to_str(input, &buffer[i], sizeof(buffer) - i, 0);
    buffer[i] = 0;

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);

    // updates the screen
    screen_tuner_input(input);
}

static void create_menu_tree(node_t *parent, const menu_desc_t *desc)
{
    uint8_t i;
    menu_item_t *item;

    for (i = 0; g_menu_desc[i].name; i++)
    {
        if (desc->id == g_menu_desc[i].parent_id)
        {
            item = (menu_item_t *) MALLOC(sizeof(menu_item_t));
            item->data.hover = 0;
            item->data.selected = 0xFF;
            item->data.list_count = 0;
            item->data.list = (char **) MALLOC(g_max_items_list * sizeof(char *));
            item->desc = &g_menu_desc[i];
            item->name = MALLOC(MAX_CHARS_MENU_NAME);
            strcpy(item->name, g_menu_desc[i].name);

            node_t *node;
            node = node_child(parent, item);

            if (item->desc->type == MENU_ROOT)
                create_menu_tree(node, &g_menu_desc[i]);
        }
    }
}

static void reset_menu_hover(node_t *menu_node)
{
    node_t *node;
    for (node = menu_node->first_child; node; node = node->next)
    {
        menu_item_t *item = node->data;
        if (item->desc->type == MENU_ROOT) 
            item->data.hover = 0;
        reset_menu_hover(node);
    }
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void TM_init(void)
{
    // counts the maximum items amount in menu lists
    uint8_t count = 0;

    uint8_t i = 0;
    for (i = 0; g_menu_desc[i].name; i++)
    {
        uint8_t j = i + 1;
        for (; g_menu_desc[j].name; j++)
        {
            if (g_menu_desc[i].id == g_menu_desc[j].parent_id) count++;
        }

        if (count > g_max_items_list) g_max_items_list = count;
        count = 0;
    }

    // adds one to line 'back to previous menu'
    g_max_items_list++;

    // creates the menu tree (recursively)
    const menu_desc_t root_desc = {"root", MENU_ROOT, -1, -1, NULL, 0};
    g_menu = node_create(NULL);
    create_menu_tree(g_menu, &root_desc);

    // sets current menu
    g_current_menu = g_menu;
    g_current_item = g_menu->first_child->data;
}

void TM_trigger_tool(uint8_t tool, uint8_t status)
{
    g_tool[tool].state = status;
}

uint8_t TM_status(void)
{
    int i;
    for (i = 0; i < MAX_TOOLS; i++)
    {
        if (g_tool[i].state == TOOL_ON)
            return 1;
    }

    return 0;
}

uint8_t TM_has_tool_enabled(void)
{
    int i;
    for (i = 0; i < MAX_TOOLS; i++)
    {
        if (g_tool[i].state == TOOL_ON)
            return 1;
    }

    return 0;
}


void TM_enter(uint8_t encoder)
{
    if ((!g_initialized)&&(g_update_cb)) return;

    if (tool_is_on(DISPLAY_TOOL_TUNER))
    {
        //menu_enter(encoder);
    }
    else if (tool_is_on(DISPLAY_TOOL_SYSTEM))
    {
        // calls the action callback
        //if ((g_current_item->desc->action_cb)) g_current_item->desc->action_cb(g_current_item, MENU_EV_ENTER);
    }
}

void TM_up(uint8_t encoder)
{
    if (!g_initialized) return;
        
    if (tool_is_on(DISPLAY_TOOL_TUNER))
    {
        /*
            //naveg_toggle_tool((tool_is_on(DISPLAY_TOOL_TUNER) ? DISPLAY_TOOL_TUNER : DISPLAY_TOOL_NAVIG), encoder);
            tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
            g_current_menu = g_current_main_menu;
            g_current_item = g_current_main_item;
            menu_up(encoder);
            menu_enter(encoder);
        */
    }
    else if (tool_is_on(DISPLAY_TOOL_SYSTEM))
    {
        //first encoder, check if we need to change menu or item
        if (encoder == 0)
        {
            if (g_current_item->desc->type == MENU_ROOT)
            {
                menu_up(encoder);
            }
            else 
            {

            }
        }
        else
        {

        }
    }
}

void TM_down(uint8_t encoder)
{
    if (!g_initialized) return;

    if (tool_is_on(DISPLAY_TOOL_TUNER))
    {
        /*
            //naveg_toggle_tool((tool_is_on(DISPLAY_TOOL_TUNER) ? DISPLAY_TOOL_TUNER : DISPLAY_TOOL_NAVIG), encoder);
            tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
            g_current_menu = g_current_main_menu;
            g_current_item = g_current_main_item;
            menu_down(encoder);
            menu_enter(encoder);
        */
    }
    else if (tool_is_on(DISPLAY_TOOL_SYSTEM))
    {
        //first encoder, check if we need to change menu or item
        if (encoder == 0)
        {
            if (g_current_item->desc->type == MENU_ROOT)
            {
                menu_down(encoder);
            }
            else 
            {

            }
        }
        else
        {

        }
    }
}

void TM_reset_menu(void)
{
    if (!g_initialized) return;

    g_current_menu = g_menu;
    g_current_item = g_menu->first_child->data;
    reset_menu_hover(g_menu);
}

void TM_settings_refresh(void)
{
    screen_system_menu(g_current_item);
}

void TM_menu_refresh(void)
{
    node_t *node = g_current_menu;

    //updates all items in a menu
    for (node = node->first_child; node; node = node->next)
    {
        // gets the menu item
        menu_item_t *item = node->data;

        // calls the action callback
        if ((item->desc->action_cb)) item->desc->action_cb(item, MENU_EV_NONE);
    }
    TM_settings_refresh();
}

//the menu refresh is to slow for the gains so this one is added that only updates the set value.
void TM_update_gain(uint8_t display_id, uint8_t update_id, float value, float min, float max, uint8_t dir)
{
    node_t *node = g_current_menu;

    //updates all items in a menu
    for (node = node->first_child; node; node = node->next)
    {
        // gets the menu item
        menu_item_t *item = node->data;

        // updates the value
        if ((item->desc->id == update_id))
        {
            item->data.value = value;

            char str_buf[8];
            float value_bfr;
            if (!dir)
            {
                value_bfr = MAP(item->data.value, min, max, 0, 115);
                value_bfr -= 15;
            }
            else 
            {
                value_bfr = MAP(item->data.value, min, max, 0, 100);
            }
            int_to_str(value_bfr, str_buf, sizeof(str_buf), 0);
            strcpy(item->name, item->desc->name);
            uint8_t q;
            uint8_t value_size = strlen(str_buf);
            uint8_t name_size = strlen(item->name);
            for (q = 0; q < (31 - name_size - value_size - 1); q++)
            {
                strcat(item->name, " ");
            }
            strcat(item->name, str_buf);
            strcat(item->name, "%");
        }
    }
}

void TM_menu_item_changed_cb(uint8_t item_ID, uint16_t value)
{
    //set value in system.c
    system_update_menu_value(item_ID, value);

    //are we inside the menu? if so we need to update
    if (tool_is_on(DISPLAY_TOOL_SYSTEM))
    {
            if (!tool_is_on(DISPLAY_TOOL_TUNER) && !tool_is_on(DISPLAY_TOOL_NAVIG))
            {
                TM_menu_refresh();
            }
    }

    //when we are not in the menu, did we change the master volume link?
        //TODO update the master volume link widget
}

void TM_launch_tool(uint8_t tool)
{
    screen_clear();

    switch (tool)
    {
        case TOOL_MENU:
            TM_reset_menu();
            tool_on(DISPLAY_TOOL_SYSTEM);
            menu_enter(0);
        break;

        case TOOL_TUNER:
        break;

        case TOOL_SYNC:
        break;

        case TOOL_BYPASS:
        break;
    }
}