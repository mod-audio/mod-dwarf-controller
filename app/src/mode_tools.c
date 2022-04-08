
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

#include "actuator.h"
#include "config.h"
#include "hardware.h"
#include "protocol.h"
#include "glcd.h"
#include "ledz.h"
#include "data.h"
#include "naveg.h"
#include "screen.h"
#include "node.h"
#include "ui_comm.h"
#include "sys_comm.h"
#include "mode_tools.h"
#include "images.h"

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

enum {TOOL_OFF, TOOL_ON};

#define MAX_CHARS_MENU_NAME     (128/4)

/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

static menu_desc_t g_menu_desc[] = {
    SYSTEM_MENU
    {NULL, 0, -1, -1, NULL, 0}
};

//as there are some popup items directly linked to menu items, they are not handled by the popup mode
//TODO, change var names to clearly indicate this
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
static uint8_t g_current_tool;
static uint8_t g_first_foot_tool = TOOL_TUNER;

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

static void tools_off(void)
{
    uint8_t i = 0;
    for (i = 0; i < MAX_TOOLS; i++)
    {
        g_tool[i].state = TOOL_OFF;
    }
}

static int tool_is_on(uint8_t tool)
{
    return g_tool[tool].state;
}

void set_tool_pages_led_state(void)
{
    ledz_t *led = hardware_leds(2);
    led_state_t page_state = {
        .color = TUNER_COLOR + g_current_tool - TOOL_FOOT-1,
    };
    set_ledz_trigger_by_color_id(led, LED_ON, page_state);
}

void update_tap_tempo_led(void)
{
    menu_item_t *sync_item = TM_get_menu_item_by_ID(TAP_ID);

    // convert the time unit
    uint16_t time_ms = (uint16_t)(convert_to_ms("bpm", sync_item->data.value) + 0.5);

    led_state_t tap_state = {
        .color = TEMPO_COLOR,
        .amount_of_blinks = LED_BLINK_INFINIT,
        .sync_blink = 0,
    };

    // setup the led blink
    if (time_ms > TAP_TEMPO_TIME_ON)
    {
        tap_state.time_on = TAP_TEMPO_TIME_ON;
        tap_state.time_off = time_ms - TAP_TEMPO_TIME_ON;
    }
    else
    {
        tap_state.time_on = time_ms / 2;
        tap_state.time_off = time_ms / 2;
    }

    ledz_t *led = hardware_leds(1);
    led->sync_blink = tap_state.sync_blink;
    set_ledz_trigger_by_color_id(led, LED_BLINK, tap_state);
}

node_t *get_menu_node_by_ID(uint8_t menu_id)
{
    node_t *node = g_menu->first_child->first_child;

    //make sure we have all menu value's updated
    node_t *child_nodes;
    menu_item_t *item_child;

    while(node)
    {
        item_child = node->data;
        if (item_child->desc->id == menu_id)
        {
            return node;
        }

        if (node->first_child)
        {
            child_nodes = node->first_child;
            while(child_nodes)
            {
                item_child = child_nodes->data;

                if (item_child->desc->id == menu_id)
                {
                    return child_nodes;
                }

                child_nodes = child_nodes->next;
            }
        }

        node = node->next;
    }

    return NULL;
}

static void menu_enter(uint8_t encoder)
{
    (void) encoder;
    uint8_t i;
    node_t *node = g_current_menu;
    menu_item_t *item = g_current_item;

    //we can only enter a menu from a root item
    if (item->desc->type == MENU_ROOT)
    {
        // locates the clicked item
        node = g_current_menu->first_child;
        for (i = 0; i < item->data.hover; i++) node = node->next;

        // gets the menu item
        item = node->data;
        g_current_item = node->data;
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

        screen_system_menu(item);
    }
    else if (item->desc->type == MENU_MAIN)
    {
        g_current_menu = node;

        //make sure we have all menu value's updated 
        node_t *child_nodes = node->first_child;
        menu_item_t *item_child = child_nodes->data;

        for (i = 0; i < 3; i++)
        {
            if (item_child->desc->action_cb)
                item_child->desc->action_cb(item_child, MENU_EV_NONE);

            if (!child_nodes->next)
                break;

            child_nodes = child_nodes->next;
            item_child = child_nodes->data;
        }

        //print the 3 items on screen
        screen_menu_page(node);
    }
    else if ((item->desc->type == MENU_CONFIRM) || (item->desc->type == MENU_OK))
    {
        i = 0;
        while (g_menu_popups[i].popup_content)
        {
            if (item->desc->id == g_menu_popups[i].menu_id)
            {
                item->data.list_count = 2;
                item->data.hover = 1;
                item->data.popup_content = g_menu_popups[i].popup_content;
                item->data.popup_header = g_menu_popups[i].popup_header;
                if (item->data.popup_active)
                {
                    item->data.popup_active = 0;
                    g_menu_popup_active = 0;
                }
                else
                {
                    item->data.popup_active = 1;
                    g_menu_popup_active = 1;
                    g_popup_encoder = encoder;
                }
            }
            i++;
        }

        if (item->desc->action_cb)
            item->desc->action_cb(item, MENU_EV_ENTER);
    }

    g_update_cb = NULL;
    g_update_data = NULL;
    if (item->desc->need_update)
    {
        g_update_cb = item->desc->action_cb;
        g_update_data = item;
    }

    TM_print_tool();
    TM_set_leds();
}

static void menu_up(void)
{
    menu_item_t *item = g_current_item;

    if (item->data.hover > 0)
        item->data.hover--;

    screen_system_menu(item);
}


static void menu_down(void)
{
    menu_item_t *item = g_current_item;

    if (item->data.hover < MENU_VISIBLE_LIST_CUT-1)
        item->data.hover++;

    screen_system_menu(item);
}

static void menu_change_value(uint8_t encoder, uint8_t action)
{
    uint8_t i;
    menu_item_t *item;
    if (g_current_item->desc->type == MENU_MAIN)
    {
        node_t *node = g_current_menu->first_child;

        //locate the to be changed item
        for (i = 0; i < encoder; i++)
        {
            if (!node->next)
                return;

            node = node->next;
        }
        item = node->data;
    }
    else
        item = g_current_item;

    //check if another popup in the menu is open, if so return
    if ((g_menu_popup_active) && (g_popup_encoder != encoder))
        return;

    switch (item->desc->id)
    {
        case INP_1_GAIN_ID:
        case INP_2_GAIN_ID:
        case INP_STEREO_LINK:
        case OUTP_1_GAIN_ID:
        case OUTP_2_GAIN_ID:
        case OUTP_STEREO_LINK:
        case HEADPHONE_VOLUME_ID:
        case NOISEGATE_THRES_ID:
        case NOISEGATE_DECAY_ID:
        case COMPRESSOR_RELEASE_ID:
        case COMPRESSOR_PB_VOL_ID:

            if (g_encoders_pressed[encoder])
                item->data.step = item->data.step * 10;

        break;

        default:
            item->data.step = 1;
        break;
    }

    if (action == MENU_EV_ENTER)
    {
        i = 0;
        while (g_menu_popups[i].popup_content)
        {
            //we only display a popup when the value is changed
            if ((item->desc->type == MENU_CLICK_LIST) && ((int)item->data.value == item->data.selected) && !item->data.popup_active)
                return;

            if (item->desc->id == g_menu_popups[i].menu_id)
            {
                if (item->desc->parent_id == USER_PROFILE_ID)
                {
                    static char buffer[50];
                    memset(buffer, 0, sizeof buffer);

                    strcpy(buffer, g_menu_popups[i].popup_content);
                    strcat(buffer, item->data.unit_text);
                    strcat(buffer, "?");
                    item->data.popup_content = buffer;
                }
                else
                    item->data.popup_content = g_menu_popups[i].popup_content;

                item->data.popup_header = g_menu_popups[i].popup_header;

                if (item->data.popup_active)
                {
                    item->data.popup_active = 0;
                    g_menu_popup_active = 0;

                    TM_set_leds();

                    if ((item->desc->id == UPDATE_ID) || (item->desc->id == BLUETOOTH_ID))
                    {
                        if (item->desc->action_cb)
                            item->desc->action_cb(item, action);

                        if (item->data.value == 1)
                            return;

                        //go back to main menu
                        g_current_item = g_current_menu->data;

                        TM_print_tool();
                    }
                    else if (item->desc->id == INFO_ID)
                    {
                        if (encoder == 0) {
                            //go back to main menu
                            g_current_item = g_current_menu->data;

                            TM_print_tool();
                        }

                        return;
                    }

                    TM_print_tool();
                }
                else
                {
                    item->data.popup_active = 1;
                    g_menu_popup_active = 1;
                    g_popup_encoder = encoder;
                    item->data.hover = 1;

                    TM_set_leds();
                }
            }
            i++;
        }

        if (item->desc->type == MENU_CONFIRM2)
        {
            naveg_release_dialog_semaphore();
            return;
        }

    }
    else if ((action == MENU_EV_UP) && (item->data.popup_active == 1))
    {
        if (item->data.hover < (item->data.list_count - 1))
            item->data.hover++;
    }
    else if ((action == MENU_EV_DOWN) && (item->data.popup_active == 1))
    {
        if (item->data.hover > 0)
           item->data.hover--;
    }

    if (item->desc->action_cb && !item->data.popup_active)
        item->desc->action_cb(item, action);

    if ((item->desc->id != ROOT_ID) && (item->desc->id != UPDATE_ID) && (item->desc->id != DIALOG_ID) && (item->desc->id != BLUETOOTH_ID) && (item->desc->id != INFO_ID))
        screen_menu_page(g_current_menu);
    else
        screen_system_menu(g_current_item);
}

static void create_menu_tree(node_t *parent, const menu_desc_t *desc)
{
    uint8_t i;
    menu_item_t *item;

    for (i = 0; g_menu_desc[i].name; i++)
    {
        if (desc->id == g_menu_desc[i].parent_id)
        {
            //menu does not change, so no need to free these
            item = (menu_item_t *) MALLOC(sizeof(menu_item_t));
            item->data.hover = 0;
            item->data.selected = 0xFF;
            item->data.list_count = 0;
            item->data.popup_active = 0;
            item->data.list = (char **) MALLOC(g_max_items_list * sizeof(char *));
            item->desc = &g_menu_desc[i];
            item->name = MALLOC(MAX_CHARS_MENU_NAME);
            strcpy(item->name, g_menu_desc[i].name);

            node_t *node;
            node = node_child(parent, item);

            if (item->desc->type == MENU_ROOT || item->desc->type == MENU_MAIN || item->desc->type == MENU_TOOL)
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

    system_default_tool_cb(NULL, MENU_EV_NONE);
}

void TM_set_first_foot_tool(uint8_t tool)
{
    g_first_foot_tool = TOOL_FOOT + tool;
}

void TM_update_menu(void)
{
    if (g_update_cb)
    {
        (*g_update_cb)(g_update_data, MENU_EV_NONE);
        screen_system_menu(g_update_data);
    }
}

int TM_need_update_menu(void)
{
    return (g_update_cb ? 1: 0);
}

void TM_stop_update_menu(void)
{
    g_update_cb = NULL;
    g_update_data = NULL;
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

void TM_encoder_click(uint8_t encoder)
{
    if (tool_is_on(DISPLAY_TOOL_SYSTEM))
    {
        if (g_current_item->desc->type != MENU_ROOT)
        {
            menu_change_value(encoder, MENU_EV_ENTER);
        }
        else
        {
            TM_enter(0);
        }
    }
}

void TM_enter(uint8_t button)
{
    if ((!g_initialized)&&(g_update_cb)) return;

    if (tool_is_on(TOOL_TUNER))
    {
        static uint8_t steps = 0;

        switch (steps){
            case 0:
                if (button == 0) {
                    steps++;
                }
                else{
                    steps = 0;
                }
            break;
            case 1:
                if (button == 0) {
                    steps++;
                }
                else {
                    steps = 0;
                }
            break;
            case 2:
                if (button == 1) {
                    steps++;
                }
                else {
                    steps = 0;
                }
            break;
            case 3:
                if (button == 2) {
                    steps++;
                }
                else {
                    steps = 0;
                }
            break;
            case 4:
                if (button == 1) {
                    steps++;
                }
                else {
                    steps = 0;
                }
            break;
            case 5:
                if (button == 0) {
                    steps++;
                }
                else {
                    steps = 0;
                }
            break;
            case 6:
                if (button == 0) {
                    TM_turn_off_tuner();
                    screen_image(0, mod_father);
                    g_screenshot_mode_enabled = true;

                    //set footswitch prop
                    actuator_set_prop(hardware_actuators(FOOTSWITCH2), BUTTON_HOLD_TIME, 500);
                }
                steps = 0;
            break;
        }
    }
    else if (tool_is_on(TOOL_MENU))
    {
        //first encoder, check if we need to change menu or item
        if (button == 0)
        {
            if (g_current_item->desc->type == MENU_ROOT)
            {
                menu_enter(button);
            }
            else 
            {
                //go back to main menu
                node_t *node = g_current_menu;
                g_current_menu = node->parent;
                g_current_item = g_current_menu->data;

                TM_print_tool();
                TM_set_leds();
            }
        }
        //for other buttons next / prev parrent node
        else if (button == 1)
        {
            if (g_current_item->desc->id == ROOT_ID)
            {
                //there is  a chance we changed gains, send save
                system_save_gains_cb(NULL, MENU_EV_ENTER);

                naveg_trigger_mode_change(MODE_CONTROL);
                return;
            }

            if (!g_current_menu->prev)
                return;

            node_t *node = g_current_menu->prev;
            g_current_menu = node;
            g_current_item = node->data;

            //make sure we have all menu value's updated 
            node_t *child_nodes = node->first_child;
            menu_item_t *item_child = child_nodes->data;

            uint8_t i;
            for (i = 0; i < 3; i++)
            {
                if (item_child->desc->action_cb)
                    item_child->desc->action_cb(item_child, MENU_EV_NONE);

                if (!child_nodes->next)
                    break;

                child_nodes = child_nodes->next;
                item_child = child_nodes->data;
            }

            node_t *sync_node = g_current_menu->parent;;
            menu_item_t* sync_item = sync_node->data;
            sync_item->data.hover--;

            TM_print_tool();
            TM_set_leds();
        }
        else if (button == 2)
        {
            //twice next, as there is a popup
            if(g_current_item->desc->id == ROOT_ID)
                return;

            node_t *node = g_current_menu->next;
            menu_item_t *next_item = node->data;

            if (next_item->desc->id == BLUETOOTH_ID)
                return;

            g_current_menu = node;
            g_current_item = next_item;

            //make sure we have all menu value's updated 
            node_t *child_nodes = node->first_child;
            menu_item_t *item_child = child_nodes->data;

            uint8_t i;
            for (i = 0; i < 3; i++)
            {
                if (item_child->desc->action_cb)
                    item_child->desc->action_cb(item_child, MENU_EV_NONE);

                if (!child_nodes->next)
                    break;

                child_nodes = child_nodes->next;
                item_child = child_nodes->data;
            }

            node_t *sync_node = g_current_menu->parent;;
            menu_item_t* sync_item = sync_node->data;
            sync_item->data.hover++;

            TM_print_tool();
            TM_set_leds();
        }
    }
}

void TM_up(uint8_t encoder)
{
    if (!g_initialized) return;
        
    if (tool_is_on(TOOL_SYNC))
    {
        if (encoder == 0)
            system_bpb_cb(TM_get_menu_item_by_ID(BPB_ID), MENU_EV_DOWN);
        if (encoder == 1)
        {
            if (system_get_clock_source() == 1)
                return;

            menu_item_t *tempo_item = TM_get_menu_item_by_ID(BPM_ID);

            if (g_encoders_pressed[encoder])
                tempo_item->data.step = tempo_item->data.step * 10;

            system_tempo_cb(tempo_item, MENU_EV_DOWN);
            system_update_menu_value(MENU_ID_TEMPO, tempo_item->data.value);
            system_taptempo_cb(TM_get_menu_item_by_ID(TAP_ID), MENU_EV_NONE);

            update_tap_tempo_led();
        }
        if (encoder == 2)
            system_midi_src_cb(TM_get_menu_item_by_ID(CLOCK_SOURCE_ID_2), MENU_EV_DOWN);

        TM_print_tool();
    }
    else if (tool_is_on(TOOL_MENU))
    {
        //check if another popup in the menu is open, if so return
        if ((g_menu_popup_active) && (g_popup_encoder != encoder))
            return;

        //first encoder, check if we need to change menu or item
        if (encoder == 0)
        {
            if (g_current_item->desc->type == MENU_ROOT)
            {
                menu_up();
            }
            else 
            {
                menu_change_value(encoder, MENU_EV_DOWN);
            }
        }
        else
        {
            menu_change_value(encoder, MENU_EV_DOWN);
        }
    }
}

void TM_down(uint8_t encoder)
{
    if (!g_initialized) return;

    if (tool_is_on(TOOL_SYNC))
    {
        if (encoder == 0)
            system_bpb_cb(TM_get_menu_item_by_ID(BPB_ID), MENU_EV_UP);
        else if (encoder == 1)
        {
            if (system_get_clock_source() == 1)
                return;

            menu_item_t *tempo_item = TM_get_menu_item_by_ID(BPM_ID);

            if (g_encoders_pressed[encoder])
                tempo_item->data.step = tempo_item->data.step * 10;

            system_tempo_cb(tempo_item, MENU_EV_UP);
            system_update_menu_value(MENU_ID_TEMPO, tempo_item->data.value);
            system_taptempo_cb(TM_get_menu_item_by_ID(TAP_ID), MENU_EV_NONE);

            update_tap_tempo_led();
        }
        else if (encoder == 2)
            system_midi_src_cb(TM_get_menu_item_by_ID(CLOCK_SOURCE_ID_2), MENU_EV_UP);


        TM_print_tool();
    }
    else if (tool_is_on(TOOL_MENU))
    {
        //check if another popup in the menu is open, if so return
        if ((g_menu_popup_active) && (g_popup_encoder != encoder))
            return;

        //first encoder, check if we need to change menu or item
        if (encoder == 0)
        {
            if (g_current_item->desc->type == MENU_ROOT)
            {
                menu_down();
            }
            else 
            {
                menu_change_value(encoder, MENU_EV_UP);
            }
        }
        else
        {
            menu_change_value(encoder, MENU_EV_UP);
        }
    }
}

void TM_foot_change(uint8_t foot, uint8_t pressed)
{
    if ((!g_initialized)&&(g_update_cb)) return;

    if (tool_is_on(TOOL_TUNER))
    {
        if (foot == 0)
        {
            system_tuner_mute_cb(TM_get_menu_item_by_ID(TUNER_MUTE_ID), MENU_EV_ENTER);
        }
        else if (foot == 1)
        {
            if (pressed)
                system_tuner_input_cb(TM_get_menu_item_by_ID(TUNER_INPUT_ID), MENU_EV_ENTER);
            
            led_state_t led_state = {
                .color = TUNER_COLOR,
            };

            if (pressed)
                led_state.brightness = 1;
            else
                led_state.brightness = 0.1;

            set_ledz_trigger_by_color_id(hardware_leds(1), LED_DIMMED, led_state);

            TM_print_tool();
            return;
        }

        TM_print_tool();
        TM_set_leds();
    }
    else if (tool_is_on(TOOL_SYNC))
    {
        if (foot == 0)
        {
            system_play_cb(TM_get_menu_item_by_ID(PLAY_ID), MENU_EV_ENTER);
        }
        else if (foot == 1)
        {
            menu_item_t *tempo_item = TM_get_menu_item_by_ID(TAP_ID);
            system_taptempo_cb(tempo_item, MENU_EV_ENTER);
            system_update_menu_value(MENU_ID_TEMPO, tempo_item->data.value);
            system_tempo_cb(TM_get_menu_item_by_ID(BPM_ID), MENU_EV_NONE);
        }

        TM_print_tool();
        TM_set_leds();
    }
}

void TM_reset_menu(void)
{
    if (!g_initialized) return;

    g_current_menu = g_menu;
    g_current_item = g_menu->first_child->data;
    reset_menu_hover(g_menu);
}

void TM_set_dummy_menu_item(node_t *dummy_menu)
{
    tools_off();
    tool_on(TOOL_MENU);
    g_current_tool = TOOL_MENU;

    g_current_menu = dummy_menu;
    g_current_item = dummy_menu->data;

    TM_print_tool();
}

void TM_tool_up(void)
{
    if (g_current_tool < TOOL_FOOT + FOOT_TOOL_AMOUNT)
        g_current_tool++;
    else
        g_current_tool = TOOL_FOOT+1;

    TM_launch_tool(g_current_tool);
}

void TM_launch_tool(int8_t tool)
{
    screen_clear();

    if (tool == -1)
        TM_launch_tool(g_current_tool);   

    if (tool == TOOL_FOOT)
        tool = g_first_foot_tool;

    switch (tool)
    {
        case TOOL_MENU:
            g_current_tool = TOOL_MENU;
            TM_reset_menu();
            tool_on(TOOL_MENU);
            menu_enter(0);
        break;

        case TOOL_TUNER:
                g_current_tool = TOOL_TUNER;
                tools_off();
                tool_on(TOOL_TUNER);

                ui_comm_webgui_send(CMD_TUNER_ON, strlen(CMD_TUNER_ON));

                system_tuner_mute_cb(TM_get_menu_item_by_ID(TUNER_MUTE_ID), MENU_EV_NONE);
                system_tuner_input_cb(TM_get_menu_item_by_ID(TUNER_INPUT_ID), MENU_EV_NONE);

                TM_print_tool();
                TM_set_leds();
        break;

        case TOOL_SYNC:
            g_current_tool = TOOL_SYNC;
            TM_turn_off_tuner();
            tools_off();
            tool_on(TOOL_SYNC);

            system_tempo_cb(TM_get_menu_item_by_ID(BPM_ID), MENU_EV_NONE);
            system_bpb_cb(TM_get_menu_item_by_ID(BPB_ID), MENU_EV_NONE);
            system_midi_src_cb(TM_get_menu_item_by_ID(CLOCK_SOURCE_ID_2), MENU_EV_NONE);
            system_play_cb(TM_get_menu_item_by_ID(PLAY_ID), MENU_EV_NONE);
            system_taptempo_cb(TM_get_menu_item_by_ID(TAP_ID), MENU_EV_NONE);

            TM_print_tool();
            TM_set_leds();
        break;

        case TOOL_BYPASS:
        break;
    }
}

void TM_print_tool(void)
{
    switch (g_current_tool)
    {
        //MDW_TODO set proper LED colours
        case TOOL_MENU:
            if (g_current_item->desc->type == MENU_MAIN)
            {
                //print the 3 items on screen
                screen_menu_page(g_current_menu);
            }
            else
            {
                //print the menu
                screen_system_menu(g_current_item);
            }
        break;

        case TOOL_TUNER:
            //first screen
            screen_toggle_tuner(0.0, "?", 0);

            //draw foots
            menu_item_t *tuner_item = TM_get_menu_item_by_ID(TUNER_INPUT_ID);
            screen_footer(1, tuner_item->desc->name, tuner_item->data.value ? "2":"1", FLAG_CONTROL_ENUMERATION);

            tuner_item = TM_get_menu_item_by_ID(TUNER_MUTE_ID);
            screen_footer(0, tuner_item->desc->name, tuner_item->data.value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT, FLAG_CONTROL_TOGGLED);

            //draw the index
            screen_page_index(g_current_tool - TOOL_FOOT-1, FOOT_TOOL_AMOUNT);
        break;

        case TOOL_SYNC:
            screen_tool_control_page(get_menu_node_by_ID(TEMPO_ID));

            //draw the foots
            menu_item_t *sync_item = TM_get_menu_item_by_ID(PLAY_ID);
            screen_footer(0, sync_item->desc->name, sync_item->data.value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT, FLAG_CONTROL_TOGGLED);

            if (system_get_clock_source() == 1)
            {
                screen_footer(1, "SYNC", TOGGLED_ON_FOOTER_TEXT, FLAG_CONTROL_TOGGLED);
            }
            else
            {
                sync_item = TM_get_menu_item_by_ID(TAP_ID);
                screen_footer(1, sync_item->desc->name, sync_item->data.unit_text, FLAG_CONTROL_ENUMERATION);
            }

            //draw the index
            screen_page_index(g_current_tool - TOOL_FOOT-1, FOOT_TOOL_AMOUNT);
        break;

        case TOOL_BYPASS:
        break;
    }
}

void TM_set_leds(void)
{
    naveg_turn_off_leds();

    led_state_t led_state = {};
    ledz_t *led;

    if (g_menu_popup_active)
    {
        switch (g_current_item->desc->type)
        {
            case MENU_CLICK_LIST:
            //fall-through
            case MENU_CONFIRM2:
            //fall-through
            case MENU_CONFIRM:;
                // ok / cancel or yes / no
                led = hardware_leds(3);
                led_state.color = MENU_OK_COLOR;
                set_ledz_trigger_by_color_id(led, LED_ON, led_state);

                led = hardware_leds(4);
                set_ledz_trigger_by_color_id(led, LED_OFF, led_state);

                led = hardware_leds(5);
                led_state.color = TOGGLED_COLOR;
                set_ledz_trigger_by_color_id(led, LED_ON, led_state);
                return;
            break;

        case MENU_OK:;
            //single item
            led = hardware_leds(3);
            led_state.color = MENU_OK_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);

            led = hardware_leds(4);
            set_ledz_trigger_by_color_id(led, LED_OFF, led_state);

            led = hardware_leds(5);
            set_ledz_trigger_by_color_id(led, LED_OFF, led_state);
            return;
        break;

        //TODO catches popups on a menu page now, since current_item is the main page
        //FIXME later to properly load popups inside a page
        default:
            // ok / cancel or yes / no
            led = hardware_leds(3);
            led_state.color = MENU_OK_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);

            led = hardware_leds(4);
            set_ledz_trigger_by_color_id(led, LED_OFF, led_state);

            led = hardware_leds(5);
            led_state.color = TOGGLED_COLOR;
            set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            return;
        break;
        }
    }

    switch (g_current_tool)
    {
        case TOOL_MENU:
            if (g_current_item->desc->type == MENU_MAIN)
            {
                led = hardware_leds(3);
                led_state.color = TOGGLED_COLOR;
                set_ledz_trigger_by_color_id(led, LED_ON, led_state);

                led = hardware_leds(4);
                led_state.color = MENU_OK_COLOR;
                if (!g_current_menu->prev)
                    led_state.brightness = 0.25;
                else
                    led_state.brightness = 1;
                    
                set_ledz_trigger_by_color_id(led, LED_DIMMED, led_state);

                led = hardware_leds(5);
                led_state.color = MENU_OK_COLOR;
                menu_item_t *next_menu = g_current_menu->next->data;
                if (next_menu->desc->id == BLUETOOTH_ID)
                    led_state.brightness = 0.25;
                else
                    led_state.brightness = 1;

                set_ledz_trigger_by_color_id(led, LED_DIMMED, led_state);
            }
            else
            {
                led = hardware_leds(3);
                led_state.color = MENU_OK_COLOR;
                set_ledz_trigger_by_color_id(led, LED_ON, led_state);
                led = hardware_leds(4);
                led_state.color = TOGGLED_COLOR;
                set_ledz_trigger_by_color_id(led, LED_ON, led_state);
            }
        break;

        case TOOL_TUNER:;
            led_state.brightness = 0.1;
            led_state.color = TUNER_COLOR;
            set_ledz_trigger_by_color_id(hardware_leds(1), LED_DIMMED, led_state);

            menu_item_t *tuner_item = TM_get_menu_item_by_ID(TUNER_MUTE_ID);
            led_state.color = TUNER_COLOR;
            if (tuner_item->data.value) led_state.brightness = 1;
            set_ledz_trigger_by_color_id(hardware_leds(0), LED_DIMMED, led_state);

            set_tool_pages_led_state();
        break;

        case TOOL_SYNC:;
            menu_item_t *sync_item = TM_get_menu_item_by_ID(PLAY_ID);
            led_state.color = TEMPO_COLOR;

            if (sync_item->data.value) led_state.brightness = 1;
            else led_state.brightness = 0.1;

            set_ledz_trigger_by_color_id(hardware_leds(0), LED_DIMMED, led_state);

            update_tap_tempo_led();

            set_tool_pages_led_state();
        break;

        case TOOL_BYPASS:
        break;
    }
}

menu_item_t *TM_get_menu_item_by_ID(uint8_t menu_id)
{
    return get_menu_node_by_ID(menu_id)->data;
}

menu_item_t *TM_get_current_menu_item(void)
{
    return g_current_item;
}

void TM_turn_off_tuner(void)
{
    ui_comm_webgui_send(CMD_TUNER_OFF, strlen(CMD_TUNER_OFF));
    ui_comm_webgui_wait_response();

    //turn off all tools
    tools_off();
}

uint8_t TM_check_tool_status(void)
{
    return g_current_tool;
}
