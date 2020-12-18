
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
#include <string.h>

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define FOOTER_NAME_WIDTH       50
#define FOOTER_VALUE_WIDTH      (DISPLAY_WIDTH - FOOTER_NAME_WIDTH - 10)

enum {BANKS_LIST, PEDALBOARD_LIST};

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

void print_tripple_menu_items(menu_item_t *item_child, uint8_t knob)
{
    glcd_t *display = hardware_glcds(0);

    //fist decide posistion
    uint8_t item_x;
    uint8_t item_y = 15;
    switch(knob)
    {
        case 0:
            item_x = 4;
        break;

        case 1:
            item_x = 47;
        break;

        case 2:
            item_x = 89;
        break;

        default:
            return;
        break;
    }

    //print the title
    char first_line[10] = {};
    char second_line[10] = {};
    uint8_t q, p = 0, line = 0;
    for (q = 0; q < 20; q++)
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
        glcd_text(display, (item_x + 18 - 2*strlen(first_line)), item_y, first_line, Terminal3x5, GLCD_BLACK);
        glcd_text(display, (item_x + 18 - 2*strlen(first_line)), item_y + 6, second_line, Terminal3x5, GLCD_BLACK);      
    }
    else
    {
        glcd_text(display, (item_x + 18 - 2*strlen(first_line)), item_y+3, first_line, Terminal3x5, GLCD_BLACK);
    }

    char str_bfr[6];
    uint8_t char_cnt_name = 0;
    switch(item_child->desc->type)
    {
        case MENU_TOGGLE:
            glcd_vline(display, item_x+16, item_y+13, 8, GLCD_BLACK_WHITE);
            toggle_t menu_toggle = {};
            menu_toggle.x = item_x;
            menu_toggle.y = item_y+23;
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
            bar.y = item_y + 12;
            bar.color = GLCD_BLACK;
            bar.width = 35;
            bar.height = 7;
            bar.min = item_child->data.min;
            bar.max = item_child->data.max;
            bar.value = item_child->data.value;
            widget_bar(display, &bar);
                
            if (item_child->data.unit_text)
            {
                char_cnt_name = strlen(item_child->data.unit_text);
                if (char_cnt_name > 5)
                {
                    char_cnt_name = 5;
                }
                memset(str_bfr, 0, (char_cnt_name+1)*sizeof(char));
                strncpy(str_bfr, item_child->data.unit_text, char_cnt_name);
                str_bfr[char_cnt_name] = 0;
                glcd_text(display, (item_x + 18 - char_cnt_name*2), item_y+30, str_bfr, Terminal3x5, GLCD_BLACK);
            }
        break;

        case MENU_LIST:;
            glcd_vline(display, item_x+16, item_y+13, 8, GLCD_BLACK_WHITE);
            glcd_rect(display, item_x, item_y+23, 35, 11, GLCD_BLACK);
            glcd_hline(display, item_x, item_y+28, 5, GLCD_BLACK);
            glcd_hline(display, item_x+30, item_y+28, 5, GLCD_BLACK);
                
            if (item_child->data.unit_text)
            {
                char_cnt_name = strlen(item_child->data.unit_text);
                if (char_cnt_name > 5)
                {
                    char_cnt_name = 5;
                }
                memset(str_bfr, 0, (char_cnt_name+1)*sizeof(char));
                strncpy(str_bfr, item_child->data.unit_text, char_cnt_name);
                str_bfr[char_cnt_name] = 0;
                glcd_text(display, (item_x + 18 - char_cnt_name*2), item_y+26, str_bfr, Terminal3x5, GLCD_BLACK);
            }
        break;

        //others, dont use
        case MENU_MAIN:
        case MENU_ROOT:
        case MENU_CONFIRM2:
        case MENU_OK:
        case MENU_NONE:
        case MENU_CONFIRM:
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
        strcpy(text, SCREEN_ROTARY_DEFAULT_NAME);
        text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)-1] = encoder + '1';
        text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)] = 0;

        glcd_text(display, encoder_x+5, encoder_y+10, text, Terminal3x5, GLCD_BLACK);
        return;
    }

    //draw the title
    uint8_t char_cnt_name = strlen(control->label);

    if (char_cnt_name > 8)
    {
        char_cnt_name = 8;
    }

    char title_str_bfr[char_cnt_name+1];
    memset(title_str_bfr, 0, (char_cnt_name+1)*sizeof(char));
    strncpy(title_str_bfr, control->label, char_cnt_name);
    title_str_bfr[char_cnt_name] = 0;

    //allign to middle, (full width / 2) - (text width / 2)
    glcd_text(display, (encoder_x + 18 - 2*char_cnt_name), encoder_y, title_str_bfr, Terminal3x5, GLCD_BLACK);

    // list type control
    if (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS))
    {
        static char *labels_list[10];

        uint8_t i;
        for (i = 0; i < control->scale_points_count; i++)
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
        list.count = control->scale_points_count;
        list.list = labels_list;
        list.line_space = 1;
        list.line_top_margin = 1;
        list.line_bottom_margin = 1;
        list.text_left_margin = 1;
        widget_list_value(display, &list);
    }
    else if (control->properties & FLAG_CONTROL_TRIGGER)
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
    else if (control->properties & (FLAG_CONTROL_TOGGLED | FLAG_CONTROL_BYPASS))
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
        bar.step = control->step;
        bar.steps = control->steps - 1;

        char str_bfr[15] = {0};
        if ((control->value > 99.9) || (control->properties == FLAG_CONTROL_INTEGER))
        {
            int_to_str(control->value, str_bfr, sizeof(str_bfr), 0);
        }
        else 
        {
            float_to_str((control->value), str_bfr, sizeof(str_bfr), 2);
        }
        str_bfr[14] = 0;

        bar.value = str_bfr;

        //check what to do with the unit
        bar.has_unit = 0;
        if (strcmp("", control->unit) != 0)
        {
            bar.has_unit = 1;
            bar.unit = control->unit;
        }

        widget_bar_encoder(display, &bar);
    }
}

void screen_page_index(uint8_t current, uint8_t available)
{
    char str_current[4];
    char str_available[4];
    int_to_str((current+1), str_current, sizeof(str_current), 1);
    str_current[1] = '/';
    int_to_str((available), str_available, sizeof(str_available), 1);
    str_current[2] = str_available[0];
    str_current[3] = 0;

    glcd_t *display = hardware_glcds(0);

    //clear the part
    glcd_rect_fill(display, 0, 51, 21, 13, GLCD_WHITE);

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
    glcd_rect_fill(display, 0, 13, DISPLAY_WIDTH, 34, GLCD_WHITE);
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

    // clear the footer area
    glcd_rect_fill(display, foot_x, foot_y, 50, 10, GLCD_WHITE);

    //draw the footer box
    glcd_hline(display, foot_x, foot_y, 50, GLCD_BLACK);
    glcd_vline(display, foot_x, foot_y, 10, GLCD_BLACK);
    glcd_vline(display, foot_x+50, foot_y, 10, GLCD_BLACK);

    if (name == NULL && value == NULL)
    {
        char text[sizeof(SCREEN_FOOT_DEFAULT_NAME) + 2];
        strcpy(text, SCREEN_FOOT_DEFAULT_NAME);
        text[sizeof(SCREEN_FOOT_DEFAULT_NAME)-1] = foot_id + '1';
        text[sizeof(SCREEN_FOOT_DEFAULT_NAME)] = 0;

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
        {
            glcd_rect_invert(display, foot_x + 1, foot_y + 1, 49, 9);
        }

        FREE(title_str_bfr);
    }
    else 
    {
        uint8_t char_cnt_name = strlen(name);
        uint8_t char_cnt_value = strlen(value);

        if ((char_cnt_value + char_cnt_name) > 7)
        {
            //both bigger then the limmit
            if ((char_cnt_value > 4) && (char_cnt_name > 3))
            {
                char_cnt_name = 3;
                char_cnt_value = 4;
            }
            else if (char_cnt_value > 4)
            {
                char_cnt_value = 7 - char_cnt_name;
            }
            else if (char_cnt_name > 3)
            {
                char_cnt_name = 7 - char_cnt_value;
            }
        }

        char *title_str_bfr = (char *) MALLOC((char_cnt_name + 1) * sizeof(char));
        char *value_str_bfr = (char *) MALLOC((char_cnt_value + 1) * sizeof(char));

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

void screen_tittle(const void *data, uint8_t update)
{
    static char* pedalboard_name = NULL;
    static uint8_t char_cnt = 0;
    glcd_t *display = hardware_glcds(0);

    if (pedalboard_name == NULL)
    {
        pedalboard_name = (char *) MALLOC(30 * sizeof(char));
        strcpy(pedalboard_name, "DEFAULT");
        char_cnt = 7;
    }

    if (update)
    {
        const char **name_list = (const char**)data;

        // get first list name, copy it to our string buffer
        const char *name_string = *name_list;
        strncpy(pedalboard_name, name_string, 29);
        pedalboard_name[29] = 0; // strncpy might not have final null byte

        // go to next name in list
        name_string = *(++name_list);

        while (name_string && ((strlen(pedalboard_name) + strlen(name_string) + 1) < 29))
        {
            strcat(pedalboard_name, " ");
            strcat(pedalboard_name, name_string);
            name_string = *(++name_list);
            char_cnt++;
        }
        pedalboard_name[29] = 0;

        char_cnt = strlen(pedalboard_name);
    }

    //we dont display inside a menu
    //if (naveg_is_tool_mode(0)) return;

    // clear the name area
    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 9, GLCD_WHITE);

    glcd_text(display, ((DISPLAY_WIDTH / 2) - (3*char_cnt) + 7), 1, pedalboard_name, Terminal5x7, GLCD_BLACK);

    icon_pedalboard(display, ((DISPLAY_WIDTH / 2) - (3*char_cnt) + 7) - 11, 1);

    //invert the top bar
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);
}

void screen_bank_list(bp_list_t *list)
{
    listbox_t list_box = {};

    glcd_t *display = hardware_glcds(0);

    screen_clear();

    //print outlines
    print_menu_outlines();

    // title line separator
    glcd_hline(display, 0, 4, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_vline(display, 0, 4, 3, GLCD_BLACK);
    glcd_vline(display, DISPLAY_WIDTH-1, 4, 3, GLCD_BLACK);

    //clear title area
    glcd_rect_fill(display, 41, 0, 44, 9, GLCD_WHITE);

    // draws the title
    textbox_t title_box = {};
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = Terminal5x7;
    title_box.align = ALIGN_NONE_NONE;
    title_box.text = "BANKS";
    title_box.x = 54;
    title_box.y = 1;
    widget_textbox(display, &title_box);

    icon_bank(display, 43, 1);

    //invert title area
    glcd_rect_invert(display, 41, 0, 44, 9);

    //print the 3 buttons
    //draw the first box, back
    glcd_text(display, 20, DISPLAY_HEIGHT - 7, "ENTER", Terminal3x5, GLCD_BLACK);

    //draw the second box, TODO Builder MODE
    /*box_2.x = 56;
    box_2.y = DISPLAY_HEIGHT - 7;
    box_2.text = "COPY";*/
    glcd_text(display, 26, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

    //draw the third box, save PB
    /*box_3.x = 92;
    box_3.y = DISPLAY_HEIGHT - 7;
    box_3.text = "NEW";*/
    glcd_text(display, 96, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

    // draws the list
    if (list)
    {
        uint8_t count = strarr_length(list->names);
        list_box.x = 1;
        list_box.y = 11;
        list_box.width = DISPLAY_WIDTH-3;
        list_box.height = 45;
        list_box.color = GLCD_BLACK;
        list_box.hover = list->hover - list->page_min;
        list_box.selected = list->selected - list->page_min;
        list_box.count = count;
        list_box.list = list->names;
        list_box.font = Terminal3x5;
        list_box.line_space = 2;
        list_box.line_top_margin = 1;
        list_box.line_bottom_margin = 1;
        list_box.text_left_margin = 2;
        widget_banks_listbox(display, &list_box);
    }
    else
    {
        if (naveg_ui_status())
        {
            textbox_t message;
            message.color = GLCD_BLACK;
            message.mode = TEXT_SINGLE_LINE;
            message.font = Terminal7x8;
            message.top_margin = 0;
            message.bottom_margin = 0;
            message.left_margin = 0;
            message.right_margin = 0;
            message.height = 0;
            message.width = 0;
            message.text = "To access banks here please disconnect from the graphical interface";
            message.align = ALIGN_CENTER_MIDDLE;
            widget_textbox(display, &message);
        }
    }
}

void screen_pbss_list(const char *title, bp_list_t *list, uint8_t pb_ss_toggle)
{
    listbox_t list_box;

    glcd_t *display = hardware_glcds(0);

    screen_clear();

    // draws the list
    if (list)
    {
        uint8_t char_cnt = strlen(title);
        glcd_text(display, ((DISPLAY_WIDTH / 2) - (3*char_cnt) + 7), 1, title, Terminal5x7, GLCD_BLACK);

        //snapshot
        if (!pb_ss_toggle)
            icon_bank(display, ((DISPLAY_WIDTH / 2) - (3*char_cnt) + 7) - 11, 1);
        //pb's
        else 
            icon_pedalboard(display, ((DISPLAY_WIDTH / 2) - (3*char_cnt) + 7) - 11, 1);

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

        const uint8_t *title_font = Terminal5x7;
        uint8_t count = strarr_length(list->names);
        list_box.x = 0;
        list_box.y = 11;
        list_box.width = DISPLAY_WIDTH;
        list_box.height = 45;
        list_box.color = GLCD_BLACK;
        list_box.hover = list->hover - list->page_min;
        list_box.selected = list->selected - list->page_min;
        list_box.count = count;
        list_box.list = list->names;
        list_box.font = Terminal7x8;
        list_box.line_space = 4;
        list_box.line_top_margin = 1;
        list_box.line_bottom_margin = 1;
        list_box.text_left_margin = 2;
        if (!pb_ss_toggle)
            list_box.name = "PEDALBOARDS";
        else
            list_box.name = "SNAPSHOTS";

        widget_listbox_pedalboard(display, &list_box, title_font, pb_ss_toggle);

        //print the 3 buttons
        //draw the first box, back
        
        uint8_t x;
        char *text;
        if (pb_ss_toggle)
        {
            /*box_1.x = 18;
            box_1.text = "RENAME";*/
            x = 28;
            text = "-";
        }
        else
        {
            x = 20;
            text = "BANKS";
        }
        glcd_text(display, x, DISPLAY_HEIGHT - 7, text, Terminal3x5, GLCD_BLACK);

        //draw the second box
        if (!pb_ss_toggle)
        {
            x = 56;
            text = "SAVE";
        }
        else
        {
            x = 62;
            text = "-";
        }
        glcd_text(display, x, DISPLAY_HEIGHT - 7, text, Terminal3x5, GLCD_BLACK);

        //draw the third box, save PB
        /*box_3.x = 86;
        box_3.y = DISPLAY_HEIGHT - 7;
        box_3.text = "DELETE";*/
        glcd_text(display, 96, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);
    }
    else
    {
        if (naveg_ui_status())
        {
            textbox_t message;
            message.color = GLCD_BLACK;
            message.mode = TEXT_SINGLE_LINE;
            message.font = Terminal7x8;
            message.top_margin = 0;
            message.bottom_margin = 0;
            message.left_margin = 0;
            message.right_margin = 0;
            message.height = 0;
            message.width = 0;
            message.text = "To access banks here please disconnect from the graphical interface";
            message.align = ALIGN_CENTER_MIDDLE;
            widget_textbox(display, &message);
        }
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
    glcd_text(display, 18, DISPLAY_HEIGHT - 7, "ENTER>", Terminal3x5, GLCD_BLACK);

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

    switch (item->desc->type)
    {
        case MENU_ROOT:
            list.hover = item->data.hover;
            list.selected = item->data.selected;
            list.count = item->data.list_count;
            list.list = item->data.list;
            if (item->desc->id == MENU_ROOT)
                widget_menu_listbox(display, &list);
            else 
                widget_listbox_mdx(display, &list);
        break;

        case MENU_MAIN:
        case MENU_TOGGLE:
        case MENU_BAR:
        case MENU_NONE:
        case MENU_OK:
        case MENU_LIST:
        case MENU_CONFIRM:
        case MENU_CONFIRM2:
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

    //draw the outlines
    print_menu_outlines();

    //print the 3 buttons
    //draw the first box, back
    glcd_text(display, 20, DISPLAY_HEIGHT - 7, "<BACK", Terminal3x5, GLCD_BLACK);

    //draw the second box, TODO Builder MODE
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
    if (node->next)
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

    node_t *child_nodes = node;
    child_nodes = child_nodes->first_child;
    
    uint8_t i;
    for (i = 0; i < 3; i++)
    {
        print_tripple_menu_items(child_nodes->data, i);

        if (!child_nodes->next)
            return;
        else
            child_nodes = child_nodes->next; 
    }
}

void screen_toggle_tuner(float frequency, char *note, int8_t cents, uint8_t mute, uint8_t input)
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

    //draw foots
    screen_footer(0, "MUTE", mute <= 0 ? TOGGLED_OFF_FOOTER_TEXT : TOGGLED_ON_FOOTER_TEXT, FLAG_CONTROL_TOGGLED);
    screen_footer(1, "INPUT", input==1? "1":"2", FLAG_CONTROL_ENUMERATION);
    screen_page_index(0, 1);

    //draw tuner
    widget_tuner(display, &g_tuner);
}

void screen_update_tuner(float frequency, char *note, int8_t cents)
{
    g_tuner.frequency = frequency;
    g_tuner.note = note;
    g_tuner.cents = cents;

    //draw tuner
    widget_tuner(hardware_glcds(0), &g_tuner);
}

void screen_image(uint8_t display, const uint8_t *image)
{
    glcd_t *display_img = hardware_glcds(display);
    glcd_draw_image(display_img, 0, 0, image, GLCD_BLACK);
}

void screen_shift_overlay(int8_t prev_mode)
{
    static uint8_t previous_mode;

    if (prev_mode != -1)
        previous_mode = prev_mode;

    //clear screen first
    screen_clear();

    glcd_t *display = hardware_glcds(0);

    //draw the title 
    textbox_t title = {};
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = Terminal3x5;
    title.top_margin = 1;
    title.text = "SHIFT";
    title.align = ALIGN_CENTER_TOP;
    widget_textbox(display, &title);

    //invert the title area
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 7);

    //draw the outlines
    print_menu_outlines();

    //draw the first box, menu/control mode 
    uint8_t x;
    char *text;
    if (previous_mode == MODE_CONTROL)
    {
        text = "MENU";
        x = 22;
    }
    else
    { 
        text = "CONTROL";
        x = 16;
    }
    glcd_text(display, x, DISPLAY_HEIGHT - 7, text, Terminal3x5, GLCD_BLACK);

    //draw the second box, TODO Builder MODE
    glcd_text(display, 62, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

    //draw the third box, save PB
    glcd_text(display, 84, DISPLAY_HEIGHT - 7, "SAVE PB", Terminal3x5, GLCD_BLACK);

    //print the 3 quick controls, hardcoded for now
    glcd_text(display, 8, 15, "INPUT-1", Terminal3x5, GLCD_BLACK);
    glcd_text(display, 14, 21, "GAIN", Terminal3x5, GLCD_BLACK);

    char str_bfr[7];
    uint8_t char_cnt_name = 0;

    //print the bar
    menu_bar_t bar_1 = {};
    bar_1.x = 4;
    bar_1.y = 27;
    bar_1.color = GLCD_BLACK;
    bar_1.width = 35;
    bar_1.height = 7;
    bar_1.min = 0;
    bar_1.max = 98;
    bar_1.value = system_get_gain_value(INP_1_GAIN_ID);
    widget_bar(display, &bar_1);
    
    //unit
    memset(str_bfr, 0, (6)*sizeof(char));
    int_to_str(bar_1.value, str_bfr, 8, 0);
    strcat(str_bfr, "%");
    str_bfr[6] = 0;
    char_cnt_name = strlen(str_bfr);
    glcd_text(display, 22 - char_cnt_name*2, 45, str_bfr, Terminal3x5, GLCD_BLACK);

    glcd_text(display, 51, 15, "INPUT-2", Terminal3x5, GLCD_BLACK);
    glcd_text(display, 57, 21, "GAIN", Terminal3x5, GLCD_BLACK);

    //print the bar
    menu_bar_t bar_2 = {};
    bar_2.x = 47;
    bar_2.y = 27;
    bar_2.color = GLCD_BLACK;
    bar_2.width = 35;
    bar_2.height = 7;
    bar_2.min = 0;
    bar_2.max = 98;
    bar_2.value = system_get_gain_value(INP_2_GAIN_ID);
    widget_bar(display, &bar_2);
    
    //unit
    memset(str_bfr, 0, (6)*sizeof(char));
    int_to_str(bar_2.value, str_bfr, 8, 0);
    strcat(str_bfr, "%");
    str_bfr[6] = 0;
    char_cnt_name = strlen(str_bfr);
    glcd_text(display, 65 - char_cnt_name*2, 45, str_bfr, Terminal3x5, GLCD_BLACK);

    glcd_text(display, 91, 15, "OUTPUT", Terminal3x5, GLCD_BLACK);
    glcd_text(display, 99, 21, "GAIN", Terminal3x5, GLCD_BLACK);

    //print the bar
    menu_bar_t bar_3 = {};
    bar_3.x = 89;
    bar_3.y = 27;
    bar_3.color = GLCD_BLACK;
    bar_3.width = 35;
    bar_3.height = 7;
    bar_3.min = 0;
    bar_3.max = 98;
    bar_3.value = system_get_gain_value(OUTP_1_GAIN_ID);
    widget_bar(display, &bar_3);
    
    //unit
    memset(str_bfr, 0, (6)*sizeof(char));
    int_to_str(bar_3.value, str_bfr, 8, 0);
    strcat(str_bfr, "%");
    str_bfr[6] = 0;
    char_cnt_name = strlen(str_bfr);
    glcd_text(display, 107 - char_cnt_name*2, 45, str_bfr, Terminal3x5, GLCD_BLACK);
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
        static char *labels_list[10];

        uint8_t q;
        for (q = 0; ((q < control->scale_points_count) || (q < 10)); q++)
        {
            labels_list[q] = control->scale_points[q]->label;
        }

        //trigger list overlay widget
        listbox_t list;
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
        list.name = control->label;
        list.hover = control->step;
        list.selected = control->step;
        list.count = control->scale_points_count;
        list.list = labels_list;
        widget_listbox_overlay(display, &list);
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