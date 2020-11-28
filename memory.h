/******************************************************************************
 *
 *                                memory.h
 *
 *     Assignment: um
 *     Authors:    Ryan Beckwith and Victoria Chen
 *     Date:       11/24/2020
 *
 *     Purpose:    Interface for functions representing the segmented memory
 *                 system required for the Universal Machine. Includes an
 *                 opaque pointer to the Mem_T struct, as well as functions
 *                 that can perform operations like word access/updates,
 *                 segment creation/deletion, and segment duplication.
 *
 *****************************************************************************/

#ifndef MEM_H
#define MEM_H

#include <stdint.h>

typedef struct Mem_T *Mem_T;
typedef int Mem_Address;

extern Mem_T Mem_new(); 
extern void Mem_free_memory(Mem_T *mem_p);
extern Mem_Address Mem_create_segment(Mem_T mem, int length);
extern void Mem_remove_segment(Mem_T mem, Mem_Address address);
extern void Mem_update_word(Mem_T mem, Mem_Address address, int index,
                            uint32_t value);
extern uint32_t Mem_get_word(Mem_T mem, Mem_Address address, int index); 
extern int Mem_duplicate_segment(Mem_T mem, Mem_Address address_to_dup,
                                 Mem_Address address_to_replace); 

#endif

