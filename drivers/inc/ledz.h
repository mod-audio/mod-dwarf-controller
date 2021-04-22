/*
 * LEDZ - The LED Zeppelin
 * https://github.com/ricardocrudo/ledz
 *
 * Copyright (c) 2017 Ricardo Crudo <ricardo.crudo@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef LEDZ_H
#define LEDZ_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include <stdint.h>
#include "config.h"


/*
****************************************************************************************************
*       MACROS
****************************************************************************************************
*/

#define LEDZ_VERSION     "1.1.0"


/*
****************************************************************************************************
*       CONFIGURATION
****************************************************************************************************
*/

// configure the function to set a GPIO
#define LEDZ_GPIO_SET(port,pin,value) value ? SET_PIN(port, pin) : CLR_PIN(port, pin)

// configure the function to set the PWM of a GPIO
// when the below macro is not defined (default) the PWM is generated internally
// in this case the PWM frequency = 1 / (LEDZ_TICK_PERIOD * 1E-6 * 100)
//#define LEDZ_GPIO_PWM(port,pin,duty)    gpio_pwm(port,pin,duty)

// maximum of LEDs to control (note: RGB count as 3 LEDs)
#define LEDZ_MAX_INSTANCES      (LEDS_COUNT * 3)

// configure the logic value which the led turn on (must be 0 or 1)
#define LEDZ_TURN_ON_VALUE      1

// enable/disable brightness support
// disabling the brightness saves RAM and program memory
#define LEDZ_BRIGHTNESS_SUPPORT

// tick period in us
#define LEDZ_TICK_PERIOD        LED_INTERUPT_TIME

//always blink value
#define LED_BLINK_INFINIT		-1

//defines for LED state
#define LED_OFF             0
#define LED_ON 				1
#define LED_BLINK 			2
#define LED_FADE 			3
#define LED_DIMMED			4

//update defines
#define LED_STORE_STATE     0
#define LED_UPDATE          1

/*
****************************************************************************************************
*       DATA TYPES
****************************************************************************************************
*/

/**
 * @struct ledz_type_t
 * LED types, i.e. how many LEDs are inside of the package
 */
typedef enum ledz_type_t {LEDZ_1COLOR = 1, LEDZ_2COLOR, LEDZ_3COLOR} ledz_type_t;

/**
 * @struct ledz_color_t
 * LED colors
 */
typedef enum ledz_color_t {
    LEDZ_RED    = 0x01,
    LEDZ_GREEN  = 0x02,
    LEDZ_BLUE   = 0x04,
    LEDZ_YELLOW = 0x08,
    LEDZ_CYAN   = 0x10,
    LEDZ_WHITE  = 0x20,
    LEDZ_AMBER  = 0x40,
    LEDZ_ORANGE = 0x80,
    LEDZ_ALL_COLORS = 0xFF,
} ledz_color_t;

typedef struct LED_STATE_T {
    uint8_t color;
    uint8_t state;
    int16_t time_on, time_off, fade_rate;
    int8_t amount_of_blinks;
    float brightness, fade_ratio;
} led_state_t;

/**
 * @struct ledz_t
 * An opaque structure representing a led object
 */
typedef struct LEDZ_T {
    ledz_color_t color;
    const int *pins;
    led_state_t led_state;

    struct {
        unsigned int state : 1;
        unsigned int blink : 1;
        unsigned int blink_state : 1;
        unsigned int brightness : 1;
    };

    uint16_t time_on, time_off, time;

    int8_t amount_of_blinks;

#ifdef LEDZ_BRIGHTNESS_SUPPORT
    unsigned int pwm, brightness_value;
    unsigned int fade_in, fade_out;
    unsigned int fade_min, fade_max, fade_counter;
    float fade_rate;
#endif

    struct LEDZ_T *next;
} ledz_t;

/*
****************************************************************************************************
*       FUNCTION PROTOTYPES
****************************************************************************************************
*/

void ledz_set_color(uint8_t item, int8_t value[3]);

uint8_t ledz_color_valid(uint8_t item);

/**
 * @defgroup ledz_funcs LED Functions
 * Set of functions to control the LEDs
 * @{
 */

/**
 * Create ledz object
 *
 * When creating a ledz the type must be passed as the first argument and
 * it must be one of the values defined by ledz_type_t enumeration. The second
 * argument must be an array containing the color(s) of the LED being created.
 * The third argument must be an integer array of the port and pin of each LED
 * in correspondence with the previous color(s).
 *
 * Examples:
 *      \code{.c}
 *      // create one color LED
 *      ledz_create(LEDZ_1COLOR, (const ledz_color_t []){LEDZ_RED}, (const int []){0, 1});
 *
 *      // create RGB LED
 *      const ledz_color_t colors[] = {LEDZ_RED, LEDZ_GREEN, LEDZ_BLUE};
 *      const int pins[] = {0, 1,  0, 2,  0, 3};
 *      ledz_create(LEDZ_3COLOR, colors, pins);
 *      \endcode
 *
 * @param[in] type must one of the values in ledz_type_t declaration
 * @param[in] colors its a ledz_color_t type array containing the LED colors
 * @param[in] pins an integer array of the port and pin of each LED
 *
 * @return pointer to ledz object or NULL if no more led is available
 */
ledz_t* ledz_create(ledz_type_t type, const ledz_color_t *colors, const int *pins);

/**
 * Destroy ledz_t object
 *
 * @param[in] led ledz object pointer
 */
void ledz_destroy(ledz_t* led);

/**
 * Turn LED on
 *
 * Colors can be combinated using the OR operator.
 *
 * @param[in] led ledz object pointer
 * @param[in] color the color to turn on
 */
void ledz_on(ledz_t* led, ledz_color_t color);

/**
 * Turn LED off
 *
 * Colors can be combinated using the OR operator.
 *
 * @param[in] led ledz object pointer
 * @param[in] color the color to turn off
 */
void ledz_off(ledz_t* led, ledz_color_t color);

/**
 * Toggle LED state
 *
 * Colors can be combinated using the OR operator.
 *
 * @param[in] led ledz object pointer
 * @param[in] color the color to toggle
 */
void ledz_toggle(ledz_t* led, ledz_color_t color);

/**
 * Set LED state
 *
 * Colors can be combinated using the OR operator.
 *
 * Positive value turns the LED on, zero turns the LED off and negative
 * value toggles the LED state.
 *
 * @param[in] led ledz object pointer
 * @param[in] color the color to set
 * @param[in] value the value to switch the state
 */
void ledz_set(ledz_t* led, ledz_color_t color, int value);

/**
 * Start LED blinking
 *
 * Colors can be combinated using the OR operator.
 *
 * Starts blinking the LED(s) according the requested time. To stop blinking
 * use ledz_on, ledz_off, ledz_toggle or ledz_set functions. This function
 * can also be used to stop the blinking passing zero to time_on or time_off.
 *
 * @param[in] led ledz object pointer
 * @param[in] color the color to blink
 * @param[in] time_on the time in milliseconds which the LED will be on
 * @param[in] time_off the time in milliseconds which the LED will be off
 * @param[in] the amount of blinks the led should preform, pass -1 for infinate amount.
 */
void ledz_blink(ledz_t* led, ledz_color_t color, uint16_t time_on, uint16_t time_off, int8_t amount_of_blinks);

/**
 * Set LED brightness
 *
 * Colors can be combinated using the OR operator.
 *
 * @param[in] led ledz object pointer
 * @param[in] color the color to adjust
 * @param[in] value the brightness value from 0 to 100
 */
void ledz_brightness(ledz_t* led, ledz_color_t color, unsigned int value);

/**
 * Fade in LED brightness
 *
 * Colors can be combinated using the OR operator.
 *
 * Progressively turns on the LED using the given rate. The rate is determined in
 * milliseconds per brightness unit. e.g.: rate = 10 means that the brightness is
 * increased by one every 10ms. The brightness will increase until max value.
 *
 * @param[in] led ledz object pointer
 * @param[in] color the color to adjust
 * @param[in] rate fade in rate to turn on the led
 * @param[in] max the maximum brightness value (stop value)
 */
void ledz_fade_in(ledz_t* led, ledz_color_t color, unsigned int rate, unsigned int max);

/**
 * Fade out LED brightness
 *
 * Colors can be combinated using the OR operator.
 *
 * Progressively turns off the LED using the given rate. The rate is determined in
 * milliseconds per brightness unit. e.g.: rate = 10 means that the brightness is
 * decreased by one every 10ms. The brightness will decrease until min value.
 *
 * @param[in] led ledz object pointer
 * @param[in] color the color to adjust
 * @param[in] rate fade out rate to turn off the led
 * @param[in] min the minimum brightness value (stop value)
 */
void ledz_fade_out(ledz_t* led, ledz_color_t color, unsigned int rate, unsigned int min);

void ledz_fade_up_down(ledz_t* led, ledz_color_t color, unsigned int rate, unsigned int min, unsigned int max);

/**
 * The tick function
 *
 * This function is used to define the clock of the ledz core. It must be called
 * from an interrupt service routine (ISR). The period of the interruption must be set
 * using the LEDZ_TICK_PERIOD macro.
 */
void ledz_tick(void);

/**
 * Store and toggle LED state
 *
 * Colors can be combinated using the OR operator.
 *
 * Stores the state of the LED in the driver. can be used for on, off or blink mode
 * after setting a state here the state can be recalled with ledz_restore_state.
 *
 * @param[in] led ledz object pointer
 * @param[in] color of the led
 * @param[in] state of the led (0 = off, 1 = on, 2 = blinking)
 * @param[in] time_on the time in milliseconds which the LED will be on
 * @param[in] time_off the time in milliseconds which the LED will be off
 * @param[in] the amount of blinks the led should preform, pass -1 for infinate amount. 
 */
void ledz_set_state(ledz_t* led, uint8_t state, uint8_t update);

/**
 * toggle previous LED state
 *
 * Colors can be combinated using the OR operator.
 *
 * restores the previously stored state of the LED
 *
 * @param[in] led ledz object pointer
 */
void ledz_restore_state(ledz_t* led);

/**
 * @}
 */
void set_ledz_trigger_by_color_id(ledz_t* led, uint8_t state, led_state_t led_state);

/**
 * @}
 */
void ledz_set_global_brightness(uint8_t brightness);
/*
****************************************************************************************************
*       CONFIGURATION ERRORS
****************************************************************************************************
*/

#ifndef LEDZ_GPIO_SET
#error "LEDZ_GPIO_SET macro must defined"
#endif

#if LEDZ_TURN_ON_VALUE < 0 || LEDZ_TURN_ON_VALUE > 1
#error "LEDZ_TURN_ON_VALUE must be set to 0 or 1"
#endif

#if LEDZ_TICK_PERIOD <= 0 || LEDZ_TICK_PERIOD > 1000
#error "LEDZ_TICK_PERIOD macro value must be set between 1 and 1000"
#endif

#ifdef __cplusplus
}
#endif

// LEDZ_H
#endif
