
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "glcd_widget.h"
#include "utils.h"
#include "mode_navigation.h"
#include "mod-protocol.h"

#include <stdlib.h>
#include <string.h>

/*
************************************************************************************************************************
*           LOCAL DEFINES
************************************************************************************************************************
*/

#define GRAPH_WIDTH         128
#define GRAPH_HEIGHT        32
#define GRAPH_BAR_WIDTH     3
#define GRAPH_BAR_SPACE     1
#define GRAPH_NUM_BARS      sizeof(GraphLinTable)
#define GRAPH_V_NUM_BARS    sizeof(GraphVTable)


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

#define ABS(x)      ((x) > 0 ? (x) : -(x))
#define ROUND(x)    ((x) > 0.f ? (((float)(x)) + 0.5f) : (((float)(x)) - 0.5f))

/*
************************************************************************************************************************
*           LOCAL GLOBAL VARIABLES
************************************************************************************************************************
*/


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

static uint8_t get_text_width(const char *text, const uint8_t *font)
{
    uint8_t text_width = 0;
    const char *ptext = text;

    if (FONT_IS_MONO_SPACED(font))
    {
        while (*ptext)
        {
            text_width += font[FONT_FIXED_WIDTH] + FONT_INTERCHAR_SPACE;
            ptext++;
        }
    }
    else
    {
        while (*ptext)
        {
            text_width += font[FONT_WIDTH_TABLE + ((*ptext) - font[FONT_FIRST_CHAR])] + FONT_INTERCHAR_SPACE;
            ptext++;
        }
    }

    text_width -= FONT_INTERCHAR_SPACE;

    return text_width;
}

/*
************************************************************************************************************************
*           GLOBAL FUNCTIONS
************************************************************************************************************************
*/

void widget_textbox(glcd_t *display, textbox_t *textbox)
{
    uint8_t text_width, text_height;

    if (textbox->text == NULL) return;

    if (textbox->mode == TEXT_SINGLE_LINE)
    {
        text_width = get_text_width(textbox->text, textbox->font);
        text_height = textbox->font[FONT_HEIGHT];
    }
    else
    {
        text_width = textbox->width;
        text_height = textbox->height;
    }

    if (textbox->width == 0) textbox->width = text_width;
    else if (text_width > textbox->width) text_width = textbox->width;

    if (textbox->height == 0) textbox->height = text_height;

    switch (textbox->align)
    {
        case ALIGN_LEFT_TOP:
            textbox->x = textbox->left_margin;
            textbox->y = textbox->top_margin;
            break;

        case ALIGN_CENTER_TOP:
            textbox->x = ((DISPLAY_WIDTH / 2) - (text_width / 2)) + textbox->left_margin;
            textbox->y = textbox->top_margin;
            break;

        case ALIGN_RIGHT_TOP:
            textbox->x = DISPLAY_WIDTH - text_width - textbox->right_margin;
            textbox->y = textbox->top_margin;
            break;

        case ALIGN_LEFT_MIDDLE:
            textbox->x = textbox->left_margin;
            textbox->y = (DISPLAY_HEIGHT / 2) - (text_height / 2);
            break;

        case ALIGN_CENTER_MIDDLE:
            textbox->x = ((DISPLAY_WIDTH / 2) - (text_width / 2)) + (textbox->left_margin - textbox->right_margin);
            textbox->y = ((DISPLAY_HEIGHT / 2) - (text_height / 2)) + (textbox->top_margin - textbox->bottom_margin);
            break;

        case ALIGN_RIGHT_MIDDLE:
            textbox->x = DISPLAY_WIDTH - text_width - textbox->right_margin;
            textbox->y = (DISPLAY_HEIGHT / 2) - (text_height / 2);
            break;

        case ALIGN_LEFT_BOTTOM:
            textbox->x = textbox->left_margin;
            textbox->y = DISPLAY_HEIGHT - text_height - textbox->bottom_margin;
            break;

        case ALIGN_CENTER_BOTTOM:
            textbox->x = (DISPLAY_WIDTH / 2) - (text_width / 2);
            textbox->y = DISPLAY_HEIGHT - text_height - textbox->bottom_margin;
            break;

        case ALIGN_RIGHT_BOTTOM:
            textbox->x = DISPLAY_WIDTH - text_width - textbox->right_margin;
            textbox->y = DISPLAY_HEIGHT - text_height - textbox->bottom_margin;
            break;

        default:
        case ALIGN_NONE_NONE:
            break;

        case ALIGN_LEFT_NONE:
            textbox->x = textbox->left_margin;
            textbox->y += textbox->top_margin;
            break;

        case ALIGN_RIGHT_NONE:
            textbox->x = DISPLAY_WIDTH - text_width - textbox->right_margin;
            textbox->y += textbox->top_margin;
            break;

        case ALIGN_CENTER_NONE:
            textbox->x = (DISPLAY_WIDTH / 2) - (text_width / 2);
            textbox->y += textbox->top_margin;
            break;

        // TODO: others NONE options
    }

    uint8_t i = 0, index;
    const char *ptext = textbox->text;
    char buffer[DISPLAY_WIDTH/2];

    text_width = 0;
    text_height = 0;
    index = FONT_FIXED_WIDTH;

    while (*ptext)
    {
        // gets the index of the current character
        if (!FONT_IS_MONO_SPACED(textbox->font)) index = FONT_WIDTH_TABLE + ((*ptext) - textbox->font[FONT_FIRST_CHAR]);

        // calculates the text width
        text_width += textbox->font[index] + FONT_INTERCHAR_SPACE;

        // buffering
        buffer[i++] = *ptext;

        // checks the width limit
        if (text_width >= textbox->width || *ptext == '\n')
        {
            if (*ptext == '\n') ptext++;

            // check whether is single line
            if (textbox->mode == TEXT_SINGLE_LINE) break;
            else buffer[i-1] = 0;

            glcd_text(display, textbox->x, textbox->y + text_height, buffer, textbox->font, textbox->color);
            text_height += textbox->font[FONT_HEIGHT] + 1;
            text_width = 0;
            i = 0;

            // checks the height limit
            if (text_height > textbox->height) break;
        }
        else ptext++;
    }

    // draws the line
    if (text_width > 0)
    {
        buffer[i] = 0;

        // checks the text width again
        text_width = get_text_width(buffer, textbox->font);
        if (text_width > textbox->width) buffer[i-1] = 0;

        glcd_text(display, textbox->x, textbox->y + text_height, buffer, textbox->font, textbox->color);
    }
}


void widget_listbox(glcd_t *display, listbox_t *listbox)
{
    uint8_t i, font_height, max_lines, y_line;
    uint8_t first_line, focus, center_focus, focus_height;
    char aux[DISPLAY_WIDTH/2];
    const char *line_txt;

    glcd_rect_fill(display, listbox->x, listbox->y, listbox->width, listbox->height, ~listbox->color);

    font_height = listbox->font[FONT_HEIGHT];
    max_lines = listbox->height / (font_height + listbox->line_space);

    center_focus = (max_lines / 2) - (1 - (max_lines % 2));
    first_line = 0;

    if (listbox->hover > center_focus && listbox->count > max_lines)
    {
        first_line = listbox->hover - center_focus;
        if (first_line > ABS(listbox->count - max_lines))
        {
            first_line = ABS(listbox->count - max_lines);
        }
    }

    if (max_lines > listbox->count) max_lines = listbox->count;
    focus = listbox->hover - first_line;
    focus_height = font_height + listbox->line_top_margin + listbox->line_bottom_margin;
    y_line = listbox->y + listbox->line_space;

    for (i = 0; i < max_lines; i++)
    {
        if (i < listbox->count)
        {
            line_txt = listbox->list[first_line + i];

            if ((first_line + i) == listbox->selected)
            {
                uint8_t j = 0;
                aux[j++] = ' ';
                aux[j++] = '>';
                aux[j++] = ' ';
                while (*line_txt && j < sizeof(aux)-1) aux[j++] = *line_txt++;
                aux[j] = 0;
                line_txt = aux;
            }

            glcd_text(display, listbox->x + listbox->text_left_margin, y_line, line_txt, listbox->font, listbox->color);

            if (i == focus)
            {
                glcd_rect_invert(display, listbox->x, y_line - listbox->line_top_margin, listbox->width, focus_height);
            }

            y_line += font_height + listbox->line_space;
        }
    }
}

void widget_menu_listbox(glcd_t *display, listbox_t *listbox)
{
    uint8_t i, font_height, max_lines, y_line;
    uint8_t first_line, focus, center_focus, focus_height;
    const char *line_txt;

    glcd_rect_fill(display, listbox->x, listbox->y, listbox->width, listbox->height-3, ~listbox->color);

    font_height = listbox->font[FONT_HEIGHT];
    max_lines = listbox->height / (font_height + listbox->line_space);

    center_focus = (max_lines / 2) - (1 - (max_lines % 2));
    first_line = 0;

    if (listbox->hover > center_focus && listbox->count > max_lines)
    {
        first_line = listbox->hover - center_focus;
        if (first_line > ABS(listbox->count - max_lines))
        {
            first_line = ABS(listbox->count - max_lines);
        }
    }

    if (max_lines > listbox->count) max_lines = listbox->count;
    focus = listbox->hover - first_line;
    focus_height = font_height + listbox->line_top_margin + listbox->line_bottom_margin;
    y_line = listbox->y + listbox->line_space;

    uint8_t end_of_list = 0, beginning_of_list = 0;

    if (listbox->hover > listbox->count-4)
    {
        end_of_list = 1;
    }
    else if (listbox->hover < 3)
    {
        beginning_of_list = 1;
    }

    for (i = 0; i < max_lines; i++)
    {
        if (i < listbox->count)
        {
            line_txt = listbox->list[first_line + i];

            textbox_t menu_line = {};
            menu_line.color = GLCD_BLACK;
            menu_line.mode = TEXT_SINGLE_LINE;
            menu_line.font = Terminal3x5;
            menu_line.align = ALIGN_CENTER_NONE;
            menu_line.x = listbox->x + listbox->text_left_margin;
            menu_line.y = y_line;
            menu_line.text = line_txt;
            widget_textbox(display, &menu_line);

            if (i == focus)
            {
                glcd_rect_invert(display, listbox->x, y_line - listbox->line_top_margin, listbox->width, focus_height);
            }

            y_line += font_height + listbox->line_space;
        }
    }

    //print arrows
    if (!end_of_list)
    {
        glcd_hline(display, 65, 50, 1, GLCD_BLACK);
        glcd_hline(display, 61, 50, 1, GLCD_BLACK);
        glcd_hline(display, 64, 51, 1, GLCD_BLACK);
        glcd_hline(display, 62, 51, 1, GLCD_BLACK);
        glcd_hline(display, 63, 52, 1, GLCD_BLACK);
    }

    if (!beginning_of_list)
    {
        glcd_hline(display, 65, 10, 1, GLCD_BLACK);
        glcd_hline(display, 61, 10, 1, GLCD_BLACK);
        glcd_hline(display, 64, 9, 1, GLCD_BLACK);
        glcd_hline(display, 62, 9, 1, GLCD_BLACK);
        glcd_hline(display, 63, 8, 1, GLCD_BLACK);
    }
}

void widget_banks_listbox(glcd_t *display, listbox_t *listbox)
{
    uint8_t i, font_height, max_lines, y_line;
    uint8_t first_line, focus, center_focus, focus_height;
    const char *line_txt;
    char aux[DISPLAY_WIDTH/2];

    glcd_rect_fill(display, listbox->x, listbox->y, listbox->width, listbox->height-3, ~listbox->color);

    // title line separator
    glcd_hline(display, 0, 4, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_vline(display, 0, 4, 3, GLCD_BLACK);
    glcd_vline(display, DISPLAY_WIDTH-1, 4, 3, GLCD_BLACK);

    uint8_t name_lenght = strlen(listbox->name);

    if (name_lenght > 16)
        name_lenght = 16;

    char *title_str_bfr = (char *) MALLOC(17 * sizeof(char));
    memcpy(title_str_bfr,listbox->name, name_lenght);
    title_str_bfr[name_lenght] = '\0';

    //clear title area
    glcd_rect_fill(display, (DISPLAY_WIDTH/2) - 3*name_lenght -9, 0, name_lenght*6+12, 9, GLCD_WHITE);

    glcd_text(display, (DISPLAY_WIDTH/2) - 3*name_lenght + 3, 1, title_str_bfr, Terminal5x7, listbox->color);

    // draws the title
    if (!strcmp("BANKS", listbox->name))
        icon_bank(display, (DISPLAY_WIDTH/2) - 3*name_lenght -8, 1);
    else
        icon_pedalboard(display, (DISPLAY_WIDTH/2) - 3*name_lenght -8, 1);

    //invert title area
    glcd_rect_invert(display, (DISPLAY_WIDTH/2) - 3*name_lenght -9, 0, name_lenght*6+12, 9);

    font_height = listbox->font[FONT_HEIGHT];
    max_lines = listbox->height / (font_height + listbox->line_space);

    center_focus = (max_lines / 2) - (1 - (max_lines % 2));
    first_line = 0;

    if (listbox->hover > center_focus && listbox->count > max_lines)
    {
        first_line = listbox->hover - center_focus;
        if (first_line > ABS(listbox->count - max_lines))
        {
            first_line = ABS(listbox->count - max_lines);
        }
    }

    if (max_lines > listbox->count) max_lines = listbox->count;
    focus = listbox->hover - first_line;
    focus_height = 9 + listbox->line_top_margin + listbox->line_bottom_margin;
    y_line = listbox->y + listbox->line_space;

    for (i = 0; i < max_lines; i++)
    {
        if (i < listbox->count)
        {
            line_txt = listbox->list[first_line + i];

            uint8_t j = 0;
            while (*line_txt && j < sizeof(aux)-1) aux[j++] = *line_txt++;
            aux[j] = 0;

            textbox_t banks_list = {};
            banks_list.color = GLCD_BLACK;
            banks_list.mode = TEXT_SINGLE_LINE;
            banks_list.align = ALIGN_LEFT_NONE;
            banks_list.x = listbox->x;
            banks_list.left_margin = listbox->text_left_margin;

            if (i == focus)
            {
                if (strlen(aux) > 20)
                    aux[20] = 0;

                y_line += 2;
                banks_list.font = Terminal5x7;
            }
            else
            { 
                if (strlen(aux) > 30)
                    aux[30] = 0;

                banks_list.font = Terminal3x5;
            }

            banks_list.text = aux;

            banks_list.y = y_line;
            widget_textbox(display, &banks_list);

            if (listbox->type == BANKS_LIST) {
                if ((first_line + i) == listbox->selected) {
                    if (i == focus)
                        icon_bank_selected(display, listbox->x+1, y_line+1);
                    else
                        icon_bank_selected(display, listbox->x+1, y_line);
                }
            }
            else {
                uint8_t selected = 0;

                uint8_t q;
                if ((listbox->type == BANK_LIST_CHECKBOXES_ENGAGED) || (listbox->type == PB_LIST_CHECKBOXES_ENGAGED)) {
                    for (q = 0; q < listbox->selected_count; q++) {
                        if (listbox->type == BANK_LIST_CHECKBOXES_ENGAGED) {
                            if (first_line + i + listbox->page_min_offset == listbox->selected_ids[q]) {
                                selected = 1;
                                continue;
                            }
                        }
                        else if (listbox->type == PB_LIST_CHECKBOXES_ENGAGED) {
                                if (first_line + i + listbox->page_min_offset == listbox->selected_ids[q]) {
                                selected = 1;
                                continue;
                            }
                        }
                    }
                }

                if (!strcmp("BANKS", listbox->name)) {
                    if (listbox->list_item_flags[first_line + i] == 0) {
                        if (i == focus)
                            icon_pb_checkbox(display, listbox->x+1, y_line+2, selected);
                        else
                            icon_pb_checkbox(display, listbox->x+1, y_line+1, selected);
                    }
                }
                else {
                    if (i == focus)
                        icon_pb_checkbox(display, listbox->x+1, y_line+2, selected);
                    else
                        icon_pb_checkbox(display, listbox->x+1, y_line+1, selected);
                }
            }

            //divider
            //we dont care if its selected or not as that should never happen
            if (listbox->list_item_flags[first_line + i] & FLAG_NAVIGATION_DIVIDER) {
                // We decided to not have text, so we need to add another black pixel, change back if needed someday
                //glcd_rect_fill(display, listbox->x+1, y_line+1, 4, 3, GLCD_BLACK);
                glcd_rect_fill(display, listbox->x+2, y_line+1, 4, 3, GLCD_BLACK);
                uint8_t bar_x = (strlen(aux) * 4) + listbox->x+1 + 5;
                glcd_rect_fill(display, bar_x, y_line+1, listbox->width - bar_x, 3, GLCD_BLACK);
            }

            if (i == focus)
            {
                glcd_rect_invert(display, listbox->x, y_line -1 - listbox->line_top_margin, listbox->width, focus_height);
                y_line += 11;
            }
            else 
                y_line += font_height + listbox->line_space;
        }
    }

    FREE(title_str_bfr);
}

void widget_pb_ss_title(glcd_t *display, listbox_t *listbox, const uint8_t *title_font, uint8_t toggle)
{
    //draw the title line around it
    glcd_hline(display, listbox->x, listbox->y+5, DISPLAY_WIDTH, GLCD_BLACK);

    //create title string
    uint8_t char_cnt_name = strlen(listbox->name);
    if (char_cnt_name > 16)
        char_cnt_name = 16;
    char *title_str_bfr = (char *) MALLOC((char_cnt_name + 1) * sizeof(char));
    strncpy(title_str_bfr, listbox->name, char_cnt_name);
    title_str_bfr[char_cnt_name] = '\0';

    //clear the name area
    glcd_rect_fill(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3-9, 12, ((6*char_cnt_name) +13), 9, ~listbox->color);

    //draw the title
    glcd_text(display,  ((DISPLAY_WIDTH) /2) - char_cnt_name*3 + 4, listbox->y+2, title_str_bfr, title_font, listbox->color);

    //draw the icon before
    if (!toggle)
        icon_pedalboard(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3 -7, listbox->y+2);
    else
        icon_snapshot(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3 -7, listbox->y+2);

    // invert the name area
    glcd_rect_invert(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3-9, 12, ((6*char_cnt_name) +13), 9);

    FREE(title_str_bfr);
}

void widget_listbox_pedalboard(glcd_t *display, listbox_t *listbox, const uint8_t *title_font, uint8_t toggle)
{
    widget_pb_ss_title(display, listbox, title_font, toggle);

    //create a buffer, max line length is 15
    char *item_str_bfr = (char *) MALLOC(16 * sizeof(char));

    //draw the list
    if (listbox->hover > 0) {
        uint8_t line_length = strlen(listbox->list[listbox->hover-1]);

        if (line_length > 15)
            line_length = 15;

        //commented out code is allignment to the middle
        uint8_t item_x = 7;//(DISPLAY_WIDTH / 2) - ((line_length * 8) / 2);

        memcpy(item_str_bfr, listbox->list[listbox->hover-1], line_length);
        item_str_bfr[line_length] = '\0';

        //draw indicator
        if (listbox->selected == listbox->hover-1)
            icon_pb_selected(display, item_x-5, listbox->y + 13);

        glcd_text(display, item_x, listbox->y + 12, item_str_bfr, listbox->font, listbox->color);
    }

    if (listbox->hover < (listbox->count - 1)) {
        uint8_t item_id = 0;
        if (listbox->hover == -1)
            item_id = listbox->hover + 2;
        else
            item_id = listbox->hover + 1;

        uint8_t line_length = strlen(listbox->list[item_id]);

        if (line_length > 15)
            line_length = 15;

        //commented out code is allignment to the middle
        uint8_t item_x = 7;// (DISPLAY_WIDTH / 2) - ((line_length * 8) / 2);

        memcpy(item_str_bfr, listbox->list[item_id], line_length);
        item_str_bfr[line_length] = '\0';

        //draw indicator
        if (listbox->selected == item_id)
            icon_pb_selected(display, item_x-5, listbox->y + 33);

        glcd_text(display, item_x, listbox->y + 32, item_str_bfr, listbox->font, listbox->color);
    }
    
    uint8_t item_id = 0;
    if (listbox->hover == -1)
        item_id = listbox->hover + 1;
    else
        item_id = listbox->hover;
    
    uint8_t line_length = strlen(listbox->list[item_id]);

    if (line_length > 15)
        line_length = 15;

    //commented out code is allignment to the middle
    uint8_t item_x = 7;// (DISPLAY_WIDTH / 2) - ((line_length * 8) / 2);

    memcpy(item_str_bfr, listbox->list[item_id], line_length);
    item_str_bfr[line_length] = '\0';

    //draw indicator
    if (listbox->selected == item_id)
        icon_pb_selected(display, item_x-5, listbox->y + 23);

    glcd_text(display, item_x, listbox->y + 22, item_str_bfr, listbox->font, listbox->color);

    if (listbox->hover != -1)
        glcd_rect_invert(display, listbox->x+1, listbox->y + 21, listbox->width-2, 10);
    
    FREE(item_str_bfr);
}

//TODO, this should not be handled here but way before
void widget_listbox_pedalboard_draging(glcd_t *display, listbox_t *listbox, const uint8_t *title_font, uint8_t toggle,
                               int8_t hold_item_index, const char *hold_item_label)
{
    widget_pb_ss_title(display, listbox, title_font, toggle);

    //draw the list
    //create a buffer, max line length is 15
    char *item_str_bfr = (char *) MALLOC(16 * sizeof(char));

    int16_t relative_item_index = hold_item_index-listbox->page_min_offset;

    //uper item
    if (listbox->hover > 0) {

        uint8_t item_index = listbox->hover;

        if (relative_item_index >= listbox->hover)
            item_index--;

        uint8_t line_length = strlen(listbox->list[item_index]);

        if (line_length > 15)
            line_length = 15;

        //commented out code is allignment to the middle
        uint8_t item_x = 7;//(DISPLAY_WIDTH / 2) - ((line_length * 8) / 2);

        memcpy(item_str_bfr, listbox->list[item_index], line_length);
        item_str_bfr[line_length] = '\0';

        //draw indicator
        if (listbox->selected == item_index)
            icon_pb_selected(display, item_x-5, listbox->y + 13);

        glcd_text(display, item_x, listbox->y + 12, item_str_bfr, listbox->font, listbox->color);
    }

    //lower item
    if (listbox->hover < listbox->count -1) {

        uint8_t item_index = listbox->hover+1; 

        if (relative_item_index > listbox->hover)
            item_index--;

        uint8_t line_length = strlen(listbox->list[item_index]);

        if (line_length > 15)
            line_length = 15;

        //commented out code is allignment to the middle
        uint8_t item_x = 7;// (DISPLAY_WIDTH / 2) - ((line_length * 8) / 2);

        memcpy(item_str_bfr, listbox->list[item_index], line_length);
        item_str_bfr[line_length] = '\0';

        //draw indicator
        if (listbox->selected == item_index)
            icon_pb_selected(display, item_x-5, listbox->y + 33);

        glcd_text(display, item_x, listbox->y + 32, item_str_bfr, listbox->font, listbox->color);
    }

    uint8_t line_length = strlen(hold_item_label);

    if (line_length > 14)
        line_length = 14;

    //commented out code is allignment to the middle
    uint8_t item_x = 7;// (DISPLAY_WIDTH / 2) - ((line_length * 8) / 2);

    memcpy(item_str_bfr, hold_item_label, line_length);
    item_str_bfr[line_length] = '\0';

    //draw drag indicator
    icon_pb_grabbed(display, item_x-5, listbox->y + 22, 0);
    icon_pb_grabbed(display, DISPLAY_WIDTH-7, listbox->y + 22, 1);

    glcd_text(display, item_x, listbox->y + 22, item_str_bfr, listbox->font, listbox->color);
    glcd_rect_invert(display, listbox->x+1, listbox->y + 21, listbox->width-2, 10);
    FREE(item_str_bfr);
}

void widget_add_pb_button(glcd_t *display, uint8_t x, uint8_t y, uint8_t invert)
{
    glcd_text(display, x + 3, y+2, "add pb to bank", Terminal3x5, GLCD_BLACK);

    glcd_rect(display, x, y, 61, 9, GLCD_BLACK);

    if (invert)
        glcd_rect_invert(display, x+1, y + 1, 59, 7);
}

void widget_listbox_overlay(glcd_t *display, listbox_t *listbox)
{
    //draw the box and tittle
    glcd_hline(display, listbox->x, listbox->y+5, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_hline(display, listbox->x, listbox->y+listbox->height, DISPLAY_WIDTH, GLCD_BLACK);

    //clear the area
    glcd_rect_fill(display, listbox->x, listbox->y, listbox->width, listbox->height, ~listbox->color);
    //hardcoded a bit, removes the encoder pages that are lower then the widget
    glcd_rect_fill(display, 31, listbox->y+listbox->height, 69, 5, ~listbox->color);

    glcd_hline(display, listbox->x, listbox->y+5, DISPLAY_WIDTH, GLCD_BLACK);

    uint8_t char_cnt_name = strlen(listbox->name);
    if (char_cnt_name > 15)
    {
        //limit string
        char_cnt_name = 15;
    }
    char *title_str_bfr = (char *) MALLOC((char_cnt_name + 1) * sizeof(char));
    strncpy(title_str_bfr, listbox->name, char_cnt_name);
    title_str_bfr[char_cnt_name] = '\0';

    //clear the name area
    glcd_rect_fill(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3-3, 12, ((6*char_cnt_name) +11), 9, ~listbox->color);

    //draw the title
    glcd_text(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3 + 7, listbox->y+2, title_str_bfr, listbox->font, listbox->color);

    //draw the icon before
    icon_overlay(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3 -1, listbox->y+4);

    // invert the name area
    glcd_rect_invert(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3-3, 12, ((6*char_cnt_name) +11), 9);

    //draw the list
    if (listbox->selected > 0)
    {
        uint8_t line_length = strlen(listbox->list[listbox->selected-1]);
        if (line_length > 20)
            line_length = 20;

        listbox->list[listbox->selected-1][line_length] = 0;

        glcd_text(display, (DISPLAY_WIDTH / 2) - ((line_length * 6) / 2), listbox->y + 12, listbox->list[listbox->selected-1], listbox->font, listbox->color);
    }

    if (listbox->selected < (listbox->count - 1))
    {
        uint8_t line_length = strlen(listbox->list[listbox->selected+1]);
        if (line_length > 20)
            line_length = 20;

        listbox->list[listbox->selected+1][line_length] = 0;

        glcd_text(display, (DISPLAY_WIDTH / 2) - ((line_length * 6) / 2), listbox->y + 30, listbox->list[listbox->selected+1], listbox->font, listbox->color);
    }

    uint8_t line_length = strlen(listbox->list[listbox->selected]);
    if (line_length > 20)
        line_length = 20;

    listbox->list[listbox->selected][line_length] = 0;

    glcd_text(display, (DISPLAY_WIDTH / 2) - ((line_length * 6) / 2), listbox->y + 21, listbox->list[listbox->selected], listbox->font, listbox->color);

    glcd_rect_invert(display, listbox->x+1, listbox->y + 20, listbox->width-2, 9);

    //draw the box around it
    glcd_vline(display, 0, listbox->y+5, listbox->height-5, GLCD_BLACK);
    glcd_vline(display, listbox->x+listbox->width-1, listbox->y+5, listbox->height-5, GLCD_BLACK);
    glcd_hline(display, listbox->x, listbox->y+listbox->height+1, DISPLAY_WIDTH, GLCD_WHITE);
    glcd_hline(display, listbox->x, listbox->y+listbox->height, DISPLAY_WIDTH, GLCD_BLACK);

    FREE(title_str_bfr);
}

void widget_foot_overlay(glcd_t *display, overlay_t *overlay)
{
    //clear the area
    glcd_rect_fill(display, overlay->x, overlay->y, overlay->width, overlay->height, ~overlay->color);
    //hardcoded a bit, removes the encoder pages that are lower then the widget
    glcd_rect_fill(display, 31, overlay->y+overlay->height, 69, 5, ~overlay->color);

    //draw the box around it
    glcd_hline(display, overlay->x, overlay->y+5, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_hline(display, overlay->x, overlay->y+overlay->height, DISPLAY_WIDTH, GLCD_BLACK);
    glcd_vline(display, 0, overlay->y+5, overlay->height-5, GLCD_BLACK);
    glcd_vline(display, overlay->x+overlay->width-1, overlay->y+5, overlay->height-5, GLCD_BLACK);

    uint8_t char_cnt_name = strlen(overlay->name);
    if (char_cnt_name > 19)
    {
        //limit string
        char_cnt_name = 19;
    }
    char *title_str_bfr = (char *) MALLOC((char_cnt_name + 1) * sizeof(char));
    strncpy(title_str_bfr, overlay->name, char_cnt_name);
    title_str_bfr[char_cnt_name] = '\0';

    //clear the name area
    glcd_rect_fill(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3-3, 12, ((6*char_cnt_name) +11), 9, ~overlay->color);

    textbox_t title;
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.font = Terminal5x7;
    title.top_margin = 0;
    title.bottom_margin = 0;
    title.left_margin = 0;
    title.right_margin = 0;
    title.height = 0;
    title.width = 0;
    title.align = ALIGN_NONE_NONE;
    title.y = overlay->y+2;

    title.text = title_str_bfr;

    title.x = ((DISPLAY_WIDTH) /2) - char_cnt_name*3 + 7;
    widget_textbox(display, &title);

    //draw the icon before
    icon_overlay(display, title.x - 8, overlay->y+4);

    // invert the name area
    glcd_rect_invert(display, ((DISPLAY_WIDTH) /2) - char_cnt_name*3-3, 12, ((6*char_cnt_name) +11), 9);

    //draw the value
    textbox_t value;
    uint8_t char_cnt_value = strlen(overlay->value);
    value.color = GLCD_BLACK;
    value.mode = TEXT_SINGLE_LINE;
    value.font = Terminal7x8;
    value.top_margin = 0;
    value.bottom_margin = 0;
    value.left_margin = 0;
    value.right_margin = 0;
    value.height = 0;
    value.width = 0;
    value.align = ALIGN_NONE_NONE;
    value.y = overlay->y+19;
    value.text = overlay->value;
    value.x = ((DISPLAY_WIDTH) /2) - char_cnt_value*4 + 1;
    widget_textbox(display, &value);

    //invert the value area if needed
    if (overlay->properties & FLAG_CONTROL_BYPASS)
    {
        //weird lv2 stuff, bypass on means effect off
        overlay->value_num = float_is_zero(overlay->value_num) ? 1 : 0;
    }
    else if (overlay->properties & FLAG_CONTROL_TAP_TEMPO)
    {
        //always invert a value
        overlay->value_num = 1;
    }

    if (overlay->properties & FLAG_CONTROL_TRIGGER)
    {
        glcd_rect_fill(display, overlay->x+3, overlay->y+22, 29, 3, overlay->color);
        glcd_rect_fill(display, overlay->x+97, overlay->y+22, 29, 3, overlay->color);
        //draw 2 beatifull lines next to the word 'trigger'
    }
    else
    {
        //invert the area if value is 1
        if (float_is_not_zero(overlay->value_num))
        {
            uint8_t begin_of_tittle_block = (((DISPLAY_WIDTH) /2) - char_cnt_name*3-3);
            uint8_t end_of_tittle_block = (((DISPLAY_WIDTH) /2) - char_cnt_name*3-2) + ((6*char_cnt_name) +10);
            glcd_rect_invert(display, overlay->x + 2, overlay->y + 7, begin_of_tittle_block - 3, 4);
            glcd_rect_invert(display, end_of_tittle_block + 1, overlay->y + 7, DISPLAY_WIDTH - end_of_tittle_block - 3, 4);
            //bigger invert
            glcd_rect_invert(display, overlay->x+2, overlay->y + 11, DISPLAY_WIDTH - 4, overlay->height - 12);
        }
    }

    FREE(title_str_bfr);
}

void widget_list_value(glcd_t *display, listbox_t *listbox)
{
    //clear
    glcd_rect_fill(display, listbox->x, listbox->y, listbox->width, listbox->height, ~listbox->color);

    //format list item name
    char value_str_bfr[9] = {0};

    //draw name
    strncpy(value_str_bfr, listbox->list[listbox->selected], 8);
    value_str_bfr[8] = '\0';

    //draw the current list item
    textbox_t value;
    value.color = GLCD_BLACK;
    value.mode = TEXT_SINGLE_LINE;
    value.font = Terminal3x5;
    value.height = 5;
    value.width = listbox->width;
    value.top_margin = 0;
    value.bottom_margin = 0;
    value.left_margin = 0;
    value.right_margin = 0;
    value.text = value_str_bfr;
    value.align = ALIGN_NONE_NONE;
    //allign to middle, (full width / 2) - (text width / 2)
    value.x = (listbox->x + (listbox->width / 2) - 2*strlen(value_str_bfr)) + 1;
    value.y = listbox->y + 6;
    widget_textbox(display, &value);

    //invert text area
    glcd_rect_invert(display, listbox->x, listbox->y+5, listbox->width, 7);

    //check if we need to draw arrow down
    if (listbox->selected != (listbox->count-1))
    {
        glcd_hline(display, listbox->x + 14, listbox->y + 13, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 20, listbox->y + 13, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 15, listbox->y + 14, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 19, listbox->y + 14, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 16, listbox->y + 15, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 18, listbox->y + 15, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 17, listbox->y + 16, 1, GLCD_BLACK);

    }   

    //check if we need to draw arrow up
    if (listbox->selected != 0)
    {
        glcd_hline(display, listbox->x + 14, listbox->y + 3, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 20, listbox->y + 3, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 15, listbox->y + 2, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 19, listbox->y + 2, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 16, listbox->y + 1, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 18, listbox->y + 1, 1, GLCD_BLACK);
        glcd_hline(display, listbox->x + 17, listbox->y, 1, GLCD_BLACK);
    }
}

void widget_bar_encoder(glcd_t *display, bar_t *bar)
{
    //draw the value (18 = width/2, 13 = height bar + text height + txt spacing)
    glcd_text(display, (bar->x - 2*strlen(bar->value)) + 18, bar->y + 13, bar->value, Terminal3x5, GLCD_BLACK);

    //draw the
    float OldRange, NewRange, NewValue;
    int32_t bar_possistion, NewMax, NewMin;

    NewMin = 1;
    NewMax = bar->width - 2;

    OldRange = (bar->steps);
    NewRange = (NewMax - NewMin);

    NewValue = (((bar->step) * NewRange) / OldRange) + NewMin;
    bar_possistion = ROUND(NewValue);

    //draw the square
    glcd_rect(display, bar->x, bar->y+6, bar->width, bar->height, GLCD_BLACK);

    //prevent it from trippin 
    if (bar_possistion < 1) bar_possistion = 1;
    if (bar_possistion > bar->width - 2) bar_possistion = bar->width - 2;

    //color in the position area
    glcd_rect_fill(display, (bar->x+1), (bar->y+7), bar_possistion, bar->height - 2, GLCD_BLACK);
}

void widget_bar(glcd_t *display, menu_bar_t *bar)
{
    float OldRange, NewRange, NewValue;
    int32_t bar_possistion, NewMax, NewMin;

    NewMin = 1;
    NewMax = bar->width - 2;

    //(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    OldRange = bar->max - bar->min;
    NewRange = NewMax - NewMin;

    NewValue = (((bar->value - bar->min) * NewRange) / OldRange) + NewMin;
    bar_possistion = ROUND(NewValue);

    //draw the square
    glcd_rect(display, bar->x, bar->y+6, bar->width, bar->height, GLCD_BLACK);

    //prevent it from trippin 
    if (bar_possistion < 1) bar_possistion = 1;
    if (bar_possistion > bar->width - 2) bar_possistion = bar->width - 2;

    //color in the position area
    glcd_rect_fill(display, (bar->x+1), (bar->y+7), bar_possistion, bar->height - 2, GLCD_BLACK);
}

void widget_toggle(glcd_t *display, toggle_t *toggle)
{
    //draw the square
    glcd_rect(display, toggle->x, toggle->y, toggle->width, toggle->height, GLCD_BLACK);

    textbox_t label;
    label.color = GLCD_BLACK;
    label.mode = TEXT_SINGLE_LINE;
    label.font = Terminal3x5;
    label.height = 0;
    label.width = 0;
    label.top_margin = 0;
    label.bottom_margin = 0;
    label.left_margin = 0;
    label.right_margin = 0;
    label.align = ALIGN_NONE_NONE;
    label.y = toggle->y + ((toggle->height - 5)/2);

    //trigger
    if (toggle->value >= 2)
    {
        label.x = toggle->x + (toggle->width / 2) - 7;
        label.text = "TRIG";
        widget_textbox(display, &label);

        //draw trigger lines
        glcd_hline(display, toggle->x + 2, toggle->y + 8, 7, GLCD_BLACK);
        glcd_hline(display, toggle->x + 26, toggle->y + 8, 7, GLCD_BLACK);
    }
    //toggle
    else if (floats_are_equal(toggle->value, 1.f))
    {
        label.x = toggle->x + (toggle->width / 2) - 4;
        label.text = "ON";
        widget_textbox(display, &label);

        //color in the position area
        glcd_rect_invert(display, toggle->x+2, toggle->y+2, toggle->width -4, toggle->height - 4);
    }
    else 
    {
        label.x = toggle->x + (toggle->width / 2) - 6;
        label.text = "OFF";
        widget_textbox(display, &label);

        //color in the position area
        if (toggle->inner_border)
            glcd_rect(display, toggle->x+2, toggle->y+2, toggle->width -4, toggle->height - 4, GLCD_BLACK);
    }
}

// FIXME: this widget is hardcoded
void widget_peakmeter(glcd_t *display, uint8_t pkm_id, peakmeter_t *pkm)
{
    uint8_t height, y_black, y_chess, y_peak, h_black, h_chess;
    const uint8_t h_black_max = 20, h_chess_max = 22;
    const uint8_t x_bar[] = {4, 30, 57, 83};
    const float h_max = 42.0, max_dB = 0.0, min_dB = -30.0;

    // calculates the bar height
    float value = pkm->value;
    if (value > max_dB) value = max_dB;
    if (value < min_dB) value = min_dB;
    value = ABS(min_dB) - ABS(value);
    height = (uint8_t) ROUND((h_max * value) / (max_dB - min_dB));

    // clean the peakmeter bar
    if (pkm_id == 0) glcd_rect_fill(display, 4, 13, 16, 42, GLCD_WHITE);
    else if (pkm_id == 1) glcd_rect_fill(display, 30, 13, 16, 42, GLCD_WHITE);
    else if (pkm_id == 2) glcd_rect_fill(display, 57, 13, 16, 42, GLCD_WHITE);
    else if (pkm_id == 3) glcd_rect_fill(display, 83, 13, 16, 42, GLCD_WHITE);

    // draws the black area
    if (height > h_chess_max)
    {
        h_black = height - h_chess_max;
        y_black = 13 + (h_black_max - h_black);
        glcd_rect_fill(display, x_bar[pkm_id], y_black, 16, h_black, GLCD_BLACK);
    }

    // draws the chess area
    if (height > 0)
    {
        h_chess = (height > h_chess_max ? h_chess_max : height);
        y_chess = 33 + (h_chess_max - h_chess);
        glcd_rect_fill(display, x_bar[pkm_id], y_chess, 16, h_chess, GLCD_CHESS);
    }

    // draws the peak
    if (pkm->peak > pkm->value)
    {
        y_peak = 13.f + ABS(ROUND((h_max * pkm->peak) / (max_dB - min_dB)));
        glcd_hline(display, x_bar[pkm_id], y_peak, 16, GLCD_BLACK);
    }
}

void widget_tuner(glcd_t *display, tuner_t *tuner)
{
    glcd_rect_fill(display, 1, 14, DISPLAY_WIDTH-2, DISPLAY_HEIGHT - 32, GLCD_WHITE);

    // draw outlines tuner
    glcd_vline(display, 0, 14, DISPLAY_HEIGHT - 32, GLCD_BLACK);
    glcd_vline(display, DISPLAY_WIDTH - 1, 14, DISPLAY_HEIGHT - 32, GLCD_BLACK);
    glcd_hline(display, 0, DISPLAY_HEIGHT - 51, DISPLAY_WIDTH , GLCD_BLACK);
    glcd_hline(display, 0,  45, DISPLAY_WIDTH, GLCD_BLACK);

    //draw the middle line
    glcd_hline(display, 3, 33, DISPLAY_WIDTH - 6, GLCD_BLACK);

    //draw the 11 doties
    uint8_t dotie_count;
    uint8_t x_pos_doties = 4;
    for (dotie_count = 0; dotie_count < 11; dotie_count++)
    {
        glcd_vline(display, x_pos_doties, 31, 2, GLCD_BLACK);
        x_pos_doties += 12;
    }

    //draw the note box
    glcd_vline(display, 64, 29, 4, GLCD_BLACK);
    glcd_vline(display, 63, 29, 4, GLCD_BLACK);
    glcd_vline(display, 65, 29, 4, GLCD_BLACK);
    glcd_hline(display, 51, 28, 27, GLCD_BLACK);
    glcd_vline(display, 51, 16, 12, GLCD_BLACK);
    glcd_vline(display, 77, 16, 12, GLCD_BLACK);
    glcd_hline(display, 51, 16, 26, GLCD_BLACK);

    //print note char
    uint8_t text_width = get_text_width(tuner->note, Terminal7x8);
    uint8_t textbox_x = (DISPLAY_WIDTH / 2) - (text_width / 2);
    uint8_t textbox_y = 19;

    glcd_text(display, textbox_x, textbox_y, tuner->note, Terminal7x8, GLCD_BLACK);

    //print reference frequency
    char buffer[16];
    uint8_t o = copy_command(buffer, "A4: ");
    int_to_str(TUNER_REFERENCE_FREQ_MIN + tuner->ref_freq, &buffer[o], sizeof(buffer)-o, 0);
    glcd_text(display, 8, 18, buffer, Terminal3x5, GLCD_BLACK);

    //print bar for reference frequency
    text_width = get_text_width(buffer, Terminal3x5);
    uint8_t state = ((float)tuner->ref_freq / (TUNER_REFERENCE_FREQ_MAX - TUNER_REFERENCE_FREQ_MIN) * text_width);
    glcd_rect(display, 8, 18 + 6, text_width, 4, GLCD_BLACK);
    glcd_rect_fill(display, 8, 18 + 6, state, 4, GLCD_BLACK);

    // check if reference frequency is modified
    if (TUNER_REFERENCE_FREQ_MIN + tuner->ref_freq != 440)
        glcd_rect_invert(display, 7, 17, text_width+2, 6);

    //print frequency
    float_to_str(tuner->frequency, buffer, sizeof(buffer), 2);
    strcat(buffer, " Hz");
    text_width = get_text_width(buffer, Terminal3x5);
    glcd_text(display, DISPLAY_WIDTH - text_width - 8, 18, buffer, Terminal3x5, GLCD_BLACK);

    //print cents
    float cent = tuner->cents * 0.01f;
    float_to_str(cent, buffer, sizeof(buffer), 2);
    strcat(buffer, " c ");
    text_width = get_text_width(buffer, Terminal3x5);
    glcd_text(display, DISPLAY_WIDTH - text_width - 8, 18 + 6, buffer, Terminal3x5, GLCD_BLACK);

    //print value bar
    glcd_vline(display, 64, 35, 7, GLCD_BLACK);

    // constants configurations
    uint8_t h_bar = 1;
    uint8_t num_bar_steps = 10;
    uint8_t w_bar_interval = 6;
    uint8_t y_bar = 35;
    uint8_t x, i, j;
    uint16_t c;

    // cents been given multiplied by 100
    // we use a resolution from top to bottom: 0.1, 1.0, 10.0 cent
    c = ABS(tuner->cents / 10);

    // draw the bar splitted into 3 precision levels
    for (j = 0; j < 3; j++)
    {
        // draws the left side bars
        for (i = 0, x = 4; (i < num_bar_steps) & (tuner->cents < 0); i++)
        {
            // checks if need fill the bar
            if (i < (num_bar_steps - c))
            {
                glcd_rect_fill(display, x, y_bar, w_bar_interval, h_bar, GLCD_WHITE);
            }
            else
            {
                glcd_rect_fill(display, x, y_bar, w_bar_interval, h_bar, GLCD_BLACK);
            }
            x = x + w_bar_interval;
        }

        // draws the right side bars
        for (i = 0, x = 65; (i < num_bar_steps) & (tuner->cents > 0); i++)
        {
            // checks if need fill the bar
            if (i < c )
                glcd_rect_fill(display, x, y_bar, w_bar_interval, h_bar, GLCD_BLACK);
            else
                glcd_rect_fill(display, x, y_bar, w_bar_interval, h_bar, GLCD_WHITE);
            x = x + w_bar_interval;
        }

        // set y axis for next bar
        y_bar += h_bar;

        // drop precision
        c = c / 10;
        h_bar = 3;

        // enlarge the steps when we are in the 10 cent resolution
        if (j >= 1)
        {
            h_bar = 2;
            num_bar_steps = 5;
            w_bar_interval = 12;
        }
    }

    // checks if is tuned (resolution < 3 cent)
    c = floor(ABS(cent));
    if (c <= 2)
        glcd_rect_invert(display, 51, 16, 27, 13);

    // draw the outher ends
    glcd_vline(display, 3, 25, 17, GLCD_BLACK);
    glcd_vline(display, 4, 25, 17, GLCD_BLACK);
    glcd_vline(display, 125, 25, 17, GLCD_BLACK);
    glcd_vline(display, 124, 25, 17, GLCD_BLACK);
} 

void widget_popup(glcd_t *display, popup_t *popup)
{
    // clears the popup area
    glcd_rect_fill(display, popup->x, popup->y, popup->width, popup->height, GLCD_WHITE);

    // draws the contour
    glcd_vline(display, 0, 9, DISPLAY_HEIGHT - 13, GLCD_BLACK);
    glcd_vline(display, DISPLAY_WIDTH-1, 9, DISPLAY_HEIGHT - 13, GLCD_BLACK);
    glcd_hline(display, 0, DISPLAY_HEIGHT - 5, 14, GLCD_BLACK);
    glcd_hline(display, 45, DISPLAY_HEIGHT - 5, 3, GLCD_BLACK);
    glcd_hline(display, 79, DISPLAY_HEIGHT - 5, 3, GLCD_BLACK);
    glcd_hline(display, 112, DISPLAY_HEIGHT - 5, 15, GLCD_BLACK);
    glcd_rect(display, 14, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);
    glcd_rect(display, 48, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);
    glcd_rect(display, 82, DISPLAY_HEIGHT - 9, 31, 9, GLCD_BLACK);

    // draws the title text
    textbox_t title;
    title.color = GLCD_BLACK;
    title.mode = TEXT_SINGLE_LINE;
    title.align = ALIGN_CENTER_TOP;
    title.font = Terminal5x7;
    title.top_margin = 1;
    title.bottom_margin = 0;
    title.left_margin = 0;
    title.right_margin = 0;
    title.height = 7;
    title.width = DISPLAY_WIDTH;
    title.text = popup->title;
    widget_textbox(display, &title);

    // draws the title background
    glcd_rect_invert(display, 0, 0, DISPLAY_WIDTH, 9);

    // draws the content
    textbox_t content;
    content.color = GLCD_BLACK;
    content.mode = TEXT_MULTI_LINES;
    content.align = ALIGN_NONE_NONE;
    content.top_margin = 0;
    content.bottom_margin = 0;
    content.left_margin = 0;
    content.right_margin = 0;
    content.font = popup->font;
    content.x = popup->x + 4;
    content.y = popup->y + popup->font[FONT_HEIGHT] + 5;
    content.width = popup->width - 4;
    content.height = ((popup->font[FONT_HEIGHT]+1) * 8); // FIXME: need be relative to popup height
    content.text = popup->content;
    widget_textbox(display, &content);

    // draws the buttons
    switch (popup->type)
    {
        case OK_ONLY:
            //print the 3 buttons
            //draw the first box, back
            glcd_text(display, 26, DISPLAY_HEIGHT - 7, "OK", Terminal3x5, GLCD_BLACK);

            //draw the second box, TODO Builder MODE
            /*box_2.x = 56;
            box_2.y = DISPLAY_HEIGHT - 7;
            box_2.text = "COPY";*/
            glcd_text(display, 62, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

            //draw the third box, save PB
            /*box_3.x = 92;
            box_3.y = DISPLAY_HEIGHT - 7;
            box_3.text = "NEW";*/
            glcd_text(display, 96, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

            if (popup->button_selected == 0)
                glcd_rect_invert(display, 15, DISPLAY_HEIGHT - 8, 29, 7);
            else
                glcd_rect_invert(display, 83, DISPLAY_HEIGHT - 8, 29, 7);

        break;

        case OK_CANCEL:
            //print the 3 buttons
            //draw the first box, back
            glcd_text(display, 26, DISPLAY_HEIGHT - 7, "OK", Terminal3x5, GLCD_BLACK);

            //draw the second box, TODO Builder MODE
            /*box_2.x = 56;
            box_2.y = DISPLAY_HEIGHT - 7;
            box_2.text = "COPY";*/
            glcd_text(display, 62, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

            //draw the third box, save PB
            /*box_3.x = 92;
            box_3.y = DISPLAY_HEIGHT - 7;
            box_3.text = "NEW";*/
            glcd_text(display, 86, DISPLAY_HEIGHT - 7, "CANCEL", Terminal3x5, GLCD_BLACK);

            if (popup->button_selected == 0)
                glcd_rect_invert(display, 15, DISPLAY_HEIGHT - 8, 29, 7);
            else
                glcd_rect_invert(display, 83, DISPLAY_HEIGHT - 8, 29, 7);

        break;

        case YES_NO:

            //print the 3 buttons
            //draw the first box, back
            glcd_text(display, 24, DISPLAY_HEIGHT - 7, "YES", Terminal3x5, GLCD_BLACK);

            //draw the second box, TODO Builder MODE
            /*box_2.x = 56;
            box_2.y = DISPLAY_HEIGHT - 7;
            box_2.text = "COPY";*/
            glcd_text(display, 62, DISPLAY_HEIGHT - 7, "-", Terminal3x5, GLCD_BLACK);

            //draw the third box, save PB
            /*box_3.x = 92;
            box_3.y = DISPLAY_HEIGHT - 7;
            box_3.text = "NEW";*/
            glcd_text(display, 94, DISPLAY_HEIGHT - 7, "NO", Terminal3x5, GLCD_BLACK);

            if (popup->button_selected == 0)
                glcd_rect_invert(display, 15, DISPLAY_HEIGHT - 8, 29, 7);
            else
                glcd_rect_invert(display, 83, DISPLAY_HEIGHT - 8, 29, 7);
 
        break;

        case EMPTY_POPUP:
        //we dont have a button
        break;

        //TODO
        case CANCEL_ONLY:
        break;
    }
}

//draw function works in buffers of 8 so ofsets are not possible, thats why we draw icons ourselves

void icon_snapshot(glcd_t *display, uint8_t x, uint8_t y)
{
    // clears the icon area
    glcd_rect_fill(display, x, y, 9, 7, GLCD_WHITE);

    // draws the icon

    //outer bourders
    glcd_rect(display, x, y+1, 1, 6, GLCD_BLACK);
    glcd_rect(display, x+8, y+1, 1, 6, GLCD_BLACK);

    //top 3 
    glcd_rect(display, x+6, y+1, 3, 1, GLCD_BLACK);    
    glcd_rect(display, x, y+1, 3, 1, GLCD_BLACK);
    glcd_rect(display, x+2, y, 5, 1, GLCD_BLACK);

    //bottom
    glcd_rect(display, x, y+6, 8, 1, GLCD_BLACK);

    //dots
    glcd_rect(display, x+4, y+2, 1, 1, GLCD_BLACK);
    glcd_rect(display, x+3, y+3, 1, 1, GLCD_BLACK);
    glcd_rect(display, x+5, y+3, 1, 1, GLCD_BLACK);
    glcd_rect(display, x+4, y+4, 1, 1, GLCD_BLACK);
}

void icon_pedalboard(glcd_t *display, uint8_t x, uint8_t y)
{
    // clears the icon area
    glcd_rect_fill(display, x, y, 9, 7, GLCD_WHITE);

    // draws the icon
    
    //outer bourders    
    glcd_rect(display, x, y, 1, 7, GLCD_BLACK);
    glcd_rect(display, x+8, y, 1, 7, GLCD_BLACK);

    //vertical lines
    glcd_rect(display, x, y, 9, 1, GLCD_BLACK);
    glcd_rect(display, x, y+2, 9, 1, GLCD_BLACK);
    glcd_rect(display, x, y+4, 9, 1, GLCD_BLACK);
    glcd_rect(display, x, y+6, 9, 1, GLCD_BLACK);
}

void icon_overlay(glcd_t *display, uint8_t x, uint8_t y)
{
    // clears the icon area
    glcd_rect_fill(display, x, y, 6, 3, GLCD_WHITE);

    // draws the icon
    
    //outer bourders    
    glcd_rect(display, x, y, 6, 3, GLCD_BLACK);

    //vertical lines
    glcd_rect(display, x+1, y+1, 1, 1, GLCD_BLACK);
}

void icon_bank(glcd_t *display, uint8_t x, uint8_t y)
{
    // clears the icon area
    glcd_rect_fill(display, x, y, 9, 7, GLCD_WHITE);
    // draws the icon
    glcd_rect(display, x, y, 4, 3, GLCD_BLACK);
    glcd_rect(display, x, y + 4, 4, 3, GLCD_BLACK);
    glcd_rect(display, x + 5, y, 4, 3, GLCD_BLACK);
    glcd_rect(display, x + 5, y + 4, 4, 3, GLCD_BLACK);
}

void icon_pb_selected(glcd_t *display, uint8_t x, uint8_t y)
{
    // clears the icon area
    glcd_rect_fill(display, x, y, 5, 4, GLCD_WHITE);

    // draws the icon
    glcd_rect(display, x, y, 1, 6, GLCD_BLACK);
    glcd_rect(display, x+1, y+1, 1, 4, GLCD_BLACK);
    glcd_rect(display, x+2, y+2, 1, 2, GLCD_BLACK);
}

void icon_pb_grabbed(glcd_t *display, uint8_t x, uint8_t y, uint8_t flip)
{
    // clears the icon area
    glcd_rect_fill(display, x, y, 5, 7, GLCD_WHITE);

    // draws the icon
    if (flip){
        glcd_rect_fill(display, x+2, y, 2, 7, GLCD_BLACK);
        glcd_rect_fill(display, x, y, 4, 1, GLCD_BLACK);
        glcd_rect_fill(display, x, y+7, 4, 1, GLCD_BLACK); 
    }
    else{
        glcd_rect_fill(display, x, y, 2, 7, GLCD_BLACK);
        glcd_rect_fill(display, x, y, 4, 1, GLCD_BLACK);
        glcd_rect_fill(display, x, y+7, 4, 1, GLCD_BLACK);       
    }

}

void icon_bank_selected(glcd_t *display, uint8_t x, uint8_t y)
{
    // clears the icon area
    glcd_rect_fill(display, x, y, 5, 3, GLCD_WHITE);

    // draws the icon
    glcd_rect(display, x, y, 1, 5, GLCD_BLACK);
    glcd_rect(display, x+1, y+1, 1, 3, GLCD_BLACK);
    glcd_rect(display, x+2, y+2, 1, 1, GLCD_BLACK);
}

void icon_pb_checkbox(glcd_t *display, uint8_t x, uint8_t y, uint8_t selected)
{
    // clears the icon area
    glcd_rect_fill(display, x, y, 3, 3, GLCD_WHITE);

    // draws the icon
    if (selected) {
        glcd_rect(display, x, y, 1, 1, GLCD_BLACK);
        glcd_rect(display, x+2, y, 1, 1, GLCD_BLACK);
        glcd_rect(display, x, y+2, 1, 1, GLCD_BLACK);
        glcd_rect(display, x+2, y+2, 1, 1, GLCD_BLACK);
        glcd_rect(display, x+1, y+1, 1, 1, GLCD_BLACK);
    }
    else
        glcd_rect(display, x, y, 3, 3, GLCD_BLACK);
}

//TODO transfer to bitmask
void icon_footswitch_groups(glcd_t *display, uint8_t x, uint8_t y)
{
    // clears the icon area
    glcd_rect_fill(display, x, y-1, 12, 10, GLCD_WHITE);

    // draws the icon
    //upper part
    glcd_rect_fill(display, x, y-1, 12, 4, GLCD_BLACK);

    //arrows, left
    glcd_rect(display, x, y+3, 3, 1, GLCD_BLACK);
    glcd_rect(display, x, y+4, 2, 1, GLCD_BLACK);
    glcd_rect(display, x, y+5, 1, 1, GLCD_BLACK);

    //middle
    glcd_rect(display, x+4, y+3, 2, 1, GLCD_BLACK);
    glcd_rect(display, x+5, y+4, 2, 1, GLCD_BLACK);
    glcd_rect(display, x+6, y+5, 2, 1, GLCD_BLACK);


    //right
    glcd_rect(display, x+11, y+3, 1, 1, GLCD_BLACK);
    glcd_rect(display, x+10, y+4, 2, 1, GLCD_BLACK);
    glcd_rect(display, x+9,  y+5, 3, 1, GLCD_BLACK);

    //part under arrows
    glcd_rect_fill(display, x, y+6, 12, 3, GLCD_BLACK);

    //uncomment below to not have the arrows filled in
    //inside down
    //glcd_rect(display, x+7, y+3, 3, 1, GLCD_BLACK);
    //glcd_rect(display, x+8, y+4, 1, 1, GLCD_BLACK);
    //inside up
    //glcd_rect(display, x+3, y+4, 1, 1, GLCD_BLACK);
    //glcd_rect(display, x+2, y+5, 3, 1, GLCD_BLACK);

}

void icon_keyboard_invert(glcd_t *display, uint8_t char_index)
{
    uint8_t x = 4, y = 26;

    uint8_t row = char_index / 15;
    uint8_t colum = (char_index % 15) / 5;

    x += ((char_index % 15) * 7) + (colum * 7) + ((colum==2)?1:0);

    y += (row * 7);

    glcd_rect_invert(display, x, y, 7, 7);
}
