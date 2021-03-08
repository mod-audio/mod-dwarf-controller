
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <string.h>
#include "config.h"
#include "serial.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "mod-protocol.h"

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define SYSTEM_MAX_SEM_COUNT   5


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

static  void (*g_system_response_cb)(void *data, menu_item_t *item) = NULL;
static  menu_item_t *g_current_item;
static volatile uint8_t  g_system_blocked;
static volatile xSemaphoreHandle g_system_sem = NULL;
static  ringbuff_t *g_system_rx_rb;


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

static void system_rx_cb(serial_t *serial)
{
    uint8_t buffer[SERIAL_MAX_RX_BUFF_SIZE] = {};
    uint32_t size = serial_read(serial->uart_id, buffer, sizeof(buffer));
    if (size > 0)
    {
        ringbuff_write(g_system_rx_rb, buffer, size);
        // check end of message
        uint32_t i;
        for (i = 0; i < size; i++)
        {
            if (buffer[i] == 0)
            {
                portBASE_TYPE xHigherPriorityTaskWoken;
                xHigherPriorityTaskWoken = pdFALSE;
                xSemaphoreGiveFromISR(g_system_sem, &xHigherPriorityTaskWoken);
                portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
                break;
            }
        }
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void sys_comm_init(void)
{
    g_system_sem = xSemaphoreCreateCounting(SYSTEM_MAX_SEM_COUNT, 0);
    g_system_rx_rb = ringbuff_create(SYSTEM_COMM_RX_BUFF_SIZE);

    serial_set_callback(SYSTEM_SERIAL, system_rx_cb);
}

void sys_comm_send(const char *command, const char *arguments)
{
    char buffer[20];
    memset(buffer, 0, sizeof buffer);

    //copy command
    uint8_t i = copy_command(buffer, command); 

    if (arguments)
    {

        //add size as hex number
        char str_bfr[9] = {};
        i+=2;
        i += int_to_hex_str(strlen(arguments), str_bfr);
        strcat(buffer, str_bfr);

        buffer[i++] = ' ';

        //add arguments
        strcat(buffer, arguments);
    }
    else
    {
        buffer[_CMD_SYS_LENGTH] = '\0';
    }

    //calc total size
    uint32_t data_size = strlen(buffer);
    serial_send(SYSTEM_SERIAL, (const uint8_t*)buffer, data_size+1);
}

ringbuff_t* sys_comm_read(void)
{
    if (xSemaphoreTake(g_system_sem, portMAX_DELAY) == pdTRUE)
    {
        return g_system_rx_rb;
    }

    return NULL;
}

void sys_comm_set_response_cb(void (*resp_cb)(void *data, menu_item_t *item), menu_item_t *item)
{
    g_current_item = item;
    g_system_response_cb = resp_cb;
}

void sys_comm_response_cb(void *data)
{
    if (g_system_response_cb)
    {
        g_system_response_cb(data, g_current_item);
        g_system_response_cb = NULL;
    }

    g_system_blocked = 0;
}

void sys_comm_wait_response(void)
{
    g_system_blocked = 1;
    while (g_system_blocked);
}

//clear the ringbuffer
void sys_comm_clear(void)
{
    ringbuff_flush(g_system_rx_rb);
}
