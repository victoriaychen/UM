/******************************************************************************
 *
 *                                memory.c
 *
 *     Assignment: um
 *     Authors:    Ryan Beckwith and Victoria Chen
 *     Date:       11/24/2020
 *
 *     Purpose:    Represents the implementation of the segmented main memory
 *                 structure outlined by the interface in memory.h.
 *                 Defines the Mem_T struct and its corresponding functions,
 *                 which can perform operations like word access/updates,
 *                 segment creation/deletion, and segment duplication.
 *
 *****************************************************************************/

#include <stdlib.h>
#include "memory.h"
#include "uarray.h"
#include "seq.h"
#include "assert.h"

#define SIZE_OF_UINT32 4

static void Mem_delete_segment(Mem_T mem, Mem_Address address);

/* Memory struct, details hidden from client */
// struct Mem_T {
//     Seq_T main_memory;
//     Seq_T deleted_addresses;
// };

/* Mem_new
 * Purpose:    Dynamically allocates space for a new Mem_T struct instance and
 *             returns a pointer to that struct. Also initializes its members.
 * Parameters: none
 * Returns:    Mem_T - a pointer to the newly allocated Mem_T struct
 * Notes:      Leaves memory on the heap after this function terminates.
 */
Mem_T Mem_new()
{
    Mem_T mem = malloc(sizeof(*mem));
    /* Ensure that malloc was successful */
    assert(mem != NULL);

    /* Instantiate each struct element */
    mem->main_memory = Seq_new(0);
    mem->deleted_addresses = Seq_new(0);
    return mem; 
}

/* Mem_free_memory
 * Purpose:    Deallocates all heap allocated memory associated with a Mem_T
 *             instance. 
 * Parameters: Mem_T *mem_p - A double pointer to a Mem_T struct instance, 
 *                            which will be freed. mem_p may be null, but if it
 *                            is not null, *mem_p cannot be null.
 * Returns:    none
 */
void Mem_free_memory(Mem_T *mem_p)
{
    if (mem_p == NULL) {
        return;
    }
    Mem_T mem = *mem_p;
    assert(mem != NULL);

    /* Iterate over main memory and delete all existing segments */
    int main_mem_len = Seq_length(mem->main_memory);
    for (int i = 0; i < main_mem_len; i++) {
        if (Seq_get(mem->main_memory, i) != NULL) {
            Mem_delete_segment(mem, i);
        }
    }

    /* Free remaining struct memory before freeing the struct itself */
    Seq_free(&(mem->deleted_addresses));
    Seq_free(&(mem->main_memory));
    free(mem);
}

/* Mem_create_segment
 * Purpose:    Creates a new segment with the specified length at a free index
 *             in main memory. The index of the new segment is returned to the
 *             client.
 * Parameters: Mem_T mem - an instance of Mem_T (must not be null)
 *             int length - length of the segment
 * Returns:    Mem_Address - the address of the newly instantiated segment
 */
Mem_Address Mem_create_segment(Mem_T mem, int length)
{
    // assert(mem != NULL);
    /* Use a macro instead of sizeof(uint32_t) for efficiency */
    Mem_Address address;
    Seq_T deleted_addresses = mem->deleted_addresses;
    Seq_T main_memory = mem->main_memory;
    /* In this case, add a new segment (which will expand the sequence) */
    if (Seq_length(deleted_addresses) == 0) {
        UArray_T segment = UArray_new(length, SIZE_OF_UINT32);
        address = Seq_length(main_memory);
        Seq_addhi(main_memory, segment);
    } else {
        /* In this case, use the top element of the stack as the address */
        address = (uintptr_t)Seq_remhi(deleted_addresses);
        UArray_T segment = Seq_get(main_memory, address); 
        int curr_seg_length = UArray_length(segment);
        if (curr_seg_length < length) {
            // for (int i = 0; i < curr_seg_length; i++) {
            //     *((uint32_t *)UArray_at(segment, i)) = 0;
            // }
            UArray_resize(segment, length);
        }
    }
    return address;
}

/* Mem_delete_segment
 * Purpose:    Remove a segment corresponding with a specified index 
 *             and stores its address in a Hanson sequence representing 
 *             the deleted addresses. 
 * Parameters: Mem_T mem - an instance of Mem_T (must not be null)
 *             Mem_Address address - 32-bit address corresponding with an 
 *                                   existing segment
 * Returns: none
 */
static void Mem_delete_segment(Mem_T mem, Mem_Address address)
{
    Seq_T main_memory = mem->main_memory;
    // assert(mem != NULL);
    UArray_T segment = Seq_get(main_memory, address);
    UArray_free(&segment);
    Seq_put(main_memory, address, NULL); 
    /* Add the removed index to deleted addresses */
    Seq_addhi(mem->deleted_addresses, (void *)(uintptr_t)address); 
}

void Mem_remove_segment(Mem_T mem, Mem_Address address)
{
    Seq_addhi(mem->deleted_addresses, (void *)(uintptr_t)address); 
}

/* Mem_update_word
 * Purpose:    Inserts a given 32-bit value into a segment corresponding with 
 *             a specified index. 
 * Parameters: Mem_T mem - an instance of Mem_T (must not be null)
 *             Mem_Address address - 32-bit address corresponding with an 
 *                                   existing segment
 *             int index - index of the word to update within the segment 
 *             uint32_t value - 32-bit updated value of the specified word
 * Returns:    none
 */
void Mem_update_word(Mem_T mem, Mem_Address address, int index, uint32_t value)
{
    // assert(mem != NULL);
    UArray_T segment = Seq_get(mem->main_memory, address);
    *((uint32_t *)UArray_at(segment, index)) = value;
}

/* Mem_get_word
 * Purpose:    Returns the value of the 32-bit word at the specified index
 *             within the segment specified by the given address.
 * Parameters: Mem_T mem - an instance of Mem_T (must not be null)
 *             Mem_Address address - 32-bit address corresponding with an 
 *                                   existing segment
 * Returns:    uint32_t - the 32-bit word at the specified address/index
 */
uint32_t Mem_get_word(Mem_T mem, Mem_Address address, int index)
{
    // assert(mem != NULL);
    UArray_T segment = Seq_get(mem->main_memory, address);
    return *((uint32_t *)UArray_at(segment, index));
} 

/* Mem_duplicate_segment
 * Purpose:    Duplicates the segment at the first address and stores the 
 *             duplicate at the second address. 
 * Parameters: Mem_T mem - an instance of Mem_T (must not be null)
 *             Mem_Address address_to_dup - 32-bit address corresponding with 
 *                                          an existing segment to duplicate
 *             Mem_Address address_to_replace - 32-bit address corresponding 
 *                                              with an existing segment to 
 *                                              replace
 * Returns:    int - the length of the newly duplicated segment
 */
int Mem_duplicate_segment(Mem_T mem, Mem_Address address_to_dup,
                          Mem_Address address_to_replace)
{
    // assert(mem != NULL);
    UArray_T segment_to_dup = Seq_get(mem->main_memory, address_to_dup); 
    int segment_to_dup_len = UArray_length(segment_to_dup);
    Mem_remove_segment(mem, address_to_replace);
    Mem_create_segment(mem, segment_to_dup_len);
    
    /* Populate new segment with 32-bit words from the duplicated segment */
    for (int i = 0; i < segment_to_dup_len; i++) {
        Mem_update_word(mem, address_to_replace, i,
                        Mem_get_word(mem, address_to_dup, i));
    }
    return segment_to_dup_len;
} 

UArray_T Mem_get_segment(Mem_T mem, Mem_Address address) {
   return Seq_get(mem->main_memory, address); 
}

uint32_t Mem_get_word_from_seg(UArray_T segment, int index)
{
    // assert(mem != NULL);
    return *((uint32_t *)UArray_at(segment, index));
} 