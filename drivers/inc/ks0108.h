
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef  KS0108_H
#define  KS0108_H


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
#define KS0108_WHITE            0
#define KS0108_BLACK            1
#define KS0108_BLACK_WHITE      2
#define KS0108_WHITE_BLACK      3
#define KS0108_CHESS            4

// backlight
#define KS0108_BACKLIGHT_ON     1
#define KS0108_BACKLIGHT_OFF    0

// chip defines
#define CHIP_COUNT              2
#define CHIP_HEIGHT             64
//#define CHIP_WIDTH              64


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
#define KS0108_BACKLIGHT_TURN_ON_WITH_ONE


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

typedef struct KS0108_T {
    uint8_t id;
    uint8_t data_bus_port;
    uint8_t cs1_port, cs1_pin;
    uint8_t cs2_port, cs2_pin;
    uint8_t cd_port, cd_pin;
    uint8_t en_port, en_pin;
    uint8_t rw_port, rw_pin;
    uint8_t rst_port, rst_pin;
    uint8_t backlight_port, backlight_pin;

    uint8_t buffer[DISPLAY_HEIGHT/8][DISPLAY_WIDTH];
} ks0108_t;


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

void ks0108_init(ks0108_t *disp);
void ks0108_backlight(ks0108_t *disp, uint8_t state);
void ks0108_clear(ks0108_t *disp, uint8_t color);
void ks0108_set_pixel(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t color);
void ks0108_hline(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t color);
void ks0108_vline(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t height, uint8_t color);
void ks0108_line(ks0108_t *disp, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void ks0108_rect(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void ks0108_rect_fill(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void ks0108_rect_invert(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void ks0108_draw_image(ks0108_t *disp, uint8_t x, uint8_t y, const uint8_t *image, uint8_t color);
void ks0108_text(ks0108_t *disp, uint8_t x, uint8_t y, const char *text, const uint8_t *font, uint8_t color);
void ks0108_update(ks0108_t *disp);


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
