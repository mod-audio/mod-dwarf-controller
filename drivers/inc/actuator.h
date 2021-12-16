
#ifndef  ACTUATOR_H
#define  ACTUATOR_H


/*
*********************************************************************************************************
*   INCLUDE FILES
*********************************************************************************************************
*/

#include <stdint.h>
#include "config.h"


/*
*********************************************************************************************************
*   DO NOT CHANGE THESE DEFINES
*********************************************************************************************************
*/

// Actuators types
typedef enum {
    BUTTON, ROTARY_ENCODER
} actuator_type_t;

// Actuators properties
typedef enum {
    BUTTON_HOLD_TIME, ENCODER_STEPS, BUTTON_DOUBLE_TIME
} actuator_prop_t;

// Events definition
#define EV_NONE                     0x00
#define EV_BUTTON_CLICKED           0x01
#define EV_BUTTON_PRESSED           0x02
#define EV_BUTTON_RELEASED          0x04
#define EV_BUTTON_HELD              0x08
#define EV_BUTTON_PRESSED_DOUBLE    0x80
#define EV_ALL_BUTTON_EVENTS        0x8F

#define EV_ENCODER_TURNED           0x10
#define EV_ENCODER_TURNED_CW        0x20
#define EV_ENCODER_TURNED_ACW       0x40
#define EV_ALL_ENCODER_EVENTS       0x6F


/*
*********************************************************************************************************
*   CONFIGURATION DEFINES
*********************************************************************************************************
*/

#define MAX_ACTUATORS               15

// The actuator is activated with ZERO or ONE?
#define BUTTON_ACTIVATED            0
#define ENCODER_ACTIVATED           0

// Clock peririod definition (in miliseconds)
#define CLOCK_PERIOD                1

// Debounce configuration (in miliseconds)
#define BUTTON_PRESS_DEBOUNCE           15
#define BUTTON_DOUBLE_PRESS_DEBOUNCE    50
#define BUTTON_RELEASE_DEBOUNCE         30

#define ENCODER_PRESS_DEBOUNCE          35
#define ENCODER_RELEASE_DEBOUNCE        100

// Encoders configuration
#define ENCODER_RESOLUTION          24

//flags
#define NO_DOUBLE_PRESS_LINK    -1
#define DOUBLE_PRESSED_LINKED   -2
#define DOUBLE_PRESSED_LOCKED   -3

/*
*********************************************************************************************************
*   DATA TYPES
*********************************************************************************************************
*/

#define actuators_common_fields         \
    uint8_t id;                         \
    actuator_type_t type;               \
    uint8_t status, control;            \
    uint8_t events_flags;               \
    void (*event) (void *actuator);     \
    uint8_t debounce;

typedef struct BUTTON_T {
    actuators_common_fields

    uint8_t port, pin;
    uint16_t hold_time, hold_time_counter;
    int8_t double_press_button_id;
    int32_t last_pressed_time, last_pressed_time_counter;
    uint32_t hardware_press_time;
} button_t;

typedef struct ENCODER_T {
    actuators_common_fields

    uint8_t port, pin, port_chA, pin_chA, port_chB, pin_chB;
    uint16_t hold_time, hold_time_counter;
    uint8_t steps, state;
    int8_t counter;
} encoder_t;


/*
*********************************************************************************************************
*   GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*   MACRO'S
*********************************************************************************************************
*/

// Macros to read actuators state
#define BUTTON_CLICKED(status)      ((status) & EV_BUTTON_CLICKED)
#define BUTTON_PRESSED(status)      ((status) & EV_BUTTON_PRESSED)
#define BUTTON_RELEASED(status)     ((status) & EV_BUTTON_RELEASED)
#define BUTTON_HOLD(status)         ((status) & EV_BUTTON_HELD)
#define BUTTON_DOUBLE(status)       ((status) & EV_BUTTON_PRESSED_DOUBLE)
#define ENCODER_TURNED(status)      ((status) & EV_ENCODER_TURNED)
#define ENCODER_TURNED_CW(status)   ((status) & EV_ENCODER_TURNED_CW)
#define ENCODER_TURNED_ACW(status)  ((status) & EV_ENCODER_TURNED_ACW)


/*
*********************************************************************************************************
*   FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void actuator_create(actuator_type_t type, uint8_t id, void *actuator);
void actuator_set_pins(void *actuator, const uint8_t *pins);
void actuator_set_prop(void *actuator, actuator_prop_t prop, uint16_t value);
void actuator_set_link(void *actuator, uint8_t linked_actuator_id);
void actuator_enable_event(void *actuator, uint8_t events_flags);
void actuator_set_event(void *actuator, void (*event)(void *actuator));
uint8_t actuator_get_status(void *actuator);
uint32_t actuator_get_click_time(void *actuator);
void actuators_clock(void);


/*
*********************************************************************************************************
*   CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*   END HEADER
*********************************************************************************************************
*/

#endif

