
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef UTILS_H
#define UTILS_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "config.h"


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

// uncomment the define below to enable the quotation marks evaluation on strarr_split parser
#define ENABLE_QUOTATION_MARKS


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

typedef struct RINGBUFF_T {
    uint32_t head, tail;
    uint8_t *buffer;
    uint32_t size;
} ringbuff_t;


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
// copy an command to buffer
uint8_t copy_command(char *buffer, const char *command);
// splits the string in each whitespace occurrence and returns a array of strings NULL terminated
char** strarr_split(char *str, const char token);
// returns the string array length
uint32_t strarr_length(char** const str_array);
// joins a string array in a single string
char* strarr_join(char** const str_array);

// converts integer to string and returns the string length
uint32_t int_to_str(int32_t num, char *string, uint32_t string_size, uint8_t zero_leading);
// converts float to string  and returns the string length
uint32_t float_to_str(float num, char *string, uint32_t string_size, uint8_t precision);

// duplicate a string (alternative to strdup)
char *str_duplicate(const char *str);
// handy function to make a copy of a C-string array (char**)
char** str_array_duplicate(char** list, uint16_t count);

// delay functions
void delay_us(volatile uint32_t time);
void delay_ms(volatile uint32_t time);

// time convertion functions
// known units (it isn't case sensitive): bpm, hz, s, ms
float convert_to_ms(const char *unit_from, float value);
float convert_from_ms(const char *unit_to, float value);

// ring buffer functions
// ringbuff_create: allocates memory to ring buffer
ringbuff_t *ringbuff_create(uint32_t buffer_size);
// ringbuff_destroy: de-allocates memory of the ring buffer
void ringbuff_destroy(ringbuff_t *rb);
// ringbuff_write: returns number of bytes written
uint32_t ringbuff_write(ringbuff_t *rb, const uint8_t *data, uint32_t data_size);
// ringbuff_read: returns number of bytes read
uint32_t ringbuff_read(ringbuff_t *rb, uint8_t *buffer, uint32_t buffer_size);
// ringbuff_read_until: read ring buffer until find the token and returns number of bytes read
uint32_t ringbuff_read_until(ringbuff_t *rb, uint8_t *buffer, uint32_t buffer_size, uint8_t token);
// ringbuffer_used_space: returns amount of unread bytes
uint32_t ringbuffer_used_space(ringbuff_t *rb);
// ringbuff_available_space: returns amount of free bytes
uint32_t ringbuff_available_space(ringbuff_t *rb);
// ringbuff_is_full: returns non zero if buffer is full
uint32_t ringbuff_is_full(ringbuff_t *rb);
// ringbuff_is_empty: returns non zero if buffer is empty
uint32_t ringbuff_is_empty(ringbuff_t *rb);
// ringbuff_flush: reset buffer indexes (old data will be overwritten)
void ringbuff_flush(ringbuff_t *rb);
// ringbuff_count: returns amount of ocurrencies of byte in the buffer
uint32_t ringbuff_count(ringbuff_t *rb, uint8_t byte);
// ringbuff_peek: read buffer keeping data in the ringbuffer
void ringbuff_peek(ringbuff_t *rb, uint8_t *buffer, uint8_t peek_size);
// ringbuff_search: search for an array of bytes and return how many bytes there
//                  are before first byte of the array
int32_t ringbuff_search(ringbuff_t *rb, const uint8_t *to_search, uint32_t size);
int32_t ringbuff_search2(ringbuff_t *rb, const uint8_t *to_search, uint32_t size);

// put "> " at begin of string
void select_item(char *item_str);
// remove "> " from begin of string
void deselect_item(char *item_str);

// converts string to hex: returns the number of elements copied to array
uint16_t str_to_hex(const char *str, uint8_t *array, uint16_t array_size);


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
