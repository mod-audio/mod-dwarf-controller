
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "data.h"
#include "config.h"
#include "utils.h"
#include "mod-protocol.h"

#include <stdlib.h>
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

static char g_back_to_bank[] = {"< Back to bank list"};

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

control_t *data_parse_control(char **data)
{
    control_t *control = NULL;
    uint32_t len = strarr_length(data);
    const uint32_t min_params = 11;

    // checks if all data was received
    if (len < min_params - 2)
        return NULL;

    control = (control_t *) MALLOC(sizeof(control_t));
    if (!control)
        goto error;

    // fills the control struct
    control->hw_id = atoi(data[1]);
    control->label = str_duplicate(data[2]);
    control->properties  = atoi(data[3]);
    control->unit = str_duplicate(data[4]);
    control->value = atof(data[5]);
    control->maximum = atof(data[6]);
    control->minimum = atof(data[7]);
    control->steps = atoi(data[8]);
    control->scale_points_count = 0;
    //pagination on by default
    control->scale_points_flag = 1;
    control->scale_point_index = 0;
    control->scale_points = NULL;

    // checks the memory allocation
    if (!control->label || !control->unit)
        goto error;

    // checks if has scale points
    uint8_t i = 0;
    if (len >= (min_params+1) && (control->properties & (FLAG_CONTROL_ENUMERATION | FLAG_CONTROL_SCALE_POINTS | FLAG_CONTROL_REVERSE)))
    {
        control->scale_points_count = atoi(data[min_params - 2]);
        if (control->scale_points_count == 0) return control;

        control->scale_points = (scale_point_t **) MALLOC(sizeof(scale_point_t*) * control->scale_points_count);
        if (!control->scale_points) goto error;

        // initializes the scale points pointers
        for (i = 0; i < control->scale_points_count; i++) control->scale_points[i] = NULL;

        for (i = 0; i < control->scale_points_count; i++)
        {
            control->scale_points[i] = (scale_point_t *) MALLOC(sizeof(scale_point_t));
            control->scale_points[i]->label = str_duplicate(data[(min_params + 1) + (i*2)]);
            control->scale_points[i]->value = atof(data[(min_params + 2) + (i*2)]);

            if (!control->scale_points[i] || !control->scale_points[i]->label) goto error;
        }

        control->scale_points_flag = atoi(data[10]);
        control->scale_point_index = atoi(data[11]);
    }

    return control;

error:
    data_free_control(control);
    return NULL;
}

void data_free_control(control_t *control)
{
    if (!control) return;

    FREE(control->label);
    FREE(control->unit);

    if (control->scale_points)
    {
        uint8_t i;
        for (i = 0; i < control->scale_points_count; i++)
        {
            if (control->scale_points[i])
            {
                FREE(control->scale_points[i]->label);
                FREE(control->scale_points[i]);
            }
        }

        FREE(control->scale_points);
    }

    FREE(control);
    return;
}

bp_list_t *data_parse_banks_list(char **list_data, uint32_t list_count)
{
    if (!list_data || list_count == 0 || (list_count % 2)) return NULL;

    list_count = (list_count / 2);

    // create an array of banks
    bp_list_t *bp_list = (bp_list_t *) MALLOC(sizeof(bp_list_t));
    if (!bp_list) goto error;

    // clear allocated memory
    memset(bp_list, 0, sizeof(bp_list_t));

    // allocate string arrays
    const size_t list_size = sizeof(char *) * (list_count + 1);

    if ((bp_list->names = (char **) MALLOC(list_size)))
        memset(bp_list->names, 0, list_size);

    if ((bp_list->uids = (char **) MALLOC(list_size)))
        memset(bp_list->uids, 0, list_size);

    // check memory allocation
    if (!bp_list->names || !bp_list->uids) goto error;

    // fill the bp_list struct
    for (uint32_t i = 0, j = 0; list_data[i] && j < list_count; i += 2, j++)
    {
        bp_list->names[j] = str_duplicate(list_data[i + 0]);
        bp_list->uids[j] = str_duplicate(list_data[i + 1]);

        // check memory allocation
        if (!bp_list->names[j] || !bp_list->uids[j]) goto error;
    }

    return bp_list;

error:
    data_free_banks_list(bp_list);
    return NULL;
}

void data_free_banks_list(bp_list_t *bp_list)
{
    if (!bp_list) return;

    uint32_t i;

    if (bp_list->names)
    {
        for (i = 0; bp_list->names[i]; i++)
            FREE(bp_list->names[i]);

        FREE(bp_list->names);
    }

    if (bp_list->uids)
    {
        for (i = 0; bp_list->uids[i]; i++)
            FREE(bp_list->uids[i]);

        FREE(bp_list->uids);
    }

    FREE(bp_list);
    return;
}

bp_list_t *data_parse_pedalboards_list(char **list_data, uint32_t list_count)
{
    if (!list_data || list_count == 0 || (list_count % 2)) return NULL;

    list_count = (list_count / 2) + 1;

    // create an array of banks
    bp_list_t *bp_list = (bp_list_t *) MALLOC(sizeof(bp_list_t));
    if (!bp_list) goto error;

    // clear allocated memory
    memset(bp_list, 0, sizeof(bp_list_t));

    // allocate string arrays
    const size_t list_size = sizeof(char *) * (list_count + 1);

    if ((bp_list->names = (char **) MALLOC(list_size)))
        memset(bp_list->names, 0, list_size);

    if ((bp_list->uids = (char **) MALLOC(list_size)))
        memset(bp_list->uids, 0, list_size);

    // check memory allocation
    if (!bp_list->names || !bp_list->uids) goto error;

    // first line is 'back to banks list'
    bp_list->names[0] = g_back_to_bank;
    bp_list->uids[0] = NULL;

    // fill the bp_list struct
    for (uint32_t i = 0, j = 1; list_data[i] && j < list_count; i += 2, j++)
    {
        bp_list->names[j] = str_duplicate(list_data[i + 0]);
        bp_list->uids[j] = str_duplicate(list_data[i + 1]);

        // check memory allocation
        if (!bp_list->names[j] || !bp_list->uids[j]) goto error;
    }

    return bp_list;

error:
    data_free_pedalboards_list(bp_list);
    return NULL;
}

void data_free_pedalboards_list(bp_list_t *bp_list)
{
    if (!bp_list) return;

    uint32_t i;

    if (bp_list->names)
    {
        for (i = 1; bp_list->names[i]; i++)
            FREE(bp_list->names[i]);

        FREE(bp_list->names);
    }

    if (bp_list->uids)
    {
        for (i = 1; bp_list->uids[i]; i++)
            FREE(bp_list->uids[i]);

        FREE(bp_list->uids);
    }

    FREE(bp_list);
    return;
}
