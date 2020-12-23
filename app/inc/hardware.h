
/*
************************************************************************************************************************
*
************************************************************************************************************************
*/

#ifndef HARDWARE_H
#define HARDWARE_H


/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include <stdint.h>

#include "glcd.h"
#include "ledz.h"


/*
************************************************************************************************************************
*           DO NOT CHANGE THESE DEFINES
************************************************************************************************************************
*/

enum {PROCESS, BYPASS};
enum {BLOCK, UNBLOCK};
enum {RECEPTION, TRANSMISSION};
enum {COREBOARD_INIT, COREBOARD_TURN_OFF, COREBOARD_TURN_ON};

/*
************************************************************************************************************************
*           CONFIGURATION DEFINES
************************************************************************************************************************
*/

#define MAX_BRIGHTNESS      4

//led colors
#define RED                 LEDZ_RED
#define GREEN               LEDZ_GREEN
#define BLUE                LEDZ_BLUE
#define YELLOW              LEDZ_RED | LEDZ_GREEN
#define MAGENTA             LEDZ_RED | LEDZ_BLUE
#define CYAN                LEDZ_GREEN | LEDZ_BLUE
#define WHITE               LEDZ_GREEN | LEDZ_BLUE | LEDZ_RED

/*
************************************************************************************************************************
*           DATA TYPES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           GLOBAL VARIABLES
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           MACRO'S
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           FUNCTION PROTOTYPES
************************************************************************************************************************
*/

// does the hardware setup
void hardware_setup(void);
//enable the serial interupt
void hardware_eneble_serial_interupt(uint8_t serial_port);
// returns the glcd object relative to glcd id
glcd_t *hardware_glcds();
void hardware_glcd_brightness(int level);
// returns the led object relative to led id
ledz_t *hardware_leds(uint8_t led_id);
// returns the actuator object relative to actuator id
void *hardware_actuators(uint8_t actuator_id);
// returns the timestamp (a variable increment in each millisecond)
uint32_t hardware_timestamp(void);
// turn on/off coreboard
void hardware_coreboard_power(uint8_t state);
//set timer for overlays
void hardware_set_overlay_timeout(uint32_t overlay_time_in_ms, void (*timeout_cb));
//force stop timer
void hardware_force_overlay_off(void);
//get overlay counter time
uint32_t hardware_get_overlay_counter(void);


/*
************************************************************************************************************************
*           CONFIGURATION ERRORS
************************************************************************************************************************
*/


/*
************************************************************************************************************************
*           END HEADER
************************************************************************************************************************
*/

#endif
