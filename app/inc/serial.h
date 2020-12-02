
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef SERIAL_H
#define SERIAL_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "utils.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/

// defines how many serial_t instances will be created. This value is
// equal to number of UARTs that will be used in your application
#define SERIAL_MAX_INSTANCES    4

// defines FIFO trigger used to fire the interrupt
#define FIFO_TRIGGER            8

// output enable delay (in microseconds)
#define OUTPUT_ENABLE_DELAY     50

// output enable pin level
#define OUTPUT_ENABLE_ACTIVE_IN_HIGH


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

typedef struct SERIAL_T {
    uint8_t uart_id;
    uint32_t baud_rate;
    uint8_t priority, sof, eof;
    uint8_t rx_port, rx_pin, rx_function;
    uint8_t tx_port, tx_pin, tx_function;
    uint32_t rx_buffer_size;
    uint32_t tx_buffer_size;
    ringbuff_t *rx_buffer;
    ringbuff_t *tx_buffer;
    void (*rx_callback)(struct SERIAL_T *serial);

    // output enable
    uint8_t has_oe;
    uint8_t oe_port, oe_pin;
} serial_t;


/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           MACRO'S
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           FUNCTION PROTOTYPES
************************************************************************************************************************
*/

void serial_init(serial_t *serial);
void serial_enable_interupt(serial_t *serial);
uint32_t serial_send(uint8_t uart_id, const uint8_t *data, uint32_t data_size);
uint32_t serial_read(uint8_t uart_id, uint8_t *data, uint32_t data_size);
uint32_t serial_read_until(uint8_t uart_id, uint8_t *data, uint32_t data_size, uint8_t token);
void serial_set_callback(uint8_t uart_id, void (*receive_cb)(serial_t *serial));

// this function will be called automatically from UART interrupt in case of error
// the user must create this function in your application code
// the error_bits can be: UART_LSR_OE, UART_LSR_PE, UART_LSR_FE, UART_LSR_BI, UART_LSR_RXFE
extern void serial_error(uint8_t uart_id, uint32_t error_bits);


/*
************************************************************************************************************************
*           CONFIGURATION ERRORS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           END HEADER
************************************************************************************************************************
*/

#endif
