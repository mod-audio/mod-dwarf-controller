
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

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
#include "mode_tools.h"
#include <string.h>
#include <stdlib.h>

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

#define UNUSED_PARAM(var)   do { (void)(var); } while (0)
#define ROUND(x)    ((x) > 0.0 ? (((float)(x)) + 0.5) : (((float)(x)) - 0.5))
#define MAP(x, Omin, Omax, Nmin, Nmax)      ( x - Omin ) * (Nmax -  Nmin)  / (Omax - Omin) + Nmin;


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static uint8_t g_comm_protocol_bussy = 0;
float g_in_1_volume;
float g_in_2_volume;
float g_out_1_volume;
float g_out_2_volume;
float g_hp_volume;
uint8_t g_bypass[4] = {};
uint8_t g_current_profile = 1;
int8_t g_sl_out = -1;
int8_t g_sl_in = -1;
uint8_t g_snapshot_prog_change = 0;
uint8_t g_pedalboard_prog_change = 0;
uint16_t g_beats_per_minute = 0;
uint8_t g_beats_per_bar = 0;
uint8_t g_MIDI_clk_send = 0;
uint8_t g_MIDI_clk_src = 0;
uint8_t g_play_status = 0;
uint8_t g_tuner_mute = 0;
int8_t g_display_brightness = -1;
int8_t g_display_contrast = -1;
int8_t g_actuator_hide = -1;
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

static void update_status(char *item_to_update, const char *response)
{
    if (!item_to_update) return;

    char *pstr = strstr(item_to_update, ":");
    if (pstr && response)
    {
        pstr++;
        *pstr++ = ' ';
        strcpy(pstr, response);
    }
}

static void set_menu_item_value(uint16_t menu_id, uint16_t value)
{
    if (g_comm_protocol_bussy) return;

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

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/
void system_recall_stereo_link_settings(void)
{
    //read EEPROM
    uint8_t read_buffer = 0;

    EEPROM_Read(0, SL_INPUT_ADRESS, &read_buffer, MODE_8_BIT, 1);
    g_sl_in = read_buffer;   

    EEPROM_Read(0, SL_OUTPUT_ADRESS, &read_buffer, MODE_8_BIT, 1);
    g_sl_out = read_buffer; 
}

void system_lock_comm_serial(bool busy)
{
    g_comm_protocol_bussy = busy;
}

uint8_t system_get_current_profile(void)
{
    return g_current_profile;
}

//I SHOULD NOT BE HERE
void system_save_gains_cb(void *arg, int event)
{
    UNUSED_PARAM(arg);

    if (event == MENU_EV_ENTER)
    {
        cli_command("mod-amixer save", CLI_DISCARD_RESPONSE);
    }
}

void system_update_menu_value(uint8_t item_ID, uint16_t value)
{
    switch(item_ID)
    {
        //play status
        case MENU_ID_PLAY_STATUS:
            g_play_status = value;
        break;
        //global tempo
        case MENU_ID_TEMPO:
            g_beats_per_minute = value;
        break;
        //global tempo status
        case MENU_ID_BEATS_PER_BAR:
            g_beats_per_bar = value;
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
        //sl input
        case MENU_ID_SL_IN: 
            g_sl_in = value;
        break;
        //stereo link output
        case MENU_ID_SL_OUT: 
            g_sl_out = value;
        break;
        //MIDI clock source
        case MENU_ID_MIDI_CLK_SOURCE: 
            g_MIDI_clk_src = value;
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

float system_get_gain_value(uint8_t item_ID)
{
    switch(item_ID)
    {
        case INP_1_GAIN_ID: 
            return g_in_1_volume;
        break;
        case INP_2_GAIN_ID: 
            return g_in_2_volume;
        break;
        case OUTP_1_GAIN_ID: 
            return g_out_1_volume;
        break;
        case OUTP_2_GAIN_ID: 
            return g_in_2_volume;
        break;
        case HEADPHONE_VOLUME_ID: 
            return g_hp_volume;

        break;
        default:
            return 0;
        break;
        
    }
}

void system_bluetooth_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        char resp[LINE_BUFFER_SIZE];
        if (item->desc->id == BLUETOOTH_ID)
        {
            response = cli_command("mod-bluetooth hmi", CLI_RETRIEVE_RESPONSE);
            
            strncpy(resp, response, sizeof(resp)-1);
            char **items = strarr_split(resp, '|');;

            if (items)
            {
                update_status(item->data.list[2], items[0]);
                update_status(item->data.list[3], items[1]);
                update_status(item->data.list[4], items[2]);

                FREE(items);
            } 
        }
        else if (item->desc->id == BLUETOOTH_ID+1)
        {
            cli_command("mod-bluetooth discovery", CLI_DISCARD_RESPONSE);
        }
    }
}

void system_services_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        uint8_t i = 0;
        while (systemctl_services[i])
        {
            const char *response;
            response = cli_systemctl("is-active ", systemctl_services[i]);
            update_status(item->data.list[i+1], response);
            i++;
        }
    }
}

void system_versions_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        char version[8];

        uint8_t i = 0;
        while (versions_names[i])
        {
            cli_command("mod-version ", CLI_CACHE_ONLY);
            response = cli_command(versions_names[i], CLI_RETRIEVE_RESPONSE);
            strncpy(version, response, (sizeof version) - 1);
            version[(sizeof version) - 1] = 0;
            update_status(item->data.list[i+1], version);
            screen_system_menu(item);
            i++;
        }
    }
}

void system_release_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        response = cli_command("mod-version release", CLI_RETRIEVE_RESPONSE);
        item->data.popup_content = response;
    }
}


void system_device_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        response = cli_command("cat /var/cache/mod/tag", CLI_RETRIEVE_RESPONSE);
        update_status(item->data.list[1], response);
    }
}

void system_tag_cb(void *arg, int event)
{

    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        const char *response;
        char *txt = "The serial number of your     device is:                    ";
        response =  cli_command("cat /var/cache/mod/tag", CLI_RETRIEVE_RESPONSE);
        char * bfr = (char *) MALLOC(1 + strlen(txt)+ strlen(response));
        strcpy(bfr, txt);
        strcat(bfr, response);
        item->data.popup_content = bfr;
        item->data.popup_header = "serial number";
    }
}

void system_upgrade_cb(void *arg, int event)
{
    if (event == MENU_EV_ENTER)
    {
        menu_item_t *item = arg;
        button_t *foot = (button_t *) hardware_actuators(FOOTSWITCH0);

        // check if YES option was chosen
        if (item->data.hover == 0)
        {
            uint8_t status = actuator_get_status(foot);

            // check if footswitch is pressed down
            if (BUTTON_PRESSED(status))
            {
                //clear all screens
                screen_clear();

                // start restore
                cli_restore(RESTORE_INIT);
            }
        }
    }
}

void system_inp_1_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    static const char *response = NULL;
    cli_command(NULL, CLI_DISCARD_RESPONSE);
    char value[8] = {};
    static uint32_t last_message_time = 0; 
    uint32_t message_time = hardware_timestamp();

    if ((event == MENU_EV_ENTER) || (event == MENU_EV_NONE))
    {
        response = cli_command("mod-amixer in 1 xvol", CLI_RETRIEVE_RESPONSE);
        char str[LINE_BUFFER_SIZE+1];
        strcpy(str, response);

        item->data.min = 0.0;
        item->data.max = 98.0;
        item->data.step = 1.0;

        item->data.value = atoi(str);
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (message_time - last_message_time > VOL_MESSAGE_TIMEOUT)
        {
            if (event == MENU_EV_UP) item->data.value += item->data.step;
            else item->data.value -= item->data.step;

            if (item->data.value > item->data.max) item->data.value = item->data.max;
            if (item->data.value < item->data.min) item->data.value = item->data.min;

            int_to_str(item->data.value, value, 8, 0);

            if (g_sl_in)
            {
                cli_command("mod-amixer in 0 xvol", CLI_CACHE_ONLY);
                cli_command(value, CLI_DISCARD_RESPONSE);
                g_in_1_volume = item->data.value;
                g_in_2_volume = item->data.value;
            }
            else if (item->desc->id == INP_1_GAIN_ID)
            {
                cli_command("mod-amixer in 1 xvol", CLI_CACHE_ONLY);
                cli_command(value, CLI_DISCARD_RESPONSE);
                g_in_1_volume = item->data.value;
            }
            
            last_message_time = message_time;
        }
    }

    static char str_bfr[8] = {};
    float value_bfr = 0;
    value_bfr = MAP(item->data.value, item->data.min, item->data.max, 0, 100); 
    int_to_str(value_bfr, str_bfr, 8, 0);
    strcat(str_bfr, "%");
    item->data.unit_text = str_bfr;

    if (event != MENU_EV_NONE)
    {
        if (naveg_get_current_mode() == MODE_TOOL)
            TM_print_tool();
        else if (naveg_get_current_mode() == MODE_SHIFT)
            screen_shift_overlay(0);
    }   
}

void system_inp_2_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    static const char *response = NULL;
    cli_command(NULL, CLI_DISCARD_RESPONSE);
    char value[8] = {};
    static uint32_t last_message_time = 0; 
    uint32_t message_time = hardware_timestamp();

    if ((event == MENU_EV_ENTER) || (event == MENU_EV_NONE))
    {
        ledz_on(hardware_leds(3), BLUE);
        
        response = cli_command("mod-amixer in 2 xvol", CLI_RETRIEVE_RESPONSE);
        char str[LINE_BUFFER_SIZE+1];
        strcpy(str, response);
        ledz_on(hardware_leds(4), BLUE);

        item->data.min = 0.0;
        item->data.max = 98.0;
        item->data.step = 1.0;
        ledz_on(hardware_leds(5), BLUE);
        item->data.value = atoi(str);
        ledz_on(hardware_leds(6), BLUE);
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        ledz_on(hardware_leds(3), RED);
        if (message_time - last_message_time > VOL_MESSAGE_TIMEOUT)
        {
            
            if (event == MENU_EV_UP) item->data.value += item->data.step;
            else item->data.value -= item->data.step;
            
            if (item->data.value > item->data.max) item->data.value = item->data.max;
            if (item->data.value < item->data.min) item->data.value = item->data.min;
            ledz_on(hardware_leds(4), RED);
            int_to_str(item->data.value, value, 8, 0);

            if (g_sl_in)
            {
                cli_command("mod-amixer in 0 xvol", CLI_CACHE_ONLY);
                cli_command(value, CLI_DISCARD_RESPONSE);
                g_in_1_volume = item->data.value;
                g_in_2_volume = item->data.value;
            }
            else if (item->desc->id == INP_1_GAIN_ID)
            {
                cli_command("mod-amixer in 2 xvol", CLI_CACHE_ONLY);
                cli_command(value, CLI_DISCARD_RESPONSE);
                g_in_2_volume = item->data.value;
            }
            ledz_on(hardware_leds(5), RED);
            last_message_time = message_time;
        }
    }

    static char str_bfr[8] = {};
    float value_bfr = 0;
    value_bfr = MAP(item->data.value, item->data.min, item->data.max, 0, 100); 
    ledz_on(hardware_leds(3), GREEN);
    int_to_str(value_bfr, str_bfr, 8, 0);
    strcat(str_bfr, "%");
    ledz_on(hardware_leds(4), GREEN);
    item->data.unit_text = str_bfr;

    if (event != MENU_EV_NONE)
    {
        ledz_on(hardware_leds(5), GREEN);
        if (naveg_get_current_mode() == MODE_TOOL)
            TM_print_tool();
        else if (naveg_get_current_mode() == MODE_SHIFT)
            screen_shift_overlay(0);
    }   
}

void system_outp_1_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    static const char *response = NULL;
    cli_command(NULL, CLI_DISCARD_RESPONSE);
    char value[8] = {};
    static uint32_t last_message_time = 0; 
    uint32_t message_time = hardware_timestamp();

    if ((event == MENU_EV_ENTER) || (event == MENU_EV_NONE))
    {
        response =  cli_command("mod-amixer out 1 xvol", CLI_RETRIEVE_RESPONSE);
        char str[LINE_BUFFER_SIZE+1];
        strcpy(str, response);

        item->data.min = -60.0;
        item->data.max = 0.0;
        item->data.step = 2.0;

        item->data.value = atoi(str);
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (message_time - last_message_time > VOL_MESSAGE_TIMEOUT)
        {
            if (event == MENU_EV_UP) item->data.value += item->data.step;
            else item->data.value -= item->data.step;

            if (item->data.value > item->data.max) item->data.value = item->data.max;
            if (item->data.value < item->data.min) item->data.value = item->data.min;

            int_to_str(item->data.value, value, 8, 0);

            if (g_sl_out)
            {
                cli_command("mod-amixer out 0 xvol", CLI_CACHE_ONLY);
                cli_command(value, CLI_DISCARD_RESPONSE);
                g_out_1_volume = item->data.value;
                g_out_2_volume = item->data.value;
            }
            else if (item->desc->id == OUTP_1_GAIN_ID)
            {
                cli_command("mod-amixer out 1 xvol", CLI_CACHE_ONLY);
                cli_command(value, CLI_DISCARD_RESPONSE);
                g_out_1_volume = item->data.value;
            }
            
            last_message_time = message_time;
        }
    }

    static char str_bfr[8] = {};
    float value_bfr = 0;
    value_bfr = MAP(item->data.value, item->data.min, item->data.max, 0, 100); 
    int_to_str(value_bfr, str_bfr, 8, 0);
    strcat(str_bfr, "%");
    item->data.unit_text = str_bfr;

    if (event != MENU_EV_NONE)
    {
        if (naveg_get_current_mode() == MODE_TOOL)
            TM_print_tool();
        else if (naveg_get_current_mode() == MODE_SHIFT)
            screen_shift_overlay(0);
    } 
}

void system_outp_2_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    static const char *response = NULL;
    cli_command(NULL, CLI_DISCARD_RESPONSE);
    char value[8] = {};
    static uint32_t last_message_time = 0; 
    uint32_t message_time = hardware_timestamp();

    if ((event == MENU_EV_ENTER) || (event == MENU_EV_NONE))
    {
        response = li_command("mod-amixer out 2 xvol", CLI_RETRIEVE_RESPONSE);
        char str[LINE_BUFFER_SIZE+1];
        strcpy(str, response);

        item->data.min = -60.0;
        item->data.max = 0.0;
        item->data.step = 2.0;

        item->data.value = atoi(str);
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (message_time - last_message_time > VOL_MESSAGE_TIMEOUT)
        {
            if (event == MENU_EV_UP) item->data.value += item->data.step;
            else item->data.value -= item->data.step;

            if (item->data.value > item->data.max) item->data.value = item->data.max;
            if (item->data.value < item->data.min) item->data.value = item->data.min;

            int_to_str(item->data.value, value, 8, 0);

            if (g_sl_out)
            {
                cli_command("mod-amixer out 0 xvol", CLI_CACHE_ONLY);
                cli_command(value, CLI_DISCARD_RESPONSE);
                g_out_1_volume = item->data.value;
                g_out_2_volume = item->data.value;
            }
            else if (item->desc->id == OUTP_1_GAIN_ID)
            {
                cli_command("mod-amixer out 2 xvol", CLI_CACHE_ONLY);
                cli_command(value, CLI_DISCARD_RESPONSE);
                g_out_2_volume = item->data.value;
            }
            
            last_message_time = message_time;
        }
    }

    static char str_bfr[8] = {};
    float value_bfr = 0;
    value_bfr = MAP(item->data.value, item->data.min, item->data.max, 0, 100); 
    int_to_str(value_bfr, str_bfr, 8, 0);
    strcat(str_bfr, "%");
    item->data.unit_text = str_bfr;

    if (event != MENU_EV_NONE)
    {
        if (naveg_get_current_mode() == MODE_TOOL)
            TM_print_tool();
        else if (naveg_get_current_mode() == MODE_SHIFT)
            screen_shift_overlay(0);
    } 
}

void system_hp_volume_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    static const char *response = NULL;
    cli_command(NULL, CLI_DISCARD_RESPONSE);
    char value[8] = {};
    static uint32_t last_message_time = 0; 
    uint32_t message_time = hardware_timestamp();

    if ((event == MENU_EV_ENTER) || (event == MENU_EV_NONE))
    {
        response = ccli_command("mod-amixer hp xvol", CLI_RETRIEVE_RESPONSE);
        char str[LINE_BUFFER_SIZE+1];
        strcpy(str, response);

        item->data.min = -33.0;
        item->data.max = 12.0;
        item->data.step = 3.0;

        item->data.value = atoi(str);
    }
    else if ((event == MENU_EV_UP) ||(event == MENU_EV_DOWN))
    {
        if (message_time - last_message_time > VOL_MESSAGE_TIMEOUT)
        {
            if (event == MENU_EV_UP) item->data.value += item->data.step;
            else item->data.value -= item->data.step;

            if (item->data.value > item->data.max) item->data.value = item->data.max;
            if (item->data.value < item->data.min) item->data.value = item->data.min;

            int_to_str(item->data.value, value, 8, 0);

            cli_command("mod-amixer hp xvol", CLI_CACHE_ONLY);
            cli_command(value, CLI_DISCARD_RESPONSE);
            g_hp_volume = item->data.value;
            last_message_time = message_time;
        }
    }

    static char str_bfr[8] = {};
    float value_bfr = 0;
    value_bfr = MAP(item->data.value, item->data.min, item->data.max, 0, 100); 
    int_to_str(value_bfr, str_bfr, 8, 0);
    strcat(str_bfr, "%");
    item->data.unit_text = str_bfr;

    if (event != MENU_EV_NONE)
    {
        if (naveg_get_current_mode() == MODE_TOOL)
            TM_print_tool();
        else if (naveg_get_current_mode() == MODE_SHIFT)
            screen_shift_overlay(0);
    } 
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
        item->data.step = 1;
    }

    hardware_glcd_brightness(g_display_brightness); 

    //also write to EEPROM
    uint8_t write_buffer = g_display_brightness;
    EEPROM_Write(0, DISPLAY_BRIGHTNESS_ADRESS, &write_buffer, MODE_8_BIT, 1);

    int_to_str((g_display_brightness * 25), str_bfr, 4, 0);

    item->data.value = g_display_brightness;

    strcat(str_bfr, "%");
    item->data.unit_text = str_bfr;

    if (event != MENU_EV_NONE)
        TM_print_tool();
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

    if (event == MENU_EV_ENTER)
    {
        if (g_display_contrast < item->data.max) g_display_contrast++;
        else g_display_contrast = 0;
    }
    else if (event == MENU_EV_UP)
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
        item->data.step = 1;
    }

    st7565p_set_contrast(hardware_glcds(0), g_display_contrast); 
        
    //also write to EEPROM
    uint8_t write_buffer = g_display_contrast;
    EEPROM_Write(0, DISPLAY_CONTRAST_ADRESS, &write_buffer, MODE_8_BIT, 1);

    int_to_str((g_display_contrast), str_bfr, 4, 0);

    item->data.value = g_display_contrast;

    strcat(str_bfr, "%");
    item->data.unit_text = str_bfr;

    if (event != MENU_EV_NONE)
        TM_print_tool();
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
        //screen_set_hide_non_assigned_actuators(g_actuator_hide);
    }

    if (event == MENU_EV_ENTER)
    {
        if (g_actuator_hide == 0) g_actuator_hide = 1;
        else g_actuator_hide = 0;
        
        //also write to EEPROM
        uint8_t write_buffer = g_actuator_hide;
        EEPROM_Write(0, HIDE_ACTUATOR_ADRESS, &write_buffer, MODE_8_BIT, 1);

        //write to screen.c
        //screen_set_hide_non_assigned_actuators(g_actuator_hide);
    }
    else if (event == MENU_EV_UP)
    {
        g_actuator_hide = 1;
        
        //also write to EEPROM
        uint8_t write_buffer = g_actuator_hide;
        EEPROM_Write(0, HIDE_ACTUATOR_ADRESS, &write_buffer, MODE_8_BIT, 1);

        //write to screen.c
        //screen_set_hide_non_assigned_actuators(g_actuator_hide);
    }
    else if (event == MENU_EV_DOWN)
    {
        g_actuator_hide = 0;
        
        //also write to EEPROM
        uint8_t write_buffer = g_actuator_hide;
        EEPROM_Write(0, HIDE_ACTUATOR_ADRESS, &write_buffer, MODE_8_BIT, 1);

        //write to screen.c
        //screen_set_hide_non_assigned_actuators(g_actuator_hide);
    }

    item->data.unit_text = g_actuator_hide ? "HIDE" : "SHOW";
    item->data.value = g_actuator_hide;
    
    if (event != MENU_EV_NONE)
        TM_print_tool();
}

void system_sl_in_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (g_sl_in == 0) g_sl_in = 1;
        else g_sl_in = 0;

        set_menu_item_value(MENU_ID_SL_IN, g_sl_in);
    }
    else if (event == MENU_EV_UP)
    {
        g_sl_in = 1;

        set_menu_item_value(MENU_ID_SL_IN, g_sl_in);
    }
    else if (event == MENU_EV_DOWN)
    {
        g_sl_in = 0;

        set_menu_item_value(MENU_ID_SL_IN, g_sl_in);
    }

    //if we toggled to 1, we need to change gain 2 to  gain 1
    char value_bfr[8] = {};
    if (g_sl_in == 1)
    {
        float_to_str(g_in_1_volume, value_bfr, 8, 1);
        cli_command("mod-amixer in 0 xvol ", CLI_CACHE_ONLY);
        cli_command(value_bfr, CLI_DISCARD_RESPONSE);
        //keep everything in sync
        g_in_2_volume = g_in_1_volume;

        system_save_gains_cb(NULL, MENU_EV_ENTER);
    }

    if (g_sl_in != -1)
    {
        uint8_t write_buffer = g_sl_in;
        EEPROM_Write(0, SL_INPUT_ADRESS, &write_buffer, MODE_8_BIT, 1);
    }

    item->data.value = g_sl_in;

    if (event != MENU_EV_NONE)
        TM_print_tool();
}

void system_sl_out_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (g_sl_out == 0) g_sl_out = 1; 
        else g_sl_out = 0;

        set_menu_item_value(MENU_ID_SL_OUT, g_sl_out);
    }
    if (event == MENU_EV_UP)
    {
        g_sl_out = 1; 
        
        set_menu_item_value(MENU_ID_SL_OUT, g_sl_out);
    }
    else if (event == MENU_EV_DOWN)
    {
        g_sl_out = 0;
        
        set_menu_item_value(MENU_ID_SL_OUT, g_sl_out);
    }

    //also set the gains to the same value
    char value_bfr[8] = {};
    if (g_sl_out == 1)
    {
        float_to_str(g_out_1_volume, value_bfr, 8, 1);
        cli_command("mod-amixer out 0 xvol ", CLI_CACHE_ONLY);
        cli_command(value_bfr, CLI_DISCARD_RESPONSE);
        //keep everything in sync
        g_out_2_volume = g_out_1_volume;

        system_save_gains_cb(NULL, MENU_EV_ENTER);
    }

    if (g_sl_out != -1)
    {
        uint8_t write_buffer = g_sl_out;
        EEPROM_Write(0, SL_OUTPUT_ADRESS, &write_buffer, MODE_8_BIT, 1);
    }

    item->data.value = g_sl_out;

    if (event != MENU_EV_NONE)
        TM_print_tool();
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
    char str_bfr[13] = {};
    if (g_MIDI_clk_src == 0) strcpy(str_bfr,"INTERNAL");
    else if (g_MIDI_clk_src == 1) strcpy(str_bfr,"MIDI");
    else if (g_MIDI_clk_src == 2) strcpy(str_bfr,"ABLETON LINK");

    item->data.unit_text = str_bfr;
    
    if (event != MENU_EV_NONE)
        TM_print_tool();
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
        set_menu_item_value(MENU_ID_MIDI_CLK_SOURCE, g_MIDI_clk_send);
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_MIDI_clk_send > 0) g_MIDI_clk_send--;
        set_menu_item_value(MENU_ID_MIDI_CLK_SOURCE, g_MIDI_clk_send);
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
        item->data.step = 1;
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

    char str_bfr[8] = {};
    int_to_str(g_snapshot_prog_change, str_bfr, 3, 0);
    //a value of 0 means we turn off
    if (g_snapshot_prog_change == 0) strcpy(str_bfr, "OFF");
    item->data.unit_text = str_bfr;
}

void system_pb_prog_change_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        set_menu_item_value(MENU_ID_SNAPSHOT_PRGCHGE, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        //set the item value to the snapshot_prog_change since mod-ui is master
        item->data.value = g_pedalboard_prog_change;
        item->data.min = 0;
        item->data.max = 16;
        item->data.step = 1;
    }
    else if (event == MENU_EV_UP)
    {
        if (g_pedalboard_prog_change < item->data.max) g_pedalboard_prog_change++;
        //let mod-ui know
        set_menu_item_value(MENU_ID_SNAPSHOT_PRGCHGE, g_pedalboard_prog_change);
    }
    else if (event == MENU_EV_DOWN)
    {
        if (g_pedalboard_prog_change > item->data.min) g_pedalboard_prog_change--;
        //let mod-ui know
        set_menu_item_value(MENU_ID_SNAPSHOT_PRGCHGE, g_pedalboard_prog_change);
    }

    char str_bfr[8] = {};
    int_to_str(g_pedalboard_prog_change, str_bfr, 3, 0);
    //a value of 0 means we turn off
    if (g_pedalboard_prog_change == 0) strcpy(str_bfr, "OFF");
    item->data.unit_text = str_bfr;
}

/*
//USER PROFILE X (loading)
void system_load_pro_cb(void *arg, int event)
{
    menu_item_t *item = arg;

    //if clicked and YES was selected from the pop-up
    if (event == MENU_EV_ENTER && item->data.hover == 0)
    {
        //current profile is the ID (A=1, B=2, C=3, D=4)
        g_current_profile = item->desc->id - item->desc->parent_id;
        item->data.value = 1;

        set_item_value(CMD_PROFILE_LOAD, g_current_profile);
    }

    else if (event == MENU_EV_NONE)
    {
        if ((item->desc->id - item->desc->parent_id) == g_current_profile)
        {
            add_chars_to_menu_name(item, option_enabled);
            item->data.value = 1;
        }
        //we dont want a [ ] behind every profile, so clear the name to just show the txt
        else
        { 
            strcpy(item->name, item->desc->name);
            item->data.value = 0;
        }
    }

    //we do not need tu update anything, a profile_update command will be run that handles that. 
}

//OVERWRITE CURRENT PROFILE
void system_save_pro_cb(void *arg, int event)
{
   menu_item_t *item = arg;

    //if clicked and YES was selected from the pop-up
    if (event == MENU_EV_ENTER && item->data.hover == 0)
    {
        set_item_value(CMD_PROFILE_STORE, g_current_profile);
        //since the current profile value cant change because of a menu enter here we do not need to update the name.
    }

    //if we are just entering the menu just add the current value to the menu item
    else if (event == MENU_EV_NONE)
    {
        char *profile_char = NULL;
        switch (g_current_profile)
        {
            case 1: profile_char = "[A]"; break;
            case 2: profile_char = "[B]"; break;
            case 3: profile_char = "[C]"; break;
            case 4: profile_char = "[D]"; break;
        }
        add_chars_to_menu_name(item, profile_char);
    }

    //we do not need to update, there is nothing that changes
}
*/

//Callbacks below are not part of the menu
void system_tuner_mute_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (g_tuner_mute == 0) g_tuner_mute= 1;
        else g_tuner_mute = 0;
        set_menu_item_value(MENU_ID_TUNER_MUTE, g_tuner_mute);
    }
    char str_bfr[15] = {};
    strcpy(str_bfr,"MUTE ");
    //strcat(str_bfr,(g_tuner_mute ? option_enabled : option_disabled));
    item->data.unit_text = str_bfr;
}

void system_play_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        if (g_play_status == 0) g_play_status = 1;
        else g_play_status = 0;
        set_menu_item_value(MENU_ID_PLAY_STATUS, g_play_status);
    }
    char str_bfr[15] = {};
    strcpy(str_bfr,"PLAY ");
    //strcat(str_bfr,(g_play_status ? option_enabled : option_disabled));
    item->data.unit_text = str_bfr;
}

void system_tempo_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        //we can only change tempo when its generated internaly 
        if (g_MIDI_clk_src != 1) set_menu_item_value(MENU_ID_TEMPO, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        //set the item value to the bpm since mod-ui is master
        item->data.value =  g_beats_per_minute;
        item->data.min = 20;
        item->data.max = 280;
        item->data.step = 1;
    }
    //scrolling up/down
    else 
    {
        //we can only change tempo when its generated internaly 
        if (g_MIDI_clk_src != 1)
        {
            //HMI changes the item, resync
            g_beats_per_minute = item->data.value;
            //let mod-ui know
            set_menu_item_value(MENU_ID_TEMPO, g_beats_per_minute);
        }
        else 
        {
            item->data.value = g_beats_per_minute;
        }
    }

    char str_bfr[8] = {};
    int_to_str(g_beats_per_minute, str_bfr, 4, 0);
    strcat(str_bfr, " BPM");
    item->data.unit_text = str_bfr;
}

void system_bpb_cb (void *arg, int event)
{
    menu_item_t *item = arg;

    if (event == MENU_EV_ENTER)
    {
        set_menu_item_value(MENU_ID_BEATS_PER_BAR, item->data.value);
    }
    else if (event == MENU_EV_NONE)
    {
        //set the item value to the bpb since mod-ui is master
        item->data.value =  g_beats_per_bar;
        item->data.min = 1;
        item->data.max = 16;
        item->data.step = 1;
    }
    //scrolling up/down
    else 
    {
        //HMI changes the item, resync
        g_beats_per_bar = item->data.value;
        //let mod-ui know
        set_menu_item_value(MENU_ID_BEATS_PER_BAR, g_beats_per_bar);
    }

    //add the items to the 
    char str_bfr[8] = {};
    int_to_str(g_beats_per_bar, str_bfr, 4, 0);
    strcat(str_bfr, "/4");
    item->data.unit_text = str_bfr;
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