/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    if (buffer == NULL || entry_offset_byte_rtn == NULL)
        return NULL;

    uint8_t index = buffer->out_offs;
    size_t total_bytes_seen = 0;

    while (1)
    {
        // If buffer is NOT full and we reached in_offs, it means there are no more valid entries
        if (buffer->full == false && index == buffer->in_offs)
        {
            return NULL;  
        }

        size_t current_entry_size = buffer->entry[index].size;

        // Check if char_offset falls inside this entry
        if (char_offset < total_bytes_seen + current_entry_size)
        {
            // Calculate byte offset inside this entry
            *entry_offset_byte_rtn = char_offset - total_bytes_seen;
            return &buffer->entry[index];
        }

        total_bytes_seen += current_entry_size;

        index = index + 1;
        if (index >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        {
            index = 0;
        }

        // If buffer is full and we came back to out_offs, we have checked all entries
        if (buffer->full == true && index == buffer->out_offs)
        {
            return NULL;  
        }
    }
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
	if (buffer == NULL || add_entry == NULL)
	return;

    // If buffer is full, overwrite oldest entry
    if (buffer->full) {
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }

    // Insert new entry at in_offs
    buffer->entry[buffer->in_offs] = *add_entry;

    // Move in_offs forward
    buffer->in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    // If in_offs catches up to out_offs → buffer is full
    if (buffer->in_offs == buffer->out_offs) {
        buffer->full = true;
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
