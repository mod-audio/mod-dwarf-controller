
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "device.h"

////////////////////////////////////////////////////////////////
////// SETTINGS RELATED TO HARDWARE

//// GPIO macros
#define CONFIG_PIN_INPUT(port, pin)     GPIO_SetDir((port), (1 << (pin)), GPIO_DIRECTION_INPUT)
#define CONFIG_PIN_OUTPUT(port, pin)    GPIO_SetDir((port), (1 << (pin)), GPIO_DIRECTION_OUTPUT)
#define SET_PIN(port, pin)              GPIO_SetValue((port), (1 << (pin)))
#define CLR_PIN(port, pin)              GPIO_ClearValue((port), (1 << (pin)))
#define READ_PIN(port, pin)             ((FIO_ReadValue(port) >> (pin)) & 1)
#define CONFIG_PORT_INPUT(port)         FIO_ByteSetDir((port), 0, 0xFF, GPIO_DIRECTION_INPUT)
#define CONFIG_PORT_OUTPUT(port)        FIO_ByteSetDir((port), 0, 0xFF, GPIO_DIRECTION_OUTPUT)
#define WRITE_PORT(port, value)         FIO_ByteSetValue((port), 0, (uint8_t)(value)); \
                                        FIO_ByteClearValue((port), 0, (uint8_t)(~value))
#define READ_PORT(port)                 FIO_ByteReadValue((port), (0))

//// Serial definitions
// If the UART ISR (or callbacks) uses freeRTOS API, the priorities values must be
// equal or greater than configMAX_SYSCALL_INTERRUPT_PRIORITY
// SERIAL0 (web-ui)
#define SERIAL0
#define SERIAL0_BAUD_RATE       1500000
#define SERIAL0_PRIORITY        5
#define SERIAL0_RX_PORT         0
#define SERIAL0_RX_PIN          3
#define SERIAL0_RX_FUNC         1
#define SERIAL0_RX_BUFF_SIZE    32
#define SERIAL0_TX_PORT         0
#define SERIAL0_TX_PIN          2
#define SERIAL0_TX_FUNC         1
#define SERIAL0_TX_BUFF_SIZE    32
#define SERIAL0_HAS_OE          0

// SERIAL1 (cli)
#define SERIAL1
#define SERIAL1_BAUD_RATE       115200
#define SERIAL1_PRIORITY        4
#define SERIAL1_RX_PORT         0
#define SERIAL1_RX_PIN          16
#define SERIAL1_RX_FUNC         1
#define SERIAL1_RX_BUFF_SIZE    64
#define SERIAL1_TX_PORT         0
#define SERIAL1_TX_PIN          15
#define SERIAL1_TX_FUNC         1
#define SERIAL1_TX_BUFF_SIZE    64
#define SERIAL1_HAS_OE          0

// SERIAL2 (system callbacks)
/*#define SERIAL2
#define SERIAL2_BAUD_RATE       1500000
#define SERIAL2_PRIORITY        1
#define SERIAL2_RX_PORT         2
#define SERIAL2_RX_PIN          8
#define SERIAL2_RX_FUNC         1
#define SERIAL2_RX_BUFF_SIZE    32
#define SERIAL2_TX_PORT         2
#define SERIAL2_TX_PIN          9
#define SERIAL2_TX_FUNC         1
#define SERIAL2_TX_BUFF_SIZE    32
#define SERIAL2_HAS_OE          0*/

//// Hardware peripheral definitions
// Clock power control
#define HW_CLK_PWR_CONTROL      CLKPWR_PCONP_PCTIM0  | CLKPWR_PCONP_PCTIM1  |\
                                CLKPWR_PCONP_PCUART0 | CLKPWR_PCONP_PCUART1 |\
                                CLKPWR_PCONP_PCUART2 | CLKPWR_PCONP_PCSSP0  |\
                                CLKPWR_PCONP_PCGPIO

//// LEDs configuration
// Amount of LEDS
#define LEDS_COUNT          7

// LEDs ports and pins definitions
// format definition: {R_PORT, R_PIN, G_PORT, G_PIN, B_PORT, B_PIN}
//footswitch buttons
//MDW_TODO AS THE FIRST FOOT IS NOT USED AS ACUATOR, ITS DEFINE AS THE LAST
#define LED2_PINS           {0, 13, 0, 12, 0, 31}
#define LED0_PINS           {1, 21, 1, 20, 1, 19}
#define LED1_PINS           {1, 24, 1, 23, 1, 22}
//encoder buttons
#define LED3_PINS           {4, 5, 2, 11, 0, 22}
#define LED4_PINS           {0, 10, 0, 11, 2, 13}
#define LED5_PINS           {1, 25, 1, 26, 1, 27}
//mod button
#define LED6_PINS           {4, 31, 1, 1, 5, 4}

//// GLCDs configurations
// GLCD driver, valid options: KS0108, UC1701, ST7565P
#define GLCD_DRIVER         ST7565P

// Amount of displays
#define GLCD_COUNT          1

#define GLCD0_RS_PORT       3
#define GLCD0_RS_PIN        24
#define GLCD0_RW_WR_PORT    3
#define GLCD0_RW_WR_PIN     25
#define GLCD0_E_RD_PORT     3
#define GLCD0_E_RD_PIN      26
#define GLCD0_CS_PORT       4
#define GLCD0_CS_PIN        14
#define GLCD0_RST_PORT      4
#define GLCD0_RST_PIN       13
#define GLCD0_DATA_PORT     3
#define GLCD0_DATA_PIN1     0
#define GLCD0_DATA_PIN2     1
#define GLCD0_DATA_PIN3     2
#define GLCD0_DATA_PIN4     3
#define GLCD0_DATA_PIN5     4
#define GLCD0_DATA_PIN6     5
#define GLCD0_DATA_PIN7     6
#define GLCD0_DATA_PIN8     7
#define GLCD0_BKL_PORT      3
#define GLCD0_BKL_PIN       23

//// Actuators configuration
// Actuators IDs
enum {ENCODER0, ENCODER1, ENCODER2, FOOTSWITCH0, FOOTSWITCH1, FOOTSWITCH2, BUTTON0, BUTTON1, BUTTON2, BUTTON3};

// Amount of encoders
#define ENCODERS_COUNT      3

// Encoders ports and pins definitions
// encoder definition: {ENC_BUTTON_PORT, ENC_BUTTON_PIN, ENC_CHA_PORT, ENC_CHA_PIN, ENC_CHB_PORT, ENC_CHB_PIN}
#define ENCODER0_PINS       {2, 6, 2, 5, 2, 4}
#define ENCODER1_PINS       {2, 2, 2, 0, 2, 1}
#define ENCODER2_PINS       {0, 8, 0, 6, 0, 7}

// buttons ports and pins definitions
// button definition: {BUTTON_PORT, BUTTON_PIN}

#define BUTTON0_PINS    {2, 12}
#define BUTTON1_PINS    {4, 3}
#define BUTTON2_PINS    {4, 1}
#define BUTTON3_PINS    {1, 0}

#define BUTTONS_COUNT            4

//MDW_TODO AS THE FIRST FOOT IS NOT USED AS ACUATOR, ITS DEFINE AS THE LAST
#define FOOTSWITCH2_PINS    {1, 30}
#define FOOTSWITCH0_PINS    {1, 18}
#define FOOTSWITCH1_PINS    {4, 0}

#define FOOTSWITCHES_COUNT       3

//there is always one used for navigation
#define MAX_FOOT_ASSIGNMENTS    FOOTSWITCHES_COUNT - 1

//total amount of actuators
#define TOTAL_SWITCH_ACTUATORS (BUTTONS_COUNT + FOOTSWITCHES_COUNT)
#define TOTAL_ACTUATORS (ENCODERS_COUNT + BUTTONS_COUNT + FOOTSWITCHES_COUNT)
#define TOTAL_CONTROL_ACTUATORS (ENCODERS_COUNT + (FOOTSWITCHES_COUNT - 1))

#define FOOTSWITCH_PAGES_COUNT  8
#define ENCODER_PAGES_COUNT     3

#define SHUTDOWN_BUTTON_PORT    4
#define SHUTDOWN_BUTTON_PIN     4

////////////////////////////////////////////////////////////////
////// SETTINGS RELATED TO FIRMWARE

//// webgui configuration
// define the interface
#define WEBGUI_SERIAL               0

// define how many bytes will be allocated to rx/tx buffers
#define WEBGUI_COMM_RX_BUFF_SIZE    4096
#define WEBGUI_COMM_TX_BUFF_SIZE    512

//// webgui configuration
// define the interface
#define SYSTEM_SERIAL               2

// define how many bytes will be allocated to rx/tx buffers
#define SYSTEM_COMM_RX_BUFF_SIZE    4096
#define SYSTEM_COMM_TX_BUFF_SIZE    512

//// Tools configuration
// navigation update time, this is only useful in tool mode
#define NAVEG_UPDATE_TIME   1500

// time in milliseconds to enter in tool mode (hold rotary encoder button)
#define TOOL_MODE_TIME      500

// the amount of pulses from the encoder that is equal to one up/down movement in a menu
#define SCROL_SENSITIVITY   0

#define ENCODER_ACCEL_STEP_1 2
#define ENCODER_ACCEL_STEP_2 3
#define ENCODER_ACCEL_STEP_3 5

// which display will show which tool
#define DISPLAY_TOOL_SYSTEM         0
#define DISPLAY_TOOL_TUNER          1
#define DISPLAY_TOOL_NAVIG          2
#define DISPLAY_TOOL_SYSTEM_SUBMENU 3

#define MAX_TOOLS                   5

//// Screen definitions
// defines the default rotary text
#define SCREEN_ROTARY_DEFAULT_NAME      "KNOB #"
// defines the default foot text
#define SCREEN_FOOT_DEFAULT_NAME        "FOOT #"

//// System menu configuration
// includes the system menu callbacks
#include "system.h"
// defines the menu id's
#define ROOT_ID         (0 * 10)
#define AUDIO_INP_ID    (1 * 10)
#define AUDIO_OUTP_ID   (2 * 10)
#define HEADPHONE_ID    (3 * 10)
#define NOISE_GATE_ID   (4 * 10)
#define COMPRESSOR_ID   (5 * 10)
#define SYNC_ID         (6 * 10)
#define MIDI_ID         (7 * 10)
#define USER_PROFILE_ID (8 * 10)
#define DEVICE_SET_1_ID (9 * 10)
#define DEVICE_SET_2_ID (10 * 10)
#define BLUETOOTH_ID    (11 * 10)
#define INFO_ID         (12 * 10)
#define SERVICES_ID     (13 * 10)
#define VERSIONS_ID     (14 * 10)
#define DEVICE_ID       (15 * 10)
#define UPDATE_ID       (16 * 10)

#define INP_STEREO_LINK         AUDIO_INP_ID+1
#define INP_1_GAIN_ID           AUDIO_INP_ID+2
#define INP_2_GAIN_ID           AUDIO_INP_ID+3

#define OUTP_STEREO_LINK        AUDIO_OUTP_ID+1
#define OUTP_1_GAIN_ID          AUDIO_OUTP_ID+2
#define OUTP_2_GAIN_ID          AUDIO_OUTP_ID+3

#define HEADPHONE_VOLUME_ID     HEADPHONE_ID+1

#define CLOCK_SOURCE_ID         SYNC_ID+1
#define SEND_CLOCK_ID           SYNC_ID+2

#define MIDI_PB_PC_CHANNEL_ID   MIDI_ID+1
#define MIDI_SS_PC_CHANNEL_ID   MIDI_ID+2

#define DISPLAY_BRIGHTNESS_ID   DEVICE_SET_1_ID+1
#define DISPLAY_CONTRAST_ID     DEVICE_SET_1_ID+2
#define UNASSIGNED_ACTUATRS_ID  DEVICE_SET_1_ID+3
#define DEFAULT_TOOL_ID         DEVICE_SET_2_ID+1
#define KNOB_LIST_ID            DEVICE_SET_2_ID+2
#define CONTROL_HEADER_ID       DEVICE_SET_2_ID+3

#define DIALOG_ID           230

//MDW_TODO THESE MENU ITEMS ARE NOT YET IN USE
/*
    {"NOISE GATE",                      MENU_MAIN,      NOISE_GATE_ID,          ROOT_ID,            NULL                       , 0},  \
    {"COMPRESSOR",                      MENU_MAIN,      COMPRESSOR_ID,          ROOT_ID,            NULL                       , 0},  \

    {"DEVICE SETTINGS (2/2)",           MENU_MAIN,      DEVICE_SET_2_ID,        ROOT_ID,            NULL                       , 0},  \
    {"DEFAULT TOOL",                    MENU_LIST,      DEFAULT_TOOL_ID,        DEVICE_SET_2_ID,    NULL                       , 0},  \
    {"KNOB LIST",                       MENU_TOGGLE,    KNOB_LIST_ID,           DEVICE_SET_2_ID,    NULL                       , 0},  \
    {"CONTROL HEADER",                  MENU_LIST,      CONTROL_HEADER_ID,      DEVICE_SET_2_ID,    NULL                       , 0},  \

    {"BLUETOOTH",                       MENU_ROOT,      BLUETOOTH_ID,           ROOT_ID,            system_bluetooth_cb        , 1},  \
    {"ENABLE DISCOVERY",                MENU_OK,        BLUETOOTH_ID+1,         BLUETOOTH_ID,       system_bluetooth_cb        , 0},  \
    {"STATUS:",                         MENU_NONE,      BLUETOOTH_ID+2,         BLUETOOTH_ID,       NULL                       , 0},  \
    {"NAME:",                           MENU_NONE,      BLUETOOTH_ID+3,         BLUETOOTH_ID,       NULL                       , 0},  \
    {"ADDRESS:",                        MENU_NONE,      BLUETOOTH_ID+4,         BLUETOOTH_ID,       NULL                       , 0},  \
    {"INFO",                            MENU_ROOT,      INFO_ID,                ROOT_ID,            NULL                       , 0},  \
    {"SERVICES",                        MENU_ROOT,      SERVICES_ID,            INFO_ID,            system_services_cb         , 1},  \
    {"JACK:",                           MENU_NONE,      SERVICES_ID+1,          SERVICES_ID,        NULL                       , 0},  \
    {"SSHD:",                           MENU_NONE,      SERVICES_ID+2,          SERVICES_ID,        NULL                       , 0},  \
    {"MOD-UI:",                         MENU_NONE,      SERVICES_ID+3,          SERVICES_ID,        NULL                       , 0},  \
    {"DNSMASQ:",                        MENU_NONE,      SERVICES_ID+4,          SERVICES_ID,        NULL                       , 0},  \
    {"VERSIONS",                        MENU_LIST,      VERSIONS_ID,            INFO_ID,            system_versions_cb         , 0},  \
    {"VERSION:",                        MENU_NONE,      VERSIONS_ID+1,          VERSIONS_ID,        system_release_cb          , 0},  \
    {"RESTORE:",                        MENU_NONE,      VERSIONS_ID+2,          VERSIONS_ID,        NULL                       , 0},  \
    {"SYSTEM:",                         MENU_NONE,      VERSIONS_ID+3,          VERSIONS_ID,        NULL                       , 0},  \
    {"CONTROLLER:",                     MENU_NONE,      VERSIONS_ID+4,          VERSIONS_ID,        NULL                       , 0},  \
    {"DEVICE",                          MENU_LIST,      DEVICE_ID,              INFO_ID,            NULL                       , 0},  \
    {"SERIAL NUMBER",                   MENU_OK,        DEVICE_ID+1,            DEVICE_ID,          system_tag_cb              , 0},  \

*/
#define SYSTEM_MENU     \
    {"SETTINGS",                        MENU_ROOT,      ROOT_ID,                -1,                 NULL                       , 0},  \
    {"AUDIO INPUTS",                    MENU_MAIN,      AUDIO_INP_ID,           ROOT_ID,            NULL                       , 0},  \
    {"INPUT STEREOLINK",                MENU_TOGGLE,    INP_STEREO_LINK,        AUDIO_INP_ID,       system_sl_in_cb            , 0},  \
    {"INPUT-1 GAIN",                    MENU_BAR,       INP_1_GAIN_ID,          AUDIO_INP_ID,       system_inp_1_volume_cb     , 0},  \
    {"INPUT-2 GAIN",                    MENU_BAR,       INP_2_GAIN_ID,          AUDIO_INP_ID,       system_inp_2_volume_cb     , 0},  \
    {"AUDIO OUTPUTS",                   MENU_MAIN,      AUDIO_OUTP_ID,          ROOT_ID,            NULL                       , 0},  \
    {"OUTPUT STEREOLINK",               MENU_TOGGLE,    OUTP_STEREO_LINK,       AUDIO_OUTP_ID,      system_sl_out_cb           , 0},  \
    {"OUTPUT-1 GAIN",                   MENU_BAR,       OUTP_1_GAIN_ID,         AUDIO_OUTP_ID,      system_outp_1_volume_cb    , 0},  \
    {"OUTPUT-2 GAIN",                   MENU_BAR,       OUTP_2_GAIN_ID,         AUDIO_OUTP_ID,      system_outp_2_volume_cb    , 0},  \
    {"HEADPHONE OUTPUT",                MENU_MAIN,      HEADPHONE_ID,           ROOT_ID,            NULL                       , 0},  \
    {"HEADPHONE VOLUME",                MENU_BAR,       HEADPHONE_VOLUME_ID,    HEADPHONE_ID,       system_hp_volume_cb        , 0},  \
    {"SYNC",                            MENU_MAIN,      SYNC_ID,                ROOT_ID,            NULL                       , 0},  \
    {"CLOCK SOURCE",                    MENU_LIST,      CLOCK_SOURCE_ID,        SYNC_ID,            system_midi_src_cb         , 0},  \
    {"SEND CLOCK",                      MENU_TOGGLE,    SEND_CLOCK_ID,          SYNC_ID,            system_midi_send_cb        , 0},  \
    {"MIDI",                            MENU_MAIN,      MIDI_ID,                ROOT_ID,            NULL                       , 0},  \
    {"PEDALBOARD PC-CHANNEL",           MENU_LIST,      MIDI_PB_PC_CHANNEL_ID,  MIDI_ID,            system_pb_prog_change_cb   , 0},  \
    {"SNAPSHOT PC-CHANNEL",             MENU_LIST,      MIDI_SS_PC_CHANNEL_ID,  MIDI_ID,            system_ss_prog_change_cb   , 0},  \
    {"DEVICE SETTINGS",                 MENU_MAIN,      DEVICE_SET_1_ID,        ROOT_ID,            NULL                       , 0},  \
    {"DISPLAY BRIGHTNESS",              MENU_LIST,      DISPLAY_BRIGHTNESS_ID,  DEVICE_SET_1_ID,    system_display_brightness_cb          , 0},  \
    {"DISPLAY CONTRAST",                MENU_BAR,       DISPLAY_CONTRAST_ID,    DEVICE_SET_1_ID,    system_display_contrast_cb          , 0},  \
    {"UNASSIGNED ACTUATORS",            MENU_LIST,      UNASSIGNED_ACTUATRS_ID, DEVICE_SET_1_ID,    system_hide_actuator_cb    , 0},  \
    {"SYSTEM UPGRADE",                  MENU_CONFIRM,   UPDATE_ID,              ROOT_ID     ,            system_upgrade_cb          , 0},  \

//POPUP DEFINES
//PROFILE POPUP TXT
#define PROFILE_POPUP_LOAD_TXT    "The device is about to load a\nnew profile. Continue?"
#define PROFILE_POPUP_RELOAD_TXT  "Reload active user profile?\nThis will discard any unsaved\nchanges."
#define EXP_CV_POPUP_TXT          "The device is about to switch\ninput modes. To avoid damage,\ndisconnect all devices from\nthe CV/EXP port. Continue?"
#define HP_CV_POPUP_TXT           "The device is about to switch\noutput modes. To avoid damage,\ndisconnect all devices from\nthe CV/HP port. Continue?"
// popups text content, format : {menu_id, header_content, text_content}

#define POPUP_CONTENT   \
    {BLUETOOTH_ID+1, "Enable Bluetooth", "Bluetooth discovery mode is   now enabled for 2 minutes"},  \
    {UPDATE_ID, "Start System Upgrade", "To start the system upgrade\nprocess, press and hold down\nthe leftmost button and press\nyes. "}, \

#define MENU_LINE_CHARS     31

//// Button functions leds colors, these reflect color ID's which are stored in eeprom.
#define TOGGLED_COLOR             0
#define TRIGGER_COLOR             1
#define TRIGGER_PRESSED_COLOR     2
#define TAP_TEMPO_COLOR           3
#define ENUMERATED_COLOR          4
#define ENUMERATED_PRESSED_COLOR  5
#define BYPASS_COLOR              6
#define SNAPSHOT_COLOR            7
#define SNAPSHOT_LOAD_COLOR       8
#define LED_LIST_COLOR_1          9
#define LED_LIST_COLOR_2          10
#define LED_LIST_COLOR_3          11
#define LED_LIST_COLOR_4          12
#define LED_LIST_COLOR_5          13
#define LED_LIST_COLOR_6          14
#define LED_LIST_COLOR_7          15
#define FS_PAGE_COLOR_1           16
#define FS_PAGE_COLOR_2           17
#define FS_PAGE_COLOR_3           18
#define FS_PAGE_COLOR_4           19
#define FS_PAGE_COLOR_5           20
#define FS_PAGE_COLOR_6           21
#define FS_PAGE_COLOR_7           22
#define FS_PAGE_COLOR_8           23
#define FS_PB_MENU_COLOR          24
#define FS_SS_MENU_COLOR          25
#define ENCODER_PAGE_COLOR        26
#define MAX_COLOR_ID              27

#define DEFAULT_TOGGLED_COLOR             {100,0,0}
#define DEFAULT_TRIGGER_COLOR             {100,100,100}
#define DEFAULT_TRIGGER_PRESSED_COLOR     {100,0,0}
#define DEFAULT_TAP_TEMPO_COLOR           {100,100,100}
#define DEFAULT_ENUMERATED_COLOR          {100,100,100}
#define DEFAULT_ENUMERATED_PRESSED_COLOR  {100,0,0}
#define DEFAULT_BYPASS_COLOR              {100,0,0}
#define DEFAULT_SNAPSHOT_COLOR            {100,100,100}
#define DEFAULT_SNAPSHOT_LOAD_COLOR       {0,100,100}
#define DEFAULT_LED_LIST_COLOR_1          {100,100,100}
#define DEFAULT_LED_LIST_COLOR_2          {100,0,0}
#define DEFAULT_LED_LIST_COLOR_3          {100,100,0}
#define DEFAULT_LED_LIST_COLOR_4          {0,100,100}
#define DEFAULT_LED_LIST_COLOR_5          {0,100,0}
#define DEFAULT_LED_LIST_COLOR_6          {100,0,100}
#define DEFAULT_LED_LIST_COLOR_7          {100,0,100}
#define DEFAULT_FS_PAGE_COLOR_1           {100,0,0}
#define DEFAULT_FS_PAGE_COLOR_2           {100,100,0}
#define DEFAULT_FS_PAGE_COLOR_3           {0,100,100}
#define DEFAULT_FS_PAGE_COLOR_4           {53,22,61}
#define DEFAULT_FS_PAGE_COLOR_5           {90,40,10}
#define DEFAULT_FS_PAGE_COLOR_6           {0,100,0}
#define DEFAULT_FS_PAGE_COLOR_7           {100,0,100}
#define DEFAULT_FS_PAGE_COLOR_8           {0,0,100}
#define DEFAULT_FS_PB_MENU                {53,22,61}
#define DEFAULT_FS_SS_MENU                {0,100,100}
#define DEFAULT_ENCODER_PAGE_COLOR        {100,0,0}

//alternate LED colors for lists
#define LED_LIST_AMOUNT_OF_COLORS         7

//// Tap Tempo
// defines the time that the led will stay turned on (in milliseconds)
#define TAP_TEMPO_TIME_ON       100
// defines the default timeout value (in milliseconds)
#define TAP_TEMPO_DEFAULT_TIMEOUT 3000
// defines the difference in time the taps can have to be registered to the same sequence (in milliseconds)
#define TAP_TEMPO_TAP_HYSTERESIS 100
// defines the time (in milliseconds) that the tap can be over the maximum value to be registered
#define TAP_TEMPO_MAXVAL_OVERFLOW 50

//// Toggled
// defines the toggled footer text
#define TOGGLED_ON_FOOTER_TEXT      "ON"
#define TOGGLED_OFF_FOOTER_TEXT     "OFF"

//// Bypass
// defines the bypass footer text
#define BYPASS_ON_FOOTER_TEXT       "OFF"
#define BYPASS_OFF_FOOTER_TEXT      "ON"

//// Bank configuration functions
// defines the true bypass footer text
#define TRUE_BYPASS_FOOTER_TEXT     "TRUE BYPASS"
// defines the next pedalboard footer text
#define PEDALBOARD_NEXT_FOOTER_TEXT "+"
// defines the previous pedalboard footer text
#define PEDALBOARD_PREV_FOOTER_TEXT "-"

//overlay timeouts
#define ENCODER_LIST_TIMEOUT        500
#define FOOT_CONTROLS_TIMEOUT       700

//// Command line interface configurations
// defines the cli serial
#define CLI_SERIAL                  1
// defines how much time wait for console response (in milliseconds)
#define CLI_RESPONSE_TIMEOUT        200

///EEPROM adress page defines
#define HIDE_ACTUATOR_ADRESS               0
#define DISPLAY_BRIGHTNESS_ADRESS          1
#define DISPLAY_CONTRAST_ADRESS            2
#define SL_INPUT_ADRESS                    3
#define SL_OUTPUT_ADRESS                   4

//default settings
#define DEFAULT_HIDE_ACTUATOR              0
#define DEFAULT_DISPLAY_BRIGHTNESS         2
#define DEFAULT_LED_BRIGHTNESS             2
#define DEFAULT_SL_INPUT                   0
#define DEFAULT_SL_OUTPUT                  0

//memory used for LED value's
#define LED_COLOR_EEMPROM_PAGE             2
#define LED_COLOR_ADRESS_START             0

//we use the last 2 bytes of page 0 to check the eeprom settings
#define EEPROM_VERSION_ADRESS              62

//for version control, when increasing they ALWAYS need to be bigger then the previous value
#define EEPROM_CURRENT_VERSION             1L

//for testing purposes, overwrites the EEPROM regardless of the version
#define FORCE_WRITE_EEPROM                1

#define LED_INTERUPT_TIME     10

//volume message timeout in a multiple of 500us
#define VOL_MESSAGE_TIMEOUT         200

//// Dynamic menory allocation
// defines the heap size (in bytes)
#define RTOS_HEAP_SIZE  (32 * 1024)
// these macros should be used in replacement to default malloc and free functions of stdlib.h
// The FREE function is NULL safe
#include "FreeRTOS.h"
#define MALLOC(n)       pvPortMalloc(n)
#define FREE(pv)        vPortFree(pv)

////////////////////////////////////////////////////////////////
////// DON'T CHANGE THIS DEFINES

//// Actuators types
#define NONE            0
#define FOOT            1
#define KNOB            2

// serial count definition
#define SERIAL_COUNT    4

// check serial rx buffer size
#ifndef SERIAL0_RX_BUFF_SIZE
#define SERIAL0_RX_BUFF_SIZE    0
#endif
#ifndef SERIAL1_RX_BUFF_SIZE
#define SERIAL1_RX_BUFF_SIZE    0
#endif
#ifndef SERIAL2_RX_BUFF_SIZE
#define SERIAL2_RX_BUFF_SIZE    0
#endif
#ifndef SERIAL3_RX_BUFF_SIZE
#define SERIAL3_RX_BUFF_SIZE    0
#endif

// check serial tx buffer size
#ifndef SERIAL0_TX_BUFF_SIZE
#define SERIAL0_TX_BUFF_SIZE    0
#endif
#ifndef SERIAL1_TX_BUFF_SIZE
#define SERIAL1_TX_BUFF_SIZE    0
#endif
#ifndef SERIAL2_TX_BUFF_SIZE
#define SERIAL2_TX_BUFF_SIZE    0
#endif
#ifndef SERIAL3_TX_BUFF_SIZE
#define SERIAL3_TX_BUFF_SIZE    0
#endif

#define SERIAL_MAX_RX_BUFF_SIZE     MAX(MAX(SERIAL0_RX_BUFF_SIZE, SERIAL1_RX_BUFF_SIZE), \
                                        MAX(SERIAL2_RX_BUFF_SIZE, SERIAL3_RX_BUFF_SIZE))

#define SERIAL_MAX_TX_BUFF_SIZE     MAX(MAX(SERIAL0_TX_BUFF_SIZE, SERIAL1_TX_BUFF_SIZE), \
                                        MAX(SERIAL2_TX_BUFF_SIZE, SERIAL3_TX_BUFF_SIZE))

// GLCD configurations definitions
#ifndef GLCD0_CONFIG
#define GLCD0_CONFIG
#endif

#ifndef GLCD1_CONFIG
#define GLCD1_CONFIG
#endif

#ifndef GLCD2_CONFIG
#define GLCD2_CONFIG
#endif

#ifndef GLCD3_CONFIG
#define GLCD3_CONFIG
#endif

#define DISPLAY_RIGHT 1
#define DISPLAY_LEFT  0

// GLCD drivers definitions
#define KS0108      0
#define UC1701      1
#define ST7565P     2

// GLCD driver include
#if GLCD_DRIVER == KS0108
#include "ks0108.h"
#elif GLCD_DRIVER == UC1701
#include "uc1701.h"
#elif GLCD_DRIVER == ST7565P
#include "st7565p.h"
#endif

#endif
