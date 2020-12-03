
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
    {/*
        //draw back the title line on 16
        glcd_hline(display, 0, 16, DISPLAY_WIDTH, GLCD_BLACK);

        char text[sizeof(SCREEN_ROTARY_DEFAULT_NAME) + 2];
        strcpy(text, SCREEN_ROTARY_DEFAULT_NAME);
        text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)-1] = 0 + '1';
        text[sizeof(SCREEN_ROTARY_DEFAULT_NAME)] = 0;

        textbox_t title;
        title.color = GLCD_BLACK;
        title.mode = TEXT_SINGLE_LINE;
        title.font = Terminal7x8;
        title.height = 0;
        title.width = 0;
        title.top_margin = 0;
        title.bottom_margin = 0;
        title.left_margin = 0;
        title.right_margin = 0;
        title.text = text;
        title.align = ALIGN_CENTER_NONE;
        title.y = 33 - (Terminal7x8[FONT_HEIGHT] / 2);
        widget_textbox(display, &title);*/
        return;
    }

    //draw the title
    uint8_t char_cnt_name = strlen(control->label);

    if (char_cnt_name > 8)
    {
        char_cnt_name = 8;
    }

    char title_str_bfr[char_cnt_name+1];
    memset( title_str_bfr, 0, (char_cnt_name+1)*sizeof(char));
    strncpy(title_str_bfr, control->label, char_cnt_name);
    title_str_bfr[char_cnt_name] = 0;

    textbox_t title;
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = Terminal3x5;
    title.height = 0;
    title.width = 0;
    title.top_margin = 0;
    title.bottom_margin = 0;
    title.left_margin = 0;
    title.right_margin = 0;
    title.text = title_str_bfr;
    title.align = ALIGN_NONE_NONE;
    //allign to middle, (full width / 2) - (text width / 2)
    title.x = (encoder_x + 18 - 2*char_cnt_name);
    title.y = encoder_y;
    widget_textbox(display, &title);

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
    else if (control->properties & (FLAG_CONTROL_TOGGLED | FLAG_CONTROL_BYPASS))
    {
        toggle_t toggle;
        toggle.x = encoder_x;
        toggle.y = encoder_y + 6;
        toggle.width = 35;
        toggle.height = 18;
        toggle.color = GLCD_BLACK;
        toggle.value = (control->properties == FLAG_CONTROL_TOGGLED)?control->value:!control->value;
        widget_toggle(display, &toggle);
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
    switch (current)
    {
        case 0:
            glcd_rect_fill(display, 1, 52, 5, 2, GLCD_BLACK);
        break;

        case 1:
            glcd_rect_fill(display, 6, 52, 5, 2, GLCD_BLACK);
        break;

        case 2:
            glcd_rect_fill(display, 11, 52, 5, 2, GLCD_BLACK);
        break;

        case 3:
            glcd_rect_fill(display, 16, 52, 5, 2, GLCD_BLACK);
        break;

        default:
            glcd_rect_fill(display, 1, 52, 5, 2, GLCD_BLACK);
        break;
    }

    //draw devision line
    glcd_hline(display, 0, 54, 21, GLCD_BLACK);

    // draws the text field
    textbox_t index;
    index.color = GLCD_BLACK;
    index.mode = TEXT_SINGLE_LINE;
    index.font = Terminal5x7;
    index.height = 0;
    index.width = 0;
    index.bottom_margin = 0;
    index.left_margin = 0;
    index.right_margin = 0;
    index.align = ALIGN_NONE_NONE;
    index.top_margin = 0;
    index.text = str_current;
    index.x = 3;
    index.y = 56;
    widget_textbox(display, &index);
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
    textbox_t index_1;
    index_1.color = GLCD_BLACK;
    index_1.mode = TEXT_SINGLE_LINE;
    index_1.font = Terminal3x5;
    index_1.height = 0;
    index_1.width = 0;
    index_1.bottom_margin = 0;
    index_1.left_margin = 0;
    index_1.right_margin = 0;
    index_1.align = ALIGN_NONE_NONE;
    index_1.top_margin = 0;
    index_1.text = "I";
    index_1.x = 40;
    index_1.y = 45;
    widget_textbox(display, &index_1);

    //indicator 2
    textbox_t index_2;
    index_2.color = GLCD_BLACK;
    index_2.mode = TEXT_SINGLE_LINE;
    index_2.font = Terminal3x5;
    index_2.height = 0;
    index_2.width = 0;
    index_2.bottom_margin = 0;
    index_2.left_margin = 0;
    index_2.right_margin = 0;
    index_2.align = ALIGN_NONE_NONE;
    index_2.top_margin = 0;
    index_2.text = "II";
    index_2.x = 61;
    index_2.y = 45;
    widget_textbox(display, &index_2);

    //indicator 3
    textbox_t index_3;
    index_3.color = GLCD_BLACK;
    index_3.mode = TEXT_SINGLE_LINE;
    index_3.font = Terminal3x5;
    index_3.height = 0;
    index_3.width = 0;
    index_3.bottom_margin = 0;
    index_3.left_margin = 0;
    index_3.right_margin = 0;
    index_3.align = ALIGN_NONE_NONE;
    index_3.top_margin = 0;
    index_3.text = "III";
    index_3.x = 82;
    index_3.y = 45;
    widget_textbox(display, &index_3);

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

    // draws the name field
    textbox_t footer;
    footer.color = GLCD_BLACK;
    footer.mode = TEXT_SINGLE_LINE;
    footer.font = Terminal5x7;
    footer.height = 0;
    footer.width = 0;
    footer.top_margin = 0;
    footer.bottom_margin = 0;
    footer.left_margin = 1;
    footer.right_margin = 1;
    footer.y = foot_y + 2;

    if (name == NULL && value == NULL)
    {
        char text[sizeof(SCREEN_FOOT_DEFAULT_NAME) + 2];
        strcpy(text, SCREEN_FOOT_DEFAULT_NAME);
        text[sizeof(SCREEN_FOOT_DEFAULT_NAME)-1] = foot_id + '1';
        text[sizeof(SCREEN_FOOT_DEFAULT_NAME)] = 0;

        footer.text = text;
        footer.x = foot_x + (26 - (strlen(text) * 3));
        footer.align = ALIGN_NONE_NONE;
        widget_textbox(display, &footer);
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

        footer.text = title_str_bfr;
        footer.x = foot_x + (26 - (strlen(title_str_bfr) * 3));
        footer.align = ALIGN_NONE_NONE;
        widget_textbox(display, &footer);
    
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
        footer.text = title_str_bfr;
        footer.align = ALIGN_NONE_NONE;
        footer.x = foot_x + 2;
        widget_textbox(display, &footer);

        // draws the value field
        textbox_t value_field;
        value_field.color = GLCD_BLACK;
        value_field.mode = TEXT_SINGLE_LINE;
        value_field.font = Terminal5x7;
        value_field.height = 0;
        value_field.width = 0;
        value_field.top_margin = 0;
        value_field.bottom_margin = 0;
        value_field.left_margin = 1;
        value_field.right_margin = 1;
        value_field.y = foot_y + 2;
        strncpy(value_str_bfr, value, char_cnt_value);
        value_str_bfr[char_cnt_value] = '\0';
        value_field.text = value_str_bfr;
        value_field.align = ALIGN_NONE_NONE;
        value_field.x = foot_x + (50 - ((strlen(value_str_bfr)) * 6));
        widget_textbox(display, &value_field);

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

    textbox_t title;
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = Terminal5x7;
    title.top_margin = 1;
    title.bottom_margin = 0;
    title.left_margin = 10;
    title.right_margin = 1;
    title.height = 0;
    title.width = 0;
    title.text = pedalboard_name;
    title.align = ALIGN_NONE_NONE;
    title.y = 1;
    title.x = ((DISPLAY_WIDTH / 2) - (3*char_cnt) + 7);
    widget_textbox(display, &title);

    icon_pedalboard(display, title.x - 11, 1);

    //invert the top bar
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);
}

void screen_tool(uint8_t tool, uint8_t display_id)
{
    bp_list_t *bp_list;
    glcd_t *display = hardware_glcds(display_id);

    switch (tool)
    {
        case DISPLAY_TOOL_SYSTEM:
            //naveg_reset_menu();
            //naveg_enter(DISPLAY_TOOL_SYSTEM);
            break;

        case DISPLAY_TOOL_TUNER:
            g_tuner.frequency = 0.0;
            g_tuner.note = "?";
            g_tuner.cents = 0;
            widget_tuner(display, &g_tuner);
            break;

        case DISPLAY_TOOL_NAVIG:
            /*bp_list = naveg_get_banks();

            if (naveg_banks_mode_pb() == BANKS_LIST)
            {
                screen_bp_list("BANKS", bp_list);
            }
            else 
            {
                screen_bp_list(naveg_get_current_pb_name(), bp_list);
            }*/
            break;
    }
}

void screen_bp_list(const char *title, bp_list_t *list)
{
    //if (!naveg_is_tool_mode(DISPLAY_RIGHT))
    //    return; 

    listbox_t list_box;
    textbox_t title_box;

    glcd_t *display = hardware_glcds(1);

    // clears the title
    glcd_rect_fill(display, 0, 0, DISPLAY_WIDTH, 9, GLCD_WHITE);

    // draws the title
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = Terminal3x5;
    title_box.top_margin = 0;
    title_box.bottom_margin = 0;
    title_box.left_margin = 0;
    title_box.right_margin = 0;
    title_box.height = 0;
    title_box.width = 0;
    title_box.text = title;
    title_box.align = ALIGN_CENTER_TOP;
    widget_textbox(display, &title_box);

    // title line separator
    glcd_hline(display, 0, 9, DISPLAY_WIDTH, GLCD_BLACK_WHITE);

    // draws the list
    if (list)
    {
        uint8_t count = strarr_length(list->names);
        list_box.x = 0;
        list_box.y = 11;
        list_box.width = 128;
        list_box.height = 53;
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
        widget_listbox(display, &list_box);
    }
    else
    {
        if (naveg_ui_status())
        {
            textbox_t message;
            message.color = GLCD_BLACK;
            message.mode = TEXT_SINGLE_LINE;
            message.font = alterebro24;
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
    static menu_item_t *last_item = NULL;

    glcd_t *display;
    display = hardware_glcds(0);


    //we dont display a menu on the right screen when we are in the banks.
    //if ((display == hardware_glcds(DISPLAY_RIGHT)) && (naveg_tool_is_on(DISPLAY_TOOL_NAVIG)))
    //{
    //    return;
    //} 

    // clear screen
    glcd_clear(display, GLCD_WHITE);

    // draws the title
    textbox_t title_box;
    title_box.color = GLCD_BLACK;
    title_box.mode = TEXT_SINGLE_LINE;
    title_box.font = Terminal3x5;
    title_box.top_margin = 0;
    title_box.bottom_margin = 0;
    title_box.left_margin = 0;
    title_box.right_margin = 0;
    title_box.height = 0;
    title_box.width = 0;
    title_box.align = ALIGN_CENTER_TOP;
    title_box.text = item->name;

        if ((item->desc->type == MENU_NONE) || (item->desc->type == MENU_TOGGLE))
        {
            if (last_item)
            {
                title_box.text = last_item->name;
                if (title_box.text[strlen(item->name) - 1] == ']')
                    title_box.text = last_item->desc->name;
            }
        }
        else if (title_box.text[strlen(item->name) - 1] == ']')
        {
            title_box.text = item->desc->name;
        }
        widget_textbox(display, &title_box);

    // title line separator
    glcd_hline(display, 0, 9, DISPLAY_WIDTH, GLCD_BLACK_WHITE);
    glcd_hline(display, 0, 10, DISPLAY_WIDTH, GLCD_WHITE);

    // menu list
    listbox_t list;
    list.x = 0;
    list.y = 11;
    list.width = 128;
    list.height = 53;
    list.color = GLCD_BLACK;
    list.font = Terminal3x5;
    list.line_space = 2;
    list.line_top_margin = 1;
    list.line_bottom_margin = 1;
    list.text_left_margin = 2;

    // popup
    popup_t popup;
    popup.x = 0;
    popup.y = 0;
    popup.width = DISPLAY_WIDTH;
    popup.height = DISPLAY_HEIGHT;
    popup.font = Terminal3x5;

    switch (item->desc->type)
    {
        case MENU_ROOT:
            list.hover = item->data.hover;
            list.selected = item->data.selected;
            list.count = item->data.list_count;
            list.list = item->data.list;
            widget_listbox(display, &list);
        break;

        case MENU_CONFIRM:
        case MENU_OK:
            if (item->desc->type == MENU_OK)
            {
                popup.type = OK_ONLY;
                popup.type = EMPTY_POPUP;
            }
            else
                popup.type = YES_NO;
            
            popup.title = item->data.popup_header;
            popup.content = item->data.popup_content;
            popup.button_selected = item->data.hover;
            widget_popup(display, &popup);
            break;

        case MENU_TOGGLE:
                list.hover = last_item->data.hover;
                list.selected = last_item->data.selected;
                list.count = last_item->data.list_count;
                list.list = last_item->data.list;
                widget_listbox(display, &list);
            break;

        case MENU_NONE:
            if (last_item)
            {
                list.hover = last_item->data.hover;
                list.selected = last_item->data.selected;
                list.count = last_item->data.list_count;
                list.list = last_item->data.list;
                widget_listbox(display, &list);
            }
            break;
    }
}

void screen_tuner(float frequency, char *note, int8_t cents)
{
    g_tuner.frequency = frequency;
    g_tuner.note = note;
    g_tuner.cents = cents;

    // checks if tuner is enable and update it
    //if (naveg_is_tool_mode(DISPLAY_TOOL_TUNER))
    //    widget_tuner(hardware_glcds(1), &g_tuner);
}

void screen_tuner_input(uint8_t input)
{
    g_tuner.input = input;

    // checks if tuner is enable and update it
    //if (naveg_is_tool_mode(DISPLAY_TOOL_TUNER))
    //    widget_tuner(hardware_glcds(1), &g_tuner);
}

void screen_image(uint8_t display, const uint8_t *image)
{
    glcd_t *display_img = hardware_glcds(display);
    glcd_draw_image(display_img, 0, 0, image, GLCD_BLACK);
}

void screen_shift_overlay(uint8_t prev_mode)
{
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
    glcd_vline(display, 0, 7, DISPLAY_HEIGHT - 11, GLCD_BLACK);
    glcd_vline(display, DISPLAY_WIDTH-1, 7, DISPLAY_HEIGHT - 11, GLCD_BLACK);
    glcd_hline(display, 0, DISPLAY_HEIGHT - 5, 14, GLCD_BLACK);
    glcd_hline(display, 45, DISPLAY_HEIGHT - 5, 3, GLCD_BLACK);
    glcd_hline(display, 79, DISPLAY_HEIGHT - 5, 3, GLCD_BLACK);
    glcd_hline(display, 112, DISPLAY_HEIGHT - 5, 15, GLCD_BLACK);
    glcd_rect(display, 14, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);
    glcd_rect(display, 48, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);
    glcd_rect(display, 82, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);

    //draw the first box, menu/control mode 
    textbox_t box_1 = {};
    box_1.color = GLCD_BLACK;
    box_1.mode = TEXT_SINGLE_LINE;
    box_1.font = Terminal3x5;
    box_1.align = ALIGN_NONE_NONE;
    box_1.y = DISPLAY_HEIGHT - 7;
    if (prev_mode == MODE_CONTROL)
    {
        box_1.text = "MENU";
        box_1.x = 22;
    }
    else
    { 
        box_1.text = "CONTROL";
        box_1.x = 16;
    }
    widget_textbox(display, &box_1);

    //draw the second box, TODO Builder MODE
    textbox_t box_2 = {};
    box_2.color = GLCD_BLACK;
    box_2.mode = TEXT_SINGLE_LINE;
    box_2.font = Terminal3x5;
    box_2.align = ALIGN_NONE_NONE;
    box_2.x = 62;
    box_2.y = DISPLAY_HEIGHT - 7;
    box_2.text = "-";
    widget_textbox(display, &box_2);

    //draw the second box, TODO Builder MODE
    textbox_t box_3 = {};
    box_3.color = GLCD_BLACK;
    box_3.mode = TEXT_SINGLE_LINE;
    box_3.font = Terminal3x5;
    box_3.align = ALIGN_NONE_NONE;
    box_3.x = 84;
    box_3.y = DISPLAY_HEIGHT - 7;
    box_3.text = "SAVE PB";
    widget_textbox(display, &box_3);
}