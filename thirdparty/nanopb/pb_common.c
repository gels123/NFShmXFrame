/* pb_common.c: Common support functions for pb_encode.c and pb_decode.c.
 *
 * 2014 Petteri Aimonen <jpa@kapsi.fi>
 */

#include "pb_common.h"

nanopb_frompb_log_handle* g_nanopb_frompb_log_handle = NULL;

bool pb_field_iter_begin(pb_field_iter_t *iter, const pb_field_t *fields, void *dest_struct)
{
    iter->start = fields;
    iter->pos = fields;
    iter->required_field_index = 0;
    iter->dest_struct = dest_struct;
    iter->pData = (char*)dest_struct + iter->pos->data_offset;
    iter->pSize = (char*)iter->pData + iter->pos->size_offset;
    
    return (iter->pos->tag != 0);
}

bool pb_field_iter_next(pb_field_iter_t *iter)
{
    const pb_field_t *prev_field = iter->pos;

    if (prev_field->tag == 0)
    {
        /* Handle empty message types, where the first field is already the terminator.
         * In other cases, the iter->pos never points to the terminator. */
        return false;
    }
    
    iter->pos++;
    
    if (iter->pos->tag == 0)
    {
        /* Wrapped back to beginning, reinitialize */
        (void)pb_field_iter_begin(iter, iter->start, iter->dest_struct);
        return false;
    }
    else
    {
        /* Increment the pointers based on previous field size */
        size_t prev_size = prev_field->data_size;
    
        if (PB_HTYPE(prev_field->type) == PB_HTYPE_ONEOF &&
            PB_HTYPE(iter->pos->type) == PB_HTYPE_ONEOF &&
            iter->pos->data_offset == PB_SIZE_MAX)
        {
            /* Don't advance pointers inside unions */
            return true;
        }
        else if (PB_ATYPE(prev_field->type) == PB_ATYPE_STATIC &&
                 PB_HTYPE(prev_field->type) == PB_HTYPE_REPEATED)
        {
            /* In static arrays, the data_size tells the size of a single entry and
             * array_size is the number of entries */
            prev_size *= prev_field->array_size;
        }
        else if (PB_ATYPE(prev_field->type) == PB_ATYPE_POINTER)
        {
            /* Pointer fields always have a constant size in the main structure.
             * The data_size only applies to the dynamically allocated area. */
            prev_size = sizeof(void*);
        }

        if (PB_HTYPE(prev_field->type) == PB_HTYPE_REQUIRED)
        {
            /* Count the required fields, in order to check their presence in the
             * decoder. */
            iter->required_field_index++;
        }
    
        iter->pData = (char*)iter->pData + prev_size + iter->pos->data_offset;
        iter->pSize = (char*)iter->pData + iter->pos->size_offset;
        return true;
    }
}

bool pb_field_iter_find(pb_field_iter_t *iter, uint32_t tag)
{
    const pb_field_t *start = iter->pos;
    
    do {
        if (iter->pos->tag == tag &&
            PB_LTYPE(iter->pos->type) != PB_LTYPE_EXTENSION)
        {
            /* Found the wanted field */
            return true;
        }
        
        (void)pb_field_iter_next(iter);
    } while (iter->pos != start);
    
    /* Searched all the way back to start, and found nothing. */
    return false;
}

char const* const pb_ltype_enum_to_str(uint32_t ltype)
{
    switch (ltype)
    {
        case PB_LTYPE_VARINT: return "PB_LTYPE_VARINT";
        case PB_LTYPE_UVARINT: return "PB_LTYPE_UVARINT";
        case PB_LTYPE_SVARINT: return "PB_LTYPE_SVARINT";
        case PB_LTYPE_FIXED32: return "PB_LTYPE_FIXED32";
        case PB_LTYPE_FIXED64: return "PB_LTYPE_FIXED64";
        case PB_LTYPE_BYTES: return "PB_LTYPE_BYTES";
        case PB_LTYPE_STRING: return "PB_LTYPE_STRING";
        case PB_LTYPE_SUBMESSAGE: return "PB_LTYPE_SUBMESSAGE";
        case PB_LTYPE_EXTENSION: return "PB_LTYPE_EXTENSION";
        case PB_LTYPE_FIXED_LENGTH_BYTES: return "PB_LTYPE_FIXED_LENGTH_BYTES";
        default : return "unknown";
    }
}

char const* const pb_htype_enum_to_str(uint32_t htype)
{
    switch (htype)
    {
        case PB_HTYPE_REQUIRED: return "PB_HTYPE_REQUIRED";
        case PB_HTYPE_OPTIONAL: return "PB_HTYPE_OPTIONAL";
        case PB_HTYPE_REPEATED: return "PB_HTYPE_REPEATED";
        case PB_HTYPE_ONEOF: return "PB_HTYPE_ONEOF";
        default : return "unknown";
    }
}


char const* const pb_wtype_enum_to_str(uint32_t wtype)
{
    switch (wtype)
    {
        case PB_WT_VARINT: return "PB_WT_VARINT";
        case PB_WT_64BIT: return "PB_WT_64BIT";
        case PB_WT_STRING: return "PB_WT_STRING";
        case PB_WT_32BIT: return "PB_WT_32BIT";
        default : return "unknown";
    }
}