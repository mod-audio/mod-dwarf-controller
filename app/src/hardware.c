
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "hardware.h"
#include "config.h"
#include "utils.h"
#include "ledz.h"
#include "serial.h"
#include "actuator.h"
#include "task.h"
#include "device.h"
#include "st7565p.h"


/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

// check in hardware_setup() what is the function of each timer
#define TIMER0_PRIORITY     3
#define TIMER1_PRIORITY     2

/*
************************************************************************************************************************
*           LOCAL CONSTANTS
************************************************************************************************************************
*/

//// Dynamic menory allocation
// defines the heap size
const HeapRegion_t xHeapRegions[] =
{
    { ( uint8_t * ) 0x10007000, 0x1000 },
    { ( uint8_t * ) 0x20000000, 0x8000 },
    { NULL, 0 } /* Terminates the array. */
};

static const int *LED_PINS[]  = {
#ifdef LED0_PINS
    (const int []) LED0_PINS,
#endif
#ifdef LED1_PINS
    (const int []) LED1_PINS,
#endif
#ifdef LED2_PINS
    (const int []) LED2_PINS,
#endif
#ifdef LED3_PINS
    (const int []) LED3_PINS,
#endif
#ifdef LED4_PINS
    (const int []) LED4_PINS,
#endif
#ifdef LED5_PINS
    (const int []) LED5_PINS,
#endif
#ifdef LED6_PINS
    (const int []) LED6_PINS,
#endif
};

static const uint8_t *ENCODER_PINS[] = {
#ifdef ENCODER0_PINS
    (const uint8_t []) ENCODER0_PINS,
#endif
#ifdef ENCODER1_PINS
    (const uint8_t []) ENCODER1_PINS,
#endif
#ifdef ENCODER2_PINS
    (const uint8_t []) ENCODER2_PINS,
#endif
#ifdef ENCODER3_PINS
    (const uint8_t []) ENCODER3_PINS
#endif
};

static const uint8_t *FOOTSWITCH_PINS[] = {
#ifdef FOOTSWITCH0_PINS
    (const uint8_t []) FOOTSWITCH0_PINS,
#endif
#ifdef FOOTSWITCH1_PINS
    (const uint8_t []) FOOTSWITCH1_PINS,
#endif
#ifdef FOOTSWITCH2_PINS
    (const uint8_t []) FOOTSWITCH2_PINS,
#endif
#ifdef FOOTSWITCH3_PINS
    (const uint8_t []) FOOTSWITCH3_PINS,
#endif
#ifdef FOOTSWITCH4_PINS
    (const uint8_t []) FOOTSWITCH4_PINS,
#endif
#ifdef FOOTSWITCH5_PINS
    (const uint8_t []) FOOTSWITCH5_PINS,
#endif
#ifdef FOOTSWITCH6_PINS
    (const uint8_t []) FOOTSWITCH6_PINS
#endif
};

static const uint8_t *BUTTON_PINS[] = {
#ifdef BUTTON0_PINS
    (const uint8_t []) BUTTON0_PINS,
#endif
#ifdef BUTTON1_PINS
    (const uint8_t []) BUTTON1_PINS,
#endif
#ifdef BUTTON2_PINS
    (const uint8_t []) BUTTON2_PINS,
#endif
#ifdef BUTTON3_PINS
    (const uint8_t []) BUTTON3_PINS,
#endif
#ifdef BUTTON4_PINS
    (const uint8_t []) BUTTON4_PINS,
#endif
#ifdef BUTTON5_PINS
    (const uint8_t []) BUTTON5_PINS,
#endif
#ifdef BUTTON6_PINS
    (const uint8_t []) BUTTON6_PINS
#endif
};

static const uint8_t *LED_COLORS[]  = {
#ifdef DEFAULT_TOGGLED_COLOR
    (const uint8_t []) DEFAULT_TOGGLED_COLOR,
#endif
#ifdef DEFAULT_TRIGGER_COLOR
    (const uint8_t []) DEFAULT_TRIGGER_COLOR,
#endif
#ifdef DEFAULT_TRIGGER_PRESSED_COLOR
    (const uint8_t []) DEFAULT_TRIGGER_PRESSED_COLOR,
#endif
#ifdef DEFAULT_TAP_TEMPO_COLOR
    (const uint8_t []) DEFAULT_TAP_TEMPO_COLOR,
#endif
#ifdef DEFAULT_ENUMERATED_COLOR
    (const uint8_t []) DEFAULT_ENUMERATED_COLOR,
#endif
#ifdef DEFAULT_ENUMERATED_PRESSED_COLOR
    (const uint8_t []) DEFAULT_ENUMERATED_PRESSED_COLOR,
#endif
#ifdef DEFAULT_BYPASS_COLOR
    (const uint8_t []) DEFAULT_BYPASS_COLOR,
#endif
#ifdef DEFAULT_SNAPSHOT_COLOR
    (const uint8_t []) DEFAULT_SNAPSHOT_COLOR,
#endif
#ifdef DEFAULT_SNAPSHOT_LOAD_COLOR
    (const uint8_t []) DEFAULT_SNAPSHOT_LOAD_COLOR,
#endif
#ifdef DEFAULT_LED_LIST_COLOR_1
    (const uint8_t []) DEFAULT_LED_LIST_COLOR_1,
#endif
#ifdef DEFAULT_LED_LIST_COLOR_2
    (const uint8_t []) DEFAULT_LED_LIST_COLOR_2,
#endif
#ifdef DEFAULT_LED_LIST_COLOR_3
    (const uint8_t []) DEFAULT_LED_LIST_COLOR_3,
#endif
#ifdef DEFAULT_LED_LIST_COLOR_4
    (const uint8_t []) DEFAULT_LED_LIST_COLOR_4,
#endif
#ifdef DEFAULT_LED_LIST_COLOR_5
    (const uint8_t []) DEFAULT_LED_LIST_COLOR_5,
#endif
#ifdef DEFAULT_LED_LIST_COLOR_6
    (const uint8_t []) DEFAULT_LED_LIST_COLOR_6,
#endif
#ifdef DEFAULT_LED_LIST_COLOR_7
    (const uint8_t []) DEFAULT_LED_LIST_COLOR_7,
#endif
#ifdef DEFAULT_FS_PAGE_COLOR_1
    (const uint8_t []) DEFAULT_FS_PAGE_COLOR_1,
#endif
#ifdef DEFAULT_FS_PAGE_COLOR_2
    (const uint8_t []) DEFAULT_FS_PAGE_COLOR_2,
#endif
#ifdef DEFAULT_FS_PAGE_COLOR_3
    (const uint8_t []) DEFAULT_FS_PAGE_COLOR_3,
#endif
#ifdef DEFAULT_FS_PAGE_COLOR_4
    (const uint8_t []) DEFAULT_FS_PAGE_COLOR_4,
#endif
#ifdef DEFAULT_FS_PAGE_COLOR_5
    (const uint8_t []) DEFAULT_FS_PAGE_COLOR_5,
#endif
#ifdef DEFAULT_FS_PAGE_COLOR_6
    (const uint8_t []) DEFAULT_FS_PAGE_COLOR_6,
#endif
#ifdef DEFAULT_FS_PAGE_COLOR_7
    (const uint8_t []) DEFAULT_FS_PAGE_COLOR_7,
#endif
#ifdef DEFAULT_FS_PAGE_COLOR_8
    (const uint8_t []) DEFAULT_FS_PAGE_COLOR_8,
#endif
#ifdef DEFAULT_FS_PB_MENU
    (const uint8_t []) DEFAULT_FS_PB_MENU,
#endif
#ifdef DEFAULT_FS_SS_MENU
    (const uint8_t []) DEFAULT_FS_SS_MENU,
#endif
#ifdef DEFAULT_ENCODER_PAGE_COLOR
    (const uint8_t []) DEFAULT_ENCODER_PAGE_COLOR,
#endif
};

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

#define ABS(x)                  ((x) > 0 ? (x) : -(x))

#define CPU_IS_ON()             (1 - ((FIO_ReadValue(CPU_STATUS_PORT) >> CPU_STATUS_PIN) & 1))
#define CPU_PULSE_BUTTON()      GPIO_ClearValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));    \
                                delay_ms(200);                                              \
                                GPIO_SetValue(CPU_BUTTON_PORT, (1 << CPU_BUTTON_PIN));


/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static glcd_t g_glcd;
static serial_t g_serial[SERIAL_COUNT];
static ledz_t *g_leds[LEDS_COUNT];
static encoder_t g_encoders[ENCODERS_COUNT];
static button_t g_footswitches[FOOTSWITCHES_COUNT];
static button_t g_buttons[BUTTONS_COUNT];
static uint32_t g_counter;
static int g_brightness;


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

void write_led_defaults()
{
    uint8_t eeprom_index, eeprom_page;
    uint8_t i = 0;
    uint8_t j=0;
    uint8_t write_buffer = 0;

    eeprom_page = LED_COLOR_EEMPROM_PAGE;
    eeprom_index = 0;

    for (i=0; i<(MAX_COLOR_ID-1); i++)
    {   
        //second eeprom page
        if (eeprom_index > 20)
        {
            eeprom_page++;
            eeprom_index = 0;
        }

        for (j=0; j<3; j++)
        {
            write_buffer = LED_COLORS[i][j];
            EEPROM_Write(eeprom_page, (LED_COLOR_ADRESS_START + (eeprom_index*3) + j), &write_buffer, MODE_8_BIT, 1);    
        }

        eeprom_index++;
    }

    //write LED brightness
    uint8_t led_brightness_write_buffer = DEFAULT_LED_BRIGHTNESS;
    EEPROM_Write(0, LED_BRIGHTNESS_ADRESS, &led_brightness_write_buffer, MODE_8_BIT, 1);
}

void write_o_settings_defaults()
{
    //set the eeprom default values
    uint8_t write_buffer = ST7565_DEFAULT_CONTRAST;
    EEPROM_Write(0, DISPLAY_CONTRAST_ADRESS, &write_buffer, MODE_8_BIT, 1);
    write_buffer = DEFAULT_DISPLAY_BRIGHTNESS;
    EEPROM_Write(0, DISPLAY_BRIGHTNESS_ADRESS, &write_buffer, MODE_8_BIT, 1);
    write_buffer = DEFAULT_HIDE_ACTUATOR;
    EEPROM_Write(0, HIDE_ACTUATOR_ADRESS, &write_buffer, MODE_8_BIT, 1);
}

void check_eeprom_defaults(uint16_t current_version)
{
    //if not force update, and not downgrading, check defaults
    if ((!FORCE_WRITE_EEPROM) && (current_version < EEPROM_CURRENT_VERSION))
    {
        switch (current_version)
        {
            //everything before 1.10
            case 0:
                write_o_settings_defaults();
                write_led_defaults();
            break;

            //nothing saved yet, new unit, write all settings
            default:
                write_o_settings_defaults();
                write_led_defaults();
            break;
        }
    }
    //detect downgrade, dont do anything
    else if ((current_version > EEPROM_CURRENT_VERSION) && (!FORCE_WRITE_EEPROM))
    {   
        return;
    }
    //force defaults
    else 
    {
        //write all settings
        write_o_settings_defaults();
        write_led_defaults();
    }

    //update the version 
    uint16_t write_buffer_version = EEPROM_CURRENT_VERSION;
    EEPROM_Write(0, EEPROM_VERSION_ADRESS, &write_buffer_version, MODE_16_BIT, 1);

    return;
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

//MWD_TODO EMBED EEPROM MEMORY MENAGER

void hardware_setup(void)
{
    // set system core clock
    SystemCoreClockUpdate();

    // configure the peripherals power
    CLKPWR_ConfigPPWR(HW_CLK_PWR_CONTROL, ENABLE);

    //Pass the array into vPortDefineHeapRegions().
    vPortDefineHeapRegions(xHeapRegions);

    // configure and set initial state of shutdown cpu button
    // note: CLR_PIN will make the coreboard reboot unless SET_PIN is called in less than 5s
    //       this is done in the beginning of cli_task()
    //CONFIG_PIN_OUTPUT(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
    //CLR_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);

    EEPROM_Init();

    //check if this unit is being turned on for the first time (empty EEPROM)
    uint16_t read_eeprom_version = 0;
    EEPROM_Read(0, EEPROM_VERSION_ADRESS, &read_eeprom_version, MODE_16_BIT, 1);

    //check if value is the same (we put that in the last page to detect new units (binary 10101010))
    if ((read_eeprom_version != EEPROM_CURRENT_VERSION) || FORCE_WRITE_EEPROM)
    {
        check_eeprom_defaults(read_eeprom_version);
    }

    uint8_t i;

    //init LEDS
    //set 3 color leds
    const ledz_color_t colors[] = {RED, GREEN, BLUE};

    for (i = 0; i < LEDS_COUNT; i++)
    {
        // LEDs initialization
        g_leds[i] = ledz_create(LEDZ_3COLOR, colors, LED_PINS[i]);
    }

    //MDW_TODO Fix this init, can be way cleaner

    // GLCD initialization
    g_glcd.id = 0;
    g_glcd.cs_port = GLCD0_CS_PORT;
    g_glcd.cs_pin = GLCD0_CS_PIN;
    g_glcd.a0_port = GLCD0_RS_PORT;
    g_glcd.a0_pin = GLCD0_RS_PIN;
    g_glcd.read_port = GLCD0_E_RD_PORT;
    g_glcd.read_pin = GLCD0_E_RD_PIN;
    g_glcd.write_port = GLCD0_RW_WR_PORT;
    g_glcd.write_pin = GLCD0_RW_WR_PIN;
    g_glcd.rst_port = GLCD0_RST_PORT;
    g_glcd.rst_pin = GLCD0_RST_PIN;
    g_glcd.backlight_port = GLCD0_BKL_PORT;
    g_glcd.backlight_pin = GLCD0_BKL_PIN;
    g_glcd.data_bus_port.port = GLCD0_DATA_PORT;
    g_glcd.data_bus_port.pin1 = GLCD0_DATA_PIN1;
    g_glcd.data_bus_port.pin2 = GLCD0_DATA_PIN2;
    g_glcd.data_bus_port.pin3 = GLCD0_DATA_PIN3;
    g_glcd.data_bus_port.pin4 = GLCD0_DATA_PIN4;
    g_glcd.data_bus_port.pin5 = GLCD0_DATA_PIN5;
    g_glcd.data_bus_port.pin6 = GLCD0_DATA_PIN6;
    g_glcd.data_bus_port.pin7 = GLCD0_DATA_PIN7;
    g_glcd.data_bus_port.pin8 = GLCD0_DATA_PIN8;
    glcd_init(&g_glcd);

    //init encoders
    for (i = 0; i < ENCODERS_COUNT; i++)
    {  
        // actuators creation
        actuator_create(ROTARY_ENCODER, i, hardware_actuators(ENCODER0 + i));

        // actuators pins configuration
        actuator_set_pins(hardware_actuators(ENCODER0 + i), ENCODER_PINS[i]);

        // actuators properties
        actuator_set_prop(hardware_actuators(ENCODER0 + i), ENCODER_STEPS, 4);
        actuator_set_prop(hardware_actuators(ENCODER0 + i), BUTTON_HOLD_TIME, TOOL_MODE_TIME);
    }

    //MDW_TODO check switch properties (hold timeout, double press)

    //init foots
    for (i = 0; i < FOOTSWITCHES_COUNT; i++)
    {  
        // actuators creation
        actuator_create(BUTTON, i, hardware_actuators(FOOTSWITCH0 + i));

        // actuators pins configuration
        actuator_set_pins(hardware_actuators(FOOTSWITCH0 + i), FOOTSWITCH_PINS[i]);
    
        //set links for double press
        if (i > 0)
        {
            actuator_set_prop(hardware_actuators(FOOTSWITCH0 + i), BUTTON_DOUBLE_TIME, BUTTON_DOUBLE_PRESS_DEBOUNCE);
            actuator_set_link(hardware_actuators(FOOTSWITCH0 + i), FOOTSWITCH0);
        }
    }

    //init buttons
    for (i = 0; i < BUTTONS_COUNT; i++)
    {  
        // actuators creation
        actuator_create(BUTTON, i + FOOTSWITCHES_COUNT, hardware_actuators(BUTTON0 + i));

        // actuators pins configuration
        actuator_set_pins(hardware_actuators(BUTTON0 + i), BUTTON_PINS[i]);
    }

    // default glcd brightness
    g_brightness = MAX_BRIGHTNESS;

    //set the display contrast
    uint8_t display_contrast = 0;
    EEPROM_Read(0, DISPLAY_CONTRAST_ADRESS, &display_contrast, MODE_8_BIT, 1);
    st7565p_set_contrast(hardware_glcds(0), display_contrast);

    //set led colors
    uint8_t led_color_value[3] = {};
    uint8_t eeprom_index, eeprom_page;
    eeprom_page = LED_COLOR_EEMPROM_PAGE;
    eeprom_index = 0;
    for (i=0; i<(MAX_COLOR_ID-1); i++)
    {
        //second eeprom page
        if (eeprom_index > 20)
        {
            eeprom_page++;
            eeprom_index = 0;
        }

        uint8_t j=0;
        uint8_t read_buffer = 0;
        for (j=0; j<3; j++)
        {
            EEPROM_Read(eeprom_page, (LED_COLOR_ADRESS_START + (eeprom_index*3) + j), &read_buffer, MODE_8_BIT, 1);
            led_color_value[j] = read_buffer;
        }
        ledz_set_color(i, led_color_value);
    
        eeprom_index++;
    }

    //set white color for dissables
    led_color_value[0] = 100;
    led_color_value[1] = 100;
    led_color_value[2] = 100;
    ledz_set_color(MAX_COLOR_ID, led_color_value);

    ////////////////////////////////////////////////////////////////
    // Timer 0 configuration
    // this timer is used to LEDs PWM

    // timer structs declaration
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct ;
    // initialize timer 0, prescale count time of 10us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = LED_INTERUPT_TIME;
    // use channel 0, MR0
    TIM_MatchConfigStruct.MatchChannel = 0;
    // enable interrupt when MR0 matches the value in TC register
    TIM_MatchConfigStruct.IntOnMatch = TRUE;
    // enable reset on MR0: TIMER will reset if MR0 matches it
    TIM_MatchConfigStruct.ResetOnMatch = TRUE;
    // stop on MR0 if MR0 matches it
    TIM_MatchConfigStruct.StopOnMatch = FALSE;
    // set Match value, count value of 1 (1 * 10us = 10us --> 100 kHz)
    TIM_MatchConfigStruct.MatchValue = 1;
    // set configuration for Tim_config and Tim_MatchConfig
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);
    TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
    // set priority
    NVIC_SetPriority(TIMER0_IRQn, TIMER0_PRIORITY);
    // enable interrupt for timer 0
    NVIC_EnableIRQ(TIMER0_IRQn);
    // to start timer
    TIM_Cmd(LPC_TIM0, ENABLE);

    ////////////////////////////////////////////////////////////////
    // Timer 1 configuration
    // this timer is for actuators clock and timestamp

    // initialize timer 1, prescale count time of 500us
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = 500;
    // use channel 1, MR1
    TIM_MatchConfigStruct.MatchChannel = 1;
    // enable interrupt when MR1 matches the value in TC register
    TIM_MatchConfigStruct.IntOnMatch = TRUE;
    // enable reset on MR1: TIMER will reset if MR1 matches it
    TIM_MatchConfigStruct.ResetOnMatch = TRUE;
    // stop on MR1 if MR1 matches it
    TIM_MatchConfigStruct.StopOnMatch = FALSE;
    // set Match value, count value of 1
    TIM_MatchConfigStruct.MatchValue = 1;
    // set configuration for Tim_config and Tim_MatchConfig
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &TIM_ConfigStruct);
    TIM_ConfigMatch(LPC_TIM1, &TIM_MatchConfigStruct);
    // set priority
    NVIC_SetPriority(TIMER1_IRQn, TIMER1_PRIORITY);
    // enable interrupt for timer 1
    NVIC_EnableIRQ(TIMER1_IRQn);
    // to start timer
    TIM_Cmd(LPC_TIM1, ENABLE);

    ////////////////////////////////////////////////////////////////
    // Serial initialization

    #ifdef SERIAL0
    g_serial[0].uart_id = 0;
    g_serial[0].baud_rate = SERIAL0_BAUD_RATE;
    g_serial[0].priority = SERIAL0_PRIORITY;
    g_serial[0].rx_port = SERIAL0_RX_PORT;
    g_serial[0].rx_pin = SERIAL0_RX_PIN;
    g_serial[0].rx_function = SERIAL0_RX_FUNC;
    g_serial[0].rx_buffer_size = SERIAL0_RX_BUFF_SIZE;
    g_serial[0].tx_port = SERIAL0_TX_PORT;
    g_serial[0].tx_pin = SERIAL0_TX_PIN;
    g_serial[0].tx_function = SERIAL0_TX_FUNC;
    g_serial[0].tx_buffer_size = SERIAL0_TX_BUFF_SIZE;
    g_serial[0].has_oe = SERIAL0_HAS_OE;
    #if SERIAL0_HAS_OE
    g_serial[0].oe_port = SERIAL0_OE_PORT;
    g_serial[0].oe_pin = SERIAL0_OE_PIN;
    #endif
    serial_init(&g_serial[0]);
    #endif

    #ifdef SERIAL1
    g_serial[1].uart_id = 1;
    g_serial[1].baud_rate = SERIAL1_BAUD_RATE;
    g_serial[1].priority = SERIAL1_PRIORITY;
    g_serial[1].rx_port = SERIAL1_RX_PORT;
    g_serial[1].rx_pin = SERIAL1_RX_PIN;
    g_serial[1].rx_function = SERIAL1_RX_FUNC;
    g_serial[1].rx_buffer_size = SERIAL1_RX_BUFF_SIZE;
    g_serial[1].tx_port = SERIAL1_TX_PORT;
    g_serial[1].tx_pin = SERIAL1_TX_PIN;
    g_serial[1].tx_function = SERIAL1_TX_FUNC;
    g_serial[1].tx_buffer_size = SERIAL1_TX_BUFF_SIZE;
    g_serial[1].has_oe = SERIAL1_HAS_OE;
    #if SERIAL1_HAS_OE
    g_serial[1].oe_port = SERIAL1_OE_PORT;
    g_serial[1].oe_pin = SERIAL1_OE_PIN;
    #endif
    serial_init(&g_serial[1]);
    #endif

    #ifdef SERIAL2
    g_serial[2].uart_id = 2;
    g_serial[2].baud_rate = SERIAL2_BAUD_RATE;
    g_serial[2].priority = SERIAL2_PRIORITY;
    g_serial[2].rx_port = SERIAL2_RX_PORT;
    g_serial[2].rx_pin = SERIAL2_RX_PIN;
    g_serial[2].rx_function = SERIAL2_RX_FUNC;
    g_serial[2].rx_buffer_size = SERIAL2_RX_BUFF_SIZE;
    g_serial[2].tx_port = SERIAL2_TX_PORT;
    g_serial[2].tx_pin = SERIAL2_TX_PIN;
    g_serial[2].tx_function = SERIAL2_TX_FUNC;
    g_serial[2].tx_buffer_size = SERIAL2_TX_BUFF_SIZE;
    g_serial[2].has_oe = SERIAL2_HAS_OE;
    #if SERIAL2_HAS_OE
    g_serial[2].oe_port = SERIAL2_OE_PORT;
    g_serial[2].oe_pin = SERIAL2_OE_PIN;
    #endif
    serial_init(&g_serial[2]);
    #endif

    #ifdef SERIAL3
    g_serial[3].uart_id = 3;
    g_serial[3].baud_rate = SERIAL3_BAUD_RATE;
    g_serial[3].priority = SERIAL3_PRIORITY;
    g_serial[3].rx_port = SERIAL3_RX_PORT;
    g_serial[3].rx_pin = SERIAL3_RX_PIN;
    g_serial[3].rx_function = SERIAL3_RX_FUNC;
    g_serial[3].rx_buffer_size = SERIAL3_RX_BUFF_SIZE;
    g_serial[3].tx_port = SERIAL3_TX_PORT;
    g_serial[3].tx_pin = SERIAL3_TX_PIN;
    g_serial[3].tx_function = SERIAL3_TX_FUNC;
    g_serial[3].tx_buffer_size = SERIAL3_TX_BUFF_SIZE;
    g_serial[3].has_oe = SERIAL3_HAS_OE;
    #if SERIAL3_HAS_OE
    g_serial[3].oe_port = SERIAL3_OE_PORT;
    g_serial[3].oe_pin = SERIAL3_OE_PIN;
    #endif
    serial_init(&g_serial[3]);
    #endif

    hardware_eneble_serial_interupt(CLI_SERIAL);
}

void hardware_eneble_serial_interupt(uint8_t serial_port)
{
    serial_enable_interupt(&g_serial[serial_port]);
}

glcd_t *hardware_glcds()
{
    return &g_glcd;
}

void hardware_glcd_brightness(int level)
{
    g_brightness = level;
}

ledz_t *hardware_leds(uint8_t led_id)
{
    if (led_id >= LEDS_COUNT) return NULL;
    ledz_t *led = g_leds[led_id];
    return led;
}

void *hardware_actuators(uint8_t actuator_id)
{
    if ((int8_t)actuator_id >= ENCODER0 && actuator_id < (ENCODER0 + ENCODERS_COUNT))
    {
        return (&g_encoders[actuator_id - ENCODER0]);
    }

    if ((int8_t)actuator_id >= FOOTSWITCH0 && actuator_id < (FOOTSWITCH0 + FOOTSWITCHES_COUNT))
    {
        return (&g_footswitches[actuator_id - FOOTSWITCH0]);
    }

    if ((int8_t)actuator_id >= BUTTON0 && actuator_id < (BUTTON0 + BUTTONS_COUNT))
    {
        return (&g_buttons[actuator_id - BUTTON0]);
    }

    return NULL;
}

uint32_t hardware_timestamp(void)
{
    return g_counter;
}

void hardware_coreboard_power(uint8_t state)
{
    // coreboard sometimes requires 1s pulse to initialize
    if (state == COREBOARD_INIT)
    {
        vTaskDelay(1000 / portTICK_RATE_MS);
        SET_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
    }
    // coreboard requires 2s pulse to turn on
    else if (state == COREBOARD_TURN_ON)
    {
        CLR_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
        vTaskDelay(2000 / portTICK_RATE_MS);
        SET_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
    }
    // coreboard requires 5s pulse to turn off
    else if (state == COREBOARD_TURN_OFF)
    {
        CLR_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
        vTaskDelay(5000 / portTICK_RATE_MS);
        SET_PIN(SHUTDOWN_BUTTON_PORT, SHUTDOWN_BUTTON_PIN);
    }
}

void TIMER0_IRQHandler(void)
{
    static int count = 1, state;

    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET)
    {
        // LEDs PWM
        ledz_tick();

        if (g_brightness == 0)
        {
            count = 1;
            state = 1;
        }
        else if (g_brightness == MAX_BRIGHTNESS)
        {
            count = 1;
            state = 0;
        }

        if (--count == 0)
        {
            if (state)
            {
                count = MAX_BRIGHTNESS - g_brightness;
                glcd_backlight(&g_glcd, 0);
            }
            else
            {
                count = g_brightness;
                glcd_backlight(&g_glcd, 1);
            }

            state ^= 1;
        }
    }

    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

void TIMER1_IRQHandler(void)
{
    if (TIM_GetIntStatus(LPC_TIM1, TIM_MR1_INT) == SET)
    {
        actuators_clock();
        g_counter++;
    }

    TIM_ClearIntPending(LPC_TIM1, TIM_MR1_INT);
}
