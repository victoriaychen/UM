/******************************************************************************
 *
 *                                unpacker.c
 *
 *     Assignment: um
 *     Authors:    Ryan Beckwith and Victoria Chen
 *     Date:       11/24/2020
 *
 *     Purpose:    Represents the implementation of the unpacking interface
 *                 outlined in unpacker.h. Implements functions for obtaining
 *                 the opcode of a given UM instruction, as well as separate
 *                 functions for unpacking words corresponding with the
 *                 load-value operation versus the other operations.
 *
 *****************************************************************************/

#include "unpacker.h"
#include "bitpack.h"

/* Named constants for bitpacking */
#define OPCODE_WIDTH 4
#define OPCODE_LSB 28

#define RA_WIDTH 3
#define RA_LSB 6
#define RB_WIDTH 3
#define RB_LSB 3
#define RC_WIDTH 3
#define RC_LSB 0

#define RA_13_WIDTH 3
#define RA_13_LSB 25
#define VALUE_WIDTH 25
#define VALUE_LSB 0

/* Unpacker_get_opcode
 * Purpose:    Unpacks the opcode (the most significant 4 bits) from a
 *             specified 32-bit word and returns it.
 * Parameters: uint32_t instruction - a 32-bit word corresponding with a valid
 *                                    UM instruction
 * Returns:    int - the opcode (0-13)
 */
int Unpacker_get_opcode(uint32_t instruction)
{
    return Bitpack_getu(instruction, OPCODE_WIDTH, OPCODE_LSB);
}

/* Unpacker_unpack_13
 * Purpose:    Unpacks a specified 32-bit word 
 *             (where the opcode is expected to be 13). 
 * Parameters: uint32_t instruction - a 32-bit word corresponding with a valid
 *                                    UM instruction
 * Returns:    Operation_13 - a struct containing a register index and 
 *             a value to load into that register.
 */
Operation_13 Unpacker_unpack_13(uint32_t instruction)
{
    Operation_13 op_13;
    op_13.rA = Bitpack_getu(instruction, RA_13_WIDTH, RA_13_LSB);
    op_13.value = Bitpack_getu(instruction, VALUE_WIDTH, VALUE_LSB);
    return op_13;
}

/* Unpacker_unpack
 * Purpose:    Unpacks a specified 32-bit word 
 *             (where the opcode is expected to be 0 to 12). 
 * Parameters: uint32_t instruction - a 32-bit word corresponding with a valid
 *                                    UM instruction
 * Returns:    Operation - a struct containing three register indices. 
 */
Operation Unpacker_unpack(uint32_t instruction)
{
    Operation op;
    op.rA = Bitpack_getu(instruction, RA_WIDTH, RA_LSB);
    op.rB = Bitpack_getu(instruction, RB_WIDTH, RB_LSB);
    op.rC = Bitpack_getu(instruction, RC_WIDTH, RC_LSB);
    return op;
}
