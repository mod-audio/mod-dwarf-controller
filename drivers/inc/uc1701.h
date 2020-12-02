
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef UC1701_H
#define UC1701_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>

#include "config.h"
#include "fonts.h"
#include "utils.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

// display size
#define DISPLAY_WIDTH       128
#define DISPLAY_HEIGHT      64

// chip definitions
#define CHIP_COLUMNS        132
#define CHIP_ROWS           65

// colors
#define UC1701_WHITE          0
#define UC1701_BLACK          1
#define UC1701_BLACK_WHITE    2
#define UC1701_WHITE_BLACK    3
#define UC1701_CHESS          4

// display status
#define NEED_UPDATE     1
#define UPDATING        2
#define FORCE_REFRESH   4


/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/

// delay macros definition
#define DELAY_us(time)      delay_us(time)
#define DELAY_ms(time)      delay_ms(time)

// uc1701 register default values
#define UC1701_PM_DEFAULT   7
#define UC1701_RR_DEFAULT   7

// display backlight turn on definition
#define UC1701_BACKLIGHT_TURN_ON_WITH_ONE


/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/

typedef struct UC1701_T {
    void *ssp_module;
    uint32_t ssp_clock;
    uint8_t ssp_clk_port, ssp_clk_pin, ssp_clk_func;
    uint8_t ssp_mosi_port, ssp_mosi_pin, ssp_mosi_func;
    uint8_t cs_port, cs_pin;
    uint8_t cd_port, cd_pin;
    uint8_t rst_port, rst_pin;
    uint8_t backlight_port, backlight_pin;

    uint8_t status;
    uint8_t buffer[DISPLAY_HEIGHT/8][DISPLAY_WIDTH];
} uc1701_t;


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

void uc1701_init(uc1701_t *disp);
void uc1701_backlight(uc1701_t *disp, uint8_t state);
void uc1701_clear(uc1701_t *disp, uint8_t color);
void uc1701_update(uc1701_t *disp);
void uc1701_set_pixel(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t color);
void uc1701_hline(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t color);
void uc1701_vline(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t height, uint8_t color);
void uc1701_line(uc1701_t *disp, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void uc1701_rect(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void uc1701_rect_fill(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void uc1701_rect_invert(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void uc1701_draw_image(uc1701_t *disp, uint8_t x, uint8_t y, const uint8_t *image, uint8_t color);
void uc1701_text(uc1701_t *disp, uint8_t x, uint8_t y, const char *text, const uint8_t *font, uint8_t color);


/*
************************************************************************************************************************
*           CONFIGURATION ERRORS
************************************************************************************************************************
*/

#ifndef DELAY_us
#error "DELAY_us macro must be defined"
#endif

#ifndef DELAY_ms
#error "DELAY_ms macro must be defined"
#endif


/*
************************************************************************************************************************
*           END HEADER
************************************************************************************************************************
*/

#endif
