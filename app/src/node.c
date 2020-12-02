
/*
************************************************************************************************************************
*           INCLUDE FILES
************************************************************************************************************************
*/

#include "node.h"


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

node_t *node_create(void *data)
{
    node_t *self = (node_t *) MALLOC(sizeof(node_t));

    if (self)
    {
        self->data = data;
        self->parent = 0;
        self->first_child = 0;
        self->last_child = 0;
        self->next = 0;
        self->prev = 0;
    }

    return self;
}


node_t *node_child(node_t *parent, void *data)
{
    node_t *self = node_create(data);

    // has parent
    if (parent && self)
    {
        // first child
        if (!parent->first_child) parent->first_child = self;

        // already has child
        if (parent->last_child) parent->last_child->next = self;

        // store the parent and sibling
        self->parent = parent;
        self->prev = parent->last_child;

        // now this is the last child
        parent->last_child = self;
    }

    return self;
}


node_t *node_cut(node_t *node)
{
    if (!node) return 0;

    // if has previous...
    if (node->prev)
    {
        // ... and doesn't has next, this is the last node
        if (!node->next)
        {
            if (node->parent) node->parent->last_child = node->prev;
            node->prev->next = 0;
        }

        // ... and has next, this is a intermediary node
        else
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
    }

    // if reach this point and has next this is the first node
    else if (node->next)
    {
        if (node->parent) node->parent->first_child = node->next;
        node->next->prev = 0;
    }

    // if doesn't has siblings but has parent...
    else if (node->parent)
    {
        node->parent->first_child = 0;
        node->parent->last_child = 0;
    }

    // No parent, no siblings :(
    node->parent = 0;
    node->prev = 0;
    node->next = 0;

    return node;
}


void node_join(node_t *node1, node_t *node2)
{
    // to keeps the nodes connected
    if (node1->next) node2->next = node1->next;

    // insert the node on sequence
    node1->next = node2;
    node2->prev = node1;

    // if don't has a parent adopts it
    if (!node2->parent && node1->parent)
    {
        node2->parent = node1->parent;

        if (node1->parent->last_child == node1) node1->parent->last_child = node2;
    }
}


void node_destroy(node_t *node)
{
    if (node)
    {
        node = node_cut(node);
        while (node->first_child) node_destroy(node->first_child);
        FREE(node);
    }
}
