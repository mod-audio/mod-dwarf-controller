
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "cli.h"
#include "config.h"
#include "serial.h"
#include "utils.h"
#include "screen.h"
#include "glcd_widget.h"
#include "hardware.h"
#include "actuator.h"
#include "naveg.h"
#include "images.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "mode_control.h"

#include <string.h>


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define NEW_LINE            "\r\n"

#define MOD_USER            "hmi"
#define MOD_PASSWORD        "mod"
#define DISABLE_ECHO        "stty -echo"
#define SET_SP1_VAR         "export PS1=\"\""
#define RESTORE_HOSTNAME    "mod-restore"

#define PEEK_SIZE           3
#define LINE_BUFFER_SIZE    60
#define RESPONSE_TIMEOUT    (CLI_RESPONSE_TIMEOUT / portTICK_RATE_MS)
#define BOOT_TIMEOUT        (3000 / portTICK_RATE_MS)


/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

static char *g_boot_steps[] = {
    "U-Boot",
    "Hit any key",
    "Starting kernel",
    "login:",
    "Password:",
    "$"
};

enum {UBOOT_STARTING, UBOOT_HITKEY, KERNEL_STARTING, LOGIN, PASSWORD, SHELL_CONFIG, N_BOOT_STEPS};


/*
************************************************************************************************************************
*           LOCAL DATA TYPES
************************************************************************************************************************
*/

typedef struct CLI_T {
    char received[LINE_BUFFER_SIZE+1];
    char response[LINE_BUFFER_SIZE+1];
    uint8_t boot_step, pre_uboot;
    uint8_t waiting_response;
    uint8_t status, debug;
} cli_t;


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

static cli_t g_cli;
static xSemaphoreHandle g_received_sem, g_response_sem;


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

static void process_response(serial_t *serial)
{
    portBASE_TYPE xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    // try read data until find a new line
    uint32_t read;
    read = serial_read_until(CLI_SERIAL, (uint8_t *) g_cli.received, LINE_BUFFER_SIZE, '\n');
    if (read == 0)
    {
        // if can't find new line force reading
        read = serial_read(CLI_SERIAL, (uint8_t *) g_cli.received, LINE_BUFFER_SIZE);
    }

    // make message null termined
    g_cli.received[read] = 0;

    // remove new line from response
    if (g_cli.received[read-2] == '\r')
        g_cli.received[read-2] = 0;

    // all remaining data on buffer are not useful
    ringbuff_flush(serial->rx_buffer);

    xSemaphoreGiveFromISR(g_received_sem, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

// this callback is called from a ISR
static void serial_cb(serial_t *serial)
{
    portBASE_TYPE xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;

    // booting process
    if (g_cli.boot_step < N_BOOT_STEPS)
    {
        if (g_cli.waiting_response)
        {
            process_response(serial);
        }
        else
        {
            uint32_t len = strlen(g_boot_steps[g_cli.boot_step]);

            // check if logging in on regular system or restore
            if (g_cli.boot_step == LOGIN)
            {
                int32_t found =
                    ringbuff_search(serial->rx_buffer, (uint8_t *) RESTORE_HOSTNAME, (sizeof RESTORE_HOSTNAME) - 1);

                if (found >= 0)
                    g_cli.status = LOGGED_ON_RESTORE;
                else
                    g_cli.status = LOGGED_ON_SYSTEM;
            }

            // search for boot messages
            int32_t found =
                ringbuff_search2(serial->rx_buffer, (uint8_t *) g_boot_steps[g_cli.boot_step], len);

            // set pre uboot flag
            if (found < 0 && g_cli.boot_step == 0)
                g_cli.pre_uboot = 1;
            else
                g_cli.pre_uboot = 0;

            // doesn't need to retrieve data, so flush them out
            if (ringbuff_is_full(serial->rx_buffer))
                ringbuff_flush(serial->rx_buffer);

            // wake task if boot message match to pattern or match is not required (NULL)
            if (found >= 0 || len == 0)
            {
                xSemaphoreGiveFromISR(g_received_sem, &xHigherPriorityTaskWoken);
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
    else if (g_cli.waiting_response || g_cli.status == LOGGED_ON_RESTORE)
    {
        process_response(serial);
    }
    else
    {
        // check if received login message when it's not waiting for response nor in booting process
        uint32_t len = strlen(g_boot_steps[LOGIN]);
        int32_t found = ringbuff_search(serial->rx_buffer, (uint8_t *) g_boot_steps[LOGIN], len);
        if (found >= 0)
        {
            g_cli.boot_step = LOGIN;
            xSemaphoreGiveFromISR(g_received_sem, &xHigherPriorityTaskWoken);
            portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
        }

        ringbuff_flush(serial->rx_buffer);
    }
}

static void write_msg(const char *msg)
{
    glcd_clear(hardware_glcds(1), GLCD_WHITE);
    glcd_clear(hardware_glcds(0), GLCD_WHITE);
    textbox_t msg_box;
    msg_box.color = GLCD_BLACK;
    msg_box.mode = TEXT_MULTI_LINES;
    msg_box.align = ALIGN_LEFT_TOP;
    msg_box.top_margin = 0;
    msg_box.bottom_margin = 0;
    msg_box.left_margin = 0;
    msg_box.right_margin = 0;
    msg_box.height = DISPLAY_HEIGHT;
    msg_box.width = DISPLAY_WIDTH;
    msg_box.font = Terminal5x7;
    msg_box.text = msg;
    widget_textbox(hardware_glcds(0), &msg_box);
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void cli_init(void)
{
    vSemaphoreCreateBinary(g_received_sem);
    vSemaphoreCreateBinary(g_response_sem);
    serial_set_callback(CLI_SERIAL, serial_cb);

    // vSemaphoreCreateBinary is created as available which makes
    // first xSemaphoreTake pass even if semaphore has not been given
    // http://sourceforge.net/p/freertos/discussion/382005/thread/04bfabb9
    xSemaphoreTake(g_received_sem, 0);
    xSemaphoreTake(g_response_sem, 0);
}

void cli_process(void)
{
    static portTickType xTicksToWait = BOOT_TIMEOUT;
    portBASE_TYPE xReturn;

    xReturn = xSemaphoreTake(g_received_sem, xTicksToWait);

    // check if it's booting
    if (g_cli.boot_step < N_BOOT_STEPS)
    {
        // if got timeout ...
        if (xReturn == pdFALSE)
        {
            // and booting is in pre uboot stage
            if (g_cli.pre_uboot)
            {
                return;
            }

            // and still in the first step ...
            if (g_cli.boot_step == 0)
            {
                // force login step
                g_cli.boot_step = LOGIN;

                // send new line to force interrupt
                cli_command("echo", CLI_DISCARD_RESPONSE);
                return;
            }

            // and already tried login ...
            else if (g_cli.boot_step == LOGIN)
            {
                // assume boot is done
                g_cli.boot_step = SHELL_CONFIG;

                // send new line to force interrupt
                cli_command("echo", CLI_DISCARD_RESPONSE);
                return;
            }
        }

        switch (g_cli.boot_step)
        {
            case UBOOT_STARTING:
                xTicksToWait = portMAX_DELAY;
                break;

            case UBOOT_HITKEY:
                if (g_cli.status == LOGGED_ON_RESTORE)
                {
                    // stop auto boot and load variables
                    cli_command(NULL, CLI_RETRIEVE_RESPONSE);
                    cli_command("run loadbootenv", CLI_RETRIEVE_RESPONSE);

                    // set debug mode if required
                    if (g_cli.debug)
                        cli_command("setenv extraargs mod_restore=debug", CLI_RETRIEVE_RESPONSE);

                    // boot restore
                    cli_command("run boot_restore", CLI_RETRIEVE_RESPONSE);
                }
                break;

            case LOGIN:
                cli_command(MOD_USER, CLI_DISCARD_RESPONSE);
                break;

            case PASSWORD:
                cli_command(MOD_PASSWORD, CLI_DISCARD_RESPONSE);
                break;

            case SHELL_CONFIG:
                xTicksToWait = portMAX_DELAY;
                cli_command(DISABLE_ECHO, CLI_RETRIEVE_RESPONSE);
                cli_command(SET_SP1_VAR, CLI_RETRIEVE_RESPONSE);
                break;
        }

        g_cli.boot_step++;
    }

    // restore mode
    else if (g_cli.status == LOGGED_ON_RESTORE)
    {
        char *msg = &g_cli.received[4];
        if (strncmp(g_cli.received, "hmi:", 4) == 0)
        {
            write_msg(msg);
        }
    }

    // check if it's waiting command response
    else if (g_cli.waiting_response)
    {
        strcpy(g_cli.response, g_cli.received);
        xSemaphoreGive(g_response_sem);
    }
}

const char* cli_command(const char *command, uint8_t response_action)
{
    g_cli.waiting_response = (response_action == CLI_RETRIEVE_RESPONSE ? 1 : 0);

    // default response
    g_cli.response[0] = 0;

    // send command
    if (command)
    {
        serial_send(CLI_SERIAL, (uint8_t *) command, strlen(command));
        if (response_action == CLI_CACHE_ONLY)
            return NULL;
    }
    serial_send(CLI_SERIAL, (uint8_t *) NEW_LINE, 2);

    // take semaphore to wait for response
    if (response_action == CLI_RETRIEVE_RESPONSE)
    {
        if (xSemaphoreTake(g_response_sem, RESPONSE_TIMEOUT) == pdTRUE)
        {
            g_cli.waiting_response = 0;
            return g_cli.response;
        }
    }

    g_cli.waiting_response = 0;
    return NULL;
}

const char* cli_systemctl(const char *command, const char *service)
{
    if (!command || !service) return NULL;

    // build command
    serial_send(CLI_SERIAL, (uint8_t *) "systemctl ", 10);
    serial_send(CLI_SERIAL, (uint8_t *) command, strlen(command));
    serial_send(CLI_SERIAL, (uint8_t *) service, strlen(service));

    const char *response = cli_command(NULL, CLI_RETRIEVE_RESPONSE);
    // default response
    if (!response) strcpy(g_cli.response, "unknown");

    return g_cli.response;
}

void cli_package_version(const char *package_name)
{
    if (!package_name) return;
}

void cli_bluetooth(uint8_t what_info)
{
    // default response
    strcpy(g_cli.response, "unknown");

    switch (what_info)
    {
        case BLUETOOTH_NAME:
            break;

        case BLUETOOTH_ADDRESS:
            break;
    }
}

uint8_t cli_restore(uint8_t action)
{
    if (action == RESTORE_INIT)
    {
        // remove all controls
        uint8_t j = 0;
        for (j=0; j < TOTAL_CONTROL_ACTUATORS; j++)
        {
            CM_remove_control(j);
        }

        // disable system menu
        //naveg_toggle_tool(DISPLAY_TOOL_SYSTEM, DISPLAY_TOOL_SYSTEM);

        // clear screens
        screen_clear();

        // turn off leds
        uint8_t i;
        for (i = 0; i < LEDS_COUNT; i++)
            ledz_off(hardware_leds(i), LEDZ_ALL_COLORS);

        // force status to trigger restore after reboot
        g_cli.boot_step = 0;
        g_cli.debug = 0;
        g_cli.status = LOGGED_ON_RESTORE;
        cli_command("reboot", CLI_DISCARD_RESPONSE);
        write_msg("starting upgrade\nplease wait");
    }
    else if (action == RESTORE_CHECK_BOOT)
    {
        button_t *foot_restore = (button_t *) hardware_actuators(BUTTON0);
        encoder_t *knob_restore = (encoder_t *) hardware_actuators(ENCODER0);

        button_t *foot_maskrom = (button_t *) hardware_actuators(BUTTON2);
        encoder_t *knob_maskrom = (encoder_t *) hardware_actuators(ENCODER2);

        // check if first footswitch and first encoder is pressed down if so enter restore mode
        if (BUTTON_PRESSED(actuator_get_status(foot_restore)) &&
            BUTTON_PRESSED(actuator_get_status(knob_restore)))
        {
            // force entering on restore mode using debug
            g_cli.boot_step = 0;
            g_cli.debug = 1;
            g_cli.status = LOGGED_ON_RESTORE;
        }

        // check if first footswitch and first encoder is pressed down if so enter maskrom mode
        if (BUTTON_PRESSED(actuator_get_status(foot_maskrom)) &&
            BUTTON_PRESSED(actuator_get_status(knob_maskrom)))
        {
            // force entering on restore mode using debug
            g_cli.boot_step = 0;
            g_cli.debug = 1;
            g_cli.status = LOGGED_ON_MASKROM;
        }
    }

    return g_cli.status;
}
