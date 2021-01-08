
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/


#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <float.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "config.h"
#include "hardware.h"
#include "serial.h"
#include "protocol.h"
#include "glcd.h"
#include "ledz.h"
#include "actuator.h"
#include "data.h"
#include "naveg.h"
#include "screen.h"
#include "cli.h"
#include "ui_comm.h"
#include "sys_comm.h"
#include "images.h"
#include "mode_control.h"
#include "mode_tools.h"

//reset actuator queue
void reset_queue(void);
/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

enum {TT_INIT, TT_COUNTING};

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

struct TAP_TEMPO_T {
    uint32_t time, max;
    uint8_t state;
} g_tap_tempo[MAX_FOOT_ASSIGNMENTS];

/*
************************************************************************************************************************
*           LOCAL MACROS
************************************************************************************************************************
*/

#define MAP(x, Omin, Omax, Nmin, Nmax)      ( x - Omin ) * (Nmax -  Nmin)  / (Omax - Omin) + Nmin;

/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static control_t *g_controls[ENCODERS_COUNT], *g_foots[MAX_FOOT_ASSIGNMENTS];

uint8_t g_scroll_dir = 1;

static uint8_t g_current_foot_control_page = 0;
static uint8_t g_current_encoder_page = 0;
static uint8_t g_fs_page_available[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static int8_t g_current_overlay_actuator = -1;

int8_t g_overlay_actuator_lock = 0;

/*
************************************************************************************************************************
*           LOCAL FUNCTION PROTOTYPES
************************************************************************************************************************
*/

static void encoder_control_add(control_t *control);
static void encoder_control_rm(uint8_t hw_id);

static void foot_control_add(control_t *control);
static void foot_control_rm(uint8_t hw_id);

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

// calculates the control value using the step
static void step_to_value(control_t *control)
{
    // about the calculation: http://lv2plug.in/ns/ext/port-props/#rangeSteps

    float p_step = ((float) control->step) / ((float) (control->steps - 1));
    if (control->properties & FLAG_CONTROL_LOGARITHMIC)
    {
        control->value = control->minimum * pow(control->maximum / control->minimum, p_step);
    }
    else if (control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
        control->value = control->scale_points[control->step]->value;
    }
    else if (!(control->properties & (FLAG_CONTROL_TRIGGER | FLAG_CONTROL_TOGGLED | FLAG_CONTROL_BYPASS)))
    {
        control->value = (p_step * (control->maximum - control->minimum)) + control->minimum;
    }

    if (control->value > control->maximum) control->value = control->maximum;
    if (control->value < control->minimum) control->value = control->minimum;
}

void set_footswitch_pages_led_state(void)
{
    ledz_t *led = hardware_leds(2);

    led->led_state.color = FS_PAGE_COLOR_1 + g_current_foot_control_page;
    ledz_set_state(led, LED_ON);
}

void set_encoder_pages_led_state(void)
{
    uint8_t i;
    ledz_t *led;
    
    for (i = 0; i < ENCODER_PAGES_COUNT; i++)
    {
        led = hardware_leds(i+ENCODER_PAGES_COUNT);
        led->led_state.color = ENCODER_PAGE_COLOR;

        if (i == g_current_encoder_page)
            ledz_set_state(led, LED_ON);
    } 
}

void restore_led_states(void)
{
    uint8_t i;
    for (i = 0; i < 6; i++)
    {
        ledz_restore_state(hardware_leds(i));
    }
}

uint8_t foot_pages_available(void)
{
    //sum available pages
    uint8_t j;
    uint8_t pages_available = 0;
    for (j = 0; j < FOOTSWITCH_PAGES_COUNT; j++)
    {
        if (g_fs_page_available[j])
            pages_available++;
    }

    return pages_available;
}

// control assigned to display
static void encoder_control_add(control_t *control)
{
    if (control->hw_id >= ENCODERS_COUNT) return;

    // checks if is already a control assigned in this display and remove it
    if (g_controls[control->hw_id])
        data_free_control(g_controls[control->hw_id]);

    // assign the new control
    g_controls[control->hw_id] = control;

    if (control->properties & FLAG_CONTROL_LOGARITHMIC)
    {
        if (control->minimum == 0.0)
            control->minimum = FLT_MIN;

        if (control->maximum == 0.0)
            control->maximum = FLT_MIN;

        if (control->value == 0.0)
            control->value = FLT_MIN;

        control->step =
            (control->steps - 1) * log(control->value / control->minimum) / log(control->maximum / control->minimum);
    }
    else if (control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
        control->step = 0;
        uint8_t i;
        //control->scroll_dir = g_scroll_dir;
        for (i = 0; i < control->scale_points_count; i++)
        {
            if (control->value == control->scale_points[i]->value)
            {
                control->step = i;
                break;
            }
        }
        control->steps = control->scale_points_count;
    }
    else if (control->properties & FLAG_CONTROL_INTEGER)
    {
        control->steps = (control->maximum - control->minimum) + 1;
        control->step =
            (control->value - control->minimum) / ((control->maximum - control->minimum) / control->steps);
    }
    else if (control->properties & (FLAG_CONTROL_BYPASS | FLAG_CONTROL_TOGGLED))
    {
        control->steps = 1;
        control->step = control->value;
    }
    else
    {
        control->step =
            (control->value - control->minimum) / ((control->maximum - control->minimum) / control->steps);
    }

    if (naveg_get_current_mode() == MODE_CONTROL)
    {
        //if screen overlay active, update that
        if ((hardware_get_overlay_counter() || !control->scroll_dir) && (g_current_overlay_actuator == control->hw_id))
        {
            CM_print_control_overlay(control, ENCODER_LIST_TIMEOUT);
            return;
        }

        // update the control screen
        if (g_current_overlay_actuator == -1)
            screen_encoder(control, control->hw_id);
    }
}

// control removed from display
static void encoder_control_rm(uint8_t hw_id)
{
    if (hw_id > ENCODERS_COUNT) return;

    if ((!g_controls[hw_id]) && (naveg_get_current_mode() == MODE_CONTROL))
    {
        screen_encoder(NULL, hw_id);
        return;
    }

    control_t *control = g_controls[hw_id];

    if (control)
    {
        data_free_control(control);
        g_controls[hw_id] = NULL;
        if (naveg_get_current_mode() == MODE_CONTROL)
            screen_encoder(NULL, hw_id);
    }
}

static void set_alternated_led_list_colour(control_t *control)
{
    ledz_t *led = hardware_leds(control->hw_id - ENCODERS_COUNT);

    led->led_state.color = LED_LIST_COLOR_1 + (control->scale_point_index % LED_LIST_AMOUNT_OF_COLORS);

    ledz_set_state(led, LED_ON);
}

// control assigned to foot
static void foot_control_add(control_t *control)
{
    uint8_t i;

    // checks the actuator id
    if (control->hw_id >= MAX_FOOT_ASSIGNMENTS + ENCODERS_COUNT)
        return;

    // checks if the foot is already used by other control and not is state updating
    if (g_foots[control->hw_id - ENCODERS_COUNT] && g_foots[control->hw_id - ENCODERS_COUNT] != control)
    {
        data_free_control(control);
        return;
    }

    // stores the foot
    g_foots[control->hw_id - ENCODERS_COUNT] = control;
    ledz_t *led = hardware_leds(control->hw_id - ENCODERS_COUNT);

    if (naveg_get_current_mode() != MODE_CONTROL)
        return;

    if (control->properties & FLAG_CONTROL_MOMENTARY)
    {
        if ((control->scroll_dir == 0)||(control->scroll_dir == 2))
            led->led_state.color = TRIGGER_COLOR;
        else
            led->led_state.color = TRIGGER_COLOR;
        
        // updates the footer
        screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                     (control->value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT), control->properties);
        ledz_set_state(led, LED_ON);
    }
    else if (control->properties & FLAG_CONTROL_TRIGGER)
    {
        // trigger specification: http://lv2plug.in/ns/ext/port-props/#trigger
        // updates the led
        //check if its assigned to a trigger and if the button is released
        if (control->scroll_dir == 2) 
            led->led_state.color = TRIGGER_COLOR;
        else
            led->led_state.color = TRIGGER_COLOR;

        // updates the footer (a getto fix here, the screen.c file did not regognize the NULL pointer so it did not allign the text properly, TODO fix this)
        screen_footer(control->hw_id - ENCODERS_COUNT, control->label, BYPASS_ON_FOOTER_TEXT, control->properties);
        ledz_set_state(led, LED_ON);
    }
    else if (control->properties & FLAG_CONTROL_TOGGLED)
    {
        // toggled specification: http://lv2plug.in/ns/lv2core/#toggled
        // updates the led
        led->led_state.color = TOGGLED_COLOR;
        if (control->value <= 0)
            ledz_set_state(led, LED_OFF);
        else
            ledz_set_state(led, LED_ON);

        // updates the footer
        screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                     (control->value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT), control->properties);
    }
    else if (control->properties & FLAG_CONTROL_TAP_TEMPO)
    {
        // convert the time unit
        uint16_t time_ms = (uint16_t)(convert_to_ms(control->unit, control->value) + 0.5);

        led->led_state.color = TAP_TEMPO_COLOR;
        led->led_state.amount_of_blinks = LED_BLINK_INFINIT;

        // setup the led blink
        if (time_ms > TAP_TEMPO_TIME_ON)
        {
            led->led_state.time_on = TAP_TEMPO_TIME_ON;
            led->led_state.time_off = time_ms - TAP_TEMPO_TIME_ON;
        }
        else
        {
            led->led_state.time_on = time_ms / 2;
            led->led_state.time_off = time_ms / 2;
        }

        ledz_set_state(led, LED_BLINK);

        // calculates the maximum tap tempo value
        if (g_tap_tempo[control->hw_id - ENCODERS_COUNT].state == TT_INIT)
        {
            uint32_t max;

            // time unit (ms, s)
            if (strcmp(control->unit, "ms") == 0 || strcmp(control->unit, "s") == 0)
            {
                max = (uint32_t)(convert_to_ms(control->unit, control->maximum) + 0.5);
                //makes sure we enforce a proper timeout
                if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                    max = TAP_TEMPO_DEFAULT_TIMEOUT;
            }
            // frequency unit (bpm, Hz)
            else
            {
                //prevent division by 0 case
                if (control->minimum == 0)
                    max = TAP_TEMPO_DEFAULT_TIMEOUT;
                else
                    max = (uint32_t)(convert_to_ms(control->unit, control->minimum) + 0.5);

                //makes sure we enforce a proper timeout
                if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                    max = TAP_TEMPO_DEFAULT_TIMEOUT;
            }

            g_tap_tempo[control->hw_id - ENCODERS_COUNT].max = max;
            g_tap_tempo[control->hw_id - ENCODERS_COUNT].state = TT_COUNTING;
        }

        // footer text composition
        char value_txt[32];

        //if unit=ms or unit=bpm -> use 0 decimal points
        if (strcasecmp(control->unit, "ms") == 0 || strcasecmp(control->unit, "bpm") == 0)
            i = int_to_str(control->value, value_txt, sizeof(value_txt), 0);
        //if unit=s or unit=hz or unit=something else-> use 2 decimal points
        else
            i = float_to_str(control->value, value_txt, sizeof(value_txt), 2);

        //add space to footer
        value_txt[i++] = ' ';
        strcpy(&value_txt[i], control->unit);

        // updates the footer
        screen_footer(control->hw_id - ENCODERS_COUNT, control->label, value_txt, control->properties);
    }
    else if (control->properties & FLAG_CONTROL_BYPASS)
    {
        // updates the led
        led->led_state.color = BYPASS_COLOR;
        if (control->value <= 0)
            ledz_set_state(led, LED_ON);
        else
            ledz_set_state(led, LED_OFF);

        // updates the footer
        screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                     (control->value ? BYPASS_ON_FOOTER_TEXT : BYPASS_OFF_FOOTER_TEXT), control->properties);
    }
    else if (control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
        // updates the led
        //check if its assigned to a trigger and if the button is released
        if ((control->scroll_dir == 2) || (control->scale_points_flag & FLAG_SCALEPOINT_ALT_LED_COLOR))
        {
            if (control->scale_points_flag & FLAG_SCALEPOINT_ALT_LED_COLOR)
            {
                set_alternated_led_list_colour(control);
            }
            else
            {
                led->led_state.color = ENUMERATED_COLOR;
                ledz_set_state(led, LED_ON);
            }
        }
        else
        {
            led->led_state.color = ENUMERATED_PRESSED_COLOR;
            ledz_set_state(led, LED_ON);
        }

        // locates the current value
        control->step = 0;
        for (i = 0; i < control->scale_points_count; i++)
        {
            if (control->value == control->scale_points[i]->value)
            {
                control->step = i;
                break;
            }
        }
        control->steps = control->scale_points_count;

        if (naveg_get_current_mode() == MODE_CONTROL)
        {
            //if screen overlay active, update that
            if (hardware_get_overlay_counter() || !control->scroll_dir)
            {
                CM_print_control_overlay(control, ENCODER_LIST_TIMEOUT);
            }

            // updates the footer
            screen_footer(control->hw_id - ENCODERS_COUNT, control->label, control->scale_points[i]->label, control->properties);
        }
    }
}

// control removed from foot
static void foot_control_rm(uint8_t hw_id)
{
    uint8_t i;

    if (hw_id < ENCODERS_COUNT) return;

    for (i = 0; i < MAX_FOOT_ASSIGNMENTS; i++)
    {
        // if there is no controls assigned, load the default screen
        if ((!g_foots[i]) && (naveg_get_current_mode() == MODE_CONTROL))
        {
            screen_footer(i, NULL, NULL, 0);
            continue;
        }

        // checks if effect_instance and symbol match
        if (hw_id == g_foots[i]->hw_id)
        {
            // remove the control
            data_free_control(g_foots[i]);
            g_foots[i] = NULL;

            if (naveg_get_current_mode() == MODE_CONTROL)
            {
                ledz_t *led = hardware_leds(i);
                led->led_state.color = WHITE;
                ledz_set_state(led, LED_OFF);
                
                if (g_current_overlay_actuator != g_foots[i]->hw_id)
                    screen_footer(i, NULL, NULL, 0);
                else
                {
                    hardware_set_overlay_timeout(0, CM_print_screen);
                    CM_print_screen();
                }
            }
        }
    }
}

static void parse_control_page(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;

    control_t *control = data_parse_control(&list[1]);

    CM_add_control(control, 0);
}

static void request_control_page(control_t *control, uint8_t dir)
{
    // sets the response callback
    ui_comm_webgui_set_response_cb(parse_control_page, NULL);

    char buffer[20];
    memset(buffer, 0, sizeof buffer);
    uint8_t i;

    i = copy_command(buffer, CMD_CONTROL_PAGE); 

    // insert the hw_id on buffer
    i += int_to_str(control->hw_id, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    //bitmask for the page direction and wrap around
    uint8_t bitmask = 0;
    if (dir) bitmask |= FLAG_PAGINATION_PAGE_UP;
    if ((control->hw_id >= ENCODERS_COUNT) && (control->scale_points_flag & FLAG_SCALEPOINT_WRAP_AROUND)) bitmask |= FLAG_PAGINATION_WRAP_AROUND;

    // insert the direction on buffer
    i += int_to_str(bitmask, &buffer[i], sizeof(buffer) - i, 0);

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

static void control_set(uint8_t id, control_t *control)
{
    (void) id;

    uint32_t now, delta;

    if ((control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS)) && !(control->properties & FLAG_CONTROL_MOMENTARY))
    {
        //encoder (pagination is done in the increment / decrement functions)
        if (control->hw_id < ENCODERS_COUNT)
        {
            // update the screen
            screen_encoder(control, control->hw_id);

            //display overlay
            CM_print_control_overlay(control, ENCODER_LIST_TIMEOUT);
        }
        //footswitch (need to check for pages here)
        else if ((ENCODERS_COUNT <= control->hw_id) && ( control->hw_id < MAX_FOOT_ASSIGNMENTS + ENCODERS_COUNT))
        {            
            uint8_t trigger_led_change = 0;
            ledz_t *led = hardware_leds(control->hw_id - ENCODERS_COUNT);

            // updates the led
            //check if its assigned to a trigger and if the button is released
            if ((control->scroll_dir == 2) || (control->scale_points_flag & FLAG_SCALEPOINT_ALT_LED_COLOR))
            {
                if (control->scale_points_flag & FLAG_SCALEPOINT_ALT_LED_COLOR)
                {
                    trigger_led_change = 1;
                }
                else
                {
                    led->led_state.color = ENUMERATED_COLOR;
                    ledz_set_state(led, LED_ON);
                }
            }
            else
            {
                led->led_state.color = ENUMERATED_PRESSED_COLOR;
                ledz_set_state(led, LED_ON);
            }

            if (!(control->properties & FLAG_CONTROL_REVERSE))
            {
                // increments the step
                if (control->step < (control->steps - 1))
                {
                    if ((control->scale_points_flag & FLAG_SCALEPOINT_PAGINATED) && (control->step < (control->steps - 1)))
                    {
                        //request new data, a new control we be assigned after
                        request_control_page(control, 1);

                        //since a new control is assigned we can return
                        return;
                    }

                    control->step++;
                    control->scale_point_index++;
                }
                //if we are reaching the end of the control
                else if (control->scale_points_flag & FLAG_SCALEPOINT_END_PAGE)
                {
                    //we wrap around so the step becomes 0 again
                    if (control->scale_points_flag & FLAG_SCALEPOINT_WRAP_AROUND)
                    {
                        if (control->scale_points_flag & FLAG_SCALEPOINT_PAGINATED)
                        {
                            request_control_page(control, 1);
                            return;
                        }
                        else
                        {
                            control->step = 0;
                            control->scale_point_index = 0;
                        }
                    }
                    //we are at max and dont wrap around
                    else return; 
                }
                //we need to request a new page
                else if (control->scale_points_flag & FLAG_SCALEPOINT_PAGINATED) 
                {
                    //request new data, a new control we be assigned after
                    request_control_page(control, 1);

                    //since a new control is assigned we can return
                    return;
                }
                else return;
            }
            else 
            {
                // decrements the step
                if (control->step > 2)
                {
                    control->step--;
                    control->scale_point_index--;
                }
                //we are at the end of our list ask for more data
                else if (control->scale_points_flag & (FLAG_SCALEPOINT_PAGINATED | FLAG_SCALEPOINT_WRAP_AROUND))
                {
                    //request new data, a new control we be assigned after
                    request_control_page(control, 0);
    
                    //since a new control is assigned we can return
                    return;
                }
                else return;
            }

            // updates the value and the screen
            control->value = control->scale_points[control->step]->value;

            screen_footer(control->hw_id - ENCODERS_COUNT, control->label, control->scale_points[control->step]->label, control->properties);

            if (trigger_led_change == 1)
                set_alternated_led_list_colour(control);
        }
    }
    else if (control->properties & FLAG_CONTROL_TRIGGER)
    {
        control->value = control->maximum;
        // to update the footer and screen
        foot_control_add(control);

        if (!control->scroll_dir) return;
    }
    else if (control->properties & FLAG_CONTROL_MOMENTARY)
    {
        control->value = !control->value;
        // to update the footer and screen
        foot_control_add(control);
    }
    else if (control->properties & (FLAG_CONTROL_TOGGLED | FLAG_CONTROL_BYPASS))
    {
        if (control->hw_id < ENCODERS_COUNT)
        {
            if (g_current_overlay_actuator != -1)
            {
                hardware_set_overlay_timeout(0, NULL);
                CM_print_screen();
            }

            // update the screen
            screen_encoder(control, control->hw_id);
        }
        else 
        {
            if (control->value > control->minimum) control->value = control->minimum;
            else control->value = control->maximum;

            // to update the footer and screen
            foot_control_add(control);
        }
    }
    else if (control->properties & FLAG_CONTROL_TAP_TEMPO)
    {
        now = actuator_get_click_time(hardware_actuators(FOOTSWITCH0 + (control->hw_id - ENCODERS_COUNT)));
        delta = now - g_tap_tempo[control->hw_id - ENCODERS_COUNT].time;
        g_tap_tempo[control->hw_id - ENCODERS_COUNT].time = now;

        if (g_tap_tempo[control->hw_id - ENCODERS_COUNT].state == TT_COUNTING)
        {
            // checks if delta almost suits maximum allowed value
            if ((delta > g_tap_tempo[control->hw_id - ENCODERS_COUNT].max) &&
                ((delta - TAP_TEMPO_MAXVAL_OVERFLOW) < g_tap_tempo[control->hw_id - ENCODERS_COUNT].max))
            {
                // sets delta to maxvalue if just slightly over, instead of doing nothing
                delta = g_tap_tempo[control->hw_id - ENCODERS_COUNT].max;
            }

            // checks the tap tempo timeout
            if (delta <= g_tap_tempo[control->hw_id - ENCODERS_COUNT].max)
            {
                //get current value of tap tempo in ms
                float currentTapVal = convert_to_ms(control->unit, control->value);
                //check if it should be added to running average
                if (fabs(currentTapVal - delta) < TAP_TEMPO_TAP_HYSTERESIS)
                {
                    // converts and update the tap tempo value
                    control->value = (2*(control->value) + convert_from_ms(control->unit, delta)) / 3;
                }
                else
                {
                    // converts and update the tap tempo value
                    control->value = convert_from_ms(control->unit, delta);
                }
                // checks the values bounds
                if (control->value > control->maximum) control->value = control->maximum;
                if (control->value < control->minimum) control->value = control->minimum;

                // updates the foot
                foot_control_add(control);
            }
        }
    }
    else
    {
        if (control->hw_id < ENCODERS_COUNT)
        {
            if (g_current_overlay_actuator != -1)
            {
                hardware_set_overlay_timeout(0, NULL);
                CM_print_screen();
            }

            // update the screen
            screen_encoder(control, control->hw_id);
        }
    }

    if (ENCODERS_COUNT <= control->hw_id)
        CM_print_control_overlay(control, FOOT_CONTROLS_TIMEOUT);

    char buffer[128];
    uint8_t i;

    i = copy_command(buffer, CMD_CONTROL_SET);

    // insert the hw_id on buffer
    i += int_to_str(control->hw_id, &buffer[i], sizeof(buffer) - i, 0);
    buffer[i++] = ' ';

    // insert the value on buffer
    i += float_to_str(control->value, &buffer[i], sizeof(buffer) - i, 3);
    buffer[i] = 0;

    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    //wait for a response from mod-ui
    if (g_should_wait_for_webgui) {
        ui_comm_webgui_wait_response();
    }

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void CM_init(void)
{
    uint32_t i;

    for (i = 0; i < ENCODERS_COUNT; i++)
    {
        g_controls[i] = NULL;
    }

    // initialize the global variables
    for (i = 0; i < MAX_FOOT_ASSIGNMENTS; i++)
    {
        // initialize the foot controls pointers
        g_foots[i] = NULL;

        // initialize the tap tempo
        g_tap_tempo[i].state = TT_INIT;
    }

    system_hide_actuator_cb(NULL, MENU_EV_NONE);
}

void CM_remove_control(uint8_t hw_id)
{
    if (!g_initialized) return;

    if (hw_id < 3) encoder_control_rm(hw_id);
    else foot_control_rm(hw_id);
}

void CM_add_control(control_t *control, uint8_t protocol)
{
    if (!g_initialized) return;
    if (!control) return;

    // first tries remove the control
    CM_remove_control(control->hw_id);

    if (protocol) control->scroll_dir = 2;
    else control->scroll_dir = 0;

    if (control->hw_id < ENCODERS_COUNT)
    {
        encoder_control_add(control);
    }
    else
    {
        foot_control_add(control);     
    }
}

void CM_inc_control(uint8_t encoder)
{
    control_t *control = g_controls[encoder];

    //no control
    if (!control) return;

    if (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE))
    {
        //prepare display overlay
        CM_print_control_overlay(control, ENCODER_LIST_TIMEOUT);

        if (control->scale_points_flag & FLAG_SCALEPOINT_PAGINATED)
        {
            // increments the step
            if (control->step < (control->steps - 3))
                control->step++;
            //we are at the end of our list ask for more data
            else
            {
                //request new data, a new control we be assigned after
                request_control_page(control, 1);

                //since a new control is assigned we can return
                return;
            }       
        }
        else 
        {
            // increments the step
            if (control->step < (control->steps - 1))
                control->step++;
            else
                return; 
        }
    }
    else if (control->properties & FLAG_CONTROL_TOGGLED)
    {
        if (control->value == 1)
            return;
        else 
            control->value = 1;
    }
    else if (control->properties & FLAG_CONTROL_BYPASS)
    {
        if (control->value == 0)
            return;
        else 
            control->value = 0;
    }
    else
    {
        // increments the step
        if (control->step < (control->steps - 1))
        {
            if (g_encoders_pressed[encoder])
                control->step+=10;
            else
                control->step++;
        
            if (control->step > (control->steps - 1))
                control->step = (control->steps - 1);
        }
        else
            return;
    }
    // converts the step to absolute value
    step_to_value(control);

    // applies the control value
    control_set(encoder, control);
}

void CM_dec_control(uint8_t encoder)
{
    control_t *control = g_controls[encoder];

    //no control, return
    if (!control) return;
    
    if  (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE)) 
    {
        //prepare display overlay
        CM_print_control_overlay(control, ENCODER_LIST_TIMEOUT);

        if (control->scale_points_flag & FLAG_SCALEPOINT_PAGINATED)
        {
            // decrements the step
            if (control->step > 2)
                control->step--;
            //we are at the end of our list ask for more data
            else
            {
                //request new data, a new control we be assigned after
                request_control_page(control, 0);

                //since a new control is assigned we can return
                return;
            }
        }
        else 
        {
            // decrements the step
            if (control->step > 0)
                control->step--;
            else
                return;
        }
    }
    else if (control->properties & FLAG_CONTROL_TOGGLED)
    {
        if (control->value == 0)
            return;
        else 
            control->value = 0;
    }
    else if (control->properties & FLAG_CONTROL_BYPASS)
    {
        if (control->value == 1)
            return;
        else 
            control->value = 1;
    }
    else
    {
        // decrements the step
        if (control->step > 0)
        {
            if (g_encoders_pressed[encoder])
                control->step-=10;
            else
                control->step--;

            if (control->step < 0)
                control->step = 0;
        }
        else
            return;
    }
    // converts the step to absolute value
    step_to_value(control);

    // applies the control value
    control_set(encoder, control);
}

void CM_toggle_control(uint8_t encoder)
{
    control_t *control = g_controls[encoder];

    //no control
    if (!control) return;

    if (control->properties & FLAG_CONTROL_TRIGGER)
    {
        control->value = control->maximum;
    }
    else if ((control->properties & FLAG_CONTROL_TOGGLED) || (control->properties & FLAG_CONTROL_BYPASS))
    {
        control->value = !control->value;
    }
    else
    {
        return;
    }

    // applies the control value
    control_set(encoder, control);
}

void CM_foot_control_change(uint8_t foot, uint8_t value)
{
    // checks if there is assigned control
    if (g_foots[foot] == NULL) 
        return;

    if (!value)
    {
        ledz_t *led = hardware_leds(foot);
        //check if we use the release action for this actuator
        if (g_foots[foot]->properties & (FLAG_CONTROL_MOMENTARY | FLAG_CONTROL_TRIGGER))
        {
            led->led_state.color = TRIGGER_COLOR;
            ledz_set_state(led, LED_ON);
        }
        else if (g_foots[foot]->properties & (FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION))  
        {
                if (g_foots[foot]->scale_points_flag & FLAG_SCALEPOINT_ALT_LED_COLOR)
                {
                    set_alternated_led_list_colour(g_foots[foot]);
                }
                else
                {
                    led->led_state.color = ENUMERATED_COLOR;
                    ledz_set_state(led, LED_ON);
                }
        }

        //not used right now anymore, maybe in the future, TODO: rename to actuator flag
        g_foots[foot]->scroll_dir = value;

        //we dont actually preform an action here
        if (!(g_foots[foot]->properties & FLAG_CONTROL_MOMENTARY))
            return;
    }

    g_foots[foot]->scroll_dir = value;

    control_set(foot, g_foots[foot]);;
}

void CM_set_control(uint8_t hw_id, float value)
{
    if (!g_initialized) return;

    control_t *control = NULL;

    //encoder
    if (hw_id < ENCODERS_COUNT)
    {
        control = g_controls[hw_id];
    }
    //button
    else if (hw_id < MAX_FOOT_ASSIGNMENTS + ENCODERS_COUNT)
    {
        control = g_foots[hw_id - ENCODERS_COUNT];
    }

    if (control)
    {
        control->value = value;
        if (value < control->minimum)
            control->value = control->minimum;
        if (value > control->maximum)
            control->value = control->maximum;

        // updates the step value
        control->step =
            (control->value - control->minimum) / ((control->maximum - control->minimum) / control->steps);

        if ((naveg_get_current_mode() != MODE_CONTROL) || g_actuator_display_lock)
            return;

        //encoder
        if (hw_id < ENCODERS_COUNT)
        {
            screen_encoder(control, control->hw_id);
        }
        //button
        else if (hw_id < MAX_FOOT_ASSIGNMENTS + ENCODERS_COUNT)
        {
            ledz_t *led = hardware_leds(control->hw_id - ENCODERS_COUNT);
            uint8_t i;
            //not implemented, not sure if ever needed
            if (control->properties & FLAG_CONTROL_MOMENTARY)
            {
                // updates the footer
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                             (control->value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT), control->properties);

                return;
            }
            // trigger specification: http://lv2plug.in/ns/ext/port-props/#trigger
            else if (control->properties & FLAG_CONTROL_TRIGGER)
            {
                // updates the led
                //check if its assigned to a trigger and if the button is released
                if (!control->scroll_dir)
                {
                    led->led_state.color = TRIGGER_COLOR;
                    ledz_set_state(led, LED_ON);
                    return;
                }
                else if (control->scroll_dir == 2)
                {
                    led->led_state.color = TRIGGER_COLOR;
                    ledz_set_state(led, LED_ON);
                }
                else
                {
                    led->led_state.color = TRIGGER_PRESSED_COLOR;
                    ledz_set_state(led, LED_ON);
                }

                // updates the footer (a getto fix here, the screen.c file did not regognize the NULL pointer so it did not allign the text properly, TODO fix this)
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label, BYPASS_ON_FOOTER_TEXT, control->properties);
            }

            // toggled specification: http://lv2plug.in/ns/lv2core/#toggled
            else if (control->properties & FLAG_CONTROL_TOGGLED)
            {
                // updates the led
                if (control->value <= 0)
                {
                    led->led_state.color = TOGGLED_COLOR;
                    ledz_set_state(led, LED_OFF);
                }
                else
                {
                    led->led_state.color = TOGGLED_COLOR;
                    ledz_set_state(led, LED_ON);
                }


                // updates the footer
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                             (control->value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT), control->properties);
            }
            else if (control->properties & FLAG_CONTROL_TAP_TEMPO)
            {
                // convert the time unit
                uint16_t time_ms = (uint16_t)(convert_to_ms(control->unit, control->value) + 0.5);

                led->led_state.color = TAP_TEMPO_COLOR;
                led->led_state.amount_of_blinks = LED_BLINK_INFINIT;

                // setup the led blink
                if (time_ms > TAP_TEMPO_TIME_ON)
                {
                    led->led_state.time_on = TAP_TEMPO_TIME_ON;
                    led->led_state.time_off = time_ms - TAP_TEMPO_TIME_ON;
                }
                else
                {
                    led->led_state.time_on = time_ms / 2;
                    led->led_state.time_off = time_ms / 2;
                }

                ledz_set_state(led, LED_BLINK);

                // calculates the maximum tap tempo value
                if (g_tap_tempo[control->hw_id - ENCODERS_COUNT].state == TT_INIT)
                {
                    uint32_t max;

                    // time unit (ms, s)
                    if (strcmp(control->unit, "ms") == 0 || strcmp(control->unit, "s") == 0)
                    {
                        max = (uint32_t)(convert_to_ms(control->unit, control->maximum) + 0.5);
                        //makes sure we enforce a proper timeout
                        if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                            max = TAP_TEMPO_DEFAULT_TIMEOUT;
                    }
                    // frequency unit (bpm, Hz)
                    else
                    {
                        //prevent division by 0 case
                        if (control->minimum == 0)
                            max = TAP_TEMPO_DEFAULT_TIMEOUT;
                        else
                            max = (uint32_t)(convert_to_ms(control->unit, control->minimum) + 0.5);
                        //makes sure we enforce a proper timeout
                        if (max > TAP_TEMPO_DEFAULT_TIMEOUT)
                            max = TAP_TEMPO_DEFAULT_TIMEOUT;
                    }

                    g_tap_tempo[control->hw_id - ENCODERS_COUNT].max = max;
                    g_tap_tempo[control->hw_id - ENCODERS_COUNT].state = TT_COUNTING;
                }

                // footer text composition
                char value_txt[32];

                //if unit=ms or unit=bpm -> use 0 decimal points
                if (strcasecmp(control->unit, "ms") == 0 || strcasecmp(control->unit, "bpm") == 0)
                    i = int_to_str(control->value, value_txt, sizeof(value_txt), 0);
                //if unit=s or unit=hz or unit=something else-> use 2 decimal points
                else
                    i = float_to_str(control->value, value_txt, sizeof(value_txt), 2);

                //add space to footer
                value_txt[i++] = ' ';
                strcpy(&value_txt[i], control->unit);

                // updates the footer
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label, value_txt, control->properties);
            }
            else if (control->properties & FLAG_CONTROL_BYPASS)
            {
                // updates the led
                if (control->value <= 0)
                {
                    led->led_state.color = BYPASS_COLOR;
                    ledz_set_state(led, LED_ON);
                }
                else
                {
                    led->led_state.color = BYPASS_COLOR;
                    ledz_set_state(led, LED_OFF);
                }

                // updates the footer
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                             (control->value ? BYPASS_ON_FOOTER_TEXT : BYPASS_OFF_FOOTER_TEXT), control->properties);
            }
            else if (control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
            {
                // updates the led
                if (control->scale_points_flag & FLAG_SCALEPOINT_ALT_LED_COLOR)
                {
                    set_alternated_led_list_colour(control);
                }
                else
                {   
                    led->led_state.color = ENUMERATED_COLOR;
                    ledz_set_state(led, LED_ON);
                }

                // locates the current value
                control->step = 0;
                for (i = 0; i < control->scale_points_count; i++)
                {
                    if (control->value == control->scale_points[i]->value)
                    {
                        control->step = i;
                        break;
                    }
                }
                control->steps = control->scale_points_count;

                // updates the footer
                screen_footer(control->hw_id - ENCODERS_COUNT, control->label, control->scale_points[i]->label, control->properties);
            }
        }
    }
}

float CM_get_control_value(uint8_t hw_id)
{
    if (!g_initialized) return 0.0;

    control_t *control;

    if (hw_id < 3)
    {
        control = g_controls[hw_id];
    }
    else 
    {
        control = g_foots[hw_id - ENCODERS_COUNT];
    }

    if (control) return control->value;

    return 0.0;
}

uint8_t CM_tap_tempo_status(uint8_t id)
{
    if (g_tap_tempo[id].state == TT_INIT) return 0;
    else return 1;
}

//function that draws the 3 encoders
void CM_draw_encoders(void)
{
    uint8_t i;

    for (i = 0; i < ENCODERS_COUNT; i++)
    {
        // checks the function assigned to foot and update the footer
        if (g_controls[i])
        {
            screen_encoder(g_controls[i], i);
        }
        else
        {
            screen_encoder(NULL, i);
        }
    }
}

//function that draws the 2 foots
void CM_draw_foots(void)
{
    uint8_t i;

    for (i = 0; i < MAX_FOOT_ASSIGNMENTS; i++)
    {
        // checks the function assigned to foot and update the footer
        if (g_foots[i])
        {
            //prevent toggling of pressed light
            g_foots[i]->scroll_dir = 2;

            foot_control_add(g_foots[i]);
        }
        else
        {
            screen_footer(i, NULL, NULL, 0);
        }
    }
}

void CM_load_next_page()
{
    uint8_t pagefound = 0;
    uint8_t j = g_current_foot_control_page;
    char buffer[30];
    
    while (!pagefound)
    {
        j++;
        if (j >= FOOTSWITCH_PAGES_COUNT)
        {
            j = 0;
        }

        //we went in a loop, only one page
        if (j == g_current_foot_control_page)
        {
            break;
        }

        //page found
        if (g_fs_page_available[j])
        {
            g_current_foot_control_page = j;
            pagefound = 1;
            break;
        }
    }

    if (!pagefound)
        return;

    hardware_set_overlay_timeout(0, NULL);
    g_current_overlay_actuator = -1;

    uint8_t i = copy_command(buffer, CMD_NEXT_PAGE);
    i += int_to_str(g_current_foot_control_page, &buffer[i], sizeof(buffer) - i, 0);

    //clear controls            
    uint8_t q;
    for (q = 0; q < TOTAL_CONTROL_ACTUATORS; q++)
    {
        CM_remove_control(q);
    }

    g_current_encoder_page = 0;

    //update screen
    screen_page_index(g_current_foot_control_page, foot_pages_available());

    screen_encoder_container(g_current_encoder_page);
    CM_draw_encoders();

    CM_set_leds();

    //clear actuator queue
    reset_queue();

    ui_comm_webgui_clear();

    //lock actuators
    g_protocol_busy = true;
    system_lock_comm_serial(g_protocol_busy);

    ui_comm_webgui_send(buffer, i);

    ui_comm_webgui_wait_response();

    g_protocol_busy = false;
    system_lock_comm_serial(g_protocol_busy);
}

void CM_load_next_encoder_page(uint8_t button)
{
    char buffer[30];
    uint8_t i = 0;

    g_current_encoder_page = button;

    hardware_set_overlay_timeout(0, NULL);
    g_current_overlay_actuator = -1;

    screen_encoder_container(g_current_encoder_page); 

    //clear controls            
    uint8_t q;
    for (q = 0; q < ENCODERS_COUNT; q++)
    {
        i = copy_command(buffer, CMD_DWARF_CONTROL_SUBPAGE);
        i += int_to_str(q, &buffer[i], sizeof(buffer) - i, 0);

        // inserts one space
        buffer[i++] = ' ';

        i += int_to_str(g_current_encoder_page, &buffer[i], sizeof(buffer) - i, 0);

        CM_remove_control(q);

        ui_comm_webgui_send(buffer, i);

        ui_comm_webgui_wait_response();
    }

    //update LED's
    set_encoder_pages_led_state();

    CM_print_screen();  
}

void CM_set_state(void)
{
    CM_print_screen();

    CM_set_leds();
}

void CM_set_leds(void)
{
    set_footswitch_pages_led_state();

    set_encoder_pages_led_state();

    restore_led_states();
}

void CM_print_screen(void)
{
    //we are sure we are not in overlay anymore
    g_current_overlay_actuator = -1;

    screen_clear();

    screen_tittle(NULL, 0);

    //update screen
    screen_page_index(g_current_foot_control_page, foot_pages_available());

    screen_encoder_container(g_current_encoder_page);

    CM_draw_foots();

    CM_draw_encoders();
}

void CM_print_control_overlay(control_t *control, uint16_t overlay_time)
{
    g_current_overlay_actuator = control->hw_id;

    screen_control_overlay(control);

    hardware_set_overlay_timeout(overlay_time, CM_print_screen);
}

void CM_set_pages_available(uint8_t page_toggles[8])
{
    memcpy(g_fs_page_available, page_toggles, sizeof(g_fs_page_available));

    if  (naveg_get_current_mode() == MODE_CONTROL)
        screen_page_index(g_current_foot_control_page, foot_pages_available());
}

void CM_reset_encoder_page(void)
{
    g_current_encoder_page = 0;

    if (naveg_get_current_mode() == MODE_CONTROL)
    {
        screen_encoder_container(g_current_encoder_page);

        //update LED's
        set_encoder_pages_led_state();  
    }
}