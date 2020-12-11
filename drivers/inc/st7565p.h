
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef  ST7565P_H
#define  ST7565P_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>
#include "config.h"
#include "utils.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

// colors
#define ST7565P_WHITE            0
#define ST7565P_BLACK            1
#define ST7565P_BLACK_WHITE      2
#define ST7565P_WHITE_BLACK      3
#define ST7565P_CHESS            4

// backlight
#define ST7565P_BACKLIGHT_ON     1
#define ST7565P_BACKLIGHT_OFF    0

// chip defines
#define CHIP_COUNT              2
#define CHIP_HEIGHT             64
#define CHIP_WIDTH              128


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/

// display configuration
#define DISPLAY_WIDTH           128
#define DISPLAY_HEIGHT          64

// delay macros definition
#define DELAY_ns(time)          do {volatile uint32_t __delay = (time); while (__delay--);} while(0)
#define DELAY_us(time)          delay_us(time)

// display backlight turn on definition
#define ST7565P_BACKLIGHT_TURN_ON_WITH_ONE

#define ST7565_DEFAULT_CONTRAST 0x15
#define DISPLAY_CONTRAST_MAX    0x40
#define DISPLAY_CONTRAST_MIN    0x00
/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

typedef struct DATA_PORT_T {
    uint8_t port;
    uint8_t pin1;
    uint8_t pin2;
    uint8_t pin3;
    uint8_t pin4;
    uint8_t pin5;
    uint8_t pin6;
    uint8_t pin7;
    uint8_t pin8;
} data_port_t;

typedef struct ST7565P_T {
    uint8_t id;
    data_port_t data_bus_port;
    uint8_t cs_port, cs_pin;
    uint8_t a0_port, a0_pin;
    uint8_t read_port, read_pin;
    uint8_t write_port, write_pin;
    uint8_t rst_port, rst_pin;
    uint8_t backlight_port, backlight_pin;

    uint8_t buffer[DISPLAY_HEIGHT/8][DISPLAY_WIDTH];
} st7565p_t;


/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           MACROS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           FUNCTION PROTOTYPES
************************************************************************************************************************
*/

void st7565p_init(st7565p_t *disp);
void st7565p_backlight(st7565p_t *disp, uint8_t state);
void st7565p_clear(st7565p_t *disp, uint8_t color);
void st7565p_set_pixel(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t color);
void st7565p_hline(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t color);
void st7565p_vline(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t height, uint8_t color);
void st7565p_line(st7565p_t *disp, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void st7565p_rect(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void st7565p_rect_fill(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void st7565p_rect_invert(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void st7565p_draw_image(st7565p_t *disp, uint8_t x, uint8_t y, const uint8_t *image, uint8_t color);
void st7565p_text(st7565p_t *disp, uint8_t x, uint8_t y, const char *text, const uint8_t *font, uint8_t color);
void st7565p_update(st7565p_t *disp);
void st7565p_set_contrast(st7565p_t *disp, uint8_t contrast);


/*
************************************************************************************************************************
*           CONFIGURATION ERRORS
************************************************************************************************************************
*/

#ifndef DELAY_ns
#error "DELAY_ns macro must be defined"
#endif

#ifndef DELAY_us
#error "DELAY_us macro must be defined"
#endif


/*
************************************************************************************************************************
*           END HEADER
************************************************************************************************************************
*/

#endif
