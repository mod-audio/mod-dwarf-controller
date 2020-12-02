
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "utils.h"
#include "FreeRTOS.h"
#include "cli.h"

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

#define BUFFER_IS_FULL(rb)      (rb->tail == (rb->head + 1) % rb->size)
#define BUFFER_IS_EMPTY(rb)     (rb->head == rb->tail)
#define BUFFER_INC(rb,idx)      (rb->idx = (rb->idx + 1) % rb->size)


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/


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

static char* reverse(char* str, uint32_t str_len)
{
    char *end = str + (str_len - 1);
    char *start = str, tmp;

    while (start < end)
    {
        tmp = *end;
        *end = *start;
        *start = tmp;

        start++;
        end--;
    }

    return str;
}

static void parse_quote(char *str)
{
    char *pquote, *pstr = str;

    while (*pstr)
    {
        if (*pstr == '"')
        {
            // shift the string to left
            pquote = pstr;
            while (*pquote)
            {
                *pquote = *(pquote+1);
                pquote++;
            }
        }
        else pstr++;
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/
// copy an command to buffer
uint8_t copy_command(char *buffer, const char *command)
{
    uint8_t i = 0;
    const char *cmd = command;

    while (*cmd && (*cmd != '%' && *cmd != '.'))
    {
        buffer[i++] = *cmd;
        cmd++;
    }

    return i;
}

char** strarr_split(char *str, const char token)
{
    uint32_t count;
    char *pstr, **list = NULL;
    uint8_t quote = 0;

    if (!str) return list;

    // count the tokens
    pstr = str;
    count = 1;
    while (*pstr)
    {
        if (*pstr == token && quote == 0)
        {
            count++;
        }
#ifdef ENABLE_QUOTATION_MARKS
        if (*pstr == '"')
        {
            if (quote == 0) quote = 1;
            else
            {
                if (*(pstr+1) == '"') pstr++;
                else quote = 0;
            }
        }
#endif
        pstr++;
    }

    // allocates memory to list
    list = MALLOC((count + 1) * sizeof(char *));
    if (!list) return NULL;

    // fill the list pointers
    pstr = str;
    list[0] = pstr;
    count = 0;
    while (*pstr)
    {
        if (*pstr == token && quote == 0)
        {
            *pstr = '\0';
            list[++count] = pstr + 1;
        }
#ifdef ENABLE_QUOTATION_MARKS
        if (*pstr == '"')
        {
            if (quote == 0) quote = 1;
            else
            {
                if (*(pstr+1) == '"') pstr++;
                else quote = 0;
            }
        }
#endif
        pstr++;
    }

    list[++count] = NULL;

#ifdef ENABLE_QUOTATION_MARKS
    count = 0;
    while (list[count]) parse_quote(list[count++]);
#endif

    return list;
}


uint32_t strarr_length(char** const str_array)
{
    uint32_t count = 0;

    if (str_array) while (str_array[count]) count++;
    return count;
}


char* strarr_join(char **str_array)
{
    uint32_t i, len = strarr_length(str_array);

    if (!str_array) return NULL;

    for (i = 1; i < len; i++)
    {
        (*(str_array[i] - 1)) = ' ';
    }

    return (*str_array);
}


uint32_t int_to_str(int32_t num, char *string, uint32_t string_size, uint8_t zero_leading)
{
    char *pstr = string;
    uint8_t signal = 0;
    uint32_t str_len;

    if (!string) return 0;

    // exception case: number is zero
    if (num == 0)
    {
        *pstr++ = '0';
        if (zero_leading) zero_leading--;
    }

    // need minus signal?
    if (num < 0)
    {
        num = -num;
        signal = 1;
        string_size--;
    }

    // composes the string
    while (num)
    {
        *pstr++ = (num % 10) + '0';
        num /= 10;

        if (--string_size == 0) break;
        if (zero_leading) zero_leading--;
    }

    // checks buffer size
    if (string_size == 0)
    {
        *string = 0;
        return 0;
    }

    // fills the zeros leading
    while (zero_leading--) *pstr++ = '0';

    // put the minus if necessary
    if (signal) *pstr++ = '-';
    *pstr = 0;

    // invert the string characters
    str_len = (pstr - string);
    reverse(string, str_len);

    return str_len;
}

uint32_t float_to_str(float num, char *string, uint32_t string_size, uint8_t precision)
{
    double intp, fracp;
    char *str = string;

    if (!string) return 0;

    // TODO: check Nan and Inf

    // splits integer and fractional parts
    fracp = modf(num, &intp);

    // convert to absolute value
    if (intp < 0.0) intp = -intp;
    if (fracp < 0.0) fracp = -fracp;

    // insert minus if negative number
    if (num < 0.0)
    {
        *str = '-';
        str++;
    }

    // convert the integer part to string
    uint32_t int_len;
    int_len = int_to_str((int32_t)intp, str, string_size, 0);

    // checks if convertion fail
    if (int_len == 0)
    {
        *string = 0;
        return 0;
    }

    // adds one to avoid lost the leading zeros
    fracp += 1.0;

    // calculates the precision
    while (precision--)
    {
        fracp *= 10;
    }

    // add 0.5 to round
    fracp += 0.5;

    // convert the fractional part
    uint32_t frac_len;
    frac_len = int_to_str((int32_t)fracp, &str[int_len], string_size - int_len, 0);

    // checks if convertion fail
    if (frac_len == 0)
    {
        *string = 0;
        return 0;
    }

    // inserts the dot covering the extra one added
    str[int_len] = '.';

    return (int_len + frac_len);
}

char *str_duplicate(const char *str)
{
    if (!str) return NULL;

    char *copy = MALLOC(strlen(str) + 1);
    if (copy) strcpy(copy, str);

    return copy;
}

// handy function to make a copy of a C-string array (char**)
char** str_array_duplicate(char** list, uint16_t count)
{
    if (!list)
        return NULL;
    
    char** ret = MALLOC((count+1)*sizeof(char*));
    
    // out of memory
    if (!ret)
        return NULL;
    
    for (size_t i=0; i<count; ++i)
        ret[i] = strdup(list[i]);
    
    ret[count] = NULL;
    return ret;
}

void delay_us(volatile uint32_t time)
{
    register uint32_t _time asm ("r0");
    (void)(_time); // just to avoid warning
    _time = time;

    __asm__ volatile
    (
        "1:\n\t"
            "mov r1, #0x0015\n\t"
            "2:\n\t"
                "sub r1, r1, #1\n\t"
                "cbz r1, 3f\n\t"
                "b 2b\n\t"
            "3:\n\t"
                "sub r0, r0, #1\n\t"
                "cbz r0, 4f\n\t"
                "b 1b\n\t"
            "4:\n\t"
    );
}

void delay_ms(volatile uint32_t time)
{
    register uint32_t _time asm ("r0");
    (void)(_time); // just to avoid warning
    _time = time;

    __asm__ volatile
    (
        "1:\n\t"
            "mov r1, #0x5600\n\t"
            "2:\n\t"
                "sub r1, r1, #1\n\t"
                "cbz r1, 3f\n\t"
                "b 2b\n\t"
            "3:\n\t"
                "sub r0, r0, #1\n\t"
                "cbz r0, 4f\n\t"
                "b 1b\n\t"
            "4:\n\t"
    );
}

float convert_to_ms(const char *unit_from, float value)
{
    char unit[8];
    uint8_t i;

    // lower case unit string
    for (i = 0; unit_from[i] && i < (sizeof(unit)-1); i++)
    {
        if (i == (sizeof(unit) - 1)) break;
        unit[i] = unit_from[i] | 0x20;
    }
    unit[i] = 0;

    if (strcmp(unit, "bpm") == 0)
    {
        return (60000.0 / value);
    }
    else if (strcmp(unit, "hz") == 0)
    {
        return (1000.0 / value);
    }
    else if (strcmp(unit, "s") == 0)
    {
        return (value * 1000.0);
    }
    else if (strcmp(unit, "ms") == 0)
    {
        return value;
    }

    return 0.0;
}

float convert_from_ms(const char *unit_to, float value)
{
    char unit[8];
    uint8_t i;

    // lower case unit string
    for (i = 0; unit_to[i] && i < (sizeof(unit)-1); i++)
    {
        if (i == (sizeof(unit) - 1)) break;
        unit[i] = unit_to[i] | 0x20;
    }
    unit[i] = 0;

    if (strcmp(unit, "bpm") == 0)
    {
        return (60000.0 / value);
    }
    else if (strcmp(unit, "hz") == 0)
    {
        return (1000.0 / value);
    }
    else if (strcmp(unit, "s") == 0)
    {
        return (value / 1000.0);
    }
    else if (strcmp(unit, "ms") == 0)
    {
        return value;
    }

    return 0.0;
}

ringbuff_t *ringbuff_create(uint32_t buffer_size)
{
    ringbuff_t *rb = (ringbuff_t *) MALLOC(sizeof(ringbuff_t));

    if (rb)
    {
        rb->head = 0;
        rb->tail = 0;
        rb->size = buffer_size;
        rb->buffer = (uint8_t *) MALLOC(buffer_size);

        // checks memory allocation
        if (!rb->buffer)
        {
            FREE(rb);
            rb = NULL;
        }
    }

    return rb;
}

void ringbuff_destroy(ringbuff_t *rb)
{
    if (rb)
    {
        if (rb->buffer)
            FREE(rb->buffer);

        FREE(rb);
    }
}

uint32_t ringbuff_write(ringbuff_t *rb, const uint8_t *data, uint32_t data_size)
{
    uint32_t bytes = 0;
    const uint8_t *pdata, dummy = 0;

    pdata =  data ? data : &dummy;
    while (data_size > 0 && !BUFFER_IS_FULL(rb))
    {
        rb->buffer[rb->head] = *pdata;
        if (data) pdata++;
        BUFFER_INC(rb, head);

        data_size--;
        bytes++;
    }

    return bytes;
}

uint32_t ringbuff_read(ringbuff_t *rb, uint8_t *buffer, uint32_t buffer_size)
{
    uint32_t bytes = 0;
    uint8_t *data = buffer;

    while (buffer_size > 0 && !BUFFER_IS_EMPTY(rb))
    {
        if (buffer)
            *data++ = rb->buffer[rb->tail];

        BUFFER_INC(rb, tail);

        buffer_size--;
        bytes++;
    }

    return bytes;
}

uint32_t ringbuff_read_until(ringbuff_t *rb, uint8_t *buffer, uint32_t buffer_size, uint8_t token)
{
    uint32_t bytes = 0;
    uint8_t *data, dummy;

    data = buffer;
    if (!buffer)
    {
        data = &dummy;
        buffer_size = rb->size;
    }

    if (ringbuff_count(rb, token) > 0)
    {
        while (buffer_size > 0 && !BUFFER_IS_EMPTY(rb))
        {
            *data = rb->buffer[rb->tail];
            BUFFER_INC(rb, tail);

            buffer_size--;
            bytes++;

            if (*data == token) break;
            if (buffer) data++;
        }
    }

    return bytes;
}

uint32_t ringbuffer_used_space(ringbuff_t *rb)
{
    return ((rb->head - rb->tail) % rb->size);
}

uint32_t ringbuff_available_space(ringbuff_t *rb)
{
    return (rb->size - ((rb->head - rb->tail) % rb->size));
}

uint32_t ringbuff_is_full(ringbuff_t *rb)
{
    return BUFFER_IS_FULL(rb);
}

uint32_t ringbuff_is_empty(ringbuff_t *rb)
{
    return BUFFER_IS_EMPTY(rb);
}

void ringbuff_flush(ringbuff_t *rb)
{
    if (rb)
    {
        rb->head = 0;
        rb->tail = 0;
    }
}

void ringbuff_free(ringbuff_t *rb)
{
    if (rb)
    {
        rb->head = 0;
        rb->tail = 0;
        rb->buffer = NULL;
    }
}


uint32_t ringbuff_count(ringbuff_t *rb, uint8_t byte)
{
    uint32_t count = 0;
    uint32_t tail, head;
    uint8_t data;

    tail = rb->tail;
    head = rb->head;

    while (tail != head)
    {
        data = rb->buffer[tail];
        tail = (tail + 1) % rb->size;

        if (data == byte) count++;
    }

    return count;
}

void ringbuff_peek(ringbuff_t *rb, uint8_t *buffer, uint8_t peek_size)
{
    uint32_t tail, head;
    uint8_t *data = buffer;

    tail = rb->tail;
    head = rb->head;

    while (peek_size > 0 && tail != head)
    {
        *data++ = rb->buffer[tail];
        tail = (tail + 1) % rb->size;
        peek_size--;
    }
}

int32_t ringbuff_search(ringbuff_t *rb, const uint8_t *to_search, uint32_t size)
{
    uint32_t tail, head;
    uint32_t count = 0;

    tail = rb->tail;
    head = rb->head;

    if (!to_search) return -1;

    while (tail != head)
    {
        const uint8_t *s = to_search;
        uint8_t data;

        // search for first byte
        do
        {
            data = rb->buffer[tail];
            tail = (tail + 1) % rb->size;
            count++;
        } while (data != *s && tail != head);

        if (data == *s && size == 1)
            return (count - 1);

        // check next bytes
        uint32_t i = size - 1, a = 1;
        while (tail != head)
        {
            data = rb->buffer[tail];
            tail = (tail + 1) % rb->size;

            s++;
            if (data != *s)
            {
                count += a;
                break;
            }

            a++;

            if (--i == 0)
                return (count - 1);
        }
    }

    return -1;
}

int32_t ringbuff_search2(ringbuff_t *rb, const uint8_t *to_search, uint32_t size)
{
    if (!to_search)
        return -1;

    uint32_t tail = rb->tail;
    uint32_t match = 0;

    const uint8_t *s = to_search;
    uint8_t data;

    // search for first byte
    do
    {
        data = rb->buffer[tail];
        tail = (tail + 1) % rb->size;

        if (data == *s)
        {
            s++;
            match++;

            if (match == size)
            {
                rb->tail = tail;
                return 1;
            }
        }
        else
        {
            if (data == '\n')
                rb->tail = tail;

            s = to_search;
            match = 0;
        }

    } while (tail != rb->head);

    return -1;
}

void select_item(char *item_str)
{
    if (item_str[0] == '>') return;

    char aux[2];
    uint32_t i = 0, j = 0, k = 0;
    for (i = 0; i < (strlen(item_str) + 3); i++)
    {
        aux[j++] = item_str[i];

        if (i == 0)
            item_str[i] = '>';
        else if (i == 1)
            item_str[i] = ' ';
        else
        {
            item_str[i] = aux[k++];
            if (k > 2) k = 0;
        }

        if (j > 2) j = 0;
    }
}

void deselect_item(char *item_str)
{
    if (item_str[0] != '>') return;

    uint32_t i;
    for (i = 0; i < (strlen(item_str) - 1); i++)
    {
        item_str[i] = item_str[i+2];
    }
}

uint16_t str_to_hex(const char *str, uint8_t *array, uint16_t array_size)
{
    if (!str || !array) return 0;

    uint8_t i, num[2] = { 0, 0 };
    uint16_t count = 0;
    const char *pstr = str;

    while (*pstr)
    {
        for (i = 0; i < 2 && *pstr; i++, pstr++)
        {
            num[i] = *pstr | 0x20;

            if (num[i] >= '0' && num[i] <= '9') num[i] = (num[i] - '0');
            else if (num[i] >= 'a' && num[i] <= 'f') num[i] = (num[i] + 10 - 'a');
        }

        array[count++] = (num[0] << 4) + num[1];
        if (count >= array_size) break;
    }

    return count;
}
