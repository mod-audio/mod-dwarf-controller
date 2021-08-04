
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

#define PAGE_DIR_DOWN 0
#define PAGE_DIR_UP 1
#define PAGE_DIR_INIT 2

#define PB_MODE 0
#define SS_MODE 1

#define NO_GRAB_ITEM -1

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

static bp_list_t *g_banks, *g_pedalboards, *g_snapshots;
static bank_config_t g_bank_functions[BANK_FUNC_COUNT];
static uint16_t g_current_pedalboard, g_current_snapshot, g_current_add_bank;
static int16_t g_current_bank, g_force_update_pedalboard;
static char *g_grabbed_item_label;
static uint16_t* g_uids_to_add_to_bank;
static uint8_t g_current_list = PEDALBOARD_LIST, g_snapshots_loaded = 0;;
static int8_t g_item_grabbed = NO_GRAB_ITEM;
static uint16_t g_item_grabbed_uid;
static char* g_pedalboard_name = NULL;
static char* g_snapshot_name = NULL;


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
//use to parse commands to mod-ui
static void send_navigation_command(char *cmd, void *data, uint8_t arguments_count)
{
    // sets the response callback
    ui_comm_webgui_set_response_cb(NULL, NULL);

    char buffer[40];
    memset(buffer, 0, 20);
    uint8_t i;

    uint8_t *arguments = data;

    i = copy_command(buffer, cmd);

    uint8_t j;
    for (j=0; j < arguments_count; j++) {
        // inserts one space
        if (j != 0)
            buffer[i++] = ' ';

        // insert the data in the buffer
        i += int_to_str(arguments[j], &buffer[i], sizeof(buffer) - i, 0);
    }

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();
}
*/

static void parse_banks_list(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;
    uint16_t count = strarr_length(&list[5]);

    uint16_t prev_hover = g_current_bank;
    uint16_t prev_selected = g_current_bank;

    // workaround freeze when opening menu
    delay_ms(20);

    // free the current banks list
    if (g_banks) {
        prev_hover = g_banks->hover;
        prev_selected = g_banks->selected;

        data_free_banks_list(g_banks);
    }

    // parses the list
    g_banks = data_parse_banks_list(&list[5], count);

    if (g_banks) {
        g_banks->menu_max = (atoi(list[2]));
        g_banks->page_min = (atoi(list[3]));
        g_banks->page_max = (atoi(list[4]));
    }

    g_banks->hover = prev_hover;
    g_banks->selected = prev_selected;

    NM_set_banks(g_banks);
}

//only toggled from the naveg toggle tool function
static void request_banks_list(uint8_t dir)
{
    // sets the response callback
    ui_comm_webgui_set_response_cb(parse_banks_list, NULL);

    char buffer[40];
    memset(buffer, 0, 20);
    uint8_t i;

    i = copy_command(buffer, CMD_BANKS); 

    // insert the direction on buffer
    i += int_to_str(dir, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    //insert current bank, because first time we are entering the menu
    if (g_banks)
        i += int_to_str(g_banks->selected, &buffer[i], sizeof(buffer) - i, 0);
    else
        i += int_to_str(g_current_bank, &buffer[i], sizeof(buffer) - i, 0);

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();
}

//requested from the bp_up / bp_down functions when we reach the end of a page
static void request_next_bank_page(uint8_t dir)
{
    //we need to save our current hover and selected here, the parsing function will discard those
    uint8_t prev_hover = g_banks->hover;
    uint8_t prev_selected = g_banks->selected;
    uint8_t prev_selected_count = g_banks->selected_count;

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

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    //restore our previous hover / selected bank
    g_banks->hover = prev_hover;
    g_banks->selected = prev_selected;
    g_banks->selected_count = prev_selected_count;

    if (g_banks->selected_count != 0)
        g_banks->selected_pb_uids = g_uids_to_add_to_bank;
}

//called from the request functions and the naveg_initail_state
static void parse_pedalboards_list(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;
    uint32_t count = strarr_length(&list[5]);

    // free the navigation pedalboads list
    if (g_pedalboards)
        data_free_pedalboards_list(g_pedalboards);

    // parses the list
    g_pedalboards = data_parse_pedalboards_list(&list[5], count);

    if (g_pedalboards) {
        g_pedalboards->menu_max = (atoi(list[2]));
        g_pedalboards->page_min = (atoi(list[3]));
        g_pedalboards->page_max = (atoi(list[4])); 
    }
}

static void request_pedalboards(uint8_t dir, uint16_t bank_uid)
{
    uint8_t i;
    char buffer[40];
    memset(buffer, 0, sizeof buffer);

    // sets the response callback
    ui_comm_webgui_set_response_cb(parse_pedalboards_list, NULL);

    i = copy_command((char *)buffer, CMD_PEDALBOARDS);

    uint8_t bitmask = 0;
    if (dir == 1)
        bitmask |= FLAG_PAGINATION_PAGE_UP;
    else if (dir == 2)
        bitmask |= FLAG_PAGINATION_INITIAL_REQ;

    // insert the direction on buffer
    i += int_to_str(bitmask, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    i += int_to_str(g_pedalboards->hover, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    // copy the bank uid
    i += int_to_str((bank_uid), &buffer[i], sizeof(buffer) - i, 0);

    buffer[i++] = 0;

    uint16_t prev_hover = g_current_pedalboard;
    uint16_t prev_selected = g_current_pedalboard;
    uint16_t prev_selected_count = 0;

    if (g_pedalboards) {
        prev_hover = g_pedalboards->hover;
        prev_selected = g_pedalboards->selected;
        prev_selected_count = g_pedalboards->selected_count;
    }

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    if (g_pedalboards) {
        g_pedalboards->hover = prev_hover;
        g_pedalboards->selected = prev_selected;
        g_pedalboards->selected_count = prev_selected_count;
    }

    if (g_pedalboards->selected_count != 0)
        g_pedalboards->selected_pb_uids = g_uids_to_add_to_bank;
}

static void send_load_pedalboard(uint16_t bank_id, const char *pedalboard_uid)
{
    uint16_t i;
    char buffer[40];
    memset(buffer, 0, sizeof buffer);

    i = copy_command((char *)buffer, CMD_PEDALBOARD_LOAD);

    if (g_snapshots) {
        g_snapshots->selected = 0;
        g_snapshots->hover = 0;
    }

    // copy the bank id
    i += int_to_str(bank_id, &buffer[i], 8, 0);

    // inserts one spacesend_load_pedalboard
    buffer[i++] = ' ';

    const char *p = pedalboard_uid;
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

    // sets the response callback
    ui_comm_webgui_set_response_cb(NULL, NULL);

    // send the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboard loaded message to be received
    ui_comm_webgui_wait_response();

    CM_reset_encoder_page();
}

//called from the request functions and the naveg_initail_state
static void parse_snapshots_list(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;
    uint32_t count = strarr_length(&list[5]);

    // free the navigation pedalboads list
    if (g_snapshots)
        data_free_pedalboards_list(g_snapshots);

    // parses the list
    g_snapshots = data_parse_pedalboards_list(&list[5], count);

    if (g_snapshots) {
        g_snapshots->menu_max = (atoi(list[2]));
        g_snapshots->page_min = (atoi(list[3]));
        g_snapshots->page_max = (atoi(list[4])); 
    
        g_snapshots_loaded = 1;
    }
    else
        g_snapshots_loaded = 0;
}

static void request_snapshots(uint8_t dir)
{
    uint8_t i;
    char buffer[40];
    memset(buffer, 0, sizeof buffer);

    // sets the response callback
    ui_comm_webgui_set_response_cb(parse_snapshots_list, NULL);

    i = copy_command((char *)buffer, CMD_SNAPSHOTS);

    uint8_t bitmask = 0;
    if (dir == 1)
        bitmask |= FLAG_PAGINATION_PAGE_UP;
    else if (dir == 2)
        bitmask |= FLAG_PAGINATION_INITIAL_REQ;

    // insert the direction on buffer
    i += int_to_str(bitmask, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    // insert the current hover on buffer
    if ((dir == PAGE_DIR_INIT))
        i += int_to_str(0, &buffer[i], sizeof(buffer) - i, 0);
    else
        i += int_to_str(g_snapshots->hover, &buffer[i], sizeof(buffer) - i, 0);

    buffer[i++] = 0;

    uint16_t prev_hover = g_current_snapshot;
    uint16_t prev_selected = g_current_snapshot;

    if (g_snapshots) {
        prev_hover = g_snapshots->hover;
        prev_selected = g_snapshots->selected;
    }

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    if (g_snapshots) {
        g_snapshots->hover = prev_hover;
        g_snapshots->selected = prev_selected;
    }
}

static void send_load_snapshot(const char *snapshot_uid)
{
    uint16_t i;
    char buffer[40];
    memset(buffer, 0, sizeof buffer);

    i = copy_command((char *)buffer, CMD_SNAPSHOTS_LOAD);

    if (g_current_list == SNAPSHOT_LIST)
        NM_set_leds();

    const char *p = snapshot_uid;
    // copy the snapshot uid
    if (!*p) 
        buffer[i++] = '0';
    else {
        while (*p) {
            buffer[i++] = *p;
            p++;
        }
    }
    buffer[i] = 0;

    // sets the response callback
    ui_comm_webgui_set_response_cb(NULL, NULL);

    // send the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the snapshot loaded message to be received
    ui_comm_webgui_wait_response();
}

static void delete_from_selected_uid(int8_t uid_count, int8_t delete_index)
{
    uint16_t i;
    if (delete_index <= uid_count) {
        for(i = delete_index; i < uid_count-1; i++){
            *(g_uids_to_add_to_bank+i)=*(g_uids_to_add_to_bank+i+1);
        }
    }
}

static void add_to_selected_uid(bp_list_t *source_list)
{
    //add the id to the arguments
    if (g_current_list == BANK_LIST_CHECKBOXES)
        g_current_list = BANK_LIST_CHECKBOXES_ENGAGED;
    else if (g_current_list == PB_LIST_CHECKBOXES)
        g_current_list = PB_LIST_CHECKBOXES_ENGAGED;

    uint16_t current_uid = atoi(source_list->uids[source_list->hover - source_list->page_min]);

    //first check if we already have this item, if so, delete
    uint8_t j, found = 0;
    for (j = 0; j < source_list->selected_count; j++) {
        if (current_uid == g_uids_to_add_to_bank[j]) {
            //remove
            delete_from_selected_uid(source_list->selected_count, j);
            source_list->selected_count--;
            found = 1;

            if (source_list->selected_count == 0) {
                //add the id to the arguments
                if (g_current_list == BANK_LIST_CHECKBOXES_ENGAGED)
                    g_current_list = BANK_LIST_CHECKBOXES;
                else if (g_current_list == PB_LIST_CHECKBOXES_ENGAGED)
                    g_current_list = PB_LIST_CHECKBOXES;
            }
        }
    }

    if (!found){
        g_uids_to_add_to_bank[source_list->selected_count] = current_uid;
        source_list->selected_count++;
    }

    source_list->selected_pb_uids = g_uids_to_add_to_bank;
}

static void exit_checkbox_mode(void)
{
    //reset everything
    FREE(g_uids_to_add_to_bank);

    g_banks->selected_count = 0;
    g_pedalboards->selected_count = 0;

    g_banks->selected = g_current_add_bank;
    g_current_list = PB_LIST_BEGINNING_BOX;

    g_pedalboards->hover = 0;

    request_pedalboards(PAGE_DIR_INIT, g_banks->selected);

    NM_print_screen();
}

static void parse_selected_uids(int8_t uid_count, int8_t bank_uid)
{
    // sets the response callback
    ui_comm_webgui_set_response_cb(NULL, NULL);

    char buffer[80];
    memset(buffer, 0, 80);
    uint8_t i;

    i = copy_command(buffer, CMD_ADD_PBS_TO_BANK);

    // insert the destination bank uid on buffer
    i += int_to_str(g_current_add_bank, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    // insert the origin bank uid on buffer
    i += int_to_str(bank_uid, &buffer[i], sizeof(buffer) - i, 0);

    uint8_t j;
    for (j = 0; j < uid_count; j++) {
        // inserts one space
        buffer[i++] = ' ';

        //add the uid
        i += int_to_str(g_uids_to_add_to_bank[j], &buffer[i], sizeof(buffer) - i, 0);
    }

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    //exit this mode
    exit_checkbox_mode();
}

static void enter_bank(void)
{
    g_banks->selected = g_banks->hover;

    //make sure that we request the right page if we have a selected pedalboard
    if (g_current_bank == g_banks->selected)
        g_pedalboards->hover = g_current_pedalboard;
    else
        g_pedalboards->hover = 0;

    //index is relevent in our array so - page_min
    request_pedalboards(PAGE_DIR_INIT, atoi(g_banks->uids[g_banks->selected - g_banks->page_min]));

    // if reach here, received the pedalboards list
    if (g_current_list == BANK_LIST_CHECKBOXES)
        g_current_list = PB_LIST_CHECKBOXES;
    else if (g_pedalboards->page_max == 0)
        g_current_list = PB_LIST_BEGINNING_BOX_SELECTED;
    else if ((g_pedalboards->hover == 0) && (g_banks->selected != 0))
        g_current_list = PB_LIST_BEGINNING_BOX;
    else
        g_current_list = PEDALBOARD_LIST;

    //check if we need to display the selected item or out of bounds
    if (g_current_bank == g_banks->selected) {
        g_pedalboards->selected = g_current_pedalboard;
        g_pedalboards->hover = g_current_pedalboard;
    }
    else {
        g_pedalboards->selected = g_pedalboards->menu_max + 1;
        g_pedalboards->hover = 0;
    }

    NM_print_screen();
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
    g_snapshots = NULL;

    // initializes the bank functions
    uint8_t i;
    for (i = 0; i < BANK_FUNC_COUNT; i++)
    {
        g_bank_functions[i].function = BANK_FUNC_NONE;
        g_bank_functions[i].hw_id = 0xFF;
    }

    //init the default names
    g_pedalboard_name = (char *) MALLOC(20 * sizeof(char));
    strcpy(g_pedalboard_name, "DEFAULT");

    g_snapshot_name = (char *) MALLOC(20 * sizeof(char));
    strcpy(g_snapshot_name, "DEFAULT");
}

void NM_clear(void)
{
    // reset the banks and pedalboards state after return from ui connection
    if (g_banks) data_free_banks_list(g_banks);
    if (g_pedalboards) data_free_pedalboards_list(g_pedalboards);
    if (g_snapshots) data_free_pedalboards_list(g_snapshots);

    g_banks = NULL;
    g_pedalboards = NULL;
    g_snapshots = NULL;

    g_current_list = PEDALBOARD_LIST;

    if (g_uids_to_add_to_bank)
        FREE(g_uids_to_add_to_bank);
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
        request_banks_list(PAGE_DIR_INIT);
    }

    // checks and free the navigation pedalboads list
    if (g_pedalboards) data_free_pedalboards_list(g_pedalboards);

    // parses the list
    g_pedalboards = data_parse_pedalboards_list(pedalboards_list, strarr_length(pedalboards_list));

    if (!g_pedalboards) return;

    g_pedalboards->page_min = page_min;
    g_pedalboards->page_max = page_max;
    g_pedalboards->menu_max = max_menu;

    g_current_pedalboard = atoi(pedalboard_uid);

    g_pedalboards->hover = g_current_pedalboard;
    g_pedalboards->selected = g_current_pedalboard;

    if ((g_current_bank != 0) && (g_current_pedalboard == 0))
        g_current_list = PB_LIST_BEGINNING_BOX;
}

void NM_enter(void)
{
    if (g_item_grabbed != NO_GRAB_ITEM)
        return;

    if (!g_banks)
        return;

    switch (g_current_list) {
        case BANKS_LIST:
            enter_bank();
        break;

        case PB_LIST_BEGINNING_BOX:
        case PEDALBOARD_LIST:
            // request to GUI load the pedalboard
            //index is relevant in the array so - page_min correct here
            send_load_pedalboard(atoi(g_banks->uids[g_banks->hover - g_banks->page_min]), g_pedalboards->uids[g_pedalboards->hover - g_pedalboards->page_min -1]);

            g_current_pedalboard = g_pedalboards->hover;
            g_force_update_pedalboard = 1;

            // stores the current bank and pedalboard
            g_current_bank = g_banks->selected;

            g_pedalboards->selected = g_pedalboards->hover;
            g_current_pedalboard = g_pedalboards->hover;

            NM_print_screen();
        break;

        case SNAPSHOT_LIST:
            send_load_snapshot(g_snapshots->uids[g_snapshots->hover - g_snapshots->page_min -1]);

            g_current_snapshot = g_snapshots->hover;
            g_snapshots->selected = g_snapshots->hover;

            NM_print_screen();
        break;

        //check if we need to trigger 'add pb to bank' mode
        case PB_LIST_BEGINNING_BOX_SELECTED:
            //we always start from banks
            g_current_list = BANK_LIST_CHECKBOXES;

            //save the bank we are editing
            g_current_add_bank = g_banks->hover;

            //allocate memory for the UID's
            g_uids_to_add_to_bank = (uint16_t *) MALLOC(sizeof(uint16_t *) * 120);

            g_banks->selected_count = 0;
            g_pedalboards->selected_count = 0;

            NM_print_screen();
        return;
        break;

        case BANK_LIST_CHECKBOXES:
        case BANK_LIST_CHECKBOXES_ENGAGED:
            add_to_selected_uid(g_banks);
            NM_print_screen();
        break;

        case PB_LIST_CHECKBOXES:
        case PB_LIST_CHECKBOXES_ENGAGED:
            add_to_selected_uid(g_pedalboards);
            NM_print_screen();
        break;
    }
}

void NM_encoder_hold(uint8_t encoder)
{
    //we only suport reordering on the left encoder
    if (encoder != 0)
        return;

    //trigger 'item grab mode'
    //save the name to use later when in other subpage of list
    //this mode will be detected by NM_up and NM_down
    //these will not preform their normal actions, but instead keep indexes to be send when released
    uint8_t char_cnt_name = 0;
    if ((g_current_list == PEDALBOARD_LIST) || (g_current_list == PB_LIST_BEGINNING_BOX)) {
        if (g_banks) {
            if (g_banks->selected == 0) return;
        }
        else if (g_current_bank == 0) return;

        if (g_pedalboards->menu_max == 1)
            return;

        g_item_grabbed = g_pedalboards->hover - g_pedalboards->page_min;
        g_item_grabbed_uid = atoi(g_pedalboards->uids[g_item_grabbed]);
        char_cnt_name = strlen(g_pedalboards->names[g_item_grabbed]);

        if (char_cnt_name > 16)
            char_cnt_name = 16;

        g_grabbed_item_label = (char *) MALLOC((char_cnt_name+1) * sizeof(char));
        strncpy(g_grabbed_item_label, g_pedalboards->names[g_item_grabbed], char_cnt_name);
    }
    else if (g_current_list == SNAPSHOT_LIST) {
        g_item_grabbed = g_snapshots->hover - g_snapshots->page_min;
        g_item_grabbed_uid = atoi(g_snapshots->uids[g_item_grabbed]);
        char_cnt_name = strlen(g_snapshots->names[g_item_grabbed]);

        if (char_cnt_name > 16)
            char_cnt_name = 16;

        g_grabbed_item_label = (char *) MALLOC((char_cnt_name+1) * sizeof(char));
        strncpy(g_grabbed_item_label, g_snapshots->names[g_item_grabbed], char_cnt_name);
    }
    else
        return;

    g_grabbed_item_label[char_cnt_name] = '\0';

    NM_print_prev_screen();
}

void NM_encoder_released(uint8_t encoder)
{
    //we only suport reordering on the left encoder
    if ((encoder != 0) || (g_item_grabbed == NO_GRAB_ITEM))
        return;

    //send CMD_PEDALBOARD_CHANGE_INDEX or CMD_SNAPSHOT_CHANGE_INDEX
    ui_comm_webgui_set_response_cb(NULL, NULL);
    ui_comm_webgui_clear();
    
    char buffer[30];
    uint8_t i = 0;

    if (g_current_list == PEDALBOARD_LIST) {
        i = copy_command(buffer, CMD_REORDER_PBS_IN_BANK);
        i += int_to_str(g_banks->hover, &buffer[i], sizeof(buffer) - i, 0);
        buffer[i++] = ' ';
        i += int_to_str(g_item_grabbed_uid - 1, &buffer[i], sizeof(buffer) - i, 0);
        buffer[i++] = ' ';
        i += int_to_str(g_pedalboards->hover, &buffer[i], sizeof(buffer) - i, 0);
    }
    else {
        i = copy_command(buffer, CMD_REORDER_SSS_IN_PB);
        i += int_to_str(g_pedalboards->hover, &buffer[i], sizeof(buffer) - i, 0);
        buffer[i++] = ' ';
        i += int_to_str(g_item_grabbed_uid - 1, &buffer[i], sizeof(buffer) - i, 0);
        buffer[i++] = ' ';
        i += int_to_str(g_snapshots->hover, &buffer[i], sizeof(buffer) - i, 0);
    }

    ui_comm_webgui_send(buffer, i);
    ui_comm_webgui_wait_response();

    //dissable 'item grab mode'
    g_item_grabbed = NO_GRAB_ITEM;

    //free string in mem
    if (g_grabbed_item_label)
        FREE(g_grabbed_item_label);

    //reprint screen
    NM_print_screen();
}

uint8_t NM_up(void)
{
    if (!g_banks) return 0;

    switch (g_current_list) {
        case BANK_LIST_CHECKBOXES:
        case BANK_LIST_CHECKBOXES_ENGAGED:
        case BANKS_LIST:
            //if we are nearing the final 3 items of the page, and we already have the end of the page in memory, or if we just need to go down
            if(g_banks->page_min == 0) {
                //check if we are not already at the end
                if (g_banks->hover == 0)
                    return 0;
                else
                    g_banks->hover--;
            }
            //are we comming from the bottom of the menu?
            else {
                //we always keep 3 items in front of us, if not request new page
                if (g_banks->hover <= (g_banks->page_min + 3)) {
                    g_banks->hover--;

                    //request new page
                    request_next_bank_page(PAGE_DIR_DOWN);
                }
                //we have items, just go up
                else
                    g_banks->hover--;
            }

            NM_print_screen();
            return 1;
        break;

        case PEDALBOARD_LIST:
        case PB_LIST_CHECKBOXES:
        case PB_LIST_CHECKBOXES_ENGAGED:
            //are we reaching the bottom of the menu?
            if(g_pedalboards->page_min == 0) {

                //go down till the end
                //we still have items in our list
                if (g_pedalboards->hover > 0)
                    g_pedalboards->hover--;

                if ((g_pedalboards->hover == 0) && (g_item_grabbed == NO_GRAB_ITEM) && g_current_list != PB_LIST_CHECKBOXES && g_current_list != PB_LIST_CHECKBOXES_ENGAGED && g_banks->selected != 0)
                    g_current_list = PB_LIST_BEGINNING_BOX;
            }
            else {
                //we always keep 3 items in front of us, if not request new page
                if (g_pedalboards->hover <= (g_pedalboards->page_min + 4)) {
                    g_pedalboards->hover--;

                    //request new page
                    request_pedalboards(PAGE_DIR_DOWN, atoi(g_banks->uids[g_banks->hover - g_banks->page_min]));
                }
                //we have items, just go up
                else
                    g_pedalboards->hover--;
            }

            NM_print_screen();
            return 1;
        break;

        case SNAPSHOT_LIST:
            //are we reaching the bottom of the menu?
            if(g_snapshots->page_min == 0)  {
                //check if we are not already at the end
                if (g_snapshots->hover == 0)
                    return 0;
                else  {
                    //go down till the end
                    //we still have items in our list
                    g_snapshots->hover--;
                }
            }
            else {
                //we always keep 3 items in front of us, if not request new page
                if (g_snapshots->hover <= (g_snapshots->page_min + 4)) {
                    g_snapshots->hover--;

                    //request new page
                    request_snapshots(PAGE_DIR_DOWN);
                }
                //we have items, just go up
                else
                    g_snapshots->hover--;
            }

            NM_print_screen();
            return 1;
        break;

        case PB_LIST_BEGINNING_BOX:
            if (g_item_grabbed != NO_GRAB_ITEM)
                return 0;

            g_current_list = PB_LIST_BEGINNING_BOX_SELECTED;
            g_pedalboards->hover = -1;
            NM_print_screen();
            return 1;
        break;

        case PB_LIST_BEGINNING_BOX_SELECTED:
            //nothing to do
            return 0;
        break;
    }

    return 0;
}

uint8_t NM_down(void)
{
    if (!g_banks) return 0;

    switch (g_current_list) {
        case BANK_LIST_CHECKBOXES:
        case BANK_LIST_CHECKBOXES_ENGAGED:
        case BANKS_LIST:
            //are we reaching the bottom of the menu?
            if(g_banks->page_max == g_banks->menu_max) {
                //check if we are not already at the end
                if (g_banks->hover >= g_banks->menu_max -1)
                    return 0;
                else
                    g_banks->hover++;
            }
            else {
                //we always keep 3 items in front of us, if not request new page
                if (g_banks->hover >= (g_banks->page_max - 4)) {
                    g_banks->hover++;

                    //request new page
                    request_next_bank_page(PAGE_DIR_UP);
                }
                //we have items, just go down
                else
                    g_banks->hover++;
            }

            NM_print_screen();
            return 1;
        break;

        case PB_LIST_CHECKBOXES:
        case PB_LIST_CHECKBOXES_ENGAGED:
        case PEDALBOARD_LIST:
            //are we reaching the bottom of the menu, -1 because menu max is bigger then page_max
            if(g_pedalboards->page_max == g_pedalboards->menu_max) {
                //check if we are not already at the end
                if ((g_pedalboards->hover >= g_pedalboards->menu_max - 1) || g_pedalboards->page_max == 0)
                    return 0;
                //we still have items in our list
                else
                    g_pedalboards->hover++;
            }
            else {
                //we always keep 3 items in front of us, if not request new page
                if (g_pedalboards->hover >= (g_pedalboards->page_max - 4)) {
                    //request new page
                    request_pedalboards(PAGE_DIR_UP, atoi(g_banks->uids[g_banks->hover - g_banks->page_min]));
                    g_pedalboards->hover++;
                }
                //we have items, just go down
                else
                    g_pedalboards->hover++;
            }

            NM_print_screen();
            return 1;
        break;

        case SNAPSHOT_LIST:
            //are we reaching the bottom of the menu, -1 because menu max is bigger then page_max
            if(g_snapshots->page_max == g_snapshots->menu_max) {
                //check if we are not already at the end
                if (g_snapshots->hover >= g_snapshots->menu_max - 1)
                    return 0;
                //we still have items in our list
                else
                    g_snapshots->hover++;
            }
            else {
                //we always keep 3 items in front of us, if not request new page
                if (g_snapshots->hover >= (g_snapshots->page_max - 4)) {
                    //request new page
                    request_snapshots(PAGE_DIR_UP);
                    g_snapshots->hover++;
                }
                //we have items, just go down
                else
                    g_snapshots->hover++;
            }

            NM_print_screen();
            return 1;
        break;

        case PB_LIST_BEGINNING_BOX:
            //are we reaching the bottom of the menu, -1 because menu max is bigger then page_max
            if ((g_pedalboards->page_max == g_pedalboards->menu_max) && (g_pedalboards->hover >= g_pedalboards->menu_max - 1))
                    return 0;
            //we still have items in our list
            else
                g_pedalboards->hover++;

            g_current_list = PEDALBOARD_LIST;
            NM_print_screen();
            return 1;
        break;

        case PB_LIST_BEGINNING_BOX_SELECTED:
            if (g_pedalboards->page_max == 0)
                return 0;

            g_pedalboards->hover = 0;
            g_current_list = PB_LIST_BEGINNING_BOX;
            NM_print_screen();
            return 1;
        break;
    }

    return 0;
}

void NM_set_banks(bp_list_t *bp_list)
{
    if (!g_initialized) return;

    g_banks = bp_list;
}

bp_list_t *NM_get_banks(void)
{
    if (!g_initialized) return NULL;

    return g_banks;
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

void NM_set_current_list(uint8_t list_type)
{
    if ((list_type == PEDALBOARD_LIST) && (g_pedalboards->hover == 0))
        g_current_list = PB_LIST_BEGINNING_BOX;
    else
        g_current_list = list_type;
}

uint8_t NM_get_current_list(void)
{
    return g_current_list;
}

void NM_toggle_mode(void)
{
     switch(g_current_list)
    {
        case BANKS_LIST:
        case BANK_LIST_CHECKBOXES:
        case BANK_LIST_CHECKBOXES_ENGAGED:
        case PB_LIST_BEGINNING_BOX_SELECTED:
        case PB_LIST_BEGINNING_BOX:
        case PB_LIST_CHECKBOXES:
        case PB_LIST_CHECKBOXES_ENGAGED:
            if (g_banks) {
                g_banks->selected = g_current_bank;
                g_pedalboards->hover = g_pedalboards->selected;
            }

            g_current_list = PEDALBOARD_LIST;
        //fall-through
        case PEDALBOARD_LIST:

            //reset these
            if (g_banks && g_pedalboards) {
                g_banks->hover = g_current_bank;
                g_banks->selected = g_banks->hover;
            }
        break;

        case SNAPSHOT_LIST:
        break;
    }

    NM_print_screen();
}

void NM_update_lists(uint8_t list_type)
{
    switch(list_type)
    {
        case BANK_LIST_CHECKBOXES:
        case BANK_LIST_CHECKBOXES_ENGAGED:
        case BANKS_LIST:
        break;

            case PB_LIST_CHECKBOXES:
        case PB_LIST_CHECKBOXES_ENGAGED:
        case PEDALBOARD_LIST:
        case PB_LIST_BEGINNING_BOX_SELECTED:
        case PB_LIST_BEGINNING_BOX:
            request_banks_list(PAGE_DIR_INIT);
            request_pedalboards(PAGE_DIR_INIT, g_banks->selected);

            if (g_pedalboards->page_max == 0)
                g_current_list = PB_LIST_BEGINNING_BOX_SELECTED;
            else if ((g_pedalboards->hover == 0) && (g_banks->selected != 0))
                g_current_list = PB_LIST_BEGINNING_BOX;
        break;

        case SNAPSHOT_LIST:
            request_snapshots(PAGE_DIR_INIT);
        break;
    }

    NM_set_leds();
}

void NM_print_screen(void)
{
    switch(g_current_list)
    {
        case BANK_LIST_CHECKBOXES:
        case BANK_LIST_CHECKBOXES_ENGAGED:
        case BANKS_LIST:
            g_banks->selected = g_current_bank;
            screen_bank_list(g_banks, "BANKS");
        break;

        case PEDALBOARD_LIST:
        case PB_LIST_BEGINNING_BOX_SELECTED:
        case PB_LIST_BEGINNING_BOX:
            request_banks_list(PAGE_DIR_INIT);
            request_pedalboards(PAGE_DIR_INIT, g_banks->selected);

            if (g_pedalboards->page_max == 0)
                g_current_list = PB_LIST_BEGINNING_BOX_SELECTED;
            else if ((g_pedalboards->hover == 0) && (g_banks->selected != 0))
                g_current_list = PB_LIST_BEGINNING_BOX;
        //fall-through
        case PB_LIST_CHECKBOXES:
        case PB_LIST_CHECKBOXES_ENGAGED:
            screen_pbss_list(g_banks->names[g_banks->selected - g_banks->page_min], g_pedalboards, PB_MODE, g_item_grabbed, g_grabbed_item_label);
        break;

        case SNAPSHOT_LIST:
            request_snapshots(PAGE_DIR_INIT);

            if (!g_snapshots_loaded) //no snapshots available
                return;

            //display them
            screen_pbss_list(g_pedalboards->names[g_current_pedalboard - g_pedalboards->page_min], g_snapshots, SS_MODE, g_item_grabbed, g_grabbed_item_label);
        break;
    }

    NM_set_leds();
}

void NM_print_prev_screen(void)
{
    switch(g_current_list)
    {
        case BANKS_LIST:
            screen_bank_list(g_banks, "BANKS");
        break;

        case PB_LIST_BEGINNING_BOX:
        case PB_LIST_BEGINNING_BOX_SELECTED:
        case PEDALBOARD_LIST:
            screen_pbss_list(g_banks->names[g_banks->selected - g_banks->page_min], g_pedalboards, PB_MODE, g_item_grabbed, g_grabbed_item_label);
        break;

        case SNAPSHOT_LIST:
            if (!g_snapshots_loaded) //no snapshots available
                return;

            //display them
            screen_pbss_list(g_pedalboards->names[g_current_pedalboard - g_pedalboards->page_min], g_snapshots, SS_MODE, g_item_grabbed, g_grabbed_item_label);
        break;
    }

    NM_set_leds();
}

void NM_set_leds(void)
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

    switch(g_current_list)
    {
        case BANK_LIST_CHECKBOXES:
        case BANK_LIST_CHECKBOXES_ENGAGED:
            led = hardware_leds(3);
            led_state.color = TRIGGER_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led = hardware_leds(4);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led_state.color = TOGGLED_COLOR;
            led = hardware_leds(5);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
        break;

        case PB_LIST_CHECKBOXES_ENGAGED:
            led = hardware_leds(3);
            led_state.color = TRIGGER_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led = hardware_leds(4);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led_state.color = TOGGLED_COLOR;
            led = hardware_leds(5);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
        break;

        case PB_LIST_CHECKBOXES:
            led = hardware_leds(4);
            led_state.color = TRIGGER_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led_state.color = TOGGLED_COLOR;
            led = hardware_leds(3);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led = hardware_leds(5);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
        break;

        case BANKS_LIST:
            led = hardware_leds(3);
            led_state.color = TRIGGER_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led = hardware_leds(4);
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);

            if (g_banks->hover != 0) {
                led = hardware_leds(5);
                led_state.color = TOGGLED_COLOR;
                set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            }
        break;

        case PB_LIST_BEGINNING_BOX:
        case PEDALBOARD_LIST:
            led_state.brightness = 0.1;

            if (g_pedalboards->hover > 0)
            {
                led = hardware_leds(0);
                led_state.color = FS_PB_MENU_COLOR;
                set_ledz_trigger_by_color_id(led, LED_DIMMED, led_state);
            }

            if (g_pedalboards->hover < g_pedalboards->menu_max - 1)
            {
                led = hardware_leds(1);
                led_state.color = FS_PB_MENU_COLOR;
                set_ledz_trigger_by_color_id(led, LED_DIMMED, led_state);
            }

            if (g_banks->hover != 0) {
                led_state.color = TOGGLED_COLOR;
                led = hardware_leds(5);
                set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            }
        //fall-through
        case PB_LIST_BEGINNING_BOX_SELECTED:
            led = hardware_leds(2);
            led_state.color = FS_PB_MENU_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);

            led = hardware_leds(3);
            led_state.color = FS_PAGE_COLOR_5;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led = hardware_leds(4);
            led_state.color = TRIGGER_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
        break;

        case SNAPSHOT_LIST:
            led = hardware_leds(2);
            led_state.color = FS_SS_MENU_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            led = hardware_leds(4);
            led_state.color = TRIGGER_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);

            led = hardware_leds(5);
            //block removing the last snapshot
            if (g_snapshots->menu_max >= 2)
                set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            else
                ledz_off(led, TRIGGER_COLOR);

            led_state.brightness = 0.1;

            if (g_snapshots->hover > 0)
            {
                led = hardware_leds(0);
                led_state.color = FS_SS_MENU_COLOR;
                set_ledz_trigger_by_color_id(led, LED_DIMMED, led_state);
            }

            if (g_snapshots->hover < g_snapshots->menu_max - 1)
            {
                led = hardware_leds(1);
                led_state.color = FS_SS_MENU_COLOR;
                set_ledz_trigger_by_color_id(led, LED_DIMMED, led_state);
            }
        break;
    }
}

void NM_button_pressed(uint8_t button)
{
    switch(button)
    {
        //enter menu
        case 0:
            switch(g_current_list)
            {
                case BANK_LIST_CHECKBOXES:
                    //clear all from before
                    FREE(g_uids_to_add_to_bank);
                    //allocate memory for the UID's
                    g_uids_to_add_to_bank = (uint16_t *) MALLOC(sizeof(uint16_t *) * 120);

                    g_banks->selected_count = 0;
                    g_pedalboards->selected_count = 0;
                //fall-through
                case BANKS_LIST:
                    enter_bank();
                break;

                case BANK_LIST_CHECKBOXES_ENGAGED:
                    parse_selected_uids(g_banks->selected_count, ADD_FULL_BANKS);
                    NM_print_screen();
                break;

                case PB_LIST_CHECKBOXES:
                    //clear all from before
                    FREE(g_uids_to_add_to_bank);
                    //allocate memory for the UID's
                    g_uids_to_add_to_bank = (uint16_t *) MALLOC(sizeof(uint16_t *) * 120);

                    g_banks->selected_count = 0;
                    g_pedalboards->selected_count = 0;

                    g_current_list = BANK_LIST_CHECKBOXES;
                    NM_print_screen();
                break;

                case PB_LIST_BEGINNING_BOX_SELECTED:
                case PB_LIST_BEGINNING_BOX:
                case PEDALBOARD_LIST:
                    g_current_list = BANKS_LIST;
                    NM_print_screen();
                break;

                case PB_LIST_CHECKBOXES_ENGAGED:
                    parse_selected_uids(g_pedalboards->selected_count, g_current_bank);
                    NM_print_screen();
                break;

                //not used
                case SNAPSHOT_LIST:
                break;
            }
        break;

        case 1:
            switch(g_current_list)
            {
                case BANKS_LIST:
                    //new bank
                    naveg_trigger_popup(POPUP_NEW_BANK_ID);
                break;

                //save pedalboard
                case PB_LIST_BEGINNING_BOX:
                case PB_LIST_BEGINNING_BOX_SELECTED:
                case PEDALBOARD_LIST:
                    naveg_trigger_popup(POPUP_SAVE_PB_ID);
                break;

                case SNAPSHOT_LIST:
                    naveg_trigger_popup(POPUP_SAVE_SS_ID);
                break;

                case BANK_LIST_CHECKBOXES:
                case BANK_LIST_CHECKBOXES_ENGAGED:
                case PB_LIST_CHECKBOXES:
                case PB_LIST_CHECKBOXES_ENGAGED:
                    NM_enter();
                    NM_print_screen();
                break;
            }
        break;

        case 2:
            switch(g_current_list)
            {
                case BANKS_LIST:
                        //delete bank, cant delete "all pedalboards"
                        if (g_banks->hover == 0)
                            return;

                        //give popup, delete bank?
                        naveg_trigger_popup(POPUP_DELETE_BANK_ID);
                break;

                case PB_LIST_BEGINNING_BOX:
                case PB_LIST_BEGINNING_BOX_SELECTED:
                case PEDALBOARD_LIST:
                    //we cant delete pbs from the all bank
                    if (g_banks->selected == 0)
                            return;
                    //give popup, delete pb?
                    naveg_trigger_popup(POPUP_REMOVE_PB_ID);
                break;

                case SNAPSHOT_LIST:
                    //block removing the last snapshot
                    if (g_snapshots->menu_max <= 1)
                        return;
                    //give popup, delete ss?
                    naveg_trigger_popup(POPUP_REMOVE_SS_ID);
                break;

                case BANK_LIST_CHECKBOXES:
                case BANK_LIST_CHECKBOXES_ENGAGED:
                case PB_LIST_CHECKBOXES:
                case PB_LIST_CHECKBOXES_ENGAGED:
                    exit_checkbox_mode();
                    NM_print_screen();
                break;
            }
        break;
    } 
}

void NM_change_pbss(uint8_t next_prev, uint8_t pressed)
{
    if ((g_current_list == BANKS_LIST) || (g_current_list == PB_LIST_BEGINNING_BOX_SELECTED) || (g_current_list == BANK_LIST_CHECKBOXES) ||
        (g_current_list == BANK_LIST_CHECKBOXES_ENGAGED) || (g_current_list == PB_LIST_CHECKBOXES) || (g_current_list == PB_LIST_CHECKBOXES_ENGAGED))
        return;

    if (pressed)
    {
        if (next_prev)
        {
            if (NM_down())
            {
                NM_enter();
                if (g_current_list == SNAPSHOT_LIST)
                    ledz_on(hardware_leds(next_prev), CYAN);
                else
                {
                    ledz_on(hardware_leds(next_prev), MAGENTA);
                    ledz_blink(hardware_leds(next_prev), MAGENTA, 150, 150, LED_BLINK_INFINIT);
                }
            }
        }
        else if (g_current_list != PB_LIST_BEGINNING_BOX)
        {
            if (NM_up())
            {
                NM_enter();
                if (g_current_list == SNAPSHOT_LIST)
                    ledz_on(hardware_leds(next_prev), CYAN);
                else
                {
                    ledz_on(hardware_leds(next_prev), MAGENTA);
                    ledz_blink(hardware_leds(next_prev), MAGENTA, 150, 150, LED_BLINK_INFINIT);
                }
            }
        }
    }

    if ((g_current_list == SNAPSHOT_LIST) && !pressed)
        NM_set_leds();
}

void NM_toggle_pb_ss(void)
{
    if (g_current_list == BANKS_LIST)
        return;

    if ((g_current_list == PEDALBOARD_LIST) || (g_current_list == PB_LIST_BEGINNING_BOX) || (g_current_list == PB_LIST_BEGINNING_BOX_SELECTED))
    {
        request_snapshots(PAGE_DIR_INIT);

        if (!g_snapshots_loaded) //no snapshots available
        {
            g_current_list = PEDALBOARD_LIST;
            //prev screen funtion is needed as we can not recieve data properly from IRQ
            PM_launch_attention_overlay("No snapshots\navailable for\ncurrent pedalboard", NM_print_prev_screen);
            return;
        }

        g_current_list = SNAPSHOT_LIST;

        //display them
        screen_pbss_list(g_pedalboards->names[g_current_pedalboard - g_pedalboards->page_min], g_snapshots, SS_MODE, g_item_grabbed, g_grabbed_item_label);
    }
    else
    {
        g_banks->selected = g_current_bank;
        request_pedalboards(PAGE_DIR_INIT, g_banks->selected);
        g_pedalboards->selected = g_current_pedalboard;
        g_pedalboards->hover = g_pedalboards->selected;

        if (g_pedalboards->hover == 0)
            g_current_list = PB_LIST_BEGINNING_BOX;
        else
            g_current_list = PEDALBOARD_LIST;

        screen_pbss_list(g_banks->names[g_banks->selected - g_banks->page_min], g_pedalboards, PB_MODE, g_item_grabbed, g_grabbed_item_label);
    }

    NM_set_leds();
}

uint16_t NM_get_current_selected(uint8_t list_type)
{
    switch(list_type) {
        case PEDALBOARD_LIST:
            return g_current_pedalboard;
        break;

        case SNAPSHOT_LIST:
            return g_current_snapshot;
        break;

        case BANKS_LIST:
            return g_banks->selected;
        break;
    }

    return 0;
}

void NM_set_last_selected(uint8_t list_type)
{
    static char str_bfr[8] = {};

    switch(list_type) {
        case PEDALBOARD_LIST:;
            uint16_t pedalboard_to_load = g_pedalboards->menu_max;
            int_to_str(pedalboard_to_load, str_bfr, 8, 0);

            send_load_pedalboard(atoi(g_banks->uids[g_banks->hover - g_banks->page_min]), str_bfr);

            g_current_pedalboard = pedalboard_to_load;
            g_pedalboards->selected = pedalboard_to_load;
            g_pedalboards->hover = pedalboard_to_load;
        break;

        case SNAPSHOT_LIST:;
            uint16_t snapshot_to_load = g_snapshots->menu_max;
            int_to_str(snapshot_to_load, str_bfr, 8, 0);

            send_load_snapshot(str_bfr);

            g_current_snapshot = snapshot_to_load;
            g_snapshots->selected = snapshot_to_load;
            g_snapshots->hover = snapshot_to_load;
        break;

        case BANKS_LIST:
            //return g_banks->selected;
        break;
    }
}

void NM_set_selected_index(uint8_t list_type, uint16_t index)
{
    switch(list_type) {
        case PEDALBOARD_LIST:
            g_current_pedalboard = index;
            g_pedalboards->selected = index;
            g_pedalboards->hover = index;
        break;

        case SNAPSHOT_LIST:
            g_current_snapshot = index;
            g_snapshots->selected = index;
            g_snapshots->hover = index;
        break;

        case BANKS_LIST:
        break;
    }
}

uint16_t NM_get_current_hover(uint8_t list_type)
{
    switch(list_type) {
        case PEDALBOARD_LIST:
            return g_pedalboards->hover;
        break;

        case SNAPSHOT_LIST:
            return g_snapshots->hover;
        break;

        case BANKS_LIST:
            return g_banks->hover;
        break;
    }

    return 0;
}

uint16_t NM_get_list_count(uint8_t list_type)
{
    switch(list_type) {
        case PEDALBOARD_LIST:
            return g_pedalboards->menu_max;
        break;

        case SNAPSHOT_LIST:
            return g_snapshots->menu_max;
        break;

        case BANKS_LIST:
            return g_banks->menu_max;
        break;
    }

    return 0;
}

void NM_save_pbss_name(const void *data, uint8_t pb_ss_toggle)
{
    const char **name_list = (const char**)data;

    // get first list name, copy it to our string buffer
    const char *name_string = *name_list;
    strncpy((pb_ss_toggle ? g_snapshot_name : g_pedalboard_name), name_string, 19);
    (pb_ss_toggle ? g_snapshot_name : g_pedalboard_name)[19] = 0; // strncpy might not have final null byte

    // go to next name in list
    name_string = *(++name_list);

    while (name_string && ((strlen((pb_ss_toggle ? g_snapshot_name : g_pedalboard_name)) + strlen(name_string) + 1) < 19))
    {
        strcat((pb_ss_toggle ? g_snapshot_name : g_pedalboard_name), " ");
        strcat((pb_ss_toggle ? g_snapshot_name : g_pedalboard_name), name_string);
        name_string = *(++name_list);
    }

    (pb_ss_toggle ? g_snapshot_name : g_pedalboard_name)[19] = 0;

    screen_tittle(pb_ss_toggle);
}

char* NM_get_pbss_name(uint8_t pb_ss_toggle)
{
    if (pb_ss_toggle) {
        return g_snapshot_name;
    }
    else {
        return g_pedalboard_name;
    }
}