
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

#include "st7565p.h"
#include "fonts.h"
#include "images.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

st7565p_t *g_disp_pointer;

#define DISPLAY_ON()           Write_Instruction(g_disp_pointer, 0xaf)   //  Display on
#define DISPLAY_OFF()          Write_Instruction(g_disp_pointer, 0xae)   //  Display off
#define SET_ADC()              Write_Instruction(g_disp_pointer, 0xa1)   //  Reverse disrect (SEG131-SEG0)
#define CLEAR_ADC()            Write_Instruction(g_disp_pointer, 0xa0)   //  Normal disrect (SEG0-SEG131)
#define REVERSE_DISPLAY_ON()   Write_Instruction(g_disp_pointer, 0xa7)   //  Reverse display : 0 illuminated
#define REVERSE_DISPLAY_OFF()  Write_Instruction(g_disp_pointer, 0xa6)   //  Normal display : 1 illuminated
#define ENTIRE_DISPLAY_ON()    Write_Instruction(g_disp_pointer, 0xa5)   //  Entire dislay   Force whole LCD point
#define ENTIRE_DISPLAY_OFF()   Write_Instruction(g_disp_pointer, 0xa4)   //  Normal display
#define SET_BIAS()             Write_Instruction(g_disp_pointer, 0xa3)   //  bias 1
#define CLEAR_BIAS()           Write_Instruction(g_disp_pointer, 0xa2)   //  bias 0
#define SET_MODIFY_READ()      Write_Instruction(g_disp_pointer, 0xe0)   //  Stop automatic increment of the column address by the read instruction 
#define RESET_MODIFY_READ()    Write_Instruction(g_disp_pointer, 0xee)   //  Cancel Modify_read, column address return to its initial value just before the Set Modify Read instruction is started
#define RESET()                Write_Instruction(g_disp_pointer, 0xe2)
#define SET_SHL()              Write_Instruction(g_disp_pointer, 0xc8)   // SHL 1,COM63-COM0
#define CLEAR_SHL()            Write_Instruction(g_disp_pointer, 0xc0)   // SHL 0,COM0-COM63

#define Start_column    0x00
#define Start_page      0x00
#define StartLine_set   0x00

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
#define CS_ON(disp)                    CLR_PIN(disp->cs_port, disp->cs_pin)
#define CS_OFF(disp)                   SET_PIN(disp->cs_port, disp->cs_pin)

// buffer macros
#define READ_BUFFER(disp,x,y)           disp->buffer[(y/8)][x]
#define WRITE_BUFFER(disp,x,y,data)     disp->buffer[(y/8)][x] = (data)

// backlight macros
#if defined ST7565P_BACKLIGHT_TURN_ON_WITH_ONE
#define BACKLIGHT_TURN_ON(port, pin)    SET_PIN(port, pin)
#define BACKLIGHT_TURN_OFF(port, pin)   CLR_PIN(port, pin)
#elif defined ST7565P_BACKLIGHT_TURN_ON_WITH_ZERO
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

static void write_to_display(data_port_t bus, uint8_t byte)
{
    (byte & 0x01) ? SET_PIN(bus.port, bus.pin1) : CLR_PIN(bus.port, bus.pin1);
    (byte & 0x02) ? SET_PIN(bus.port, bus.pin2) : CLR_PIN(bus.port, bus.pin2);
    (byte & 0x04) ? SET_PIN(bus.port, bus.pin3) : CLR_PIN(bus.port, bus.pin3);
    (byte & 0x08) ? SET_PIN(bus.port, bus.pin4) : CLR_PIN(bus.port, bus.pin4);
    (byte & 0x10) ? SET_PIN(bus.port, bus.pin5) : CLR_PIN(bus.port, bus.pin5);
    (byte & 0x20) ? SET_PIN(bus.port, bus.pin6) : CLR_PIN(bus.port, bus.pin6);
    (byte & 0x40) ? SET_PIN(bus.port, bus.pin7) : CLR_PIN(bus.port, bus.pin7);
    (byte & 0x80) ? SET_PIN(bus.port, bus.pin8) : CLR_PIN(bus.port, bus.pin8);
}

void Write_Data(st7565p_t *disp, uint8_t dat)
{
    SET_PIN(disp->a0_port, disp->a0_pin);
    write_to_display(disp->data_bus_port, dat);
    CLR_PIN(disp->write_port, disp->write_pin);
    delay_us(2);
    SET_PIN(disp->write_port, disp->write_pin);
}   

void Write_Instruction(st7565p_t *disp, uint8_t cmd)
{ 
    CLR_PIN(disp->a0_port, disp->a0_pin);
    write_to_display(disp->data_bus_port, cmd);
    CLR_PIN(disp->write_port, disp->write_pin);
    delay_us(2);
    SET_PIN(disp->write_port, disp->write_pin);
}

static uint8_t read_data(st7565p_t *disp, uint8_t x, uint8_t y)
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

static void write_data(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t data)
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

//Specify DDRAM line for COM0 0~63
void Initial_Dispay_Line(st7565p_t *disp, unsigned char line)
{
    line|=0x40;
    Write_Instruction(disp, line);
    return;
}

// Set page address 0~15
void Set_Page_Address(st7565p_t *disp, unsigned char add)
{
    add=0xb0|add;
    Write_Instruction(disp, add);
    return;
}

void Set_Column_Address(st7565p_t *disp, unsigned char add)
{
    Write_Instruction(disp, (0x10|(add>>4)));
    Write_Instruction(disp, (0x0f&add));
    return;
}

void Power_Control(st7565p_t *disp, unsigned char vol)
{
    Write_Instruction(disp, (0x28|vol));
    return;
}

void Regulor_Resistor_Select(st7565p_t *disp, unsigned char r)
{
    Write_Instruction(disp, (0x20|r));
    return;
}

//a(0-63) 32default   Vev=(1-(63-a)/162)Vref   2.1v
void Set_Contrast_Control_Register(st7565p_t *disp, unsigned char mod)
{
    Write_Instruction(disp, 0x81);
    Write_Instruction(disp, mod);
    return;
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void st7565p_init(st7565p_t *disp)
{
    disp->id = g_id++;

    g_disp_pointer = disp;

#ifndef NO_SELECTOR
    // initializes the selector
    selector_init(GLCD_COUNT, (const uint8_t [])SELECTOR_DIR_PINS, (const uint8_t [])SELECTOR_CHANNELS_PINS);
    selector_direction(SELECTOR_OUTPUT_MODE);
    selector_channel(disp->id);
#endif

    //set pin functions
    PINSEL_SetPinFunc(disp->rst_port, disp->rst_pin, 0);
    PINSEL_SetPinFunc(disp->write_port, disp->write_pin, 0);
    PINSEL_SetPinFunc(disp->read_port, disp->read_pin, 0);
    PINSEL_SetPinFunc(disp->a0_port, disp->a0_pin, 0);
    PINSEL_SetPinFunc(disp->cs_port, disp->cs_pin, 0);
    PINSEL_SetPinFunc(disp->data_bus_port.port, disp->data_bus_port.pin1, 0);
    PINSEL_SetPinFunc(disp->data_bus_port.port, disp->data_bus_port.pin2, 0);
    PINSEL_SetPinFunc(disp->data_bus_port.port, disp->data_bus_port.pin3, 0);
    PINSEL_SetPinFunc(disp->data_bus_port.port, disp->data_bus_port.pin4, 0);
    PINSEL_SetPinFunc(disp->data_bus_port.port, disp->data_bus_port.pin5, 0);
    PINSEL_SetPinFunc(disp->data_bus_port.port, disp->data_bus_port.pin6, 0);
    PINSEL_SetPinFunc(disp->data_bus_port.port, disp->data_bus_port.pin7, 0);
    PINSEL_SetPinFunc(disp->data_bus_port.port, disp->data_bus_port.pin8, 0);

    // direction pins configuration
    CONFIG_PIN_OUTPUT(disp->rst_port, disp->rst_pin);
    CONFIG_PIN_OUTPUT(disp->write_port, disp->write_pin);
    CONFIG_PIN_OUTPUT(disp->read_port, disp->read_pin);
    CONFIG_PIN_OUTPUT(disp->a0_port, disp->a0_pin);
    CONFIG_PIN_OUTPUT(disp->cs_port, disp->cs_pin);
    CONFIG_PIN_OUTPUT(disp->data_bus_port.port, disp->data_bus_port.pin1);
    CONFIG_PIN_OUTPUT(disp->data_bus_port.port, disp->data_bus_port.pin2);
    CONFIG_PIN_OUTPUT(disp->data_bus_port.port, disp->data_bus_port.pin3);
    CONFIG_PIN_OUTPUT(disp->data_bus_port.port, disp->data_bus_port.pin4);
    CONFIG_PIN_OUTPUT(disp->data_bus_port.port, disp->data_bus_port.pin5);
    CONFIG_PIN_OUTPUT(disp->data_bus_port.port, disp->data_bus_port.pin6);
    CONFIG_PIN_OUTPUT(disp->data_bus_port.port, disp->data_bus_port.pin7);
    CONFIG_PIN_OUTPUT(disp->data_bus_port.port, disp->data_bus_port.pin8);

    //we do not use the read funciton
    SET_PIN(disp->read_port, disp->read_pin);

    // backlight configuration
    CONFIG_PIN_OUTPUT(disp->backlight_port, disp->backlight_pin);
    BACKLIGHT_TURN_ON(disp->backlight_port, disp->backlight_pin);

    //clear chip select pin
    CLR_PIN(disp->cs_port, disp->cs_pin);

    //send reset display controller command
    RESET();

    delay_us(100);

    //set disp controller reset pin
    SET_PIN(disp->rst_port, disp->rst_pin);

    delay_us(100);

    //make dispay reset pin low
    CLR_PIN(disp->rst_port, disp->rst_pin);

    //set disp controller reset pin, just in case ;) 
    SET_PIN(disp->rst_port, disp->rst_pin);

    //clear chip select pin
    CLR_PIN(disp->cs_port, disp->cs_pin);

    //clear internal disp adc
    CLEAR_ADC();

    //set
    SET_SHL();

    //clear any bias values
    CLEAR_BIAS();

    //set disp power reg
    Power_Control(disp, 0x07);

    //set contrast resistor val
    Regulor_Resistor_Select(disp, 0x05);

    //set contrast control
    Set_Contrast_Control_Register(disp, ST7565_DEFAULT_CONTRAST);
    
    //start at the begining :)
    Initial_Dispay_Line(disp, 0x00);

    //ready to rock
    DISPLAY_ON();

    //we use it 'normally'
    REVERSE_DISPLAY_OFF();
    ENTIRE_DISPLAY_OFF();

    st7565p_clear(disp, ST7565P_WHITE);
    st7565p_update(disp);
}

void st7565p_backlight(st7565p_t *disp, uint8_t state)
{
    if (state)
        BACKLIGHT_TURN_ON(disp->backlight_port, disp->backlight_pin);
    else
        BACKLIGHT_TURN_OFF(disp->backlight_port, disp->backlight_pin);
}

void st7565p_clear(st7565p_t *disp, uint8_t color)
{
    st7565p_rect_fill(disp, 0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, color);
}

void st7565p_set_pixel(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t color)
{
    // avoid x, y be out of the bounds
    x %= DISPLAY_WIDTH;
    y %= DISPLAY_HEIGHT;

    uint8_t data;

    data = READ_BUFFER(disp, x, y);
    if (color == ST7565P_BLACK) data |= (0x01 << (y % 8));
    else data &= ~(0x01 << (y % 8));
    WRITE_BUFFER(disp, x, y, data);
}

void st7565p_hline(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (width--)
    {
        if (color == ST7565P_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = ST7565P_BLACK;
            else tmp = ST7565P_WHITE;
        }
        else if (color == ST7565P_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = ST7565P_WHITE;
            else tmp = ST7565P_BLACK;
        }
        i++;

        st7565p_set_pixel(disp, x++, y, tmp);
    }
}

void st7565p_vline(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (height--)
    {
        if (color == ST7565P_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = ST7565P_BLACK;
            else tmp = ST7565P_WHITE;
        }
        else if (color == ST7565P_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = ST7565P_WHITE;
            else tmp = ST7565P_BLACK;
        }
        i++;

        st7565p_set_pixel(disp, x, y++, tmp);
    }
}

void st7565p_line(st7565p_t *disp, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
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
        if (color == ST7565P_BLACK_WHITE)
        {
            if ((i % 2) == 0) tmp = ST7565P_BLACK;
            else tmp = ST7565P_WHITE;
        }
        else if (color == ST7565P_WHITE_BLACK)
        {
            if ((i % 2) == 0) tmp = ST7565P_WHITE;
            else tmp = ST7565P_BLACK;
        }
        i++;

        if (steep) st7565p_set_pixel(disp, y, x, tmp);
        else st7565p_set_pixel(disp, x, y, tmp);

        error = error - deltay;
        if (error < 0)
        {
            y = y + ystep;
            error = error + deltax;
        }
    }
}

void st7565p_rect(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    st7565p_hline(disp, x, y, width, color);
    st7565p_hline(disp, x, y+height-1, width, color);
    st7565p_vline(disp, x, y, height, color);
    st7565p_vline(disp, x+width-1, y, height, color);
}

void st7565p_rect_fill(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    uint8_t i = 0, tmp = color;

    while (width--)
    {
        if (color == ST7565P_CHESS)
        {
            if ((i % 2) == 0) tmp = ST7565P_BLACK_WHITE;
            else tmp = ST7565P_WHITE_BLACK;
        }
        i++;

        st7565p_vline(disp, x++, y, height, tmp);
    }
}

void st7565p_rect_invert(st7565p_t *disp, uint8_t x, uint8_t y, uint8_t width, uint8_t height)
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

void st7565p_draw_image(st7565p_t *disp, uint8_t x, uint8_t y, const uint8_t *image, uint8_t color)
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
            if (color == ST7565P_WHITE) data = ~data;
            WRITE_BUFFER(disp, x+i, y+j, data);
        }
    }
}

void st7565p_text(st7565p_t *disp, uint8_t x, uint8_t y, const char *text, const uint8_t *font, uint8_t color)
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
                if (color == ST7565P_WHITE) data = ~data;

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
            data = (color == ST7565P_BLACK ? 0x00 : 0xFF);
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

void st7565p_update(st7565p_t *disp)
{
    uint8_t page, x;

#ifndef NO_SELECTOR
    selector_channel(disp->id);
#endif
        for (page = 0; page < (DISPLAY_HEIGHT/8); page++)
        {
            Set_Page_Address(disp, page);
            Set_Column_Address(disp, 0x00);

            for (x = 0; x < CHIP_WIDTH; x++)
            {
                uint8_t byte_to_send = disp->buffer[page][x]; 
                // write data on display
                Write_Data(disp, byte_to_send);
            }
        }
    return;
}

void st7565p_set_contrast(st7565p_t *disp, uint8_t contrast)
{
    //set contrast control
    Set_Contrast_Control_Register(disp, contrast);
}