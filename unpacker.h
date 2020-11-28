/******************************************************************************
 *
 *                                unpacker.h
 *
 *     Assignment: um
 *     Authors:    Ryan Beckwith and Victoria Chen
 *     Date:       11/24/2020
 *
 *     Purpose:    Interface for functions that extract necessary information 
 *                 from a 32-bit UM instruction. Includes functions for
 *                 obtaining the opcode of a given UM instruction, as well as
 *                 separate functions for unpacking words corresponding with
 *                 the load-value operation versus the other operations.
 *
 *****************************************************************************/

#ifndef UNPACKER_H
#define UNPACKER_H

#include <stdint.h>

/* 
 * Struct definitions are included here so clients know the names of relevant
 * data members in each struct.
 */
typedef struct Operation { 
    int rA;
    int rB;
    int rC;
} Operation;

typedef struct Operation_13 { 
    int rA;
    uint32_t value;
} Operation_13;

extern int Unpacker_get_opcode(uint32_t instruction); 
extern Operation_13 Unpacker_unpack_13(uint32_t instruction);
extern Operation Unpacker_unpack(uint32_t instruction);

#endif