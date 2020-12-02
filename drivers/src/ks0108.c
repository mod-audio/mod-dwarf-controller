
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

#include "ks0108.h"
#include "fonts.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

// displays instructions
#define DISPLAY_ON      0x3F
#define DISPLAY_OFF     0x3E
#define DISPLAY_START   0xC0
#define BUSY_FLAG       0x80
#define SET_PAGE        0xB8
#define SET_ADDRESS     0x40

// display timing defines (in nanoseconds)
#define DELAY_DDR       320    // Data Delay time (E high to valid read data)
#define DELAY_AS        140    // Address setup time (ctrl line changes to E HIGH)
#define DELAY_AH        10     // Address/Data write hold time
#define DELAY_DSW       200    // Data setup time (data lines setup to dropping E)
#define DELAY_WH        450    // E hi level width (minimum E hi pulse width)
#define DELAY_WL        450    // E lo level width (minimum E lo pulse width)

// function bits definition
// this is used to block/unblock the glcd functions
enum {SET_PIXEL, CLEAR, HLINE, VLINE, LINE, RECT, RECT_FILL, RECT_INVERT, DRAW_IMAGE, TEXT};

#if ! defined(SELECTOR_DIR_PINS) || ! defined(SELECTOR_CHANNELS_PINS)
#define NO_SELECTOR
#endif


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

// operation modes macros
#define INSTRUCTION_MODE(disp)          {CLR_PIN(disp->cd_port, disp->cd_pin); CLR_PIN(disp->rw_port, disp->rw_pin); \
                                         DELAY_ns(DELAY_AS);}
#define STATUS_MODE(disp)               {CLR_PIN(disp->cd_port, disp->cd_pin); SET_PIN(disp->rw_port, disp->rw_pin); \
                                         DELAY_ns(DELAY_AS);}
#define WRITE_DATA_MODE(disp)           {SET_PIN(disp->cd_port, disp->cd_pin); CLR_PIN(disp->rw_port, disp->rw_pin); \
                                         DELAY_ns(DELAY_AS);}
#define READ_DATA_MODE(disp)            {SET_PIN(disp->cd_port, disp->cd_pin); SET_PIN(disp->rw_port, disp->rw_pin); \
                                         DELAY_ns(DELAY_AS);}

// pins control macros
#define ENABLE_HIGH(disp)               SET_PIN(disp->en_port, disp->en_pin); DELAY_ns(DELAY_WH)
#define ENABLE_LOW(disp)                CLR_PIN(disp->en_port, disp->en_pin); DELAY_ns(DELAY_AH)
#define ENABLE_PULSE(disp)              {ENABLE_HIGH(disp); ENABLE_LOW(disp);}
#define CS1_ON(disp)                    SET_PIN(disp->cs1_port, disp->cs1_pin)
#define CS1_OFF(disp)                   CLR_PIN(disp->cs1_port, disp->cs1_pin)
#define CS2_ON(disp)                    SET_PIN(disp->cs2_port, disp->cs2_pin)
#define CS2_OFF(disp)                   CLR_PIN(disp->cs2_port, disp->cs2_pin)

// buffer macros
#define READ_BUFFER(disp,x,y)           disp->buffer[(y/8)][x]
#define WRITE_BUFFER(disp,x,y,data)     disp->buffer[(y/8)][x] = (data)

// backlight macros
#if defined KS0108_BACKLIGHT_TURN_ON_WITH_ONE
#define BACKLIGHT_TURN_ON(port, pin)    SET_PIN(port, pin)
#define BACKLIGHT_TURN_OFF(port, pin)   CLR_PIN(port, pin)
#elif defined KS0108_BACKLIGHT_TURN_ON_WITH_ZERO
#define BACKLIGHT_TURN_ON(port, pin)    CLR_PIN(port, pin)
#define BACKLIGHT_TURN_OFF(port, pin)   SET_PIN(port, pin)
#endif

// general purpose macros
#define ABS_DIFF(a, b)                  ((a > b) ? (a - b) : (b - a))
#define SWAP(a, b)                      do{uint8_t t; t = a; a = b; b = t;} while(0)

// block/unblock macros
#define BLOCK(disp,func)                g_blocked[(disp)] |= (1 << (func))
#define UNBLOCK(disp,func)              g_blocked[(disp)] &= ~(1 << (func))
#define IS_BLOCKED(disp,func)           (g_blocked[(disp)] & (1 << (func)))

// selector macros
#define ENABLE_CHANNEL(port,pin)    CLR_PIN(port, pin)
#define DISABLE_CHANNEL(port,pin)   SET_PIN(port, pin)
#define OUTPUT_MODE(port,pin)       SET_PIN(port, pin)
#define INPUT_MODE(port,pin)        CLR_PIN(port, pin)
enum {SELECTOR_OUTPUT_MODE, SELECTOR_INPUT_MODE};


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static uint8_t g_id;
#ifndef NO_SELECTOR
static uint8_t g_n_channels;
static uint8_t g_dir_pin[2], g_channels_pins[GLCD_COUNT*2];
#endif

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

static void chip_select(ks0108_t *disp, uint8_t chip)
{
    if (chip == 0)
    {
        CS1_ON(disp);
        CS2_OFF(disp);
    }
    else if (chip == 1)
    {
        CS2_ON(disp);
        CS1_OFF(disp);
    }
}

static void write_instruction(ks0108_t *disp, uint8_t chip, uint8_t data)
{
    chip_select(disp, chip);
    INSTRUCTION_MODE(disp);
    WRITE_PORT(disp->data_bus_port, data);
    DELAY_ns(DELAY_DSW);
    ENABLE_PULSE(disp);
}

static uint8_t read_data(ks0108_t *disp, uint8_t x, uint8_t y)
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

static void write_data(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t data)
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

#ifndef NO_SELECTOR
static void selector_init(uint8_t n_channels, const uint8_t *dir_pin, const uint8_t *channels_pins)
{
    // configures direction pin
    CONFIG_PIN_OUTPUT(dir_pin[0], dir_pin[1]);

    // configures channels pins
    uint8_t i;
    for (i = 0; i < n_channels; i++)
    {
        CONFIG_PIN_OUTPUT(channels_pins[(i*2) + 0], channels_pins[(i*2) + 1]);
        DISABLE_CHANNEL(channels_pins[(i*2) + 0], channels_pins[(i*2) + 1]);
    }

    // store the configs
    g_n_channels = n_channels;
    g_dir_pin[0] = dir_pin[0];
    g_dir_pin[1] = dir_pin[1];

    for (i = 0; i < GLCD_COUNT*2; i++)
    {
        g_channels_pins[i] = channels_pins[i];
    }
}

static void selector_direction(uint8_t dir)
{
    if (dir == SELECTOR_OUTPUT_MODE)
        OUTPUT_MODE(g_dir_pin[0], g_dir_pin[1]);
    else
        INPUT_MODE(g_dir_pin[0], g_dir_pin[1]);
}

static void selector_channel(uint8_t channel)
{
    uint8_t i;
    for (i = 0; i < g_n_channels; i++)
        DISABLE_CHANNEL(g_channels_pins[(i * 2) + 0], g_channels_pins[(i * 2) + 1]);

    ENABLE_CHANNEL(g_channels_pins[(channel * 2) + 0], g_channels_pins[(channel * 2) + 1]);
}
#endif


/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void ks0108_init(ks0108_t *disp)
{
    disp->id = g_id++;

#ifndef NO_SELECTOR
    // initializes the selector
    selector_init(GLCD_COUNT, (const uint8_t [])SELECTOR_DIR_PINS, (const uint8_t [])SELECTOR_CHANNELS_PINS);
    selector_direction(SELECTOR_OUTPUT_MODE);
    selector_channel(disp->id);
#endif

    // direction pins configuration
    CONFIG_PIN_OUTPUT(disp->rst_port, disp->rst_pin);
    CONFIG_PIN_OUTPUT(disp->rw_port, disp->rw_pin);
    CONFIG_PIN_OUTPUT(disp->cd_port, disp->cd_pin);
    CONFIG_PIN_OUTPUT(disp->en_port, disp->en_pin);
    CONFIG_PIN_OUTPUT(disp->cs1_port, disp->cs1_pin);
    CONFIG_PIN_OUTPUT(disp->cs2_port, disp->cs2_pin);
    CONFIG_PORT_OUTPUT(disp->data_bus_port);

    // backlight configuration
    CONFIG_PIN_OUTPUT(disp->backlight_port, disp->backlight_pin);
    BACKLIGHT_TURN_ON(disp->backlight_port, disp->backlight_pin);

    // initial pins state
    ENABLE_LOW(disp);
    CS1_OFF(disp);
    CS2_OFF(disp);
    INSTRUCTION_MODE(disp);

    // reset
    CLR_PIN(disp->rst_port, disp->rst_pin);
    DELAY_us(10000);
    SET_PIN(disp->rst_port, disp->rst_pin);
    DELAY_us(50000);

    // chip initialization
    uint8_t i;
    for (i = 0; i < CHIP_COUNT; i++)
    {
        write_instruction(disp, i, DISPLAY_ON);
        write_instruction(disp, i, DISPLAY_START);
    }

    ks0108_clear(disp, KS0108_WHITE);
    ks0108_update(disp);
}

void ks0108_backlight(ks0108_t *disp, uint8_t state)
{
    if (state)
        BACKLIGHT_TURN_ON(disp->backlight_port, disp->backlight_pin);
    else
        BACKLIGHT_TURN_OFF(disp->backlight_port, disp->backlight_pin);
}

void ks0108_clear(ks0108_t *disp, uint8_t color)
{
    ks0108_rect_fill(disp, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, color);
}

void ks0108_set_pixel(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t color)
{
    // avoid x, y be out of the bounds
    x %= DISPLAY_WIDTH;
    y %= DISPLAY_HEIGHT;

    uint8_t data;

    data = READ_BUFFER(disp, x, y);
    if (color == KS0108_BLACK) data |= (0x01 << (y % 8));
    else data &= ~(0x01 << (y % 8));
    WRITE_BUFFER(disp, x, y, data);
}

void ks0108_hline(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (width--)
    {
        if (color == KS0108_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = KS0108_BLACK;
            else tmp = KS0108_WHITE;
        }
        else if (color == KS0108_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = KS0108_WHITE;
            else tmp = KS0108_BLACK;
        }
        i++;

        ks0108_set_pixel(disp, x++, y, tmp);
    }
}

void ks0108_vline(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (height--)
    {
        if (color == KS0108_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = KS0108_BLACK;
            else tmp = KS0108_WHITE;
        }
        else if (color == KS0108_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = KS0108_WHITE;
            else tmp = KS0108_BLACK;
        }
        i++;

        ks0108_set_pixel(disp, x, y++, tmp);
    }
}

void ks0108_line(ks0108_t *disp, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
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
        if (color == KS0108_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = KS0108_BLACK;
            else tmp = KS0108_WHITE;
        }
        else if (color == KS0108_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = KS0108_WHITE;
            else tmp = KS0108_BLACK;
        }
        i++;

        if (steep) ks0108_set_pixel(disp, y, x, tmp);
        else ks0108_set_pixel(disp, x, y, tmp);

        error = error - deltay;
        if (error < 0)
        {
            y = y + ystep;
            error = error + deltax;
        }
    }
}

void ks0108_rect(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    ks0108_hline(disp, x, y, width, color);
    ks0108_hline(disp, x, y+height-1, width, color);
    ks0108_vline(disp, x, y, height, color);
    ks0108_vline(disp, x+width-1, y, height, color);
}

void ks0108_rect_fill(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (width--)
    {
        if (color == KS0108_CHESS)
        {
            if ((i % 2) == 0) tmp = KS0108_BLACK_WHITE;
            else tmp = KS0108_WHITE_BLACK;
        }
        i++;

        ks0108_vline(disp, x++, y, height, tmp);
    }
}

void ks0108_rect_invert(ks0108_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
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

void ks0108_draw_image(ks0108_t *disp, uint8_t x, uint8_t y, const uint8_t *image, uint8_t color)
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
            if (color == KS0108_WHITE) data = ~data;
            WRITE_BUFFER(disp, x+i, y+j, data);
        }
    }
}

void ks0108_text(ks0108_t *disp, uint8_t x, uint8_t y, const char *text, const uint8_t *font, uint8_t color)
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

        // draws each character piece
        for (j = 0; j < bytes; j++)
        {
            x_tmp = x;
            page = j * width;

            // draws the character
            for (i = 0; i < width; i++)
            {
                data = font[char_data_index + page + i];
                if (color == KS0108_WHITE) data = ~data;

                // if is the last piece of character...
                if (height > 8 && height < (j + 1) * 8)
                {
                    data >>= ((j + 1) * 8) - height;
                    data = read_data(disp, x_tmp, y_tmp) | data;
                }

                write_data(disp, x_tmp, y_tmp, data);

                x_tmp++;
            }

            // draws the interchar space
            data = (color == KS0108_BLACK ? 0x00 : 0xFF);
            if (height > 8 && height < (j + 1) * 8)
            {
                data >>= ((j + 1) * 8) - height;
                data = read_data(disp, x_tmp, y_tmp) | data;
            }

            if (*(text + 1) != '\0')
            {
                write_data(disp, x_tmp, y_tmp, data);
            }

            y_tmp += 8;
        }

        x += width + 1;

        text++;
    }
}

void ks0108_update(ks0108_t *disp)
{
    uint8_t chip, page, x;

#ifndef NO_SELECTOR
    selector_channel(disp->id);
#endif

    for (chip = 0 ; chip < CHIP_COUNT; chip++)
    {
        for (page = 0; page < (DISPLAY_HEIGHT/8); page++)
        {
            write_instruction(disp, chip, SET_PAGE | page);
            write_instruction(disp, chip, SET_ADDRESS);

            for (x = 0; x < CHIP_WIDTH; x++)
            {
                // write data on display
                chip_select(disp, chip);
                WRITE_DATA_MODE(disp);
                WRITE_PORT(disp->data_bus_port, disp->buffer[page][x + (chip*CHIP_WIDTH)]);
                DELAY_ns(DELAY_DSW);
                ENABLE_PULSE(disp);
            }
        }
    }
}
