
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
#include "protocol.h"
#include "glcd.h"
#include "ledz.h"
#include "actuator.h"
#include "naveg.h"
#include "screen.h"
#include "ui_comm.h"
#include "sys_comm.h"
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
static list_clone_t g_list_clone[ENCODERS_COUNT];

uint8_t g_scroll_dir = 1;

static uint8_t g_current_foot_control_page = 0;
static uint8_t g_current_encoder_page = 0;
static uint8_t g_fs_page_available[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t g_available_foot_pages = 0;
static int8_t g_current_overlay_actuator = -1;
static bool g_list_click = 0;
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
    if (control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
        control->value = control->scale_points[control->step]->value;
    }
    else if (control->properties & FLAG_CONTROL_LOGARITHMIC)
    {
        control->value = control->minimum * pow(control->maximum / control->minimum, p_step);
    }
    else if (!(control->properties & (FLAG_CONTROL_TRIGGER | FLAG_CONTROL_TOGGLED | FLAG_CONTROL_BYPASS)))
    {
        control->value = (p_step * (control->maximum - control->minimum)) + control->minimum;
    }

    if (control->value > control->maximum) control->value = control->maximum;
    if (control->value < control->minimum) control->value = control->minimum;
}

static void reset_list_encoders(void)
{
    uint8_t q, i;
    for (q = 0; q < ENCODERS_COUNT; q++) {
        control_t *control = g_controls[q];

        //do we even have a control
        if (!control)
            continue;

        //is there even a list control to check?
        if (!(control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS)))
            continue;

        //are the values out of sync?
        if (control->scale_point_index == g_list_clone[control->hw_id].scale_point_index)
            continue;

        //clear old list, free memory
        if (control->scale_points)
        {
            for (i = 0; i < control->scale_points_count; i++) {
                if (control->scale_points[i]) {
                    FREE(control->scale_points[i]->label);
                    FREE(control->scale_points[i]);
                }
            }

            FREE(control->scale_points);
        }

        //restore list from the local clone
        control->scale_points_count = g_list_clone[control->hw_id].scale_points_count;

        control->scale_points = (scale_point_t **) MALLOC(sizeof(scale_point_t*) * control->scale_points_count);

        // initializes the scale points pointers
        for (i = 0; i < control->scale_points_count; i++) control->scale_points[i] = NULL;

        for (i = 0; i < control->scale_points_count; i++) {
            control->scale_points[i] = (scale_point_t *) MALLOC(sizeof(scale_point_t));
            control->scale_points[i]->label = str_duplicate(g_list_clone[control->hw_id].scale_points[i]->label);
            control->scale_points[i]->value = g_list_clone[control->hw_id].scale_points[i]->value;
        }

        control->step = g_list_clone[control->hw_id].step;
        control->scale_point_index = g_list_clone[control->hw_id].scale_point_index;
    }
}

static void clone_list_encoders(control_t *control)
{
    uint8_t i;

    //if already a list, free memory
    if (g_list_clone[control->hw_id].hw_id != -1) {
        for (i = 0; i < g_list_clone[control->hw_id].scale_points_count; i++)
        {
            if (g_list_clone[control->hw_id].scale_points[i])
            {
                FREE(g_list_clone[control->hw_id].scale_points[i]->label);
                FREE(g_list_clone[control->hw_id].scale_points[i]);
            }
        }

        FREE(g_list_clone[control->hw_id].scale_points);
    }

    //check if we need to clone this encoder (scalepoints)
    if (control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS)) {
        g_list_clone[control->hw_id].scale_points_count = control->scale_points_count;

        g_list_clone[control->hw_id].scale_points = (scale_point_t **) MALLOC(sizeof(scale_point_t*) * control->scale_points_count);

        // initializes the scale points pointers
        for (i = 0; i < control->scale_points_count; i++) g_list_clone[control->hw_id].scale_points[i] = NULL;

        for (i = 0; i < control->scale_points_count; i++) {
            g_list_clone[control->hw_id].scale_points[i] = (scale_point_t *) MALLOC(sizeof(scale_point_t));
            g_list_clone[control->hw_id].scale_points[i]->label = str_duplicate(control->scale_points[i]->label);
            g_list_clone[control->hw_id].scale_points[i]->value = control->scale_points[i]->value;
        }

        g_list_clone[control->hw_id].hw_id = control->hw_id;
        g_list_clone[control->hw_id].step = control->step;
        g_list_clone[control->hw_id].scale_point_index = control->scale_point_index;
    }
}

static void send_control_set(control_t *control)
{
    char buffer[128];
    uint8_t i;

    i = copy_command(buffer, CMD_CONTROL_SET);

    // insert the hw_id on buffer
    i += int_to_str(control->hw_id, &buffer[i], sizeof(buffer) - i, 0);
    buffer[i++] = ' ';

    // insert the value on buffer
    i += float_to_str(control->value, &buffer[i], sizeof(buffer) - i, 3);
    buffer[i] = 0;

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    //wait for a response from mod-ui
    ui_comm_webgui_wait_response();
}

void set_footswitch_pages_led_state(void)
{
    ledz_t *led = hardware_leds(2);

    led->led_state.color = FS_PAGE_COLOR_1 + g_current_foot_control_page;
    ledz_set_state(led, LED_ON, LED_UPDATE);
}

void set_encoder_pages_led_state(void)
{
    uint8_t i;
    ledz_t *led;
    
    for (i = 0; i < ENCODER_PAGES_COUNT; i++)
    {
        led = hardware_leds(i+ENCODER_PAGES_COUNT);
        led->led_state.color = FS_PAGE_COLOR_1 + g_current_foot_control_page;

        if (i == g_current_encoder_page)
        {
            ledz_set_state(led, LED_ON, LED_UPDATE);
        }
        else
        {
            led->led_state.brightness = 0.25;
            ledz_set_state(led, LED_DIMMED, LED_UPDATE);
        }

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

static void load_control_page(uint8_t page)
{
    //first notify host
    char val_buffer[20] = {0};
    sys_comm_set_response_cb(NULL, NULL);

    int_to_str(page, val_buffer, sizeof(val_buffer), 0);

    sys_comm_send(CMD_SYS_PAGE_CHANGE, val_buffer);

    //now notify mod-ui
    char buffer[30];

    hardware_set_overlay_timeout(0, NULL);
    g_current_overlay_actuator = -1;

    uint8_t i = copy_command(buffer, CMD_NEXT_PAGE);
    i += int_to_str(page, &buffer[i], sizeof(buffer) - i, 0);

    //clear controls
    uint8_t q = 0;
    for (q = 0; q < TOTAL_CONTROL_ACTUATORS; q++)
    {
        CM_remove_control(q);
    }

    g_current_encoder_page = 0;

    CM_set_state();

    //clear actuator queue
    reset_queue();

    ui_comm_webgui_clear();

    ui_comm_webgui_send(buffer, i);
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

    if (control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
        control->step = 0;
        uint8_t i;
        for (i = 0; i < control->scale_points_count; i++)
        {
            if (control->value == control->scale_points[i]->value)
            {
                control->step = i;
                break;
            }
        }

        clone_list_encoders(control);

    }
    else if (control->properties & FLAG_CONTROL_LOGARITHMIC)
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
        if (hardware_get_overlay_counter() == 0)
            screen_encoder(NULL, hw_id);
        return;
    }

    control_t *control = g_controls[hw_id];

    if (control)
    {
        data_free_control(control);
        g_controls[hw_id] = NULL;
        if ((naveg_get_current_mode() == MODE_CONTROL) && (hardware_get_overlay_counter() == 0))
            screen_encoder(NULL, hw_id);
    }
}

static void set_alternated_led_list_colour(control_t *control)
{
    ledz_t *led = hardware_leds(control->hw_id - ENCODERS_COUNT);

    led->led_state.color = LED_LIST_COLOR_1 + (control->scale_point_index % LED_LIST_AMOUNT_OF_COLORS);

    ledz_set_state(led, LED_ON, LED_UPDATE);
}

static void foot_control_print(control_t *control)
{
    uint8_t i;

    if (control->properties & FLAG_CONTROL_MOMENTARY)
    {
        // updates the footer
        screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                     (control->value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT), control->properties);
    }
    else if (control->properties & FLAG_CONTROL_TRIGGER)
    {
        // trigger specification: http://lv2plug.in/ns/ext/port-props/#trigger
        // updates the footer (a getto fix here, the screen.c file did not regognize the NULL pointer so it did not allign the text properly, TODO fix this)
        screen_footer(control->hw_id - ENCODERS_COUNT, control->label, BYPASS_ON_FOOTER_TEXT, control->properties);
    }
    else if (control->properties & FLAG_CONTROL_TOGGLED)
    {
        // toggled specification: http://lv2plug.in/ns/lv2core/#toggled
        // updates the footer
        screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                     (control->value <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT), control->properties);
    }
    else if (control->properties & FLAG_CONTROL_TAP_TEMPO)
    {
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
        // updates the footer
        screen_footer(control->hw_id - ENCODERS_COUNT, control->label,
                     (control->value ? BYPASS_ON_FOOTER_TEXT : BYPASS_OFF_FOOTER_TEXT), control->properties);
    }
    else if (control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
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

        if (naveg_get_current_mode() == MODE_CONTROL)
        {
            //if screen overlay active, update that
            if ((hardware_get_overlay_counter() || !control->scroll_dir) && (g_current_overlay_actuator == control->hw_id))
            {
                CM_print_control_overlay(control, ENCODER_LIST_TIMEOUT);
            }

            // updates the footer
            screen_footer(control->hw_id - ENCODERS_COUNT, control->label, control->scale_points[i]->label, control->properties);
        }
    }
}

// control assigned to foot
static void foot_control_add(control_t *control)
{
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

    //check if we need to change widget:
    if ((g_foots[control->hw_id - ENCODERS_COUNT]->properties & FLAG_CONTROL_REVERSE) &&
        !(g_foots[control->hw_id - ENCODERS_COUNT]->properties & FLAG_CONTROL_MOMENTARY))
        screen_group_foots(1);

    //dont set ui when not in control mode
    if (naveg_get_current_mode() != MODE_CONTROL)
    {
        CM_set_foot_led(control, LED_STORE_STATE);
        return;
    }

    foot_control_print(control);
    CM_set_foot_led(control, LED_UPDATE);
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
            //check if we need to change widget:
            if (g_foots[i]->properties & FLAG_CONTROL_REVERSE)
                screen_group_foots(0);

            //if color was taken by hmi_widgets, invalid so normal leds work again
            if (ledz_color_valid(MAX_COLOR_ID + hw_id-ENCODERS_COUNT +1))
            {
                g_foots[i]->lock_led_actions = 0;
                int8_t value[3] = {-1, -1, -1};
                ledz_set_color(MAX_COLOR_ID + hw_id-ENCODERS_COUNT +1, value);
            }

            // remove the control
            data_free_control(g_foots[i]);
            g_foots[i] = NULL;

            ledz_t *led = hardware_leds(i);
            led->led_state.color = WHITE;

            if (naveg_get_current_mode() == MODE_CONTROL)
            {
                ledz_set_state(led, LED_OFF, LED_UPDATE);
                
                if (g_current_overlay_actuator == hw_id) {
                    hardware_set_overlay_timeout(0, NULL);
                    CM_print_screen();
                }
                else
                    screen_footer(i, NULL, NULL, 0);
            }
            else
                ledz_set_state(led, LED_OFF, LED_STORE_STATE);
        }
    }
}

static void parse_control_page(void *data, menu_item_t *item)
{
    (void) item;
    char **list = data;

    control_t *control = data_parse_control(&list[1]);

    // first tries remove the control
    CM_remove_control(control->hw_id);

    control->scroll_dir = 0;

    if (control->hw_id < ENCODERS_COUNT)
        g_controls[control->hw_id] = control;
    else
        g_foots[control->hw_id - ENCODERS_COUNT] = control;
}

static void request_control_page(control_t *control, uint8_t dir)
{
    // sets the response callback
    ui_comm_webgui_set_response_cb(parse_control_page, NULL);

    char buffer[20];
    memset(buffer, 0, sizeof buffer);
    uint8_t i;
    uint8_t hw_id = control->hw_id;

    i = copy_command(buffer, CMD_CONTROL_PAGE); 

    // insert the hw_id on buffer
    i += int_to_str(hw_id, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    //bitmask for the page direction and wrap around
    uint8_t bitmask = 0;
    if (dir) bitmask |= FLAG_PAGINATION_PAGE_UP;
    if ((control->hw_id >= ENCODERS_COUNT) && (control->scale_points_flag & FLAG_SCALEPOINT_WRAP_AROUND)) bitmask |= FLAG_PAGINATION_WRAP_AROUND;

    // insert the direction on buffer
    i += int_to_str(bitmask, &buffer[i], sizeof(buffer) - i, 0);

    // inserts one space
    buffer[i++] = ' ';

    if (dir) control->scale_point_index++;
    else control->scale_point_index--;

    // insert the index on buffer
    i += int_to_str(control->scale_point_index, &buffer[i], sizeof(buffer) - i, 0);

    uint16_t current_index = control->scale_point_index;
    uint16_t current_step = control->step;

    // sends the data to GUI
    ui_comm_webgui_send(buffer, i);

    // waits the pedalboards list be received
    ui_comm_webgui_wait_response();

    //encoder
    if (hw_id < ENCODERS_COUNT) {
        g_controls[hw_id]->scale_point_index = current_index;
        g_controls[hw_id]->step = current_step;

        //also set the control if needed
        if (!g_list_click) {
            step_to_value(g_controls[hw_id]);
            send_control_set(g_controls[hw_id]);

            //in case the user switches modes
            clone_list_encoders(g_controls[hw_id]);
        }

        //if screen overlay active, update that
        if ((hardware_get_overlay_counter() || !g_controls[hw_id]->scroll_dir) && (g_current_overlay_actuator == g_controls[hw_id]->hw_id)) {
            CM_print_control_overlay(g_controls[hw_id], ENCODER_LIST_TIMEOUT);
            return;
        }

        // update the control screen
        if (g_current_overlay_actuator == -1)
            screen_encoder(g_controls[hw_id], hw_id);
    }
    //foot
    else {
        g_foots[hw_id - ENCODERS_COUNT]->scale_point_index = current_index;
        g_foots[hw_id - ENCODERS_COUNT]->step = current_step;

        step_to_value(g_foots[hw_id - ENCODERS_COUNT]);
        send_control_set(g_foots[hw_id - ENCODERS_COUNT]);

        //check if we need to change widget:
        if ((g_foots[hw_id - ENCODERS_COUNT]->properties & FLAG_CONTROL_REVERSE) &&
            !(g_foots[hw_id - ENCODERS_COUNT]->properties & FLAG_CONTROL_MOMENTARY))
            screen_group_foots(1);

        //dont set ui when not in control mode
        if (naveg_get_current_mode() != MODE_CONTROL) {
            CM_set_foot_led(g_foots[hw_id - ENCODERS_COUNT], LED_STORE_STATE);
            return;
        }

        foot_control_print(g_foots[hw_id - ENCODERS_COUNT]);
        CM_set_foot_led(g_foots[hw_id - ENCODERS_COUNT], LED_UPDATE);
        CM_print_control_overlay(g_foots[hw_id - ENCODERS_COUNT], FOOT_CONTROLS_TIMEOUT);
    }
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
            //take care of led
            uint8_t trigger_led_change = 0;
            ledz_t *led = hardware_leds(control->hw_id - ENCODERS_COUNT);
            //check if its assigned to a trigger and if the button is released
            if ((control->scroll_dir) || (control->scale_points_flag & FLAG_SCALEPOINT_ALT_LED_COLOR)) {
                if (control->scale_points_flag & FLAG_SCALEPOINT_ALT_LED_COLOR)
                    trigger_led_change = 1;
                else {
                    led->led_state.color = ENUMERATED_COLOR;
                    ledz_set_state(led, LED_ON, LED_UPDATE);
                }
            }
            else {
                led->led_state.brightness = 0.1;
                ledz_set_state(led, LED_DIMMED, LED_UPDATE);
            }

            //are we going up or down in the list?
            //up
            if (!(control->properties & FLAG_CONTROL_REVERSE)) {
                //are we about to reach the end of a control
                if ((control->scale_points_flag & FLAG_SCALEPOINT_END_PAGE) || (control->scale_point_index >= (control->steps - 1))) {
                    //we wrap around so the step becomes 0 again
                    if (control->scale_point_index >= (control->steps - 1)) {
                        if (control->scale_points_flag & (FLAG_SCALEPOINT_WRAP_AROUND)) {
                            control->step = 0;
                            if (control->properties & FLAG_CONTROL_LOGARITHMIC) {
                                control->scale_point_index = 0;
                                step_to_value(control);
                                send_control_set(control);
                                foot_control_print(control);
                                CM_set_foot_led(control, LED_UPDATE);
                                CM_print_control_overlay(control, FOOT_CONTROLS_TIMEOUT);
                            }
                            else {
                                control->scale_point_index = -1;
                                request_control_page(control, 1);
                            }
                            return;
                        }
                        else
                            return;
                    }
                }
                //are we about to reach the end of a page
                else if (((control->step >= (control->scale_points_count - 3))) && (control->scale_points_flag & FLAG_SCALEPOINT_PAGINATED)) {
                    //request new data, a new control we be assigned after
                    request_control_page(control, 1);
                    return;
                }

                //all good, just increment
                control->step++;
                control->scale_point_index++;
            }
            //down
            else {
                if (control->scale_point_index <= 2) {
                    if (control->scale_point_index <= 0) {
                        return;
                    }
                }
                //are we about to reach the end of a page
                else if (((control->step <= (control->scale_points_count - 3))) && (control->scale_points_flag & FLAG_SCALEPOINT_PAGINATED)){
                    //request new data, a new control we be assigned after
                    request_control_page(control, 0);
                    return;
                }

                control->step--;
                control->scale_point_index--;
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
            control->value = control->maximum;
            // to update the footer and screen
            foot_control_add(control);

            if (!control->scroll_dir) return;
        }
    }
    else if (control->properties & FLAG_CONTROL_MOMENTARY)
    {
        if (control->properties & FLAG_CONTROL_REVERSE)
            control->value = control->scroll_dir;
        else
            control->value = 1 - control->scroll_dir;

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

    if (g_list_click && (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE)) 
        && (control->hw_id < ENCODERS_COUNT))
        return;

    send_control_set(control);
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
        g_list_clone[i].hw_id = -1;
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
    system_control_header_cb(NULL, MENU_EV_NONE);
    system_click_list_cb(NULL, MENU_EV_NONE);
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

control_t *CM_get_control(uint8_t hw_id)
{
    if (!g_initialized) return NULL;

    if (hw_id < ENCODERS_COUNT)
        return g_controls[hw_id];
    else
        return g_foots[hw_id - ENCODERS_COUNT];
}

void CM_inc_control(uint8_t encoder)
{
    control_t *control = g_controls[encoder];

    //no control
    if (!control) return;

    if (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE)) {
        //prepare display overlay
        CM_print_control_overlay(control, ENCODER_LIST_TIMEOUT);

        if (control->scale_points_flag & FLAG_SCALEPOINT_PAGINATED) {
            // increments the step
            if (control->step < (control->scale_points_count - 3)) {
                if (control->scale_point_index > control->steps-1)
                    return;

                control->step++;
                control->scale_point_index++;
            }
            //we are at the end of our list ask for more data
            else {
                if ((control->scale_point_index >= control->steps - 2) ) {

                    if (control->scale_point_index > control->steps - 1)
                        return;

                    control->step++;
                    control->scale_point_index++;

                    if (!g_list_click) {
                        // converts the step to absolute value
                        step_to_value(control);

                        //make sure to save this value, in case the user switches mode
                        clone_list_encoders(control);
                    }

                    // applies the control value
                    control_set(encoder, control);
                }
                else if (control->scale_point_index < control->steps - 1) {
                    //request new data, a new control we be assigned after
                    request_control_page(control, 1);
                }

                //since a new control is assigned we can return
                return;
            }       
        }
        else  {
            // increments the step
            if ((control->step < (control->steps)) && (control->step < (control->scale_points_count))) {
                control->scale_point_index++;
                control->step++;
            }
            else
                return; 
        }
    }
    else if (control->properties & FLAG_CONTROL_TRIGGER) {
        control->value = control->maximum;
    }
    else if (control->properties & FLAG_CONTROL_TOGGLED) {
        if (control->value == 1)
            return;
        else 
            control->value = 1;
    }
    else if (control->properties & FLAG_CONTROL_BYPASS) {
        if (control->value == 0)
            return;
        else 
            control->value = 0;
    }
    else {
        // increments the step
        if (control->step < (control->steps - 1)) {
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

    if ((!g_list_click) || !(control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE)) ) {
        // converts the step to absolute value
        step_to_value(control);

        //make sure to save this value, in case the user switches mode
        clone_list_encoders(control);
    }

    // applies the control value
    control_set(encoder, control);
}

void CM_dec_control(uint8_t encoder)
{
    control_t *control = g_controls[encoder];

    //no control, return
    if (!control) return;
    
    if  (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE))  {
        //prepare display overlay
        CM_print_control_overlay(control, ENCODER_LIST_TIMEOUT);

        if (control->scale_points_flag & FLAG_SCALEPOINT_PAGINATED) {
            // decrements the step
            if (control->step > 2) {
                control->step--;
                control->scale_point_index--;
            }
            //we are at the end of our list ask for more data
            else {
                if ((control->scale_point_index <= 2) && (control->scale_point_index > 0)) {
                    control->step--;
                    control->scale_point_index--;

                    if (!g_list_click) {
                        // converts the step to absolute value
                        step_to_value(control);

                        //make sure to save this value, in case the user switches mode
                        clone_list_encoders(control);
                    }

                    // applies the control value
                    control_set(encoder, control);
                }
                else if (control->scale_point_index > 0) {
                    //request new data, a new control we be assigned after
                    request_control_page(control, 0);
                }

                //since a new control is assigned we can return
                return;
            }
        }
        else {
            // decrements the step
            if (control->step > 0) {
                control->scale_point_index--;
                control->step--;
            }
            else
                return;
        }
    }
    else if (control->properties & FLAG_CONTROL_TOGGLED) {
        if (control->value == 0)
            return;
        else 
            control->value = 0;
    }
    else if (control->properties & FLAG_CONTROL_BYPASS) {
        if (control->value == 1)
            return;
        else 
            control->value = 1;
    }
    else {
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

    if ((!g_list_click) || !(control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE)) ) {
        // converts the step to absolute value
        step_to_value(control);

        //make sure to save this value, in case the user switches mode
        clone_list_encoders(control);
    }

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
    else if (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE)) 
    {
        //no overlay active, toggle
        if (g_current_overlay_actuator == -1)
        {
            CM_print_control_overlay(g_controls[encoder], ENCODER_LIST_TIMEOUT);
        }
        //list click, change and set value, then close overlay
        else if (g_list_click)
        {
            step_to_value(control);
            send_control_set(control);

            hardware_set_overlay_timeout(0, NULL);
            g_current_overlay_actuator = -1;
            CM_print_screen();

            clone_list_encoders(control);

            return;
        }
        //overlay already active, close
        else
        {
            hardware_set_overlay_timeout(0, NULL);
            g_current_overlay_actuator = -1;
            CM_print_screen();
        }

        return;
    }
    else
        return;

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
        if ((g_foots[foot]->properties & (FLAG_CONTROL_MOMENTARY | FLAG_CONTROL_TRIGGER)) && !g_foots[foot]->lock_led_actions)
        {
            led->led_state.color = TRIGGER_COLOR;
            led->led_state.brightness = 0.1;
            ledz_set_state(led, LED_DIMMED, LED_UPDATE);
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
                led->led_state.brightness = 0.1;
                ledz_set_state(led, LED_DIMMED, LED_UPDATE);
            }
        }

        //not used right now anymore, maybe in the future, TODO: rename to actuator flag
        g_foots[foot]->scroll_dir = value;

        //we dont actually preform an action here
        if (!(g_foots[foot]->properties & FLAG_CONTROL_MOMENTARY))
            return;
    }

    g_foots[foot]->scroll_dir = value;

    control_set(foot, g_foots[foot]);
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

        if ((naveg_get_current_mode() != MODE_CONTROL) || (hardware_get_overlay_counter() != 0))
            return;

        //encoder
        if (hw_id < ENCODERS_COUNT)
        {
            if (control->properties & (FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION))  {
                clone_list_encoders(control);
            }
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
                //we only get 1 msg from the webui for triggers, so do not update
            }

            // toggled specification: http://lv2plug.in/ns/lv2core/#toggled
            else if (control->properties & FLAG_CONTROL_TOGGLED)
            {
                // updates the led
                led->led_state.color = TOGGLED_COLOR;
                if (control->value <= 0)
                {
                    led->led_state.brightness = 0.1;
                    ledz_set_state(led, LED_DIMMED, LED_UPDATE);
                }
                else
                    ledz_set_state(led, LED_ON, LED_UPDATE);


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
                led->led_state.fade_ratio = 10;

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

                ledz_set_state(led, LED_BLINK, LED_UPDATE);

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
                led->led_state.color = BYPASS_COLOR;
                if (control->value <= 0)
                    ledz_set_state(led, LED_ON, LED_UPDATE);
                else
                {
                    led->led_state.brightness = 0.1;
                    ledz_set_state(led, LED_DIMMED, LED_UPDATE);
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
                    ledz_set_state(led, LED_ON, LED_UPDATE);
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

            foot_control_print(g_foots[i]);
        }
        else
        {
            screen_footer(i, NULL, NULL, 0);
        }
    }
}

void CM_draw_foot(uint8_t foot_id)
{
    // checks the function assigned to foot and update the footer
    if (g_foots[foot_id])
    {
        //prevent toggling of pressed light
        g_foots[foot_id]->scroll_dir = 2;
        foot_control_print(g_foots[foot_id]);
    }
    else
    {
        screen_footer(foot_id, NULL, NULL, 0);
    }
}

void CM_load_next_page()
{
    uint8_t pagefound = 0;
    uint8_t j = g_current_foot_control_page;

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
    {
        CM_print_screen();
        return;
    }

    load_control_page(g_current_foot_control_page);
}

void CM_reset_page(void)
{
    g_current_foot_control_page = 0;
    g_current_encoder_page = 0;
}

void CM_load_next_encoder_page(uint8_t button)
{
    hardware_set_overlay_timeout(0, NULL);
    g_current_overlay_actuator = -1;

    g_current_encoder_page = button;
    screen_encoder_container(g_current_encoder_page);

    //clear controls
    uint8_t q;
    for (q = 0; q < ENCODERS_COUNT; q++)
    {
        CM_remove_control(q);
    }

    char buffer[30];
    uint8_t i = 0;

    //first notify host
    char val_buffer[20] = {0};
    sys_comm_set_response_cb(NULL, NULL);

    int_to_str(g_current_encoder_page, val_buffer, sizeof(val_buffer), 0);

    sys_comm_send(CMD_SYS_SUBPAGE_CHANGE, val_buffer);
    sys_comm_wait_response();

    hardware_set_overlay_timeout(0, NULL);
    g_current_overlay_actuator = -1;

    set_encoder_pages_led_state();

    for (q = 0; q < ENCODERS_COUNT; q++)
    {
        i = copy_command(buffer, CMD_DWARF_CONTROL_SUBPAGE);
        i += int_to_str(q, &buffer[i], sizeof(buffer) - i, 0);

        // inserts one space
        buffer[i++] = ' ';

        i += int_to_str(g_current_encoder_page, &buffer[i], sizeof(buffer) - i, 0);

        ui_comm_webgui_send(buffer, i);

        ui_comm_webgui_wait_response();
    }
}

void CM_set_state(void)
{
    CM_print_screen();

    CM_set_leds();
}

void CM_set_foot_led(control_t *control, uint8_t update_led)
{
    //widgets do nothing
    if (control->lock_led_actions)
        return;

    ledz_t *led = hardware_leds(control->hw_id - ENCODERS_COUNT);

    if (ledz_color_valid(MAX_COLOR_ID + control->hw_id-ENCODERS_COUNT+1))
    {
        ledz_restore_state(led);
    }
    else if (control->properties & FLAG_CONTROL_MOMENTARY)
    {
        // updates the led
        led->led_state.color = TRIGGER_COLOR;
        if (control->properties & FLAG_CONTROL_BYPASS)
        {
            if (control->value <= 0)
                ledz_set_state(led, LED_ON, update_led);
            else
            {
                led->led_state.brightness = 0.1;
                ledz_set_state(led, LED_DIMMED, update_led);
            }
        }
        else
        {
            if (control->value <= 0)
            {
                led->led_state.brightness = 0.1;
                ledz_set_state(led, LED_DIMMED, update_led);
            }
            else
                ledz_set_state(led, LED_ON, update_led);
        }
    }
    else if (control->properties & FLAG_CONTROL_TRIGGER)
    {
        // updates the led
        led->led_state.color = TRIGGER_COLOR;
        if ((control->value <= 0) || (control->scroll_dir == 2))
        {
            led->led_state.brightness = 0.1;
            ledz_set_state(led, LED_DIMMED, update_led);
        }
        else
            ledz_set_state(led, LED_ON, update_led);
    }
    else if (control->properties & FLAG_CONTROL_TOGGLED)
    {
        // toggled specification: http://lv2plug.in/ns/lv2core/#toggled
        // updates the led
        led->led_state.color = TOGGLED_COLOR;
        if (control->value <= 0)
        {
            led->led_state.brightness = 0.1;
            ledz_set_state(led, LED_DIMMED, update_led);
        }
        else
            ledz_set_state(led, LED_ON, update_led);
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

        ledz_set_state(led, LED_BLINK, update_led);
    }
    else if (control->properties & FLAG_CONTROL_BYPASS)
    {
        // updates the led
        led->led_state.color = BYPASS_COLOR;
        if (control->value <= 0)
            ledz_set_state(led, LED_ON, update_led);
        else
        {
            led->led_state.brightness = 0.1;
            ledz_set_state(led, LED_DIMMED, update_led);
        }
    }
    else if (control->properties & (FLAG_CONTROL_REVERSE | FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
        // updates the led
        //check if its assigned to a trigger and if the button is released
        if (control->scale_points_flag & FLAG_SCALEPOINT_ALT_LED_COLOR)
        {
            set_alternated_led_list_colour(control);
        }
        else if ((control->scroll_dir == 2))
        {
            led->led_state.color = ENUMERATED_COLOR;
            led->led_state.brightness = 0.1;
            ledz_set_state(led, LED_DIMMED, update_led);
        }
        else
        {
            led->led_state.color = ENUMERATED_COLOR;
            ledz_set_state(led, LED_ON, update_led);
        }
    }
}

void CM_close_overlay(void)
{
    if (g_list_click)
        reset_list_encoders();

    CM_print_screen();
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

    screen_tittle(-1);

    //update screen
    screen_page_index(g_current_foot_control_page, g_available_foot_pages);

    screen_encoder_container(g_current_encoder_page);

    CM_draw_foots();

    CM_draw_encoders();
}

void CM_print_control_overlay(control_t *control, uint16_t overlay_time)
{
    g_current_overlay_actuator = control->hw_id;

    screen_control_overlay(control);

    hardware_set_overlay_timeout(overlay_time, CM_close_overlay);
}

void CM_set_pages_available(uint8_t page_toggles[8])
{
    memcpy(g_fs_page_available, page_toggles, sizeof(g_fs_page_available));

    //sum available pages
    uint8_t j;
    uint8_t pages_available = 0;
    for (j = 0; j < FOOTSWITCH_PAGES_COUNT; j++)
    {
        if (g_fs_page_available[j])
            pages_available++;
    }

    g_available_foot_pages = pages_available;

    //our current page is not available anymore, so count down
    if (!g_fs_page_available[g_current_foot_control_page])
    {
        uint8_t pagefound = 0;
        uint8_t i;
        for (i = 8; (i > 0) && !pagefound; i--)
        {
            //page found
            if (g_fs_page_available[i])
                pagefound = 1;
        }

        g_current_foot_control_page = i;

        load_control_page(g_current_foot_control_page);
    }

    if (naveg_get_current_mode() == MODE_CONTROL) {
        screen_page_index(g_current_foot_control_page, g_available_foot_pages);
        set_encoder_pages_led_state();
    }
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

void CM_set_list_behaviour(uint8_t click_list)
{
    g_list_click = click_list;
}