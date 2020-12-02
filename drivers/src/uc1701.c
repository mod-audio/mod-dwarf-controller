
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "uc1701.h"
#include "hw_uc1701.h"
#include "device.h"

#include "task.h"


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

// buffer manipulation macros
#define READ_BUFFER(disp,x,y)           disp->buffer[(y)/8][(DISPLAY_WIDTH-1)-(x)]
#define WRITE_BUFFER(disp,x,y,data)     disp->buffer[(y)/8][(DISPLAY_WIDTH-1)-(x)] = (data); \
                                        disp->status |= NEED_UPDATE; \
                                        if (disp->status & UPDATING) disp->status |= FORCE_REFRESH;

// general purpose macros
#define ABS_DIFF(a, b)                  ((a > b) ? (a - b) : (b - a))
#define SWAP(a, b)                      do{uint8_t t; t = a; a = b; b = t;} while(0)

// SSP macros
#define SEND_DATA(disp, data)           taskENTER_CRITICAL(); SSP_SendData(disp->ssp_module, data); taskEXIT_CRITICAL();\
                                        while (SSP_GetStatus(disp->ssp_module, SSP_STAT_TXFIFO_EMPTY) == RESET || \
                                               SSP_GetStatus(disp->ssp_module, SSP_STAT_BUSY) == SET);

// backlight macros
#if defined UC1701_BACKLIGHT_TURN_ON_WITH_ONE
#define BACKLIGHT_TURN_ON(port, pin)    SET_PIN(port, pin)
#define BACKLIGHT_TURN_OFF(port, pin)   CLR_PIN(port, pin)
#elif defined UC1701_BACKLIGHT_TURN_ON_WITH_ZERO
#define BACKLIGHT_TURN_ON(port, pin)    CLR_PIN(port, pin)
#define BACKLIGHT_TURN_OFF(port, pin)   SET_PIN(port, pin)
#endif


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

static void write_cmd(uc1701_t *disp, uint8_t cmd)
{
    // activate chip select
    CLR_PIN(disp->cs_port, disp->cs_pin);

    // command mode
    CLR_PIN(disp->cd_port, disp->cd_pin);
    SEND_DATA(disp, cmd);

    // deactivate chip select
    SET_PIN(disp->cs_port, disp->cs_pin);
}

static void write_double_cmd(uc1701_t *disp, uint8_t cmd, uint8_t data)
{
    // activate chip select
    CLR_PIN(disp->cs_port, disp->cs_pin);

    // command mode
    CLR_PIN(disp->cd_port, disp->cd_pin);
    SEND_DATA(disp, cmd);
    SEND_DATA(disp, data);

    // deactivate chip select
    SET_PIN(disp->cs_port, disp->cs_pin);
}

static void write_data(uc1701_t *disp, uint16_t data)
{
    // activate chip select
    CLR_PIN(disp->cs_port, disp->cs_pin);

    // data mode
    SET_PIN(disp->cd_port, disp->cd_pin);
    SEND_DATA(disp, data);

    // deactivate chip select
    SET_PIN(disp->cs_port, disp->cs_pin);
}

static uint8_t read_byte(uc1701_t *disp, uint8_t x, uint8_t y)
{
    uint8_t data, data_tmp, y_offset;

    y_offset = (y % 8);

    if (y_offset != 0)
    {
        // first page
        data_tmp = READ_BUFFER(disp, x, y);
        data = (data_tmp >> y_offset);

        // second page
        data_tmp = READ_BUFFER(disp, x, y+8);
        data |= (data_tmp << (8 - y_offset));

        return data;
    }
    else
    {
        return READ_BUFFER(disp, x, y);
    }
}

static void write_byte(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t data)
{
    uint8_t data_tmp, y_offset;

    y_offset = (y % 8);

    if (y_offset != 0)
    {
        // first page
        data_tmp = READ_BUFFER(disp, x, y);
        data_tmp |= (data << y_offset);
        WRITE_BUFFER(disp, x, y, data_tmp);

        // second page
        y += 8;
        data_tmp = READ_BUFFER(disp, x, y);
        data_tmp |= data >> (8 - y_offset);
        WRITE_BUFFER(disp, x, y, data_tmp);
    }
    else
    {
        WRITE_BUFFER(disp, x, y, data);
    }
}


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void uc1701_init(uc1701_t *disp)
{
    // TODO: check if SSP is already initialized

    // setup the GPIO pins
    CONFIG_PIN_OUTPUT(disp->cs_port, disp->cs_pin);
    CONFIG_PIN_OUTPUT(disp->rst_port, disp->rst_pin);
    CONFIG_PIN_OUTPUT(disp->cd_port, disp->cd_pin);

    // initial values of GPIO
    SET_PIN(disp->cs_port, disp->cs_pin);
    SET_PIN(disp->rst_port, disp->rst_pin);
    SET_PIN(disp->cd_port, disp->cd_pin);

    // backlight configuration
    CONFIG_PIN_OUTPUT(disp->backlight_port, disp->backlight_pin);
    BACKLIGHT_TURN_ON(disp->backlight_port, disp->backlight_pin);

    // setup the SPI
    PINSEL_SetPinFunc(disp->ssp_clk_port, disp->ssp_clk_pin, disp->ssp_clk_func);
    PINSEL_SetPinFunc(disp->ssp_mosi_port, disp->ssp_mosi_pin, disp->ssp_mosi_func);

    SSP_CFG_Type ssp_config;
    // Initialize SSP Configuration parameter structure to default state:
    // CPHA = SSP_CPHA_FIRST
    // CPOL = SSP_CPOL_HI
    // ClockRate = 1000000
    // Databit = SSP_DATABIT_8
    // Mode = SSP_MASTER_MODE
    // FrameFormat = SSP_FRAME_SPI
    SSP_ConfigStructInit(&ssp_config);

    // change the clock to user value and apply the configuration
    ssp_config.ClockRate = disp->ssp_clock;
    SSP_Init(disp->ssp_module, &ssp_config);
    SSP_Cmd(disp->ssp_module, ENABLE);

    // reset display controller
    CLR_PIN(disp->rst_port, disp->rst_pin);
    DELAY_ms(2);
    SET_PIN(disp->rst_port, disp->rst_pin);
    DELAY_ms(2);

    // bias ratio 1/7
    write_cmd(disp, UC1701_SET_BR_7);

    // set SEG direction (column)
    write_cmd(disp, UC1701_SEG_DIR_NORMAL);

    // set COM direction (row)
    write_cmd(disp, UC1701_COM_DIR_NORMAL);

    // resistor ratio
    write_cmd(disp, UC1701_SET_RR | UC1701_RR_DEFAULT);

    // set eletronic volume (PM)
    write_double_cmd(disp, UC1701_SET_PM, UC1701_PM_DEFAULT);

    // power rise step 1
    write_cmd(disp, UC1701_SET_PC | 0x04);
    DELAY_ms(2);

    // power rise step 2
    write_cmd(disp, UC1701_SET_PC | 0x06);
    DELAY_ms(2);

    // power rise step 3
    write_cmd(disp, UC1701_SET_PC | 0x07);
    DELAY_ms(2);

    // set scroll line
    write_cmd(disp, UC1701_SET_SL);

    // display enable
    write_cmd(disp, UC1701_SET_DC2_EN);

    // clear display
    uc1701_clear(disp, UC1701_WHITE);
    uc1701_update(disp);

    DELAY_ms(2);

    // apply new configurations
#ifdef UC1701_REVERSE_COLUMNS
    write_cmd(disp, UC1701_SEG_DIR_INVERSE);
#endif
#ifdef UC1701_REVERSE_ROWS
    write_cmd(disp, UC1701_COM_DIR_INVERSE);
#endif
}

void uc1701_backlight(uc1701_t *disp, uint8_t state)
{
    if (state)
        BACKLIGHT_TURN_ON(disp->backlight_port, disp->backlight_pin);
    else
        BACKLIGHT_TURN_OFF(disp->backlight_port, disp->backlight_pin);
}

void uc1701_clear(uc1701_t *disp, uint8_t color)
{
    uint8_t i, j;

    for (i = 0; i < DISPLAY_WIDTH; i++)
    {
        for (j = 0; j < (DISPLAY_HEIGHT/8); j++)
        {
            disp->buffer[j][i] = color;
        }
    }

    disp->status |= NEED_UPDATE;
}

void uc1701_update(uc1701_t *disp)
{
    if (disp->status & NEED_UPDATE)
    {
        int i, j;

        disp->status |= UPDATING;

        for(i = 0; i < (DISPLAY_HEIGHT/8); i++)
        {
            // set page address
            write_cmd(disp, UC1701_SET_PA + i);

            // set column address to first display address, considering
            // direction and difference of columns between display/chip
            write_cmd(disp, UC1701_SET_CA_MSB);
#ifdef UC1701_REVERSE_COLUMNS
            write_cmd(disp, UC1701_SET_CA_LSB + (CHIP_COLUMNS - DISPLAY_WIDTH));
#else
            write_cmd(disp, UC1701_SET_CA_LSB);
#endif

            for(j = 0; j < DISPLAY_WIDTH; j++)
            {
                if (disp->status & FORCE_REFRESH)
                {
                    i = -1;
                    disp->status &= ~FORCE_REFRESH;
                    break;
                }

                write_data(disp, disp->buffer[i][j]);
            }
        }

        disp->status &= ~(NEED_UPDATE | UPDATING);
    }
}

void uc1701_set_pixel(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t color)
{
    // avoid x, y be out of the bounds
    x %= DISPLAY_WIDTH;
    y %= DISPLAY_HEIGHT;

    uint8_t data = READ_BUFFER(disp, x, y);

    // clear the bit
    data &= ~(1 << (y % 8));

    // set bit color
    data |= ((color & 0x01) << (y % 8));

    WRITE_BUFFER(disp, x, y, data);
}

void uc1701_hline(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (width--)
    {
        if (color == UC1701_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = UC1701_BLACK;
            else tmp = UC1701_WHITE;
        }
        else if (color == UC1701_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = UC1701_WHITE;
            else tmp = UC1701_BLACK;
        }
        i++;

        uc1701_set_pixel(disp, x++, y, tmp);
    }
}

void uc1701_vline(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (height--)
    {
        if (color == UC1701_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = UC1701_BLACK;
            else tmp = UC1701_WHITE;
        }
        else if (color == UC1701_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = UC1701_WHITE;
            else tmp = UC1701_BLACK;
        }
        i++;

        uc1701_set_pixel(disp, x, y++, tmp);
    }
}

void uc1701_line(uc1701_t *disp, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    uint8_t deltax, deltay, x, y, steep;
    int8_t error, ystep;

    steep = ABS_DIFF(y1, y2) > ABS_DIFF(x1, x2);

    if (steep)
    {
        SWAP(x1, y1);
        SWAP(x2, y2);
    }

    if (x1 > x2)
    {
        SWAP(x1, x2);
        SWAP(y1, y2);
    }

    deltax = x2 - x1;
    deltay = ABS_DIFF(y2, y1);
    error = deltax / 2;
    y = y1;
    if (y1 < y2) ystep = 1;
    else ystep = -1;

    uint8_t i = 0, tmp = color;

    for (x = x1; x <= x2; x++)
    {
        if (color == UC1701_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = UC1701_BLACK;
            else tmp = UC1701_WHITE;
        }
        else if (color == UC1701_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = UC1701_WHITE;
            else tmp = UC1701_BLACK;
        }
        i++;

        if (steep) uc1701_set_pixel(disp, y, x, tmp);
        else uc1701_set_pixel(disp, x, y, tmp);

        error = error - deltay;
        if (error < 0)
        {
            y = y + ystep;
            error = error + deltax;
        }
    }
}

void uc1701_rect(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    uc1701_hline(disp, x, y, width, color);
    uc1701_hline(disp, x, y+height-1, width, color);
    uc1701_vline(disp, x, y, height, color);
    uc1701_vline(disp, x+width-1, y, height, color);
}

void uc1701_rect_fill(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (width--)
    {
        if (color == UC1701_CHESS)
        {
            if ((i % 2) == 0) tmp = UC1701_BLACK_WHITE;
            else tmp = UC1701_WHITE_BLACK;
        }
        i++;

        uc1701_vline(disp, x++, y, height, tmp);
    }
}

void uc1701_rect_invert(uc1701_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    uint8_t mask, page_offset, h, i, data, data_tmp, x_tmp;

    page_offset = y % 8;
    mask = 0xFF;
    if (height < (8 - page_offset))
    {
        mask >>= (8 - height);
        h = height;
    }
    else
    {
        h = 8 - page_offset;
    }
    mask <<= page_offset;

    // First do the fractional pages at the top of the region
    for (i = 0; i < width; i++)
    {
        x_tmp = x + i;

        data = READ_BUFFER(disp, x_tmp, y);
        data_tmp = ~data;
        data = (data_tmp & mask) | (data & ~mask);
        WRITE_BUFFER(disp, x_tmp, y, data);
    }

    // Now do the full pages
    while((h + 8) <= height)
    {
        h += 8;
        y += 8;

        for (i = 0; i < width; i++)
        {
            x_tmp = x + i;

            data = READ_BUFFER(disp, x_tmp, y);
            WRITE_BUFFER(disp, x_tmp, y, ~data);
        }
    }

    // Now do the fractional pages at the bottom of the region
    if (h < height)
    {
        mask = ~(0xFF << (height-h));
        y += 8;

        for (i = 0; i < width; i++)
        {
            x_tmp = x + i;

            data = READ_BUFFER(disp, x_tmp, y);
            data_tmp = ~data;
            data = (data_tmp & mask) | (data & ~mask);
            WRITE_BUFFER(disp, x_tmp, y, data);
        }
    }
}

void uc1701_draw_image(uc1701_t *disp, uint8_t x, uint8_t y, const uint8_t *image, uint8_t color)
{
    uint8_t i, j, height, width;
    char data;

    width = (uint8_t) *image++;
    height = (uint8_t) *image++;

    for (j = 0; j < height; j += 8)
    {
        for(i = 0; i < width; i++)
        {
            data = *image++;
            if (color == UC1701_WHITE) data = ~data;
            WRITE_BUFFER(disp, x+i, y+j, data);
        }
    }
}

void uc1701_text(uc1701_t *disp, uint8_t x, uint8_t y, const char *text, const uint8_t *font, uint8_t color)
{
    uint8_t i, j, x_tmp, y_tmp, c, bytes, data;
    uint16_t char_data_index, page;

    // default font
    if (!font) font = FONT_DEFAULT;
    uint8_t width = font[FONT_FIXED_WIDTH];
    uint8_t height = font[FONT_HEIGHT];
    uint8_t first_char = font[FONT_FIRST_CHAR];
    uint8_t char_count = font[FONT_CHAR_COUNT];

    while (*text)
    {
        c = *text;
        if (c < first_char || c >= (first_char + char_count))
        {
            text++;
            continue;
        }

        c -= first_char;
        bytes = (height + 7) / 8;

        if (FONT_IS_MONO_SPACED(font))
        {
            char_data_index = (c * width * bytes) + FONT_WIDTH_TABLE;
        }
        else
        {
            width = font[FONT_WIDTH_TABLE + c];
            char_data_index = 0;
            for (i = 0; i < c; i++) char_data_index += font[FONT_WIDTH_TABLE + i];
            char_data_index = (char_data_index * bytes) + FONT_WIDTH_TABLE + char_count;
        }

        y_tmp = y;

        // check if character fits on display before write it
        if ((x + width) > DISPLAY_WIDTH)
            break;

        // draws each character piece
        for (j = 0; j < bytes; j++)
        {
            x_tmp = x;
            page = j * width;

            // draws the character
            for (i = 0; i < width; i++)
            {
                data = font[char_data_index + page + i];
                if (color == UC1701_WHITE) data = ~data;

                // if is the last piece of character...
                if (height > 8 && height < (j + 1) * 8)
                {
                    data >>= ((j + 1) * 8) - height;
                    data = read_byte(disp, x_tmp, y_tmp) | data;
                }

                write_byte(disp, x_tmp, y_tmp, data);

                x_tmp++;
            }

            // draws the interchar space
            data = (color == UC1701_BLACK ? 0x00 : 0xFF);
            if (height > 8 && height < (j + 1) * 8)
            {
                data >>= ((j + 1) * 8) - height;
                data = read_byte(disp, x_tmp, y_tmp) | data;
            }

            if (*(text + 1) != '\0')
            {
                write_byte(disp, x_tmp, y_tmp, data);
            }

            y_tmp += 8;
        }

        x += width + 1;

        text++;
    }
}