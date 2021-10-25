
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "screen.h"
#include "utils.h"
#include "glcd.h"
#include "glcd_widget.h"
#include "naveg.h"
#include "hardware.h"
#include "images.h"
#include "protocol.h"
#include "mode_tools.h"
#include "mode_navigation.h"
#include <string.h>

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

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

/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/

static tuner_t g_tuner = {0, NULL, 0, 1};
static bool g_hide_non_assigned_actuators = 0;
static bool g_control_mode_header = 0;
static bool g_foots_grouped = 0;
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

void print_menu_outlines(void)
{
    glcd_t *display = hardware_glcds(0);
    glcd_vline(display, 0, 7, DISPLAY_HEIGHT - 11, GLCD_BLACK);
    glcd_vline(display, DISPLAY_WIDTH-1, 7, DISPLAY_HEIGHT - 11, GLCD_BLACK);
    glcd_hline(display, 0, DISPLAY_HEIGHT - 5, 14, GLCD_BLACK);
    glcd_hline(display, 45, DISPLAY_HEIGHT - 5, 3, GLCD_BLACK);
    glcd_hline(display, 79, DISPLAY_HEIGHT - 5, 3, GLCD_BLACK);
    glcd_hline(display, 112, DISPLAY_HEIGHT - 5, 15, GLCD_BLACK);
    glcd_rect(display, 14, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);
    glcd_rect(display, 48, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);
    glcd_rect(display, 82, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);
}

void print_tripple_menu_items(menu_item_t *item_child, uint8_t knob, uint8_t tool_mode)
{
    glcd_t *display = hardware_glcds(0);

    //fist decide posistion
    uint8_t item_x;
    uint8_t item_y = 15;

    switch(knob)
    {
        case 0:
            item_x = 5;
        break;

        case 1:
            item_x = 47;
        break;

        case 2:
            item_x = 90;
        break;

        default:
            return;
        break;
    }

    //print the title
    char first_line[10] = {};
    char second_line[10] = {};

    uint8_t q, p = 0, line = 0;
    for (q = 0; q < 21; q++)
    {
        if (item_child->desc->name[q] != 0)
        {
            if (line)
            {
                second_line[p] = item_child->desc->name[q];
                p++;
            }
            else
            {
                if (item_child->desc->name[q] ==  ' ')
                {
                    first_line[q] = 0;
                    line = 1;
                }
                else
                    first_line[q] = item_child->desc->name[q];
            }
        }
        else
            break;
    }
    second_line[p] = 0;

    //check if we have 2 lines
    if (second_line[0] != 0)
    {
        glcd_text(display, (item_x + 17 - 2*strlen(first_line)), item_y, first_line, Terminal3x5, GLCD_BLACK);
        glcd_text(display, (item_x + 17 - 2*strlen(second_line)), item_y + 6, second_line, Terminal3x5, GLCD_BLACK);      
    }
    else
    {
        glcd_text(display, (item_x + 17 - 2*strlen(first_line)), item_y+3, first_line, Terminal3x5, GLCD_BLACK);
    }

    switch(item_child->desc->type)
    {
        case MENU_FOOT:
        case MENU_TOGGLE:
            if (tool_mode)
                glcd_vline(display, item_x+16, item_y+13, 8, GLCD_BLACK_WHITE);
            toggle_t menu_toggle = {};
            menu_toggle.x = item_x;
            menu_toggle.y = item_y+23 - (tool_mode?15:0);
            menu_toggle.color = GLCD_BLACK;
            menu_toggle.width = 35;
            menu_toggle.height = 11;
            menu_toggle.value = item_child->data.value;
            widget_toggle(display, &menu_toggle);
        break;

        case MENU_BAR:;
            //print the bar
            menu_bar_t bar = {};
            bar.x = item_x;
            bar.y = item_y + 12 - (tool_mode?5:0);
            bar.color = GLCD_BLACK;
            bar.width = 35;
            bar.height = 7;
            bar.min = item_child->data.min;
            bar.max = item_child->data.max;
            bar.value = item_child->data.value;
            widget_bar(display, &bar);
                
            if (item_child->data.unit_text)
            {
                char bfr_upper_line[7] = {};
                char bfr_lower_line[7] = {};

                uint8_t t, r = 0, enter = 0;
                for (t = 0; t < 15; t++) {
                    if (item_child->data.unit_text[t] != 0) {
                        if (enter) {
                            bfr_lower_line[r] = item_child->data.unit_text[t];
                            r++;
                        }
                        else {
                            if (item_child->data.unit_text[t] ==  ' ') {
                                bfr_upper_line[t] = 0;
                                enter = 1;
                            }
                            else
                                bfr_upper_line[t] = item_child->data.unit_text[t];
                        }
                    }
                    else
                        break;
                }
                bfr_lower_line[r] = 0;

                //check if we have 2 lines
                if (bfr_lower_line[0] != 0) {
                    //we dont have this in tool mode, so join the strings
                    if (tool_mode) {
                        char str_bfr[14] = {};
                        strcat(str_bfr, bfr_upper_line);
                        strcat(str_bfr, " ");
                        strcat(str_bfr, bfr_lower_line);
                        glcd_text(display, (item_x + 19 - 2*strlen(str_bfr)), item_y+22, str_bfr, Terminal3x5, GLCD_BLACK);
                    }
                    else {
                        glcd_text(display, (item_x + 19 - 2*strlen(bfr_upper_line)), item_y+(tool_mode?22:27), bfr_upper_line, Terminal3x5, GLCD_BLACK);
                        glcd_text(display, (item_x + 19 - 2*strlen(bfr_lower_line)), item_y+(tool_mode?22:33), bfr_lower_line, Terminal3x5, GLCD_BLACK);
                    }
                }
                else {
                    glcd_text(display, (item_x + 19 - 2*strlen(bfr_upper_line)), item_y+(tool_mode?22:30), bfr_upper_line, Terminal3x5, GLCD_BLACK);
                }

                print_menu_outlines();
            }
        break;

        case MENU_CLICK_LIST:
        // fall through
        case MENU_LIST:
            if (item_child->data.unit_text)
            {
                //print the value
                char first_val_line[10] = {};
                char second_val_line[10] = {};

                p = 0;
                uint8_t val_line = 0;
                
                for (q = 0; q < 21; q++)
                {
                    if(item_child->data.unit_text[q] != 0)
                    {
                        if (val_line)
                        {
                            second_val_line[p] = item_child->data.unit_text[q];
                            p++;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
                        }
                        else
                        {
                            if (item_child->data.unit_text[q] == ' ')
                            {
                                first_val_line[q] = 0;
                                val_line = 1;                                
                            }
                            else
                                first_val_line[q] = item_child->data.unit_text[q];
                        }
                    }
                    else
                        break;
                }

                //check if we have 2 lines
                second_val_line[p] = 0;
                if (p != 0)
                {
                    glcd_text(display, (item_x + 17 - 2*strlen(first_val_line)), item_y+(tool_mode?17:24), first_val_line, Terminal3x5, GLCD_BLACK);
                    glcd_text(display, (item_x + 17 - 2*strlen(second_val_line)), item_y+(tool_mode?24:31), second_val_line, Terminal3x5, GLCD_BLACK);
                }
                else
                {
                    glcd_text(display, (item_x + 17 - 2*strlen(first_val_line)), item_y+(tool_mode?19:26), first_val_line, Terminal3x5, GLCD_BLACK);
                }

                //display 'click msg'
                if ((item_child->desc->type == MENU_CLICK_LIST) && ((int)item_child->data.value != item_child->data.selected))
                {
                    //text
                    glcd_text(display, item_x + 7, item_y+16, "CLICK", Terminal3x5, GLCD_BLACK);

                    //boxes
                    glcd_rect(display, item_x + 5, item_y+14, 24, 9, GLCD_BLACK);
                    glcd_rect(display, item_x-2, item_y+14, 37, 24, GLCD_BLACK);

                    //incert area
                    glcd_rect_invert(display, item_x-3, item_y+13, 39, 26);
                }
                else
                    glcd_vline(display, item_x+16, item_y+13, tool_mode?4:10, GLCD_BLACK_WHITE);
            }
        break;

        //others, dont use
        //TODO check if remove? most come from MDX codebase
        case MENU_MAIN:
        case MENU_ROOT:
        case MENU_CONFIRM2:
        case MENU_OK:
        case MENU_NONE:
        case MENU_CONFIRM:
        case MENU_TOOL:
        break;
    }
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void screen_clear(void)
{
    glcd_clear(hardware_glcds(0), GLCD_WHITE);
}

void screen_force_update(void)
{
    glcd_update(hardware_glcds(0));
}

void screen_set_hide_non_assigned_actuators(uint8_t hide)
{
    g_hide_non_assigned_actuators = hide;
}

void screen_set_control_mode_header(uint8_t toggle)
{
    g_control_mode_header = toggle;
}

void screen_group_foots(uint8_t toggle)
{
    g_foots_grouped = toggle;
}

void screen_encoder(control_t *control, uint8_t encoder)
{    
    glcd_t *display = hardware_glcds(0);

    //fist decide posistion
    uint8_t encoder_x;
    uint8_t encoder_y = 16;
    switch(encoder)
    {
        case 0:
            encoder_x = 4;
        break;

        case 1:
            encoder_x = 47;
        break;

        case 2:
            encoder_x = 90;
        break;

        default:
            return;
        break;
    }

    //clear the designated area
    glcd_rect_fill(display, encoder_x, encoder_y, 36, 27, GLCD_WHITE);

    //check what control type we need

    //no control
    if (!control)
    {
        char text[sizeof(SCREEN_ROTARY_DEFAULT_NAME) + 2];
        uint8_t item_x = encoder_x+5;
        if (g_hide_non_assigned_actuators)
        {
            text[0] = '-';
            text[1] = 0;
            item_x+=10;
        }
        else
        {
            strcpy(text, SCREEN_ROTARY_DEFAULT_NAME);
            text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)-1] = encoder + '1';
            text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)] = 0;
        }

        glcd_text(display, item_x, encoder_y+10, text, Terminal3x5, GLCD_BLACK);
        return;
    }

    //draw the title
    uint8_t char_cnt_name = strlen(control->label);

    if (char_cnt_name > 8)
    {
        char_cnt_name = 8;
    }

    char *title_str_bfr = (char *) MALLOC((char_cnt_name + 1) * sizeof(char));
    strncpy(title_str_bfr, control->label, char_cnt_name);
    title_str_bfr[char_cnt_name] = '\0';

    //allign to middle, (full width / 2) - (text width / 2)
    glcd_text(display, (encoder_x + 18 - 2*char_cnt_name), encoder_y, title_str_bfr, Terminal3x5, GLCD_BLACK);

    FREE(title_str_bfr);

    // list type control
    if (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
        uint8_t scalepoint_count_local = control->scale_points_count > 64 ? 64 : control->scale_points_count;

        char **labels_list = MALLOC(sizeof(char*) * scalepoint_count_local);

        uint8_t i;
        for (i = 0; i < scalepoint_count_local; i++)
        {
            labels_list[i] = control->scale_points[i]->label;
        }

        listbox_t list;
        list.x = encoder_x;
        list.y = encoder_y + 7;
        list.width = 36;
        list.height = 19;
        list.color = GLCD_BLACK;
        list.font = Terminal3x5;
        list.font_highlight = Terminal5x7;
        list.selected = control->step;
        list.count = scalepoint_count_local;
        list.list = labels_list;
        list.line_space = 1;
        list.line_top_margin = 1;
        list.line_bottom_margin = 1;
        list.text_left_margin = 1;
        widget_list_value(display, &list);

        FREE(labels_list);
    }
    else if ((control->properties & FLAG_CONTROL_TRIGGER) && (control->screen_indicator_widget_val == -1))
    {
        toggle_t toggle;
        toggle.x = encoder_x;
        toggle.y = encoder_y + 6;
        toggle.width = 35;
        toggle.height = 18;
        toggle.color = GLCD_BLACK;
        //we use 2 and 3 to indicate a trigger in the widget
        toggle.value = control->value ? 3 : 2;
        toggle.inner_border = 1;
        widget_toggle(display, &toggle);
    }
    else if ((control->properties & (FLAG_CONTROL_TOGGLED | FLAG_CONTROL_BYPASS)) && (control->screen_indicator_widget_val == -1))
    {
        toggle_t toggle;
        toggle.x = encoder_x;
        toggle.y = encoder_y + 6;
        toggle.width = 35;
        toggle.height = 18;
        toggle.color = GLCD_BLACK;
        toggle.value = (control->properties == FLAG_CONTROL_TOGGLED)?control->value:!control->value;
        toggle.inner_border = 1;
        widget_toggle(display, &toggle);
    }
    //linear / log / int
    else
    {
        bar_t bar;
        bar.x = encoder_x;
        bar.y = encoder_y;
        bar.width = 35;
        bar.height = 6;
        bar.color = GLCD_BLACK;
        if (control->screen_indicator_widget_val == -1) {
            bar.step = control->step;
            bar.steps = control->steps - 1;
        }
        else {
            bar.step = control->screen_indicator_widget_val * 100;
            bar.steps = 100;
        }

        if (!control->value_string)
        {
            char str_bfr[15] = {0};

            if ((control->properties == FLAG_CONTROL_INTEGER) || (control->value > 999.9) || (control->value < -999.9))
                int_to_str(control->value, str_bfr, sizeof(str_bfr), 0);
            else if ((control->value > 99.99) || (control->value < -99.99))
                float_to_str((control->value), str_bfr, sizeof(str_bfr), 1);
            else if ((control->value > 9.99) || (control->value < -9.99))
                float_to_str((control->value), str_bfr, sizeof(str_bfr), 2);
            else
                float_to_str((control->value), str_bfr, sizeof(str_bfr), 3);

            str_bfr[14] = 0;

            bar.value = str_bfr;

            widget_bar_encoder(display, &bar);
        }
        else
        {
            //draw the value string
            uint8_t char_cnt_value = strlen(control->value_string);

            if (char_cnt_value > 8)
                char_cnt_value = 8;

            char *value_str_bfr = (char *) MALLOC((char_cnt_value + 1) * sizeof(char));
            strncpy(value_str_bfr, control->value_string, char_cnt_value);
            value_str_bfr[char_cnt_value] = '\0';
            bar.value = value_str_bfr;

            widget_bar_encoder(display, &bar);

            FREE(value_str_bfr);
        }

        //check what to do with the unit
        if (strcmp("", control->unit) != 0)
        {
            uint8_t char_cnt_unit = strlen(control->unit);

            if (char_cnt_unit > 7)
            {
                //limit string
                char_cnt_unit = 7;
            }

            char *unit_str_bfr = (char *) MALLOC((char_cnt_unit + 1) * sizeof(char));
            strncpy(unit_str_bfr, control->unit, char_cnt_unit);
            unit_str_bfr[char_cnt_unit] = '\0';

            glcd_text(display, (encoder_x + 18 - 2*char_cnt_unit), encoder_y + 12 + 7, unit_str_bfr, Terminal3x5, GLCD_BLACK);

            FREE(unit_str_bfr);
            return;
        }
    }
}

void screen_page_index(uint8_t current, uint8_t available)
{
    //precent widget from tripin
    if (current > available)
        current = available;

    char str_current[4];
    char str_available[4];
    int_to_str((current+1), str_current, sizeof(str_current), 1);
    str_current[1] = '/';
    int_to_str((available), str_available, sizeof(str_available), 1);
    str_current[2] = str_available[0];
    str_current[3] = 0;

    glcd_t *display = hardware_glcds(0);

    //clear the part
    glcd_rect_fill(display, 0, 51, 24, 13, GLCD_WHITE);

    //draw the square
    glcd_hline(display, 0, 51, 21, GLCD_BLACK);
    glcd_vline(display, 0, 51, 13, GLCD_BLACK);
    glcd_vline(display, 21, 51, 13, GLCD_BLACK);

    //draw the indicator
    //we have 19 pixels available and possible 8 pages
    uint8_t amount_of_pixel_per_page = (int)20/available;
    uint8_t remaining_pixels = 20%available;

    uint8_t i, page_bar_x = 1;;
    for (i = 0; i < current; i++)
    {
        page_bar_x += amount_of_pixel_per_page;

        //add a remainder pixel
        if (remaining_pixels > i)
           page_bar_x++; 
    }

    glcd_rect_fill(display, page_bar_x, 52, (remaining_pixels > i) ? amount_of_pixel_per_page+1:amount_of_pixel_per_page, 2, GLCD_BLACK);

    //draw devision line
    glcd_hline(display, 0, 54, 21, GLCD_BLACK);

    // draws the text field
    glcd_text(display, 3, 56, str_current, Terminal5x7, GLCD_BLACK);
}

void screen_encoder_container(uint8_t current_encoder_page)
{
    glcd_t *display = hardware_glcds(0);

    //clear the part
    glcd_rect_fill(display, 0, 12, DISPLAY_WIDTH, 38, GLCD_WHITE);
    //clear the part of the small boxes below
    glcd_rect_fill(display, 31, 46, 66, 5, GLCD_WHITE);

    //draw the 3 main lines 
    glcd_hline(display, 0, 13, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_vline(display, 0, 13, 34, GLCD_BLACK);
    glcd_vline(display, DISPLAY_WIDTH - 1, 13, 34, GLCD_BLACK);

    //draw the 3 boxes
    glcd_rect(display, 31, 43, 21, 9, GLCD_BLACK);
    glcd_rect(display, 54, 43, 21, 9, GLCD_BLACK);
    glcd_rect(display, 77, 43, 21, 9, GLCD_BLACK);

    //draw the bottom lines
    glcd_hline(display, 0,  47, 31, GLCD_BLACK);
    glcd_hline(display, 52, 47, 2, GLCD_BLACK);
    glcd_hline(display, 75, 47, 2, GLCD_BLACK);
    glcd_hline(display, 97, 47, 31, GLCD_BLACK);

    //indicator 1
    glcd_text(display, 40, 45, "I", Terminal3x5, GLCD_BLACK);

    //indicator 2
    glcd_text(display, 61, 45, "II", Terminal3x5, GLCD_BLACK);

    //indicator 3
    glcd_text(display, 82, 45, "III", Terminal3x5, GLCD_BLACK);

    //invert the current one
    switch (current_encoder_page)
    {
        case 0:
            glcd_rect_invert(display, 32, 44, 19, 7);
        break;

        case 1:
            glcd_rect_invert(display, 55, 44, 19, 7);
        break;

        case 2:
            glcd_rect_invert(display, 78, 44, 19, 7);;
        break;

        default:
            glcd_rect_invert(display, 32, 44, 19, 7);
        break;
    }
}

void screen_footer(uint8_t foot_id, const char *name, const char *value, int16_t property)
{
    glcd_t *display = hardware_glcds(0);

    uint8_t foot_y = 54;
    uint8_t foot_x;
    switch(foot_id)
    {
        case 0:
            foot_x = 24;
        break;

        case 1:
            foot_x = 77;
        break;

        default:
            foot_x = 24;
        break;
    }

    if (g_foots_grouped && (naveg_get_current_mode() == MODE_CONTROL))
    {
        glcd_rect_fill(display, 24, foot_y, 102, 10, GLCD_WHITE);
        glcd_hline(display, 24, foot_y, 104, GLCD_BLACK);
        glcd_vline(display, 24, foot_y, 10, GLCD_BLACK);
    }
    else
    {
        // clear the footer area
        if (foot_id == 0)
            glcd_rect_fill(display, foot_x, foot_y, 53, 10, GLCD_WHITE);
        else
            glcd_rect_fill(display, foot_x, foot_y, 50, 10, GLCD_WHITE);

        //draw the footer box
        glcd_hline(display, foot_x, foot_y, 50, GLCD_BLACK);
        glcd_vline(display, foot_x, foot_y, 10, GLCD_BLACK);
        glcd_vline(display, foot_x+50, foot_y, 10, GLCD_BLACK);
    }

    if (name == NULL || value == NULL)
    {
        char text[sizeof(SCREEN_FOOT_DEFAULT_NAME) + 2];

        if (g_hide_non_assigned_actuators)
        {
            text[0] = '-';
            text[1] = 0;
        }
        else
        {
            strcpy(text, SCREEN_FOOT_DEFAULT_NAME);
            if (foot_id == 0)
                text[sizeof(SCREEN_FOOT_DEFAULT_NAME)-1] = 'B';
            else 
                text[sizeof(SCREEN_FOOT_DEFAULT_NAME)-1] = 'C';
            text[sizeof(SCREEN_FOOT_DEFAULT_NAME)] = 0;
        }

        glcd_text(display, foot_x + (26 - (strlen(text) * 3)), foot_y + 2, text, Terminal5x7, GLCD_BLACK);
        return;
    }

    //if we are in toggle, trigger or byoass mode we dont have a value
    else if ((property & FLAG_CONTROL_TOGGLED) || (property & FLAG_CONTROL_BYPASS) || (property & FLAG_CONTROL_TRIGGER) || (property & FLAG_CONTROL_MOMENTARY))
    {
        uint8_t char_cnt_name = strlen(name);
        if (char_cnt_name > 7)
        {
            //limit string
            char_cnt_name = 7;
        }

        char *title_str_bfr = (char *) MALLOC((char_cnt_name + 1) * sizeof(char));
        strncpy(title_str_bfr, name, char_cnt_name);
        title_str_bfr[char_cnt_name] = '\0';

        glcd_text(display, foot_x + (26 - (strlen(title_str_bfr) * 3)), foot_y + 2, title_str_bfr, Terminal5x7, GLCD_BLACK);
    
        if (value[1] == 'N')
            glcd_rect_invert(display, foot_x + 1, foot_y + 1, 49, 9);

        if ((property & FLAG_CONTROL_BYPASS) && (property & FLAG_CONTROL_MOMENTARY))
            glcd_rect_invert(display, foot_x + 1, foot_y + 1, 49, 9);

        FREE(title_str_bfr);
    }
    else
    {
        uint8_t char_cnt_name = strlen(name);
        uint8_t char_cnt_value = strlen(value);

        if (g_foots_grouped && (naveg_get_current_mode() == MODE_CONTROL))
        {
            //limit the strings for the screen properly
            if ((char_cnt_value + char_cnt_name) > 14) {
                //both bigger then the limmit
                if ((char_cnt_value > 7) && (char_cnt_name > 7)) {
                    char_cnt_name = 7;
                    char_cnt_value = 7;
                }
                else if (char_cnt_value > 7) {
                    if ((14 - char_cnt_name) < char_cnt_value)
                        char_cnt_value = 14 - char_cnt_name;
                }
                else if (char_cnt_name > 7) {
                    if ((14 - char_cnt_value) < char_cnt_name)
                        char_cnt_name = 14 - char_cnt_value;
                }
            }

            char *group_str_bfr = (char *) MALLOC((char_cnt_name + char_cnt_value + 2) * sizeof(char));
            memset(group_str_bfr, 0, (char_cnt_name + char_cnt_value + 2) * sizeof(char));

            strncpy(group_str_bfr, name, char_cnt_name);
            strcat(group_str_bfr, ":");
            strncat(group_str_bfr, value, char_cnt_value);
            group_str_bfr[char_cnt_name + char_cnt_value + 1] = '\0';
            glcd_text(display, 26, foot_y + 2, group_str_bfr, Terminal5x7, GLCD_BLACK);

            //group icon
            icon_footswitch_groups(display, DISPLAY_WIDTH-12, foot_y+1);

            FREE(group_str_bfr);
        }
        else
        {
            if ((char_cnt_value + char_cnt_name) > 7) {
                //both bigger then the limmit
                if ((char_cnt_value > 4) && (char_cnt_name > 3)) {
                    char_cnt_name = 3;
                    char_cnt_value = 4;
                }
                else if (char_cnt_value > 4) {
                    char_cnt_value = 7 - char_cnt_name;
                }
                else if (char_cnt_name > 3) {
                    char_cnt_name = 7 - char_cnt_value;
                }
            }

            char *title_str_bfr = (char *) MALLOC((char_cnt_name + 1) * sizeof(char));
            char *value_str_bfr = (char *) MALLOC((char_cnt_value + 1) * sizeof(char));
            memset(title_str_bfr, 0, (char_cnt_name + 1) * sizeof(char));
            memset(value_str_bfr, 0, (char_cnt_value + 1) * sizeof(char));

            //draw name
            strncpy(title_str_bfr, name, char_cnt_name);
            title_str_bfr[char_cnt_name] = '\0';
            glcd_text(display, foot_x + 2, foot_y + 2, title_str_bfr, Terminal5x7, GLCD_BLACK);

            // draws the value field
            strncpy(value_str_bfr, value, char_cnt_value);
            value_str_bfr[char_cnt_value] = '\0';
            glcd_text(display, foot_x + (50 - ((strlen(value_str_bfr)) * 6)), foot_y + 2, value_str_bfr, Terminal5x7, GLCD_BLACK);
        
            FREE(title_str_bfr);
            FREE(value_str_bfr);
        }
    }
    
}

void screen_tittle(int8_t pb_ss)
{
    glcd_t *display = hardware_glcds(0);

    if (pb_ss == -1)
        pb_ss = g_control_mode_header;

    //we dont display inside a menu
    if (naveg_get_current_mode() != MODE_CONTROL) return;

    //we dont display a not selected mode
    if (g_control_mode_header != pb_ss) return;

    char* name = NM_get_pbss_name(pb_ss);
    uint8_t name_len = strlen(name);

    // clear the name area
    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 9, GLCD_WHITE);

    glcd_text(display, ((DISPLAY_WIDTH / 2) - (3*name_len) + 7), 1, name, Terminal5x7, GLCD_BLACK);

    if (pb_ss)
        icon_snapshot(display, ((DISPLAY_WIDTH / 2) - (3*name_len) + 7 ) - 11, 1);
    else
        icon_pedalboard(display, ((DISPLAY_WIDTH / 2) - (3*name_len) + 7) - 11, 1);

    //invert the top bar
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);
}

void screen_bank_list(bp_list_t *list, const char *name)
{
    listbox_t list_box = {};

    glcd_t *display = hardware_glcds(0);
    uint8_t type = NM_get_current_list();
    screen_clear();

    //print outlines
    print_menu_outlines();

    //print the 3 buttons
    switch (type) {
        case BANKS_LIST:
            //draw the first box, enter bank
            glcd_text(display, 16, DISPLAY_HEIGHT - 7, "ENTER >", Terminal3x5, GLCD_BLACK);
            //draw the second box
            glcd_text(display, 58, DISPLAY_HEIGHT - 7, "NEW", Terminal3x5, GLCD_BLACK);
            //draw the third box, new bank
            if (list->hover == 0)
                glcd_text(display, 96, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);
            else
                glcd_text(display, 86, DISPLAY_HEIGHT - 7, "DELETE", Terminal3x5, GLCD_BLACK);

            list_box.selected_ids = NULL;
        break;

        case BANK_LIST_CHECKBOXES:
            glcd_text(display, 16, DISPLAY_HEIGHT - 7, "ENTER >", Terminal3x5, GLCD_BLACK);
            if (NM_get_current_hover(BANKS_LIST != 0))
                glcd_text(display, 52, DISPLAY_HEIGHT - 7, "SELECT", Terminal3x5, GLCD_BLACK);
            else
               glcd_text(display, 62, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);
            glcd_text(display, 86, DISPLAY_HEIGHT - 7, "CANCEL", Terminal3x5, GLCD_BLACK);
            list_box.selected_ids = NULL;
        break;

        case PB_LIST_CHECKBOXES:
            glcd_text(display, 18, DISPLAY_HEIGHT - 7, "< BACK", Terminal3x5, GLCD_BLACK);
            glcd_text(display, 52, DISPLAY_HEIGHT - 7, "SELECT", Terminal3x5, GLCD_BLACK);
            glcd_text(display, 86, DISPLAY_HEIGHT - 7, "CANCEL", Terminal3x5, GLCD_BLACK);
            list_box.selected_ids = NULL;
        break;

        case BANK_LIST_CHECKBOXES_ENGAGED:
            if (NM_get_current_hover(BANKS_LIST != 0))
                glcd_text(display, 52, DISPLAY_HEIGHT - 7, "SELECT", Terminal3x5, GLCD_BLACK);
            else
                glcd_text(display, 62, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

            glcd_text(display, 24, DISPLAY_HEIGHT - 7, "ADD", Terminal3x5, GLCD_BLACK);
            glcd_text(display, 86, DISPLAY_HEIGHT - 7, "CANCEL", Terminal3x5, GLCD_BLACK);

            list_box.selected_ids = list->selected_pb_uids;
            list_box.selected_count = list->selected_count;
        break;

        case PB_LIST_CHECKBOXES_ENGAGED:
            glcd_text(display, 24, DISPLAY_HEIGHT - 7, "ADD", Terminal3x5, GLCD_BLACK);
            glcd_text(display, 52, DISPLAY_HEIGHT - 7, "SELECT", Terminal3x5, GLCD_BLACK);
            glcd_text(display, 86, DISPLAY_HEIGHT - 7, "CANCEL", Terminal3x5, GLCD_BLACK);

            list_box.selected_ids = list->selected_pb_uids;
            list_box.selected_count = list->selected_count;
        break;
    }

    // draws the list, check if there are items to avoid a crash
    if (list)
    {
        uint8_t count = strarr_length(list->names);
        list_box.x = 1;
        list_box.name = name;
        list_box.y = 11;
        list_box.width = DISPLAY_WIDTH-2;
        list_box.height = 39;
        list_box.color = GLCD_BLACK;
        list_box.hover = list->hover - list->page_min;
        list_box.selected = list->selected - list->page_min;
        list_box.count = count;
        list_box.list = list->names;
        list_box.font = Terminal3x5;
        list_box.type = type;
        list_box.line_space = 2;
        list_box.line_top_margin = 1;
        list_box.line_bottom_margin = 1;
        list_box.text_left_margin = 7;
        list_box.page_min_offset = list->page_min;
        widget_banks_listbox(display, &list_box);
    }
}

void screen_pbss_list(const char *title, bp_list_t *list, uint8_t pb_ss_toggle, int8_t hold_item_index, 
                      const char *hold_item_label)
{
    listbox_t list_box;

    glcd_t *display = hardware_glcds(0);
    uint8_t type = NM_get_current_list();
    screen_clear();

    // draws the list, check if there are items to avoid a crash
    if (list)
    {
        //(ab)use bank function
        if ((type == PB_LIST_CHECKBOXES) || (type == PB_LIST_CHECKBOXES_ENGAGED)){
            screen_bank_list(list, title);
            return;
        }

        char str_bfr[18];
        uint8_t char_cnt = strlen(title);
        if (char_cnt > 17)
            char_cnt = 17;

        memset(str_bfr, 0, sizeof(str_bfr));
        strncpy(str_bfr, title, sizeof(str_bfr)-1);
        str_bfr[char_cnt] = 0;

        glcd_text(display, ((DISPLAY_WIDTH / 2) - (3*char_cnt) + 7), 1, str_bfr, Terminal5x7, GLCD_BLACK);
        //snapshot
        if (!pb_ss_toggle)
            icon_bank(display, ((DISPLAY_WIDTH / 2) - (3*char_cnt) + 7) - 12, 1);
        //pb's
        else 
            icon_pedalboard(display, ((DISPLAY_WIDTH / 2) - (3*char_cnt) + 7) - 12, 1);

        //invert the top bar
        glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);

        glcd_vline(display, 0, 17, DISPLAY_HEIGHT - 21, GLCD_BLACK);
        glcd_vline(display, DISPLAY_WIDTH-1, 17, DISPLAY_HEIGHT - 21, GLCD_BLACK);
        glcd_hline(display, 0, DISPLAY_HEIGHT - 5, 14, GLCD_BLACK);
        glcd_hline(display, 45, DISPLAY_HEIGHT - 5, 3, GLCD_BLACK);
        glcd_hline(display, 79, DISPLAY_HEIGHT - 5, 3, GLCD_BLACK);
        glcd_hline(display, 112, DISPLAY_HEIGHT - 5, 15, GLCD_BLACK);
        glcd_rect(display, 14, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);
        glcd_rect(display, 48, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);
        glcd_rect(display, 82, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);

        uint8_t count = strarr_length(list->names);

        const uint8_t *title_font = Terminal5x7;
        list_box.x = 0;
        list_box.y = 11;
        list_box.width = DISPLAY_WIDTH;
        list_box.height = 45;
        list_box.color = GLCD_BLACK;
        list_box.count = count;
        list_box.font = Terminal7x8;
        list_box.line_space = 4;
        list_box.line_top_margin = 1;
        list_box.line_bottom_margin = 1;
        list_box.text_left_margin = 2;
        if (!pb_ss_toggle)
            list_box.name = "PEDALBOARDS";
        else
            list_box.name = "SNAPSHOTS";

        if (count != 0) {
            list_box.hover = list->hover - list->page_min;
            list_box.selected = list->selected - list->page_min;
            list_box.list = list->names;

            if (hold_item_index != -1)
                widget_listbox_pedalboard_draging(display, &list_box, title_font, pb_ss_toggle, hold_item_index, hold_item_label);
            else
                widget_listbox_pedalboard(display, &list_box, title_font, pb_ss_toggle);
        }
        else {
            //finish drawing some stuff
            widget_pb_ss_title(display, &list_box, title_font, pb_ss_toggle);
        }

        //if we are in pb mode, and at the top of the list, display the 'add pb to bank' button
        if ((((type == PB_LIST_BEGINNING_BOX) || (type == PB_LIST_BEGINNING_BOX_SELECTED))
            && (!pb_ss_toggle)) && (NM_get_current_selected(BANKS_LIST) != 0)) {
            widget_add_pb_button(display, 31, 22, (type == PB_LIST_BEGINNING_BOX_SELECTED)?1:0);
        }

        //print the 3 buttons
        //draw the first box, back
        
        uint8_t x;
        char *text;
        if (pb_ss_toggle) {
            x = 22;
            text = "SAVE";
        }
        else {
            x = 16;
            text = "< BANKS";
        }
        glcd_text(display, x, DISPLAY_HEIGHT - 7, text, Terminal3x5, GLCD_BLACK);

        //draw the second box
        glcd_text(display, 62, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

        //draw the third box, we can remove any pb or ss, except from the all-pb bank
        if ((NM_get_current_list() == BANKS_LIST) || (pb_ss_toggle && (list->menu_max > 1)) || (!pb_ss_toggle && (NM_get_current_selected(BANKS_LIST) != 0) && (type != PB_LIST_BEGINNING_BOX_SELECTED)))
            glcd_text(display, 86, DISPLAY_HEIGHT - 7, "REMOVE", Terminal3x5, GLCD_BLACK);
        else
            glcd_text(display, 96, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);
    }
}

void screen_system_menu(menu_item_t *item)
{
    glcd_t *display;
    display = hardware_glcds(0);

    // clear screen
    glcd_clear(display, GLCD_WHITE);

    // draws the title
    textbox_t title_box = {};
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = Terminal3x5;
    title_box.top_margin = 1;
    title_box.align = ALIGN_CENTER_TOP;
    title_box.text = item->name;
    widget_textbox(display, &title_box);

    //invert the title area
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 7);

    print_menu_outlines();

    //print the 3 buttons
    //draw the first box, back
    glcd_text(display, 16, DISPLAY_HEIGHT - 7, "ENTER >", Terminal3x5, GLCD_BLACK);

    //draw the second box, TODO Builder MODE
    glcd_text(display, 56, DISPLAY_HEIGHT - 7, "EXIT", Terminal3x5, GLCD_BLACK);

    //draw the third box, save PB
    glcd_text(display, 96, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

    // menu list
    listbox_t list;
    list.x = 6;
    list.y = 12;
    list.width = 116;
    list.height = 40;
    list.color = GLCD_BLACK;
    list.font = Terminal3x5;
    list.line_space = 2;
    list.line_top_margin = 1;
    list.line_bottom_margin = 1;
    list.text_left_margin = 2;

    popup_t popup = {};
    switch (item->desc->type)
    {
        case MENU_ROOT:
            list.hover = item->data.hover;
            list.selected = item->data.selected;
            list.count = MENU_VISIBLE_LIST_CUT;
            list.list = item->data.list;
            widget_menu_listbox(display, &list);
        break;

        case MENU_CONFIRM:
            // popup
            popup.width = DISPLAY_WIDTH;
            popup.height = DISPLAY_HEIGHT;
            popup.font = Terminal3x5;
            popup.type = OK_CANCEL;
            popup.title = item->data.popup_header;
            popup.content = item->data.popup_content;
            popup.button_selected = item->data.hover;
            widget_popup(display, &popup);
            return;
        break;

        case MENU_CONFIRM2:
            // popup
            popup.width = DISPLAY_WIDTH;
            popup.height = DISPLAY_HEIGHT;
            popup.font = Terminal3x5;
            popup.type = YES_NO;
            popup.title = item->data.popup_header;
            popup.content = item->data.popup_content;
            popup.button_selected = item->data.hover;
            widget_popup(display, &popup);
            return;
        break;

        case MENU_OK:
            // popup
            popup.width = DISPLAY_WIDTH;
            popup.height = DISPLAY_HEIGHT;
            popup.font = Terminal3x5;
            popup.type = OK_ONLY;
            popup.title = item->data.popup_header;
            popup.content = item->data.popup_content;
            popup.button_selected = item->data.hover;
            widget_popup(display, &popup);
            return;
        break;

        case MENU_MAIN:
        case MENU_TOGGLE:
        case MENU_BAR:
        case MENU_NONE:
        case MENU_LIST:
        case MENU_TOOL:
        case MENU_FOOT:
        case MENU_CLICK_LIST:
        break;
    }
}

void screen_menu_page(node_t *node)
{
    //clear screen first
    screen_clear();

    glcd_t *display = hardware_glcds(0);

    menu_item_t *main_item = node->data;

    //print the title
    //draw the title 
    char title_str[45] = {0};
    strncpy(title_str, "SETTINGS > ", 12);
    strcat(title_str, main_item->desc->name);
    title_str[32] = '\0';
    textbox_t title = {};
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = Terminal3x5;
    title.top_margin = 1;
    title.text = title_str;
    title.align = ALIGN_CENTER_TOP;
    widget_textbox(display, &title);

    //invert the title area
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 7);

    //print the 3 buttons
    //draw the first box, back
    glcd_text(display, 18, DISPLAY_HEIGHT - 7, "< BACK", Terminal3x5, GLCD_BLACK);

    uint8_t x;
    char *text;
    if (node->prev)
    {
        x = 56;
        text = "PREV";
    }
    else
    {
        x = 62;
        text = "-";
    }
    glcd_text(display, x, DISPLAY_HEIGHT - 7, text, Terminal3x5, GLCD_BLACK);

    //draw the third box, save PB
    menu_item_t *end_item = node->next->data;
    if (end_item->desc->id != BLUETOOTH_ID)
    {
        text = "NEXT";
        x = 90;
    }
    else
    {
        text = "-";
        x = 96; 
    }
    glcd_text(display, x, DISPLAY_HEIGHT - 7, text, Terminal3x5, GLCD_BLACK);

    node_t *child_nodes = node->first_child;
    
    uint8_t i;
    for (i = 0; i < 3; i++)
    {
        menu_item_t *item_child = child_nodes->data;

        if (item_child->data.popup_active == 1)
        {
            // popup
            popup_t popup = {};
            popup.width = DISPLAY_WIDTH;
            popup.height = DISPLAY_HEIGHT;
            popup.font = Terminal3x5;

            if (item_child->desc->parent_id == USER_PROFILE_ID)
                popup.type = YES_NO;
            else
                popup.type = OK_CANCEL;

            popup.title = item_child->data.popup_header;
            popup.content = item_child->data.popup_content;
            popup.button_selected = item_child->data.hover;
            widget_popup(display, &popup);
            return;
        }

        print_tripple_menu_items(item_child, i, 0);

        if (!child_nodes->next)
        {
            if (i <= 0)
                glcd_text(display, 62, 26, "-", Terminal3x5, GLCD_BLACK);

            if (i <= 1)
                glcd_text(display, 105, 26, "-", Terminal3x5, GLCD_BLACK);

            return;
        }
        else
            child_nodes = child_nodes->next; 

        //draw the outlines
        print_menu_outlines();
    }
}

void screen_tool_control_page(node_t *node)
{
    //clear screen first
    screen_clear();

    //something off
    if (!node)
        return;

    glcd_t *display = hardware_glcds(0);

    //draw the outlines
    glcd_vline(display, 0, 14, DISPLAY_HEIGHT - 32, GLCD_BLACK);
    glcd_vline(display, DISPLAY_WIDTH - 1, 14, DISPLAY_HEIGHT - 32, GLCD_BLACK);
    glcd_hline(display, 0, DISPLAY_HEIGHT - 51, DISPLAY_WIDTH , GLCD_BLACK);
    glcd_hline(display, 0,  45, DISPLAY_WIDTH, GLCD_BLACK);

    //root node (name is title)
    menu_item_t *item = node->data;

    //draw the title
    textbox_t title = {};
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = Terminal5x7;
    title.top_margin = 1;
    title.text = item->name;
    title.align = ALIGN_CENTER_TOP;
    widget_textbox(display, &title);

    //invert the top bar
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);

    //draw the 3 menu items if applicable
    node_t *child_nodes = node->first_child;
    uint8_t i;
    for (i = 0; i < 3; i++)
    {
        menu_item_t *item_child = child_nodes->data;

        //reached footer section
        if (item_child->desc->type == MENU_FOOT)
            break;

        print_tripple_menu_items(item_child, i, 1);

        //check end of items
        if (!child_nodes->next)
            break;
        else
            child_nodes = child_nodes->next;
    }

    //clear some extra menu lines
    glcd_rect_fill(display, 0, 9, DISPLAY_WIDTH, 4, GLCD_WHITE);
    glcd_rect_fill(display, 0, DISPLAY_HEIGHT-18, 3, 5, GLCD_WHITE);
    glcd_rect_fill(display, DISPLAY_WIDTH-3, DISPLAY_HEIGHT-18, 3, 8, GLCD_WHITE);
}

void screen_toggle_tuner(float frequency, char *note, int8_t cents)
{
    screen_clear();

    glcd_t *display = hardware_glcds(0);

    g_tuner.frequency = frequency;
    g_tuner.note = note;
    g_tuner.cents = cents;

    textbox_t title = {};
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = Terminal5x7;
    title.top_margin = 1;
    title.text = "TOOL - TUNER";
    title.align = ALIGN_CENTER_TOP;
    widget_textbox(display, &title);

    //invert the top bar
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);

    //draw tuner
    widget_tuner(display, &g_tuner);
}

void screen_update_tuner(float frequency, char *note, int8_t cents)
{
    g_tuner.frequency = frequency;
    g_tuner.note = note;
    g_tuner.cents = cents;

    //draw tuner
    if (naveg_get_current_mode() == MODE_TOOL_FOOT)
        widget_tuner(hardware_glcds(0), &g_tuner);
}

void screen_image(uint8_t display, const uint8_t *image)
{
    glcd_t *display_img = hardware_glcds(display);
    glcd_draw_image(display_img, 0, 0, image, GLCD_BLACK);
}

void screen_shift_overlay(int8_t prev_mode, int16_t *item_ids, uint8_t ui_connection)
{
    //TODO WE NEED THIS VALUE ONCE BUILDER MODE IS SELECTABLE
    (void) ui_connection;

    static uint8_t previous_mode;
    static int16_t last_item_ids[3] = {-1};
    uint8_t i;
    
    if (prev_mode != -1)
    {
        previous_mode = prev_mode;
    }

    if (item_ids)
    {
        for (i = 0; i < 3; i++)
        {
            last_item_ids[i] = item_ids[i];
        }
    }

    //we dont have any items to display
    if (last_item_ids[0] == -1)
        return;

    //clear screen first
    screen_clear();

    glcd_t *display = hardware_glcds(0);

    //draw the title 
    textbox_t title = {};
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = Terminal5x7;
    title.top_margin = 1;
    title.text = "MENU";
    title.align = ALIGN_CENTER_TOP;
    widget_textbox(display, &title);

    //invert the title area
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);

    //draw the outlines
    print_menu_outlines();

    //draw the first box, save pb
    glcd_text(display, 22, DISPLAY_HEIGHT - 7, "SAVE", Terminal3x5, GLCD_BLACK);

    //draw the second box, menu/control mode
    glcd_text(display, 50, DISPLAY_HEIGHT - 7, "SETTNGS", Terminal3x5, GLCD_BLACK);
    //invert because this is active rn
    if (previous_mode == MODE_TOOL_MENU)
        glcd_rect_invert(display, 49, DISPLAY_HEIGHT - 8, 30, 8);

    //draw the third box, TODO BUILDER MODE
    glcd_text(display, 96, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

    //print the 3 quick controls
    for (i = 0; i < 3; i++)
    {
        print_tripple_menu_items(TM_get_menu_item_by_ID(last_item_ids[i]), i, 0);
    }
}

void screen_control_overlay(control_t *control)
{
    overlay_t overlay;
    overlay.x = 0;
    overlay.y = 11;
    overlay.width = DISPLAY_WIDTH;
    overlay.height = 38;
    overlay.value_num = control->value;

    glcd_t *display = hardware_glcds(0);

    uint8_t foot_val = control->value;
    if (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
        uint8_t scalepoint_count_local = control->scale_points_count > 64 ? 64 : control->scale_points_count;

        char **labels_list = MALLOC(sizeof(char*) * scalepoint_count_local);

        uint8_t i;
        for (i = 0; i < scalepoint_count_local; i++)
        {
            labels_list[i] = control->scale_points[i]->label;
        }

        //trigger list overlay widget
        listbox_t list;
        list.name = control->label;
        list.hover = control->step;
        list.selected = control->step;
        list.count = control->scale_points_count;
        list.list = labels_list;
        list.x = 0;
        list.y = 11;
        list.width = DISPLAY_WIDTH;
        list.height = 38;
        list.color = GLCD_BLACK;
        list.font = Terminal5x7;
        list.line_space = 2;
        list.line_top_margin = 1;
        list.line_bottom_margin = 1;
        list.text_left_margin = 0;
        widget_listbox_overlay(display, &list);

        FREE(labels_list);
    }
    else if (control->properties & FLAG_CONTROL_TRIGGER)
    {
        //trigger trigger overlay widget
        overlay.color = GLCD_BLACK;
        overlay.font = Terminal5x7;
        overlay.name = control->label;
        overlay.value = "TRIGGER";
        overlay.properties = control->properties;

        widget_foot_overlay(display, &overlay);
    }
    else if (control->properties & (FLAG_CONTROL_TOGGLED | FLAG_CONTROL_BYPASS))
    {    
        if (control->properties & FLAG_CONTROL_BYPASS)
            foot_val = !foot_val;

        //trigger toggle overlay widget
        overlay.color = GLCD_BLACK;
        overlay.font = Terminal5x7;
        overlay.name = control->label;
        overlay.value = foot_val?"ON":"OFF";
        overlay.properties = control->properties;

        widget_foot_overlay(display, &overlay);
    }
    else
    {
        // footer text composition
        char value_txt[32];
        uint8_t i = 0;

        //if unit=ms or unit=bpm -> use 0 decimal points
        if (strcasecmp(control->unit, "ms") == 0 || strcasecmp(control->unit, "bpm") == 0)
            i = int_to_str(control->value, value_txt, sizeof(value_txt), 0);
        //if unit=s or unit=hz or unit=something else-> use 2 decimal points
        else
            i = float_to_str(control->value, value_txt, sizeof(value_txt), 2);

        //add space to footer
        value_txt[i++] = ' ';
        strcpy(&value_txt[i], control->unit);

        //trigger trigger overlay widget
        overlay.color = GLCD_BLACK;
        overlay.font = Terminal5x7;
        overlay.name = control->label;
        overlay.value = value_txt;
        overlay.properties = control->properties;
        //trigger value overlay widget
        widget_foot_overlay(display, &overlay);
    }
}

void screen_popup(system_popup_t *popup_data)
{
    glcd_t *display = hardware_glcds(0);

    //clear screen
    screen_clear();

    //display the popup
    popup_t popup = {};
    popup.width = DISPLAY_WIDTH;
    popup.height = DISPLAY_HEIGHT;
    popup.font = Terminal3x5;
    popup.type = EMPTY_POPUP;
    popup.title = popup_data->title;
    popup.content = popup_data->popup_text;
    popup.button_selected = popup_data->button_value;
    widget_popup(display, &popup);

    //full buttons
    glcd_text(display, 30 - (2*strlen(popup_data->btn1_txt)), DISPLAY_HEIGHT - 7, popup_data->btn1_txt, Terminal3x5, GLCD_BLACK);
    glcd_text(display, 64 - (2*strlen(popup_data->btn2_txt)), DISPLAY_HEIGHT - 7, popup_data->btn2_txt, Terminal3x5, GLCD_BLACK);
    glcd_text(display, 98 - (2*strlen(popup_data->btn3_txt)), DISPLAY_HEIGHT - 7, popup_data->btn3_txt, Terminal3x5, GLCD_BLACK);

    //when we have a naming widget, we can not use the encoders to trigger the button actions, so dont print
    //we do need to print the naming box and name
    if (popup_data->has_naming_input) {
        //box
        glcd_rect(display, 4, 11, DISPLAY_WIDTH - 8, 14, GLCD_BLACK);

        //text
        glcd_text(display, 8, 14, popup_data->input_name, Terminal5x7, GLCD_BLACK);

        //cursor
        glcd_rect(display, 8 + (6*popup_data->cursor_index), 22, 5, 1, GLCD_BLACK);
    }
    else {
        switch (popup_data->button_value) {
            case 0:
                glcd_rect_invert(display, 15, DISPLAY_HEIGHT - 8, 29, 7);
            break;
            case 1:
                if (popup_data->button_max == 2)
                    glcd_rect_invert(display, 83, DISPLAY_HEIGHT - 8, 29, 7);
                else
                    glcd_rect_invert(display, 49, DISPLAY_HEIGHT - 8, 29, 7);
            break;
            case 2:
                glcd_rect_invert(display, 83, DISPLAY_HEIGHT - 8, 29, 7);
            break;
        }
    }
}

void screen_keyboard(system_popup_t *popup_data, uint8_t keyboard_index)
{
    glcd_t *display = hardware_glcds(0);

    //clear screen
    screen_clear();

    screen_image(0, MDW_Naming_Widget_withButtonsandSpace);

    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 9, GLCD_WHITE);

    //draw the tittle
    switch(popup_data->id){
        case POPUP_SAVE_PB_ID:
            glcd_text(display, 20, 1, "SAVE PEDALBOARD", Terminal5x7, GLCD_BLACK);
        break;

        case POPUP_SAVE_SS_ID:
            glcd_text(display, 26, 1, "SAVE SNAPSHOT", Terminal5x7, GLCD_BLACK);
        break;

        case POPUP_NEW_BANK_ID:
            glcd_text(display, 32, 1, "CREATE BANK", Terminal5x7, GLCD_BLACK);
        break;
    }

    // draws the title background
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);

    //draw the highlighted char
    icon_keyboard_invert(display, keyboard_index);

    //draw the current name
    glcd_text(display, 8, 14, popup_data->input_name, Terminal5x7, GLCD_BLACK);

    //draw the current cursor
    glcd_rect(display, 8 + (6*popup_data->cursor_index), 22, 5, 1, GLCD_BLACK);

    //draw the buttons
    glcd_text(display, 30 - (2*strlen("DONE")), DISPLAY_HEIGHT - 7, "DONE", Terminal3x5, GLCD_BLACK);
    glcd_text(display, 64 - (2*strlen("CLEAR")), DISPLAY_HEIGHT - 7, "CLEAR", Terminal3x5, GLCD_BLACK);
    glcd_text(display, 98 - (2*strlen("DELETE")), DISPLAY_HEIGHT - 7, "DELETE", Terminal3x5, GLCD_BLACK);
}

void screen_msg_overlay(char *message)
{
    glcd_t *display;
    display = hardware_glcds(0);

    // clear screen
    glcd_clear(display, GLCD_WHITE);

    glcd_text(display, 42, 1, "ATTENTION", Terminal5x7, GLCD_BLACK);

    //drraw the title area
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);

    //draw the outlinbes
    glcd_vline(display, 0, 7, DISPLAY_HEIGHT - 8, GLCD_BLACK);
    glcd_vline(display, DISPLAY_WIDTH - 1, 7, DISPLAY_HEIGHT - 8, GLCD_BLACK);
    glcd_hline(display, 0, DISPLAY_HEIGHT - 1, DISPLAY_WIDTH, GLCD_BLACK);

    //draw the message
    textbox_t text_box;
    text_box.color = GLCD_BLACK;
    text_box.mode = TEXT_MULTI_LINES;
    text_box.font = Terminal5x7;
    text_box.top_margin = 10;
    text_box.bottom_margin = 2;
    text_box.left_margin = 1;
    text_box.right_margin = 2;
    text_box.height = 53;
    text_box.width = 126;
    text_box.text = message;
    text_box.align = ALIGN_CENTER_TOP;
    widget_textbox(display, &text_box);
}

void screen_text_box(uint8_t x, uint8_t y, const char *text)
{
    glcd_t *hardware_display = hardware_glcds(0);

    textbox_t text_box;
    text_box.color = GLCD_BLACK;
    text_box.mode = TEXT_MULTI_LINES;
    text_box.font = Terminal3x5;
    text_box.top_margin = 1;
    text_box.bottom_margin = 0;
    text_box.left_margin = 1;
    text_box.right_margin = 0;
    text_box.height = 63;
    text_box.width = 127;
    text_box.text = text;
    text_box.align = ALIGN_NONE_NONE;
    text_box.y = y;
    text_box.x = x;
    widget_textbox(hardware_display, &text_box);
}
