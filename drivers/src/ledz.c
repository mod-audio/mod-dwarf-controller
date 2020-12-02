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

/*
****************************************************************************************************
*       INCLUDE FILES
****************************************************************************************************
*/

#include "ledz.h"
#include "config.h"

/*
****************************************************************************************************
*       INTERNAL MACROS
****************************************************************************************************
*/

// calculate (rounded) how many ticks are need to reach a period of 1ms
//new define (HARDWARE DEPENDENT), finetuned by experimentation:
#define TICKS_TO_1ms        (10000 / ((LEDZ_TICK_PERIOD * 2) + 0.6) / 10)

// invert led value if user has defined LEDZ_TURN_ON_VALUE as zero
#define LED_VALUE(val)      (!(LEDZ_TURN_ON_VALUE ^ (val)))

// macro to set led GPIO
#define LED_SET(led, val)   LEDZ_GPIO_SET(led->pins[0], led->pins[1], LED_VALUE(val)); \
                            led->state = (val);

// macro to set PWM
#ifdef LEDZ_GPIO_PWM
#define LED_PWM(led,duty)   LEDZ_GPIO_PWM(led->pins[0], led->pins[1], duty)
#else
#define LED_PWM(led,duty)
#endif


/*
****************************************************************************************************
*       INTERNAL CONSTANTS
****************************************************************************************************
*/

// table from: http://jared.geek.nz/2013/feb/linear-led-pwm
#ifdef LEDZ_BRIGHTNESS_SUPPORT
const unsigned char cie1931[101] = {
      0,   0,   0,   0,   0,   1,   1,   1,   1,   1,
      1,   1,   1,   2,   2,   2,   2,   2,   3,   3,
      3,   3,   4,   4,   4,   4,   5,   5,   5,   6,
      6,   7,   7,   8,   8,   8,   9,  10,  10,  11,
     11,  12,  12,  13,  14,  15,  15,  16,  17,  18,
     18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
     28,  29,  30,  32,  33,  34,  35,  37,  38,  39,
     41,  42,  44,  45,  47,  48,  50,  52,  53,  55,
     57,  58,  60,  62,  64,  66,  68,  70,  72,  74,
     76,  78,  81,  83,  85,  88,  90,  92,  95,  97,
    100,
};
#endif


/*
****************************************************************************************************
*       INTERNAL DATA TYPES
****************************************************************************************************
*/

struct LEDZ_T {
    ledz_color_t color;
    const int *pins;

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
#endif

    ledz_t *next;
};

typedef struct LED_STATE_T {
    ledz_t *led;
    uint8_t color;
    uint8_t state;
    int16_t time_on, time_off;
    int8_t amount_of_blinks; 
} led_state_t;

/*
****************************************************************************************************
*       INTERNAL GLOBAL VARIABLES
****************************************************************************************************
*/

static ledz_t g_leds[LEDZ_MAX_INSTANCES];
static unsigned int g_leds_available = LEDZ_MAX_INSTANCES;
static led_state_t g_led_state[LEDZ_MAX_INSTANCES];
static uint8_t led_colors[MAX_COLOR_ID + 1][3];
static float g_ledz_brightness = 1;
/*
****************************************************************************************************
*       INTERNAL FUNCTIONS
****************************************************************************************************
*/

static inline ledz_t* ledz_take(void)
{
    static unsigned int ledz_counter;

    // first request round
    if (ledz_counter < LEDZ_MAX_INSTANCES)
    {
        g_leds_available--;
        ledz_t *led = &g_leds[ledz_counter++];
        return led;
    }

    // iterate all array searching for a free spot
    // a led is considered free when pins is null
    int i;
    for (i = 0; i < LEDZ_MAX_INSTANCES; i++)
    {
        ledz_t *led  = &g_leds[i];

        if (led->pins == 0)
        {
            g_leds_available--;
            return led;
        }
    }

    return 0;
}

static inline void ledz_give(ledz_t *led)
{
    if (led)
    {
        led->pins = 0;
        g_leds_available++;
    }
}

static inline ledz_color_t get_color_by_id(uint8_t color_pin_id)
{
    switch(color_pin_id)
    {   
        //red
        case 0:
            return LEDZ_RED;
        break;

        //green
        case 1:
            return LEDZ_GREEN;
        break;

        //blue
        case 2:
            return LEDZ_BLUE;
        break;

        default:
            return LEDZ_ALL_COLORS;
        break;
    }
}
/*
****************************************************************************************************
*       GLOBAL FUNCTIONS
****************************************************************************************************
*/

void ledz_set_color(uint8_t item, uint8_t value[3])
{
    led_colors[item][0] = value[0];
    led_colors[item][1] = value[1];
    led_colors[item][2] = value[2];
}

void ledz_set_brightness(uint8_t brightness)
{
    float brightness_calc = 1;
    switch (brightness)
    {
        //low
        case 0:
            brightness_calc = 0.3;
        break;

        //mid
        case 1:
            brightness_calc = 0.6;
        break;

        //high
        case 2:
        default:
            brightness_calc = 1;
        break;
    }

    //save globaly
    g_ledz_brightness = brightness_calc;
}

ledz_t* ledz_create(ledz_type_t type, const ledz_color_t *colors, const int *pins)
{
    if (g_leds_available < type)
        return 0;

    ledz_t *next = 0;
    
    int i;
    for (i = type - 1; i >= 0; i--)
    {
        ledz_t *led = ledz_take();
        led->color = colors[i];
        led->pins = &pins[i * 2];
        led->state = 0;
        led->blink = 0;
        //config pin
        CONFIG_PIN_OUTPUT(led->pins[0], led->pins[1]);
        //set pin function
        PINSEL_SetPinFunc(led->pins[0], led->pins[1], 0);
        //initial state off
        CLR_PIN(led->pins[0], led->pins[1]);
        led->next = next;
        next = led;
    }

    return next;
}

void ledz_destroy(ledz_t* led)
{
    if (led->next)
        ledz_destroy(led->next);

    ledz_give(led);
}

void ledz_on(ledz_t* led, ledz_color_t color)
{
    ledz_set(led, color, 1);
}

void ledz_off(ledz_t* led, ledz_color_t color)
{
    // disable blinking and brightness control
    led->blink = 0;
    led->time_on = 0;
    led->time_off = 0;
    led->brightness = 0;
    led->amount_of_blinks = -1;
    
    ledz_set(led, color, 0);
}

void ledz_toggle(ledz_t* led, ledz_color_t color)
{
    ledz_set(led, color, -1);
}

void ledz_set(ledz_t* led, ledz_color_t color, int value)
{
    // adjust value
    if (value >= 1)
        value = 1;
    
    int i;
    for (i = 0; led; led = led->next, i++)
    {
        if (led->color & color)
        {
            // disable blinking and brightness control
            led->blink = 0;
            led->time_on = 0;
            led->time_off = 0;
            led->brightness = 0;
            led->amount_of_blinks = -1;
            
            // skip update if value match current state
            if (led->state == value)
                continue;

            // toggle led if value is negative
            if (value < 0)
                value = 1 - led->state;

            // update led GPIO
            LED_SET(led, value);
        }
    }
}

void ledz_blink(ledz_t* led, ledz_color_t color, uint16_t time_on, uint16_t time_off, int8_t amount_of_blinks)
{
    if (time_on == 0 || time_off == 0)
    {
        led->blink = 0;
        return;
    }
    
    int i;
    for (i = 0; led; led = led->next, i++)
    {
        if (led->color & color)
        {
            led->time_on = time_on;
            led->time_off = time_off;

            // load counter according current state
            if (led->state)
            {
                led->blink_state = 1;
                led->time = time_on;
            }
            else
            {
                led->blink_state = 0;
                led->time = time_off;
            }

            // start blinking
            led->blink = 1;
            led->amount_of_blinks = amount_of_blinks;
        }
    }
}

#ifdef LEDZ_BRIGHTNESS_SUPPORT
void ledz_brightness(ledz_t* led, ledz_color_t color, unsigned int value)
{
    if (value >= 100)
        value = 100;

    value = value * g_ledz_brightness;

    int i;
    for (i = 0; led; led = led->next, i++)
    {
        if (led->color & color)
        {    
            // convert brightness value to duty cycle according cie 1931
            int duty_cycle = cie1931[value];

            // enable hardware PWM
            if (duty_cycle > 0 && duty_cycle < 100)
                LED_PWM(led, duty_cycle);

            // does not use PWM if value is min or max
            else
                LED_SET(led, (duty_cycle >> 2) & 1);

            // enable brightness control
            led->pwm = 0;
            led->brightness_value = value;
            led->brightness = 1;
        }
    }
}

void ledz_fade_in(ledz_t* led, ledz_color_t color, unsigned int rate, unsigned int max)
{
    int i;
    for (i = 0; led; led = led->next, i++)
    {
        if (led->color & color)
        {
            led->fade_in = rate;
            led->fade_max = max;

            if (!led->brightness)
            {
                led->pwm = 0;
                led->brightness_value = 0;
                led->brightness = 1;
            }
        }
    }
}

void ledz_fade_out(ledz_t* led, ledz_color_t color, unsigned int rate, unsigned int min)
{
    int i;
    for (i = 0; led; led = led->next, i++)
    {
        if (led->color & color)
        {
            led->fade_out = rate;
            led->fade_min = min;
        }
    }
}
#endif

void ledz_tick(void)
{
    static uint16_t counter_1ms;
    int flag_1ms = 0;

    // check if 1ms has been passed
    if (++counter_1ms >= TICKS_TO_1ms)
    {
        counter_1ms = 0;
        flag_1ms = 1;
    }

    int i;  
    for (i = 0; i < LEDZ_MAX_INSTANCES; i++)
    {
        ledz_t *led = &g_leds[i];

        // skip if led is not in use
        if (led->pins == 0)
            continue;

        // execute blink control if 1ms has been passed
        if (led->blink && flag_1ms)
        {
            if (led->time > 0)
                led->time--;

            if (led->time == 0)
            {
                if (led->amount_of_blinks != 0)
                {
                    if (led->blink_state)
                    {
                        // disable hardware PWM
                        LED_PWM(led, 0);

                        // turn off led
                        LED_SET(led, 0);

                        // load counter with time off value
                        led->time = led->time_off;
                    }
                    else
                    {
                        // turn on led
                        LED_SET(led, 1);

                        // enable hardware PWM
                        LED_PWM(led, cie1931[led->brightness_value]);

                        // load counter with time on value
                        led->time = led->time_on;

                        //substract a blink
                        if (led->amount_of_blinks > -1)
                        led->amount_of_blinks--;
                    }

                }
                //stop blinking
                else 
                {
                    ledz_set_state(led, i, led->color, 1, 0, 0, 0);
                }

                // toggle blink state
                led->blink_state = 1 - led->blink_state;

                // go to next led if blink control has updated led state
                continue;
            }
        }

#ifdef LEDZ_BRIGHTNESS_SUPPORT

        // PWM generation for brightness control
#ifndef LEDZ_GPIO_PWM
        if (led->brightness && (!led->blink || (led->blink && led->blink_state)))
        {
            if (led->pwm > 0)
                led->pwm--;

            if (led->pwm == 0)
            {
                // load counter with duty cycle according led state
                if (led->state)
                    led->pwm = 100 - cie1931[led->brightness_value];
                else
                    led->pwm = cie1931[led->brightness_value];

                // change led state only if value is between min and max
                if (led->pwm > 0 && led->pwm < 100)
                   { LED_SET(led, !led->state);}
            }
        }
#endif

        // fade control
        if (flag_1ms)
        {
            // fade in
            if (led->fade_in > 0 && led->brightness_value < led->fade_max)
            {
                if (++led->fade_counter == led->fade_in)
                {
                    led->brightness_value++;
                    led->fade_counter = 0;
                    LED_PWM(led, cie1931[led->brightness_value]);
                }
            }
            else
            {
                // disable fade in
                led->fade_in = 0;
            }

            // fade out
            if (led->fade_out > 0 && led->brightness_value > led->fade_min)
            {
                if (++led->fade_counter == led->fade_out)
                {
                    led->brightness_value--;
                    led->fade_counter = 0;
                    LED_PWM(led, cie1931[led->brightness_value]);
                }
            }
            else
            {
                // disable fade out
                led->fade_out = 0;
            }
        }
#endif
    }
}

void ledz_set_state(ledz_t* led, uint8_t led_id, uint8_t color, uint8_t state, uint16_t time_on, uint16_t time_off, int8_t amount_of_blinks)
{
    set_ledz_trigger_by_color_id(led, color, state, time_on, time_off, amount_of_blinks);

    //store the LED state
    g_led_state[led_id].led = led;
    g_led_state[led_id].color = color;
    g_led_state[led_id].state = state;
    g_led_state[led_id].time_on = time_on;
    g_led_state[led_id].time_off = time_off;
    g_led_state[led_id].amount_of_blinks = amount_of_blinks;
}

void ledz_restore_state(ledz_t* led, uint8_t led_id)
{
    set_ledz_trigger_by_color_id(led, g_led_state[led_id].color, g_led_state[led_id].state, g_led_state[led_id].time_on, g_led_state[led_id].time_off, g_led_state[led_id].amount_of_blinks);
}

void set_ledz_trigger_by_color_id(ledz_t* led, uint8_t color_id, uint8_t state, uint16_t time_on, uint16_t time_off, int8_t amount_of_blinks)
{
    uint8_t i = 0;
    for (i = 0; i < 3; i++)
    {
        ledz_color_t ledz_color = get_color_by_id(i);

        //on
        if (state == 1)
        {
            ledz_on(led, ledz_color);
            ledz_brightness(led, ledz_color, led_colors[color_id][i]);
        }
        //off
        else if (state == 0)
        {
            ledz_off(led, ledz_color);
            ledz_brightness(led, ledz_color, 0);
        }
        //blink
        else
        {
            ledz_on(led, ledz_color);
            ledz_blink(led, ledz_color, time_on, time_off, amount_of_blinks);
            ledz_brightness(led, ledz_color, led_colors[color_id][i]);
        }
    }
}