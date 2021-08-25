/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "system.h"
#include "config.h"
#include "data.h"
#include "naveg.h"
#include "hardware.h"
#include "actuator.h"
#include "cli.h"
#include "screen.h"
#include "glcd_widget.h"
#include "glcd.h"
#include "utils.h"
#include "mod-protocol.h"
#include "ui_comm.h"
#include "sys_comm.h"
#include "mode_tools.h"
#include "mode_control.h"
#include "naveg.h"

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define SHIFT_MENU_ITEMS_COUNT      20

/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

// systemctl services names
const char *systemctl_services[] = {
    "jack2",
    "sshd",
    "mod-ui",
    "dnsmasq",
    NULL
};

const char *versions_names[] = {
    "version",
    "restore",
    "system",
    "controller",
    NULL
};

static const int16_t SHIFT_ITEM_IDS[SHIFT_MENU_ITEMS_COUNT] = {INP_STEREO_LINK, INP_1_GAIN_ID, INP_2_GAIN_ID, OUTP_STEREO_LINK, OUTP_1_GAIN_ID, OUTP_2_GAIN_ID,\
                                                               HEADPHONE_VOLUME_ID, CLOCK_SOURCE_ID, SEND_CLOCK_ID, MIDI_PB_PC_CHANNEL_ID, MIDI_SS_PC_CHANNEL_ID,\
                                                               DISPLAY_BRIGHTNESS_ID, BPM_ID, BPB_ID, NOISEGATE_CHANNEL_ID, NOISEGATE_THRES_ID, NOISEGATE_DECAY_ID,\
                                                               COMPRESSOR_MODE_ID, COMPRESSOR_RELEASE_ID, COMPRESSOR_PB_VOL_ID};

/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/

struct TAP_TEMPO_T {
    uint32_t time, max;
    uint8_t state;
} g_tool_tap_tempo;

/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/

#define UNUSED_PARAM(var)   do { (void)(var); } while (0)
#define MAP(x, Omin, Omax, Nmin, Nmax)      ( x - Omin ) * (Nmax -  Nmin)  / (Omax - Omin) + Nmin;


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

uint8_t g_bypass[4] = {};
uint8_t g_current_profile = 1;
uint8_t g_snapshot_prog_change = 0;
uint8_t g_pedalboard_prog_change = 0;
uint16_t g_beats_per_minute = 0;
uint8_t g_beats_per_bar = 0;
uint8_t g_MIDI_clk_send = 0;
uint8_t g_MIDI_clk_src = 0;
uint8_t g_play_status = 0;
uint8_t g_tuner_mute = 0;
uint8_t g_tuner_input = 0;
int8_t g_display_brightness = -1;
int8_t g_display_contrast = -1;
int8_t g_actuator_hide = -1;
int8_t g_click_list = -1;
int8_t g_shift_item[3] = {-1, -1, -1};
int8_t g_default_tool = -1;
int8_t g_list_mode = -1;
int8_t g_control_header = -1;
int8_t g_usb_mode = -1;
int8_t g_noise_removal_mode = -1;
int8_t g_shift_mode = -1;

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

static void update_gain_item_value(uint8_t menu_id, float value)
{
    menu_item_t *item = TM_get_menu_item_by_ID(menu_id);
    item->data.value = value;

    static char str_bfr[8] = {};
    float value_bfr = 0;

    if ((menu_id == INP_1_GAIN_ID) || (menu_id == INP_2_GAIN_ID)) {
        value_bfr = MAP(item->data.value, item->data.min, item->data.max, -12, 25);
    }
    else {
        value_bfr = item->data.value;
    }

    float_to_str(value_bfr, str_bfr, 8, 2);
    strcat(str_bfr, " dB");
    item->data.unit_text = str_bfr;
}

void set_item_value(char *command, uint16_t value)
{
    uint8_t i;
    char buffer[50];

    i = copy_command((char *)buffer, command);

    // copy the value
    char str_buf[8];
    int_to_str(value, str_buf, 4, 0);
    const char *p = str_buf;
    while (*p)
    {
        buffer[i++] = *p;
        p++;
    }
    buffer[i] = 0;

    // sets the response callback
    ui_comm_webgui_set_response_cb(NULL, NULL);

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);
}

static void set_menu_item_value(uint16_t menu_id, uint16_t value)
{
    uint8_t i = 0;
    char buffer[50];
    memset(buffer, 0, sizeof buffer);

    i = copy_command((char *)buffer, CMD_MENU_ITEM_CHANGE);

    // copy the id
    i += int_to_str(menu_id, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    // copy the value
    i += int_to_str(value, &buffer[i], sizeof(buffer) - i, 0);

    buffer[i++] = 0;

    // sets the response callback
    ui_comm_webgui_set_response_cb(NULL, NULL);

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);
}

static void recieve_sys_value(void *data, menu_item_t *item)
{
    char **values = data;

    //protocol ok
    if (atoi(values[1]) == 0)
        item->data.value = atof(values[2]);
}

static void recieve_bluetooth_info(void *data, menu_item_t *item)
{
    char **values = data;

    //protocol ok
    if (atof(values[1]) != 0)
        return;

    char resp[LINE_BUFFER_SIZE];
    strncpy(resp, values[2], sizeof(resp)-1);

    //merge all arguments, do not split by space for this specific case
    if (values[3]) {
        uint8_t i;
        for (i = 3; values[i]; i++) {
            strcat(resp, " ");
            strcat(resp, values[i]);
        }
    }

    char **items = strarr_split(resp, '|');;

    if (items)
    {
        char *buffer = (char *) MALLOC(150 * sizeof(char));

        strcpy(buffer, "\nEnable Bluetooth discovery\nmode for 2 minutes?");
        strcat(buffer, "\n\nSTATUS: ");
        strcat(buffer, items[0]);
        strcat(buffer, "\nNAME: ");
        strcat(buffer, items[1]);
        strcat(buffer, "\nADDRESS: ");
        strcat(buffer, items[2]);

        item->data.popup_content = buffer;

        FREE(items);
        FREE(buffer);
    }
}

static void append_sys_value_popup(void *data, menu_item_t *item)
{
    char **values = data;

    //protocol ok
    if (atof(values[1]) != 0)
        return;

    char resp[LINE_BUFFER_SIZE];

    memset(resp, 0, LINE_BUFFER_SIZE*sizeof(char));
            
    strncpy(resp, values[2], sizeof(resp)-1);

    strcat(item->data.popup_content, resp);
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

int16_t system_get_shift_item(uint8_t index)
{
    if (index < SHIFT_MENU_ITEMS_COUNT)
        return SHIFT_ITEM_IDS[index];
    else
        return SHIFT_ITEM_IDS[0];
}

uint8_t system_get_clock_source(void)
{
    return g_MIDI_clk_src;
}

uint8_t system_get_current_profile(void)
{
    return g_current_profile;
}

void system_save_gains_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);

    if (event == MENU_EV_ENTER)
    {
        uint8_t i = 0;
        char buffer[50];
        memset(buffer, 0, sizeof buffer);

        i = copy_command((char *)buffer, CMD_SYS_AMIXER_SAVE);

        buffer[i++] = 0;

        // sends the data to GUI
        sys_comm_send(buffer, NULL);

        sys_comm_wait_response();
    }
}

void system_update_menu_value(uint8_t item_ID, uint16_t value)
{
    switch(item_ID)
    {
        //play status
        case MENU_ID_PLAY_STATUS:
            g_play_status = value;
            //check if we need to update leds/display
            if ((naveg_get_current_mode() == MODE_TOOL_FOOT) && (TM_check_tool_status() == TOOL_SYNC)) {
                menu_item_t *play_item = TM_get_menu_item_by_ID(PLAY_ID);
                play_item->data.value = g_play_status;
                TM_print_tool();
                TM_set_leds();
            }
        break;
        //global tempo
        case MENU_ID_TEMPO:
            g_beats_per_minute = value;
            //check if we need to update leds/display
            if ((naveg_get_current_mode() == MODE_TOOL_FOOT) && (TM_check_tool_status() == TOOL_SYNC)) {
                menu_item_t *tempo_item = TM_get_menu_item_by_ID(BPM_ID);
                tempo_item->data.value = g_beats_per_minute;

                static char str_bfr[12] = {};
                int_to_str(g_beats_per_minute, str_bfr, 4, 0);
                strcat(str_bfr, " BPM");
                tempo_item->data.unit_text = str_bfr;

                tempo_item = TM_get_menu_item_by_ID(TAP_ID);
                tempo_item->data.value = g_beats_per_minute;
                TM_print_tool();
                TM_set_leds();
            }
        break;
        //global tempo status
        case MENU_ID_BEATS_PER_BAR:
            g_beats_per_bar = value;
            //check if we need to update leds/display
            if ((naveg_get_current_mode() == MODE_TOOL_FOOT) && (TM_check_tool_status() == TOOL_SYNC)) {
                menu_item_t *bar_item = TM_get_menu_item_by_ID(BPB_ID);
                bar_item->data.value = g_beats_per_bar;
                TM_print_tool();
                TM_set_leds();
            }
        break;
        //tuner mute
        case MENU_ID_TUNER_MUTE: 
            g_tuner_mute = value;
        break;
        //bypass channel 1
        case MENU_ID_BYPASS1: 
            g_bypass[0] = value;
        break;
        //bypass channel 2
        case MENU_ID_BYPASS2: 
            g_bypass[1] = value;
        break;
        //MIDI clock source
        case MENU_ID_MIDI_CLK_SOURCE: 
            g_MIDI_clk_src = value;
            //check if we need to update leds/display
            if ((naveg_get_current_mode() == MODE_TOOL_FOOT) && (TM_check_tool_status() == TOOL_SYNC)) {
                menu_item_t *clk_source_item = TM_get_menu_item_by_ID(CLOCK_SOURCE_ID_2);
                clk_source_item->data.value = g_MIDI_clk_src;
                TM_print_tool();
                TM_set_leds();
            }
        break;
        //send midi clock
        case MENU_ID_MIDI_CLK_SEND: 
            g_MIDI_clk_send = value;
        break;
        //snapshot prog change 
        case MENU_ID_SNAPSHOT_PRGCHGE: 
            g_snapshot_prog_change = value;
        break;
        //pedalboard prog change 
        case MENU_ID_PB_PRGCHNGE: 
            g_pedalboard_prog_change = value;
        break;
        //user profile change 
        case MENU_ID_CURRENT_PROFILE: 
            g_current_profile = value;
        break;
        //display brightness
        case MENU_ID_BRIGHTNESS: 
            g_display_brightness = value;
            hardware_glcd_brightness(g_display_brightness); 
        break;
        default:
            return;
        break;
        
    }
}

void system_bluetooth_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    sys_comm_set_response_cb(recieve_bluetooth_info, item);

    sys_comm_send(CMD_SYS_BT_STATUS, NULL);
    sys_comm_wait_response();

    if ((event == MENU_EV_ENTER) && !item->data.popup_active)
    {
        TM_stop_update_menu();

        if (item->data.hover == 0)
        {
            sys_comm_set_response_cb(NULL, NULL);

            sys_comm_send(CMD_SYS_BT_DISCOVERY, NULL);
            sys_comm_wait_response();
        }
    }
}

void system_info_cb(void *arg, int event)
{
    menu_item_t *item = arg;
    item->data.hover = 0;
    item->data.min = 0;
    item->data.max = 0;
    item->data.list_count = 0;

    if (event == MENU_EV_ENTER)
    {
        static char buffer[70];
        memset(buffer, 0, sizeof buffer);
        strcpy(buffer, item->data.popup_content);
        item->data.popup_content = buffer;

        sys_comm_set_response_cb(append_sys_value_popup, item);
        sys_comm_send(CMD_SYS_VERSION, "release");
        sys_comm_wait_response();

        strcat(item->data.popup_content, "\n\nDevice Serial:\n");

        sys_comm_set_response_cb(append_sys_value_popup, item);
        sys_comm_send(CMD_SYS_SERIAL, NULL);
        sys_comm_wait_response();
    }
}

void system_upgrade_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        button_t *foot = (button_t *) hardware_actuators(FOOTSWITCH2);

        // check if OK option was chosen
        if (item->data.hover == 0)
        {
            uint8_t foot_status = actuator_get_status(foot);

            // check if footswitch is pressed down
            if (BUTTON_PRESSED(foot_status))
            {
                //clear all screens
                screen_clear();

                // start restore
                cli_restore(RESTORE_INIT);

                item->data.value = 1;

                return;
            }
        }
    }

    item->data.value = 0;
}

void system_inp_0_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);

        q = copy_command(val_buffer, "0 2");
        val_buffer[q] = 0;
        
        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.min = 0.0f;
        item->data.max = 74.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        q = copy_command(val_buffer, "0 0 ");
        update_gain_item_value(INP_1_GAIN_ID, item->data.value);
        update_gain_item_value(INP_2_GAIN_ID, item->data.value);

        // insert the value on buffer
        q += float_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 1);
        val_buffer[q] = 0;

        sys_comm_set_response_cb(NULL, NULL);
        ui_comm_webgui_set_response_cb(NULL, NULL);
        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.step = 1.0f;
    }

    static char str_bfr[10] = {};
    float scaled_val = MAP(item->data.value, item->data.min, item->data.max, -12, 25);
    float_to_str(scaled_val, str_bfr, sizeof(str_bfr), 2);
    strcat(str_bfr, " dB");
    item->data.unit_text = str_bfr;

    item->data.step = 1.0f;
}

void system_inp_1_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);

        q = copy_command(val_buffer, "0 2");
        val_buffer[q] = 0;
        
        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.min = 0.0f;
        item->data.max = 74.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        q = copy_command(val_buffer, "0 2 ");

        // insert the value on buffer
        q += float_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 1);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.step = 1.0f;
    }

    static char str_bfr[10] = {};
    float scaled_val = MAP(item->data.value, item->data.min, item->data.max, -12, 25);
    float_to_str(scaled_val, str_bfr, sizeof(str_bfr), 2);
    strcat(str_bfr, " dB");
    item->data.unit_text = str_bfr;

    item->data.step = 1.0f;
}

void system_inp_2_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);

        q = copy_command(val_buffer, "0 1");
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.min = 0.0f;
        item->data.max = 74.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        q = copy_command(val_buffer, "0 1 ");

        // insert the value on buffer
        q += float_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 1);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.step = 1.0f;
    }

    static char str_bfr[10] = {};
    float scaled_val = MAP(item->data.value, item->data.min, item->data.max, -12, 25);
    float_to_str(scaled_val, str_bfr, sizeof(str_bfr), 2);
    strcat(str_bfr, " dB");
    item->data.unit_text = str_bfr;

    item->data.step = 1.0f;
}

void system_outp_0_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[30];
    uint8_t q;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);

        q = copy_command(val_buffer, "1 2");
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.min = -127.5f;
        item->data.max = 0.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (item->data.value == 0.0)
            item->data.step = 0.5;

        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        q = copy_command(val_buffer, "1 0 ");
        update_gain_item_value(OUTP_1_GAIN_ID, item->data.value);
        update_gain_item_value(OUTP_2_GAIN_ID, item->data.value);

        // insert the value on buffer
        q += float_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 2);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.step = 1.0f;
    }

    static char str_bfr[10] = {};
    float_to_str(item->data.value, str_bfr, sizeof(str_bfr), 2);
    strcat(str_bfr, " dB");
    item->data.unit_text = str_bfr;

    item->data.step = 1.0f;
}

void system_outp_1_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[30];
    uint8_t q;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);

        q = copy_command(val_buffer, "1 2");
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.min = -127.5f;
        item->data.max = 0.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (item->data.value == 0.0)
            item->data.step = 0.5;

        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        q = copy_command(val_buffer, "1 2 ");

        // insert the value on buffer

        q += float_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 2);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.step = 1.0f;
    }

    static char str_bfr[14] = {};
    float_to_str(item->data.value, str_bfr, sizeof(str_bfr), 2);
    strcat(str_bfr, " dB");
    item->data.unit_text = str_bfr;

    item->data.step = 1.0f;
}

void system_outp_2_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[30];
    uint8_t q;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);

        q = copy_command(val_buffer, "1 1");
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.min = -127.5f;
        item->data.max = 0.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (item->data.value == 0.0)
            item->data.step = 0.5;

        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        q = copy_command(val_buffer, "1 1 ");

        // insert the value on buffer
        q += float_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 2);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.step = 1.0f;
    }

    static char str_bfr[10] = {};
    float_to_str(item->data.value, str_bfr, sizeof(str_bfr), 2);
    strcat(str_bfr, " dB");
    item->data.unit_text = str_bfr;

    item->data.step = 1.0f;
}

void system_hp_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q=0;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);

        sys_comm_send(CMD_SYS_HP_GAIN, NULL);
        sys_comm_wait_response();

        item->data.min = -33.0f;
        item->data.max = 12.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        // insert the value on buffer
        q += float_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 1);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_HP_GAIN, val_buffer);
        sys_comm_wait_response();

        item->data.step = 3.0f;
    }

    static char str_bfr[10] = {};
    float_to_str(item->data.value, str_bfr, sizeof(str_bfr), 2);
    strcat(str_bfr, " dB");
    item->data.unit_text = str_bfr;

    item->data.step = 3.0f;
}

void system_display_brightness_cb(void *arg, int event)
{
    menu_item_t *item = arg;
    static char str_bfr[8];

    if (g_display_brightness == -1)
    {
        //read EEPROM
        uint8_t read_buffer = 0;
        EEPROM_Read(0, DISPLAY_BRIGHTNESS_ADRESS, &read_buffer, MODE_8_BIT, 1);

        g_display_brightness = read_buffer;
    }

    if (event == MENU_EV_ENTER)
    {
        if (g_display_brightness < MAX_BRIGHTNESS) g_display_brightness++;
        else g_display_brightness = 0;
    }
    else if (event == MENU_EV_UP)
    {
        if (g_display_brightness < MAX_BRIGHTNESS) g_display_brightness++;
        else return;
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_display_brightness > 0) g_display_brightness--;
        else return;
    }
    else if (event == MENU_EV_NONE)
    {
        //only display value
        item->data.value = g_display_brightness;
        item->data.min = 0;
        item->data.max = 4;
    }

    if (item->data.value != g_display_brightness)
    {
        hardware_glcd_brightness(g_display_brightness);

        //also write to EEPROM
        uint8_t write_buffer = g_display_brightness;
        EEPROM_Write(0, DISPLAY_BRIGHTNESS_ADRESS, &write_buffer, MODE_8_BIT, 1);


        item->data.value = g_display_brightness;
    }

    int_to_str((g_display_brightness * 25), str_bfr, 4, 0);
    strcat(str_bfr, "%");
    item->data.unit_text = str_bfr;

    item->data.step = 1;
}

void system_display_contrast_cb(void *arg, int event)
{
    menu_item_t *item = arg;
    static char str_bfr[8];

    if (g_display_contrast == -1)
    {
        //read EEPROM
        uint8_t read_buffer = 0;
        EEPROM_Read(0, DISPLAY_CONTRAST_ADRESS, &read_buffer, MODE_8_BIT, 1);

        g_display_contrast = read_buffer;
    }

    if (event == MENU_EV_UP)
    {
        if (g_display_contrast < item->data.max) g_display_contrast++;
        else return;
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_display_contrast > item->data.min) g_display_contrast--;
        else return;
    }
    else if (event == MENU_EV_NONE)
    {
        //only display value
        item->data.value = g_display_contrast;
        item->data.min = DISPLAY_CONTRAST_MIN;
        item->data.max = DISPLAY_CONTRAST_MAX;
    }

    if (item->data.value != g_display_contrast)
    {
        st7565p_set_contrast(hardware_glcds(0), g_display_contrast);
        
        //also write to EEPROM
        uint8_t write_buffer = g_display_contrast;
        EEPROM_Write(0, DISPLAY_CONTRAST_ADRESS, &write_buffer, MODE_8_BIT, 1);

        item->data.value = g_display_contrast;
    }

    int mapped_value = MAP(g_display_contrast, (int)DISPLAY_CONTRAST_MIN, (int)DISPLAY_CONTRAST_MAX, 0, 100);
    int_to_str(mapped_value, str_bfr, 4, 0);

    strcat(str_bfr, "%");
    item->data.unit_text = str_bfr;

    item->data.step = 1;
}

void system_hide_actuator_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (g_actuator_hide == -1)
    {
        //read EEPROM
        uint8_t read_buffer = 0;
        EEPROM_Read(0, HIDE_ACTUATOR_ADRESS, &read_buffer, MODE_8_BIT, 1);

        g_actuator_hide = read_buffer;

        //write to screen.c
        screen_set_hide_non_assigned_actuators(g_actuator_hide);

        if (!item) return;
    }

    if (event == MENU_EV_ENTER)
    {
        if (g_actuator_hide == 0) g_actuator_hide = 1;
        else g_actuator_hide = 0;
        
        //also write to EEPROM
        uint8_t write_buffer = g_actuator_hide;
        EEPROM_Write(0, HIDE_ACTUATOR_ADRESS, &write_buffer, MODE_8_BIT, 1);

        //write to screen.c
        screen_set_hide_non_assigned_actuators(g_actuator_hide);
    }
    else if (event == MENU_EV_UP)
    {
        g_actuator_hide = 1;
        
        //also write to EEPROM
        uint8_t write_buffer = g_actuator_hide;
        EEPROM_Write(0, HIDE_ACTUATOR_ADRESS, &write_buffer, MODE_8_BIT, 1);

        //write to screen.c
        screen_set_hide_non_assigned_actuators(g_actuator_hide);
    }
    else if (event == MENU_EV_DOWN)
    {
        g_actuator_hide = 0;
        
        //also write to EEPROM
        uint8_t write_buffer = g_actuator_hide;
        EEPROM_Write(0, HIDE_ACTUATOR_ADRESS, &write_buffer, MODE_8_BIT, 1);

        //write to screen.c
        screen_set_hide_non_assigned_actuators(g_actuator_hide);
    }

    item->data.unit_text = g_actuator_hide ? "HIDE" : "SHOW";
    item->data.value = g_actuator_hide;
}

void system_click_list_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (g_click_list == -1)
    {
        //read EEPROM
        uint8_t read_buffer = 0;
        EEPROM_Read(0, CLICK_LIST_ADRESS, &read_buffer, MODE_8_BIT, 1);

        g_click_list = read_buffer;
        CM_set_list_behaviour(g_click_list);

        if (!item) return;
    }

    if (event == MENU_EV_ENTER)
    {
        if (g_click_list == 0) g_click_list = 1;
        else g_click_list = 0;

        //also write to EEPROM
        uint8_t write_buffer = g_click_list;
        EEPROM_Write(0, CLICK_LIST_ADRESS, &write_buffer, MODE_8_BIT, 1);
    }
    else if (event == MENU_EV_UP)
    {
        g_click_list = 1;

        //also write to EEPROM
        uint8_t write_buffer = g_click_list;
        EEPROM_Write(0, CLICK_LIST_ADRESS, &write_buffer, MODE_8_BIT, 1);
    }
    else if (event == MENU_EV_DOWN)
    {
        g_click_list = 0;

        //also write to EEPROM
        uint8_t write_buffer = g_click_list;
        EEPROM_Write(0, CLICK_LIST_ADRESS, &write_buffer, MODE_8_BIT, 1);
    }

    //write to mode_control.c
    CM_set_list_behaviour(g_click_list);

    item->data.unit_text = g_click_list ? "Click" : "Direct";
    item->data.value = g_click_list;
}

void system_midi_src_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (g_MIDI_clk_src < 2) g_MIDI_clk_src++;
        else g_MIDI_clk_src = 0;
        set_menu_item_value(MENU_ID_MIDI_CLK_SOURCE, g_MIDI_clk_src);
    }
    else if (event == MENU_EV_UP)
    {
        if (g_MIDI_clk_src < 2) g_MIDI_clk_src++;
        set_menu_item_value(MENU_ID_MIDI_CLK_SOURCE, g_MIDI_clk_src);
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_MIDI_clk_src > 0) g_MIDI_clk_src--;
        set_menu_item_value(MENU_ID_MIDI_CLK_SOURCE, g_MIDI_clk_src);
    }

    //translate the int to string value for the menu
    if (g_MIDI_clk_src == 0) item->data.unit_text ="INTERNAL";
    else if (g_MIDI_clk_src == 1) item->data.unit_text = "MIDI";
    else if (g_MIDI_clk_src == 2) item->data.unit_text ="ABLETON LINK";
}

void system_midi_send_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (g_MIDI_clk_send == 0) g_MIDI_clk_send = 1;
        else g_MIDI_clk_send = 0;
        set_menu_item_value(MENU_ID_MIDI_CLK_SEND, g_MIDI_clk_send);
    }
    else if (event == MENU_EV_UP)
    {
        if (g_MIDI_clk_send < 1) g_MIDI_clk_send++;
        set_menu_item_value(MENU_ID_MIDI_CLK_SEND, g_MIDI_clk_send);
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_MIDI_clk_send > 0) g_MIDI_clk_send--;
        set_menu_item_value(MENU_ID_MIDI_CLK_SEND, g_MIDI_clk_send);
    }

    item->data.value = g_MIDI_clk_send;
}

void system_ss_prog_change_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        set_menu_item_value(MENU_ID_SNAPSHOT_PRGCHGE, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        //set the item value to the snapshot_prog_change since mod-ui is master
        item->data.value = g_snapshot_prog_change;
        item->data.min = 0;
        item->data.max = 16;
    }
    else if (event == MENU_EV_UP)
    {
        if (g_snapshot_prog_change < item->data.max) g_snapshot_prog_change++;
        //let mod-ui know
        set_menu_item_value(MENU_ID_SNAPSHOT_PRGCHGE, g_snapshot_prog_change);
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_snapshot_prog_change > item->data.min) g_snapshot_prog_change--;
        //let mod-ui know
        set_menu_item_value(MENU_ID_SNAPSHOT_PRGCHGE, g_snapshot_prog_change);
    }

    static char str_bfr[8] = {};
    int_to_str(g_snapshot_prog_change, str_bfr, 3, 0);
    //a value of 0 means we turn off
    if (g_snapshot_prog_change == 0) strcpy(str_bfr, "OFF");
    item->data.unit_text = str_bfr;

    item->data.step = 1;
}

void system_pb_prog_change_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        set_menu_item_value(MENU_ID_PB_PRGCHNGE, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        //set the item value to the snapshot_prog_change since mod-ui is master
        item->data.value = g_pedalboard_prog_change;
        item->data.min = 0;
        item->data.max = 16;
    }
    else if (event == MENU_EV_UP)
    {
        if (g_pedalboard_prog_change < item->data.max) g_pedalboard_prog_change++;
        //let mod-ui know
        set_menu_item_value(MENU_ID_PB_PRGCHNGE, g_pedalboard_prog_change);
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_pedalboard_prog_change > item->data.min) g_pedalboard_prog_change--;
        //let mod-ui know
        set_menu_item_value(MENU_ID_PB_PRGCHNGE, g_pedalboard_prog_change);
    }

    static char str_bfr[8] = {};
    int_to_str(g_pedalboard_prog_change, str_bfr, 3, 0);
    //a value of 0 means we turn off
    if (g_pedalboard_prog_change == 0) strcpy(str_bfr, "OFF");
    item->data.unit_text = str_bfr;

    item->data.step = 1;
}

//USER PROFILE X (loading)
void system_load_pro_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    //if clicked and YES was selected from the pop-up
    if ((event == MENU_EV_ENTER) && (item->data.hover == 0))
    {
        //current profile is the ID (A=1, B=2, C=3, D=4)
        g_current_profile = item->data.value;
        item->data.selected = g_current_profile;
        set_item_value(CMD_PROFILE_LOAD, g_current_profile);

        naveg_update_shift_item_ids();
        naveg_update_shift_item_values();
    }

    if (item->data.popup_active)
        return;

    if ((event == MENU_EV_UP) && (item->data.value < item->data.max))
        item->data.value++;
    else if ((event == MENU_EV_DOWN) && (item->data.value > item->data.min))
        item->data.value--;
    else if (event == MENU_EV_NONE)
    {
        //display current profile number
        item->data.value = g_current_profile;
        item->data.selected = g_current_profile;
        item->data.min = 1;
        item->data.max = 4;
        item->data.list_count = 2;
        item->data.hover = 1;
    }

    //display the current profile
    switch ((int)item->data.value)
    {
        case 1: item->data.unit_text = "[A]"; break;
        case 2: item->data.unit_text = "[B]"; break;
        case 3: item->data.unit_text = "[C]"; break;
        case 4: item->data.unit_text = "[D]"; break;
    }

    item->data.step = 1;
}

//OVERWRITE CURRENT PROFILE
void system_save_pro_cb(void *arg, int event)
{
   menu_item_t *item = arg;

    //if clicked and YES was selected from the pop-up
    if ((event == MENU_EV_ENTER) && (item->data.hover == 0))
    {
        item->data.selected = item->data.value;
        set_item_value(CMD_PROFILE_STORE, item->data.value);
    }

    if (item->data.popup_active)
        return;

    if ((event == MENU_EV_UP) && (item->data.value < item->data.max))
        item->data.value++;
    else if ((event == MENU_EV_DOWN) && (item->data.value > item->data.min))
        item->data.value--;
    else if (event == MENU_EV_NONE)
    {
        //display current profile number
        item->data.value = 1;
        item->data.selected = 1;
        item->data.min = 1;
        item->data.max = 4;
        item->data.list_count = 2;
        item->data.hover = 1;
    }

    //display the current profile
    switch ((int)item->data.value)
    {
        case 1: item->data.unit_text = "[A]"; break;
        case 2: item->data.unit_text = "[B]"; break;
        case 3: item->data.unit_text = "[C]"; break;
        case 4: item->data.unit_text = "[D]"; break;
    }

    item->data.step = 1;
}

void system_shift_item_cb(void *arg, int event)
{
    menu_item_t *item = arg;
    
    uint8_t shift_item_id = item->desc->id - SHIFT_ITEMS_ID - 1;
    
    if (g_shift_item[shift_item_id] == -1)
    {
        //read EEPROM
        uint8_t read_buffer = 0;
        EEPROM_Read(0, SHIFT_ITEM_ADRESS + shift_item_id, &read_buffer, MODE_8_BIT, 1);

        if (read_buffer < SHIFT_MENU_ITEMS_COUNT)
            g_shift_item[shift_item_id] = read_buffer;
        else
            g_shift_item[shift_item_id] = 0;
    }

    if (event == MENU_EV_ENTER)
    {
        if (g_shift_item[shift_item_id] < item->data.max) g_shift_item[shift_item_id]++;
        else g_shift_item[shift_item_id] = 0;
    }
    else if (event == MENU_EV_UP)
    {
        if (g_shift_item[shift_item_id] < item->data.max) g_shift_item[shift_item_id]++;
        else return;
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_shift_item[shift_item_id] > 0) g_shift_item[shift_item_id]--;
        else return;
    }
    else if (event == MENU_EV_NONE)
    {
        //only display value
        item->data.value = g_shift_item[shift_item_id];
        item->data.min = 0;
        item->data.max = SHIFT_MENU_ITEMS_COUNT-1;
    }

    if (item->data.value != g_shift_item[shift_item_id])
    {
        //also write to EEPROM
        uint8_t write_buffer = g_shift_item[shift_item_id];
        EEPROM_Write(0, SHIFT_ITEM_ADRESS + shift_item_id, &write_buffer, MODE_8_BIT, 1);

        item->data.value = g_shift_item[shift_item_id];

        naveg_update_single_shift_item(shift_item_id, SHIFT_ITEM_IDS[(int)item->data.value]);
    }

    //add item to text
    item->data.unit_text = TM_get_menu_item_by_ID(SHIFT_ITEM_IDS[(int)item->data.value])->name;

    item->data.step = 1;
}

void system_default_tool_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (g_default_tool == -1)
    {
        //read EEPROM
        uint8_t read_buffer = 0;
        EEPROM_Read(0, DEFAULT_TOOL_ADRESS, &read_buffer, MODE_8_BIT, 1);

        g_default_tool = read_buffer;

        TM_set_first_foot_tool(g_default_tool+1);

        if (!item) return;
    }

    if (event == MENU_EV_ENTER)
    {
        if (g_default_tool < FOOT_TOOL_AMOUNT-1) g_default_tool++;
        else g_default_tool = 0;
    }
    else if (event == MENU_EV_UP)
    {
        if (g_default_tool < FOOT_TOOL_AMOUNT-1)
            g_default_tool += item->data.step;
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_default_tool > 0)
            g_default_tool -= item->data.step;
    }
    else if (event == MENU_EV_NONE)
    {
        //only display value
        item->data.value = g_default_tool;
        item->data.min = 0;
        item->data.max = FOOT_TOOL_AMOUNT;
    }

    if (event != MENU_EV_NONE)
    {
        TM_set_first_foot_tool(g_default_tool+1);

        //also write to EEPROM
        uint8_t write_buffer = g_default_tool;
        EEPROM_Write(0, DEFAULT_TOOL_ADRESS, &write_buffer, MODE_8_BIT, 1);

        item->data.value = g_default_tool;
    }
    
    switch ((int)item->data.value)
    {
        case 0: item->data.unit_text = "TUNER"; break;
        case 1: item->data.unit_text = "TEMPO"; break;
    }

    item->data.step = 1;
}

void system_control_header_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (g_control_header == -1)
    {
        //read EEPROM
        uint8_t read_buffer = 0;
        EEPROM_Read(0, CONTROL_HEADER_ADRESS, &read_buffer, MODE_8_BIT, 1);

        g_control_header = read_buffer;

        screen_set_control_mode_header(g_control_header); 

        if (!item) return;
    }

    if (event == MENU_EV_ENTER)
        g_control_header = !g_control_header;
    else if (event == MENU_EV_UP)
        g_control_header = 1;
    else if (event == MENU_EV_DOWN)
        g_control_header = 0;
    else if (event == MENU_EV_NONE)
    {
        //only display value
        item->data.value = g_control_header;
        item->data.min = 0;
        item->data.max = 1;
    }

    if (item->data.value != g_control_header)
    {
        screen_set_control_mode_header(g_control_header);

        //also write to EEPROM
        uint8_t write_buffer = g_control_header;
        EEPROM_Write(0, CONTROL_HEADER_ADRESS, &write_buffer, MODE_8_BIT, 1);

        item->data.value = g_control_header;
    }

    if (g_control_header)
        item->data.unit_text = "Snapshot name";
    else
        item->data.unit_text = "Pedalboard name";

    item->data.step = 1;
}

//Callbacks below are not part of the device menu
void system_tuner_mute_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_NONE)
    {
        item->data.value = g_tuner_mute;
    }
    else if (event == MENU_EV_ENTER)
    {
        if (item->data.value == 0) item->data.value = 1;
        else item->data.value = 0;

        g_tuner_mute = item->data.value;

        set_menu_item_value(MENU_ID_TUNER_MUTE, g_tuner_mute);
    }
}

void system_tuner_input_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_NONE)
    {
        item->data.value = g_tuner_mute;
    }
    else if (event == MENU_EV_ENTER)
    {
        if (item->data.value == 0) item->data.value = 1;
        else item->data.value = 0;

        g_tuner_input = item->data.value;

        ui_comm_webgui_set_response_cb(NULL, NULL);

        char buffer[40];
        memset(buffer, 0, 20);
        uint8_t i;

        i = copy_command(buffer, CMD_TUNER_INPUT); 

        //insert the input
        i += int_to_str(g_tuner_input + 1, &buffer[i], sizeof(buffer) - i, 0);

        // sends the data to GUI
        ui_comm_webgui_send(buffer, i);

        // waits the pedalboards list be received
        ui_comm_webgui_wait_response();
    }
}

void system_play_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_NONE)
    {
        item->data.value = g_play_status;
    }
    else if (event == MENU_EV_ENTER)
    {
        if (item->data.value == 0) item->data.value = 1;
        else item->data.value = 0;

        g_play_status = item->data.value;

        set_menu_item_value(MENU_ID_PLAY_STATUS, g_play_status);
    }
}

void system_tempo_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if ((event == MENU_EV_NONE) || (event == MENU_EV_ENTER))
    {
        item->data.value = g_beats_per_minute;
        item->data.min = 20;
        item->data.max = 280;
    }
    //scrolling up/down
    else if ((event == MENU_EV_UP) && (g_MIDI_clk_src != 1))
    {
        item->data.value += item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;

        g_beats_per_minute = item->data.value;

        //let mod-ui know
        set_menu_item_value(MENU_ID_TEMPO, g_beats_per_minute);
    }
    else if ((event == MENU_EV_DOWN) && (g_MIDI_clk_src != 1))
    {
        item->data.value -= item->data.step;

        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        g_beats_per_minute = item->data.value;

        //let mod-ui know
        set_menu_item_value(MENU_ID_TEMPO, g_beats_per_minute);
    }

    static char str_bfr[12] = {};
    int_to_str(g_beats_per_minute, str_bfr, 4, 0);
    strcat(str_bfr, " BPM");
    item->data.unit_text = str_bfr;

    item->data.step = 1;
}

void system_bpb_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if ((event == MENU_EV_NONE) || (event == MENU_EV_ENTER))
    {
        item->data.value = g_beats_per_bar;
        item->data.min = 1;
        item->data.max = 16;
    }
    //scrolling up/down
    else if (event == MENU_EV_UP)
    {
        if (item->data.value < item->data.max) item->data.value++;
        g_beats_per_bar = item->data.value;

        //let mod-ui know
        set_menu_item_value(MENU_ID_BEATS_PER_BAR, g_beats_per_bar);
    }
    else if (event == MENU_EV_DOWN)
    {
        if (item->data.value > item->data.min) item->data.value--;
        g_beats_per_bar = item->data.value;

        //let mod-ui know
        set_menu_item_value(MENU_ID_BEATS_PER_BAR, g_beats_per_bar);
    }

    //add the items to the 
    static char str_bfr[8] = {};
    int_to_str(g_beats_per_bar, str_bfr, 4, 0);
    strcat(str_bfr, "/4");
    item->data.unit_text = str_bfr;

    item->data.step = 1;
}

void system_taptempo_cb (void *arg, int event)
{
    menu_item_t *item = arg;
    uint32_t now, delta;

    if (event == MENU_EV_NONE)
    {
        item->data.value = g_beats_per_minute;
        item->data.min = 20;
        item->data.max = 280;

        // calculates the maximum tap tempo value
        uint32_t max = (uint32_t)(convert_to_ms("bpm", item->data.min) + 0.5);

        //makes sure we enforce a proper timeout
        if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
            max = TAP_TEMPO_DEFAULT_TIMEOUT;

        g_tool_tap_tempo.max = max;
    }
    else if ((event == MENU_EV_ENTER) && (g_MIDI_clk_src != 1))
    {
        now = actuator_get_click_time(hardware_actuators(FOOTSWITCH1));
        delta = now - g_tool_tap_tempo.time;
        g_tool_tap_tempo.time = now;

        // checks if delta almost suits maximum allowed value
        if ((delta > g_tool_tap_tempo.max) &&
            ((delta - TAP_TEMPO_MAXVAL_OVERFLOW) < g_tool_tap_tempo.max))
        {
            // sets delta to maxvalue if just slightly over, instead of doing nothing
            delta = g_tool_tap_tempo.max;
        }

        // checks the tap tempo timeout
        if (delta <= g_tool_tap_tempo.max)
        {
            //get current value of tap tempo in ms
            float currentTapVal = convert_to_ms("bpm", g_beats_per_minute);
            //check if it should be added to running average
            if (fabs(currentTapVal - delta) < TAP_TEMPO_TAP_HYSTERESIS)
            {
                // converts and update the tap tempo value
                item->data.value = (2*(item->data.value) + convert_from_ms("bpm", delta)) / 3;
            }
            else
            {
                // converts and update the tap tempo value
                item->data.value = convert_from_ms("bpm", delta);
            }
            // checks the values bounds
            if (item->data.value > item->data.max) item->data.value = item->data.max;
            if (item->data.value < item->data.min) item->data.value = item->data.min;

            // updates the foot
            g_beats_per_minute = item->data.value;
        }

        set_menu_item_value(MENU_ID_TEMPO, g_beats_per_minute);
    }
    
    static char str_bfr[12] = {};
    int_to_str(g_beats_per_minute, str_bfr, 4, 0);
    item->data.unit_text = str_bfr;

    item->data.step = 1;
}

/*
void system_bypass_cb (void *arg, int event)
{
    menu_item_t *item = arg; 

    //0=in1, 1=in2, 2=in1&2
    switch (item->desc->id)
    {
        //in 1
        case BP1_ID:
            //we need to toggle the bypass
            if (event == MENU_EV_ENTER)
            {
                //we toggle the bypass 
                g_bypass[0] = !g_bypass[0];
                set_menu_item_value(MENU_ID_BYPASS1, g_bypass[0]);
            }
            add_chars_to_menu_name(item, (g_bypass[0]? option_enabled : option_disabled));
        break;

        //in2
        case BP2_ID:
            //we need to toggle the bypass
            if (event == MENU_EV_ENTER)
            {
                //we toggle the bypass 
                g_bypass[1] = !g_bypass[1];
                set_menu_item_value(MENU_ID_BYPASS2, g_bypass[1]);
            }
            add_chars_to_menu_name(item, (g_bypass[1]? option_enabled : option_disabled));
        break;

        case BP12_ID:
            if (event == MENU_EV_ENTER)
            {
                //toggle the bypasses
                g_bypass[2] = !g_bypass[2];
                set_menu_item_value(MENU_ID_BYPASS1, g_bypass[2]);
                set_menu_item_value(MENU_ID_BYPASS2, g_bypass[2]);
                g_bypass[0] = g_bypass[2];
                g_bypass[1] = g_bypass[2];
            }
            add_chars_to_menu_name(item, (g_bypass[2]? option_enabled : option_disabled));
        break;
    }

    //if both are on after a change we need to change bypass 1&2 as well
    if (g_bypass[0] && g_bypass[1])
    {
        g_bypass[2] = 1;
    }
    else g_bypass[2] = 0;

    //other items can change because of this, update the whole menu on the right, and left because of the quick bypass
    if (event == MENU_EV_ENTER)
    {
        TM_menu_refresh(DISPLAY_LEFT);
        TM_menu_refresh(DISPLAY_RIGHT);
    }
}*/

void system_usb_mode_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    //first time, fetch value
    if (g_usb_mode == -1)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);

        sys_comm_send(CMD_SYS_USB_MODE, NULL);
        sys_comm_wait_response();

        g_usb_mode = item->data.value;
        item->data.selected = g_usb_mode;
        item->data.min = 0;
        item->data.max = 2;
        item->data.list_count = 2;
        item->data.hover = 1;
    }

    //if clicked and YES was selected from the pop-up
    if ((event == MENU_EV_ENTER) && (item->data.hover == 0))
    {
        g_usb_mode = item->data.value;
        item->data.selected = g_usb_mode;

        //set the value
        char str_buf[8];
        int_to_str(item->data.value, str_buf, 4, 0);
        str_buf[1] = 0;

        //send value
        sys_comm_send(CMD_SYS_USB_MODE, str_buf);
        sys_comm_wait_response();

        //tell the system to reboot
        sys_comm_send(CMD_SYS_REBOOT, NULL);
    }
    //cancel, reset widget
    else if ((event == MENU_EV_ENTER) && (item->data.hover == 1)) {
        item->data.selected = g_usb_mode;
        item->data.value = g_usb_mode;
    }

    if (item->data.popup_active)
        return;

    if (event == MENU_EV_NONE)
        item->data.value = g_usb_mode;

    if ((event == MENU_EV_UP) && (item->data.value < item->data.max))
        item->data.value++;
    else if ((event == MENU_EV_DOWN) && (item->data.value > item->data.min))
        item->data.value--;

    switch ((int)item->data.value)
    {
        case 0: item->data.unit_text = "NETWORK (DEFAULT)"; break;
        case 1: item->data.unit_text = "NET+MIDI"; break;
        case 2: item->data.unit_text = "NET+MIDI (WINDOWS)"; break;
    }

    item->data.step = 1;
}

void system_shift_mode_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (g_shift_mode == -1)
    {
        //read EEPROM
        uint8_t read_buffer = 0;
        EEPROM_Read(0, SHIFT_MODE_ADRESS, &read_buffer, MODE_8_BIT, 1);

        g_shift_mode = read_buffer;

        naveg_set_shift_mode(g_shift_mode);

        if (!item) return;
    }

    if (event == MENU_EV_ENTER)
    {
        if (g_shift_mode < FOOT_TOOL_AMOUNT-1) g_shift_mode++;
        else g_shift_mode = 0;
    }
    else if (event == MENU_EV_UP)
    {
        if (g_shift_mode < FOOT_TOOL_AMOUNT-1)
            g_shift_mode += item->data.step;
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_shift_mode > 0)
            g_shift_mode -= item->data.step;
    }
    else if (event == MENU_EV_NONE)
    {
        //only display value
        item->data.value = g_shift_mode;
        item->data.min = 0;
        item->data.max = 1;
    }

    if (event != MENU_EV_NONE)
    {
        naveg_set_shift_mode(g_shift_mode);

        //also write to EEPROM
        uint8_t write_buffer = g_shift_mode;
        EEPROM_Write(0, SHIFT_MODE_ADRESS, &write_buffer, MODE_8_BIT, 1);

        item->data.value = g_shift_mode;
    }
    
    switch ((int)item->data.value)
    {
        case 0: item->data.unit_text = "Momentary"; break;
        case 1: item->data.unit_text = "Latching"; break;
    }


    item->data.step = 1;
}

void system_noisegate_channel_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q = 0;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);
        
        sys_comm_send(CMD_SYS_NG_CHANNEL, NULL);
        sys_comm_wait_response();

        item->data.min = 0.0f;
        item->data.max = 3.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        // insert the value on buffer
        q += int_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 0);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_NG_CHANNEL, val_buffer);
        sys_comm_wait_response();
    }

    switch ((int)item->data.value)
    {
        case 0: item->data.unit_text = "None"; break;
        case 1: item->data.unit_text = "Input 1"; break;
        case 2: item->data.unit_text = "Input 2"; break;
        case 3: item->data.unit_text = "Stereo"; break;
    }

    item->data.step = 1.0f;
}

void system_noisegate_thres_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q = 0;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);
        
        sys_comm_send(CMD_SYS_NG_THRESHOLD, NULL);
        sys_comm_wait_response();

        item->data.min = -80.0f;
        item->data.max = -10.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        // insert the value on buffer
        q += float_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 2);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_NG_THRESHOLD, val_buffer);
        sys_comm_wait_response();
    }

    static char str_bfr[20] = {};
    float_to_str(item->data.value, str_bfr, sizeof(str_bfr), 2);
    strcat(str_bfr, " dB");
    item->data.unit_text = str_bfr;

    item->data.step = 0.5f;
}

void system_noisegate_decay_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q = 0;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);
        
        sys_comm_send(CMD_SYS_NG_DECAY, NULL);
        sys_comm_wait_response();

        item->data.min = 1.0f;
        item->data.max = 500.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        // insert the value on buffer
        q += int_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 0);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_NG_DECAY, val_buffer);
        sys_comm_wait_response();
    }

    static char str_bfr[10] = {};
    int_to_str((int)item->data.value, str_bfr, sizeof(str_bfr), 0);
    strcat(str_bfr, " ms");
    item->data.unit_text = str_bfr;

    item->data.step = 2.0f;
}

void system_comp_mode_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q = 0;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);
        
        sys_comm_send(CMD_SYS_COMP_MODE, NULL);
        sys_comm_wait_response();

        item->data.min = 0.0f;
        item->data.max = 3.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        // insert the value on buffer
        q += int_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 0);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_COMP_MODE, val_buffer);
        sys_comm_wait_response();
    }

    switch ((int)item->data.value)
    {
        case 0: item->data.unit_text = "Off"; break;
        case 1: item->data.unit_text = "Light Comp"; break;
        case 2: item->data.unit_text = "Mild Comp"; break;
        case 3: item->data.unit_text = "heavy Comp"; break;
    }

    item->data.step = 1.0f;
}

void system_comp_release_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q = 0;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);
        
        sys_comm_send(CMD_SYS_COMP_RELEASE, NULL);
        sys_comm_wait_response();

        item->data.min = 50.0f;
        item->data.max = 500.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        // insert the value on buffer
        q += int_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 0);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_COMP_RELEASE, val_buffer);
        sys_comm_wait_response();
    }

    static char str_bfr[10] = {};
    int_to_str((int)item->data.value, str_bfr, sizeof(str_bfr), 0);
    strcat(str_bfr, " ms");
    item->data.unit_text = str_bfr;

    item->data.step = 2.0f;
}

void system_comp_pb_vol_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    char val_buffer[20];
    uint8_t q = 0;

    if (event == MENU_EV_NONE)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);
        
        sys_comm_send(CMD_SYS_COMP_PEDALBOARD_GAIN, NULL);
        sys_comm_wait_response();

        item->data.min = -30.0f;
        item->data.max = 20.0f;
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (event == MENU_EV_UP)
            item->data.value += item->data.step;
        else
            item->data.value -= item->data.step;

        if (item->data.value > item->data.max)
            item->data.value = item->data.max;
        if (item->data.value < item->data.min)
            item->data.value = item->data.min;

        // insert the value on buffer
        q += float_to_str(item->data.value, &val_buffer[q], sizeof(val_buffer) - q, 2);
        val_buffer[q] = 0;

        sys_comm_send(CMD_SYS_COMP_PEDALBOARD_GAIN, val_buffer);
        sys_comm_wait_response();
    }

    static char str_bfr[10] = {};
    if (item->data.value != -30.0) {
        float_to_str(item->data.value, str_bfr, sizeof(str_bfr), 2);
        strcat(str_bfr, " dB");
    }
    else
        strcpy(str_bfr, "-INF DB");

    item->data.unit_text = str_bfr;

    item->data.step = 0.25f;
}

void system_noise_removal_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    //first time, fetch value
    if (g_noise_removal_mode == -1)
    {
        sys_comm_set_response_cb(recieve_sys_value, item);

        sys_comm_send(CMD_SYS_NOISE_REMOVAL, NULL);
        sys_comm_wait_response();

        g_noise_removal_mode = item->data.value;
        item->data.selected = g_noise_removal_mode;
        item->data.min = 0;
        item->data.max = 1;
        item->data.step = 1;
        item->data.list_count = 2;
        item->data.hover = 1;
    }

    //if clicked and YES was selected from the pop-up
    if ((event == MENU_EV_ENTER) && (item->data.hover == 0))
    {
        g_noise_removal_mode = item->data.value;
        item->data.selected = g_noise_removal_mode;

        //set the value
        char str_buf[8];
        int_to_str(item->data.value, str_buf, 4, 0);
        str_buf[1] = 0;

        //send value
        sys_comm_send(CMD_SYS_NOISE_REMOVAL, str_buf);
        sys_comm_wait_response();
    }
    //cancel, reset widget
    else if ((event == MENU_EV_ENTER) && (item->data.hover == 1)) {
        item->data.selected = g_noise_removal_mode;
        item->data.value = g_noise_removal_mode;
    }

    if (item->data.popup_active)
        return;

    if (event == MENU_EV_NONE)
        item->data.value = g_noise_removal_mode;

    if ((event == MENU_EV_UP) && (item->data.value < item->data.max))
        item->data.value++;
    else if ((event == MENU_EV_DOWN) && (item->data.value > item->data.min))
        item->data.value--;

    switch ((int)item->data.value)
    {
        case 0: item->data.unit_text = "OFF"; break;
        case 1: item->data.unit_text = "ON"; break;
    }
}