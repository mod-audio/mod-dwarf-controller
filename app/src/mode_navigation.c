
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
#include "ui_comm.h"
#include "sys_comm.h"
#include "images.h"

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

enum {BANKS_LIST, PEDALBOARD_LIST, SNAPSHOT_LIST};

#define PAGE_DIR_INIT 0
#define PAGE_DIR_DOWN 1
#define PAGE_DIR_UP 2

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

static bp_list_t *g_banks, *g_pedalboards, g_snapshots;
static uint16_t g_bp_state, g_current_pedalboard, g_bp_first, g_pb_footswitches;
static menu_item_t *g_current_item, *g_current_main_item;
static bank_config_t g_bank_functions[BANK_FUNC_COUNT];
static int16_t g_current_bank, g_force_update_pedalboard;
static void (*g_update_cb)(void *data, int event);
static void *g_update_data;

static control_t *g_controls[ENCODERS_COUNT], *g_foots[FOOTSWITCHES_COUNT];

static uint8_t g_current_list = PEDALBOARD_LIST;

/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

//static uint8_t bank_config_check(uint8_t foot);
//static void bank_config_update(uint8_t bank_func_idx);
//static void bank_config_footer(void);

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
static void parse_banks_list(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;
    uint16_t count = strarr_length(&list[5]);

    // workaround freeze when opening menu
    delay_ms(20);

    // free the current banks list
    if (g_banks) data_free_banks_list(g_banks);

    // parses the list
    g_banks = data_parse_banks_list(&list[5], count);

    if (g_banks)
    {
        g_banks->menu_max = (atoi(list[2]));
        g_banks->page_min = (atoi(list[3]));
        g_banks->page_max = (atoi(list[4]));
    }

    NM_set_banks(g_banks);
}

//only toggled from the naveg toggle tool function
static void request_banks_list(uint8_t dir)
{
    g_bp_state = BANKS_LIST;

    // sets the response callback
    comm_webgui_set_response_cb(parse_banks_list, NULL);

    char buffer[40];
    memset(buffer, 0, 20);
    uint8_t i;

    i = copy_command(buffer, CMD_BANKS); 

    // insert the direction on buffer
    i += int_to_str(dir, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    //insert current bank, because first time we are entering the menu
    i += int_to_str(g_current_bank, &buffer[i], sizeof(buffer) - i, 0);

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);

    g_banks->hover = g_current_bank;
    g_banks->selected = g_current_bank;
}

//requested from the bp_up / bp_down functions when we reach the end of a page
static void request_next_bank_page(uint8_t dir)
{
    //we need to save our current hover and selected here, the parsing function will discard those
    uint8_t prev_hover = g_banks->hover;
    uint8_t prev_selected = g_banks->selected;

    //request the banks as usual
    g_bp_state = BANKS_LIST;

    // sets the response callback
    ui_comm_webgui_set_response_cb(parse_banks_list, NULL);

    char buffer[40];
    memset(buffer, 0, sizeof buffer);
    uint8_t i;

    i = copy_command(buffer, CMD_BANKS); 

    // insert the direction on buffer
    i += int_to_str(dir, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    i += int_to_str(g_banks->hover, &buffer[i], sizeof(buffer) - i, 0);

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);

    //restore our previous hover / selected bank
    g_banks->hover = prev_hover;
    g_banks->selected = prev_selected;
}

//called from the request functions and the naveg_initail_state
static void parse_pedalboards_list(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;
    uint32_t count = strarr_length(&list[5]);

    // workaround freeze when opening menu
    delay_ms(20);

    // free the navigation pedalboads list
    if (g_pedalboards)
        data_free_pedalboards_list(g_pedalboards);

    // parses the list
    g_pedalboards = data_parse_pedalboards_list(&list[5], count);

    if (g_pedalboards)
    {
        g_pedalboards->menu_max = (atoi(list[2]));
        g_pedalboards->page_min = (atoi(list[3]));
        g_pedalboards->page_max = (atoi(list[4])); 
    }
}

//requested when clicked on a back
static void request_pedalboards(uint8_t dir, uint16_t bank_uid)
{
    uint8_t i;
    char buffer[40];
    memset(buffer, 0, sizeof buffer);

    // sets the response callback
    ui_comm_webgui_set_response_cb(parse_pedalboards_list, NULL);

    i = copy_command((char *)buffer, CMD_PEDALBOARDS);

    uint8_t bitmask = 0;
    if (dir == 1) {bitmask |= FLAG_PAGINATION_PAGE_UP;}
    else if (dir == 2) bitmask |= FLAG_PAGINATION_INITIAL_REQ;

    // insert the direction on buffer
    i += int_to_str(bitmask, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    if ((dir == 2) && (g_current_bank != g_banks->hover))
    {
        // insert the current hover on buffer
        i += int_to_str(0, &buffer[i], sizeof(buffer) - i, 0);
    } 
    else
    {
        // insert the current hover on buffer
        i += int_to_str(g_pedalboards->hover - 1, &buffer[i], sizeof(buffer) - i, 0);
    }

    // inserts one space
    buffer[i++] = ' ';

    // copy the bank uid
    i += int_to_str((bank_uid), &buffer[i], sizeof(buffer) - i, 0);

    buffer[i++] = 0;

    uint16_t prev_hover = g_current_pedalboard;
    uint16_t prev_selected = g_current_pedalboard;

    if (g_pedalboards)
    {
        prev_hover = g_pedalboards->hover;
        prev_selected = g_pedalboards->selected;
    }
    
    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);

    if (g_pedalboards)
    {
        g_pedalboards->hover = prev_hover;
        g_pedalboards->selected = prev_selected;
    }
}

static void send_load_pedalboard(uint16_t bank_id, const char *pedalboard_uid)
{
    uint16_t i;
    char buffer[40];
    memset(buffer, 0, sizeof buffer);

    i = copy_command((char *)buffer, CMD_PEDALBOARD_LOAD);

    // copy the bank id
    i += int_to_str(bank_id, &buffer[i], 8, 0);

    // inserts one space
    buffer[i++] = ' ';

    const char *p = pedalboard_uid;
    // copy the pedalboard uidf
    if (!*p) 
    {
        buffer[i++] = '0';
    }
    else
    {
        while (*p)
        {
            buffer[i++] = *p;
            p++;
        }
    }
    buffer[i] = 0;

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    // sets the response callback
    ui_comm_webgui_set_response_cb(NULL, NULL);

    // send the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboard loaded message to be received
    ui_comm_webgui_wait_response();

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void NM_init(void)
{
    g_banks = NULL;
    g_pedalboards = NULL;
    g_bp_state = BANKS_LIST; 

    // initializes the bank functions
    uint8_t i;
    for (i = 0; i < BANK_FUNC_COUNT; i++)
    {
        g_bank_functions[i].function = BANK_FUNC_NONE;
        g_bank_functions[i].hw_id = 0xFF;
    }
}

void NM_initial_state(uint16_t max_menu, uint16_t page_min, uint16_t page_max, char *bank_uid, char *pedalboard_uid, char **pedalboards_list)
{
    if (!pedalboards_list)
    {
        if (g_banks)
        {
            g_banks->selected = 0;
            g_banks->hover = 0;
            g_banks->page_min = 0;
            g_banks->page_max = 0;
            g_banks->menu_max = 0;
            g_current_bank = 0;
        }

        if (g_pedalboards)
        {
            g_pedalboards->selected = 0;
            g_pedalboards->hover = 0;
            g_pedalboards->page_min = 0;
            g_pedalboards->page_max = 0;
            g_pedalboards->menu_max = 0;
            g_current_pedalboard = 0;
        }

        return;
    }

    // sets the bank index
    uint16_t bank_id = atoi(bank_uid);
    g_current_bank = bank_id;
    if (g_banks)
    {
        g_banks->selected = bank_id;
        g_banks->hover = bank_id;
    }

    // checks and free the navigation pedalboads list
    if (g_pedalboards) data_free_pedalboards_list(g_pedalboards);

    // parses the list
    g_pedalboards = data_parse_pedalboards_list(pedalboards_list, strarr_length(pedalboards_list));

    if (!g_pedalboards) return;

    g_pedalboards->page_min = page_min;
    g_pedalboards->page_max = page_max;
    g_pedalboards->menu_max = max_menu;

    g_current_pedalboard = atoi(pedalboard_uid) + 1;

    g_pedalboards->hover = g_current_pedalboard;
    g_pedalboards->selected = g_current_pedalboard;

    //MDW_TODO INIT SNAPSHOTS HERE
}

void NM_enter(void)
{
    const char *title;
/*
    if (naveg_ui_status())
    {
        tool_off(DISPLAY_TOOL_NAVIG);
        tool_on(DISPLAY_TOOL_SYSTEM, 0);
        tool_on(DISPLAY_TOOL_SYSTEM_SUBMENU, 1);
        screen_system_menu(g_current_item);
    }
*/
    if (!g_banks)
        return;

    if (g_bp_state == BANKS_LIST)
    {
        //make sure that we request the right page if we have a selected pedalboard
        if (g_current_bank == g_banks->hover)
            g_pedalboards->hover = g_current_pedalboard;

        //index is relevent in our array so - page_min
        request_pedalboards(PAGE_DIR_INIT, atoi(g_banks->uids[g_banks->hover - g_banks->page_min]));

        // if reach here, received the pedalboards list
        g_bp_state = PEDALBOARD_LIST;
        //index is relevent in our array so - page_min
        title = g_banks->names[g_banks->hover - g_banks->page_min];
        g_bp_first = 1;
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
        // checks if is the first option (back to banks list)
        if (g_pedalboards->hover == 0)
        {
            g_bp_state = BANKS_LIST;
            title = "BANKS";
        }
        else
        {
            g_bp_first=0;
            g_pb_footswitches = 1; 

            // request to GUI load the pedalboard
            //index is relevant in the array so - page_min, also the HMI array is always shifted right 1 because of back to banks, correct here
            send_load_pedalboard(atoi(g_banks->uids[g_banks->hover - g_banks->page_min]), g_pedalboards->uids[g_pedalboards->hover - g_pedalboards->page_min - 1]);

            g_current_pedalboard = g_pedalboards->hover;

            g_force_update_pedalboard = 1;

            // stores the current bank and pedalboard
            g_banks->selected = g_banks->hover;
            g_current_bank = g_banks->selected;

            // sets the variables to update the screen
            title = g_banks->names[g_banks->hover - g_banks->page_min];
        }
    }
    else
    {
        return;
    }

    //check if we need to display the selected item or out of bounds
    if (g_current_bank == g_banks->hover)
    {
        g_pedalboards->selected = g_current_pedalboard;
        g_pedalboards->hover = g_current_pedalboard;
    }
    else 
    {
        g_pedalboards->selected = g_pedalboards->menu_max + 1;
        g_pedalboards->hover = 0;
    }

    screen_bp_list(title, (g_bp_state == PEDALBOARD_LIST)? g_pedalboards : g_banks);
}

void NM_up(void)
{
    bp_list_t *bp_list = 0;
    const char *title = 0;

    if (!g_banks) return;

    if (g_bp_state == BANKS_LIST)
    {
        //if we are nearing the final 3 items of the page, and we already have the end of the page in memory, or if we just need to go down
        if(g_banks->page_min == 0) 
        {
            //check if we are not already at the end
            if (g_banks->hover == 0) return;
            else 
            {
                //we still have banks in our list
                g_banks->hover--;

                bp_list = g_banks;
                title = "BANKS";
            }
        }
        //are we comming from the bottom of the menu?
        else
        {
            //we always keep 3 items in front of us, if not request new page
            if (g_banks->hover <= (g_banks->page_min + 3))
            {
                g_banks->hover--;
                title = "BANKS";

                //request new page
                request_next_bank_page(PAGE_DIR_DOWN);

                bp_list = g_banks;
            }   
            //we have items, just go up
            else 
            {
                g_banks->hover--;
                bp_list = g_banks;
                title = "BANKS";
            }
        }
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
        //are we reaching the bottom of the menu?
        if(g_pedalboards->page_min == 0) 
        {
            //check if we are not already at the end
            if (g_pedalboards->hover == 0) return;
            else 
            {
                //go down till the end
                //we still have items in our list
                g_pedalboards->hover--;
                bp_list = g_pedalboards;
                title = g_banks->names[g_banks->hover - g_banks->page_min];
            }
        }
        else
        {
            //we always keep 3 items in front of us, if not request new page, we add 4 because the bottom item is always "> back to banks" and we dont show that here
            if (g_pedalboards->hover <= (g_pedalboards->page_min + 5))
            {
                g_pedalboards->hover--;
                title = g_banks->names[g_banks->hover - g_banks->page_min];
                //request new page
                request_pedalboards(PAGE_DIR_DOWN, atoi(g_banks->uids[g_banks->hover - g_banks->page_min]));

                bp_list = g_pedalboards;
            }   
            //we have items, just go up
            else 
            {
                g_pedalboards->hover--;
                bp_list = g_pedalboards;
                title = g_banks->names[g_banks->hover - g_banks->page_min];
            }
        }
    }
    else return;

    screen_bp_list(title, bp_list);
}

void NM_down(void)
{
    bp_list_t *bp_list = 0;
    const char *title = 0;

    if (!g_banks) return;

    if (g_bp_state == BANKS_LIST)
    {
        //are we reaching the bottom of the menu?
        if(g_banks->page_max == g_banks->menu_max) 
        {
            //check if we are not already at the end
            if (g_banks->hover >= g_banks->menu_max -1) return;
            else 
            {
                //we still have banks in our list
                g_banks->hover++;
                bp_list = g_banks;
                title = "BANKS";
            }
        }
        else 
        {
            //we always keep 3 items in front of us, if not request new page
            if (g_banks->hover >= (g_banks->page_max - 4))
            {
                g_banks->hover++;
                title = "BANKS";
                //request new page
                request_next_bank_page(PAGE_DIR_UP);

                bp_list = g_banks;
            }   
            //we have items, just go down
            else 
            {
                g_banks->hover++;
                bp_list = g_banks;
                title = "BANKS";
            }
        }
    }
    else if (g_bp_state == PEDALBOARD_LIST)
    {
        //are we reaching the bottom of the menu, -1 because menu max is bigger then page_max
        if(g_pedalboards->page_max == g_pedalboards->menu_max) 
        {
            //check if we are not already at the end, we dont need so substract by one, since the "> back to banks" item is added on parsing
            if (g_pedalboards->hover == g_pedalboards->menu_max) return;
            else 
            {
                //we still have items in our list
                g_pedalboards->hover++;
                bp_list = g_pedalboards;
                title = g_banks->names[g_banks->hover - g_banks->page_min];
            }
        }
        else 
        {
            //we always keep 3 items in front of us, if not request new page, we need to substract by 5, since the "> back to banks" item is added on parsing 
            if (g_pedalboards->hover >= (g_pedalboards->page_max - 4))
            {
                title = g_banks->names[g_banks->hover - g_banks->page_min];
                //request new page
                request_pedalboards(PAGE_DIR_UP, atoi(g_banks->uids[g_banks->hover - g_banks->page_min]));

                g_pedalboards->hover++;
                bp_list = g_pedalboards;
            }   
            //we have items, just go down
            else 
            {
                g_pedalboards->hover++;
                bp_list = g_pedalboards;
                title = g_banks->names[g_banks->hover - g_banks->page_min];
            }
        }
    }
    else return;

    screen_bp_list(title, bp_list);
}

void NM_set_banks(bp_list_t *bp_list)
{
    if (!g_initialized) return;

    g_banks = bp_list;
}

bp_list_t *NM_get_banks(void)
{
    if (!g_initialized) return NULL;

    return (g_bp_state == BANKS_LIST) ? g_banks : g_pedalboards;
}


void NM_set_pedalboards(bp_list_t *bp_list)
{
    if (!g_initialized) return;

    g_pedalboards = bp_list;
}

bp_list_t *NM_get_pedalboards(void)
{
    if (!g_initialized) return NULL;

    return g_pedalboards;
}

char* NM_get_current_pb_name(void)
{
    return g_banks->names[g_banks->hover];
}

void NM_update(void)
{
    if (g_update_cb)
    {
        (*g_update_cb)(g_update_data, MENU_EV_ENTER);
        screen_system_menu(g_update_data);
    }
}

int NM_need_update(void)
{
    return (g_update_cb ? 1: 0);
}

void NM_print_screen(void)
{
    switch(g_current_list)
    {
        case BANKS_LIST:
        break;

        case PEDALBOARD_LIST:
            screen_bp_list("PEDALBOARDS", g_pedalboards);
        break;

        case SNAPSHOT_LIST:
        break;
    }
}