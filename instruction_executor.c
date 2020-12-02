/******************************************************************************
 *
 *                          instruction_executor.c
 *
 *     Assignment: um
 *     Authors:    Ryan Beckwith and Victoria Chen
 *     Date:       11/24/2020
 *
 *     Purpose:    Implements the Universal Machine, which can execute
 *                 corresponding binary .um files. Responsible for reading UM
 *                 instructions from a specified input file, which are executed
 *                 sequentially. Possible operations include conditional
 *                 loading of register values, interactions with the segmented
 *                 main memory interface, traditional arithmetic, the
 *                 bitwise NAND operation, input/output from the standard I/O
 *                 streams, loading a new program to replace the current
 *                 program, and loading values into registers directly.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include "assert.h"
#include "bitpack.h"
#include "seq.h"
// #include "memory.h"
//#include "unpacker.h"

/* Named constants for instruction execution */
#define PROG_ADDRESS 0
#define NUM_REGISTERS 8

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

#define SIZE_OF_UINT32 4

/* Enumeration for each opcode value */
typedef enum Um_opcode {
    CMOV = 0, SLOAD, SSTORE, ADD, MUL, DIV,
    NAND, HALT, ACTIVATE, INACTIVATE, OUT, IN, LOADP, LV
} Um_opcode;

typedef struct Mem_T {
    Seq_T main_memory;
    Seq_T deleted_addresses;
} *Mem_T;

typedef int Mem_Address;

/* #define MEM_UPDATE_WORD(main_mem, address, index, value) \
do { \
    UArray_T segment = Seq_get(main_mem->main_memory, address); \
    *((uint32_t *)UArray_at(segment, index)) = value; \
} while (0) */

/*****************************************************************************/

typedef struct SArray_T {
    uint32_t *data;
    int length;
} *SArray_T;

static SArray_T SArray_new(int length)
{
    uint32_t *data = malloc(length * SIZE_OF_UINT32);
    assert(data != NULL);
    SArray_T sarr = malloc(sizeof *sarr);
    assert(sarr != NULL);
    sarr->data = data;
    sarr->length = length;
    return sarr; 
}

static uint32_t *SArray_at(SArray_T sarr, int index)
{
    return (sarr->data) + index; 
}

static uint32_t SArray_get(SArray_T sarr, int index)
{
    return sarr->data[index];
}

// new_length must be greater than length
static void SArray_expand(SArray_T sarr, int new_length)
{
    int length = sarr->length;
    uint32_t *data = sarr->data;
    uint32_t *new_data = malloc(new_length * SIZE_OF_UINT32);
    assert(new_data != NULL);
    for (int i = 0; i < length; i++) {
        new_data[i] = data[i];
    }
    free(data);
    sarr->data = new_data;
    sarr->length = new_length;
}

static void SArray_free(SArray_T *sarr)
{
    free((*sarr)->data);
    free(*sarr); 
}
/*****************************************************************************/

// static void Mem_delete_segment(Mem_T mem, Mem_Address address);

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
static Mem_T Mem_new()
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
static void Mem_free_memory(Mem_T *mem_p)
{
    if (mem_p == NULL) {
        return;
    }
    Mem_T mem = *mem_p;
    assert(mem != NULL);

    Seq_T main_memory = mem->main_memory;
    Seq_T deleted_addresses = mem->deleted_addresses;
    /* Iterate over main memory and delete all existing segments */
    int main_mem_len = Seq_length(main_memory);
    for (int i = 0; i < main_mem_len; i++) {
        //if (Seq_get(main_memory, i) != NULL) {
            // Mem_delete_segment(mem, i);
        SArray_T segment = Seq_get(main_memory, i);
        SArray_free(&segment);
        //}
    }

    /* Free remaining struct memory before freeing the struct itself */
    Seq_free(&deleted_addresses);
    Seq_free(&main_memory);
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
static Mem_Address Mem_create_segment(Mem_T mem, int length)
{
    // assert(mem != NULL);
    /* Use a macro instead of sizeof(uint32_t) for efficiency */
    Mem_Address address;
    Seq_T deleted_addresses = mem->deleted_addresses;
    Seq_T main_memory = mem->main_memory;
    /* In this case, add a new segment (which will expand the sequence) */
    if (Seq_length(deleted_addresses) == 0) {
        SArray_T segment = SArray_new(length);
        address = Seq_length(main_memory);
        Seq_addhi(main_memory, segment);
    } else {
        /* In this case, use the top element of the stack as the address */
        address = (uintptr_t)Seq_remhi(deleted_addresses);
        SArray_T segment = Seq_get(main_memory, address); 
        int curr_seg_length = segment->length;
        if (curr_seg_length < length) {
            // for (int i = 0; i < curr_seg_length; i++) {
            //     *((uint32_t *)UArray_at(segment, i)) = 0;
            // }
            SArray_expand(segment, length);
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
// static void Mem_delete_segment(Mem_T mem, Mem_Address address)
// {
//     Seq_T main_memory = mem->main_memory;
//     // assert(mem != NULL);
//     SArray_T segment = Seq_get(main_memory, address);
//     SArray_free(&segment);
//     // Seq_put(main_memory, address, NULL); 
//     /* Add the removed index to deleted addresses */
//     // Seq_addhi(mem->deleted_addresses, (void *)(uintptr_t)address);
// }

static void Mem_remove_segment(Mem_T mem, Mem_Address address)
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
static void Mem_update_word(Mem_T mem, Mem_Address address, int index, uint32_t value)
{
    // assert(mem != NULL);
    SArray_T segment = Seq_get(mem->main_memory, address);
    *SArray_at(segment, index) = value;
}

/* Mem_get_word
 * Purpose:    Returns the value of the 32-bit word at the specified index
 *             within the segment specified by the given address.
 * Parameters: Mem_T mem - an instance of Mem_T (must not be null)
 *             Mem_Address address - 32-bit address corresponding with an 
 *                                   existing segment
 * Returns:    uint32_t - the 32-bit word at the specified address/index
 */
static uint32_t Mem_get_word(Mem_T mem, Mem_Address address, int index)
{
    // assert(mem != NULL);
    SArray_T segment = Seq_get(mem->main_memory, address);
    return SArray_get(segment, index);
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
static int Mem_duplicate_segment(Mem_T mem, Mem_Address address_to_dup,
                          Mem_Address address_to_replace)
{
    // assert(mem != NULL);
    SArray_T segment_to_dup = Seq_get(mem->main_memory, address_to_dup); 
    int segment_to_dup_len = segment_to_dup->length;
    Mem_remove_segment(mem, address_to_replace);
    Mem_create_segment(mem, segment_to_dup_len);
    
    /* Populate new segment with 32-bit words from the duplicated segment */
    for (int i = 0; i < segment_to_dup_len; i++) {
        Mem_update_word(mem, address_to_replace, i,
                        Mem_get_word(mem, address_to_dup, i));
    }
    return segment_to_dup_len;
} 

static SArray_T Mem_get_segment(Mem_T mem, Mem_Address address) {
   return Seq_get(mem->main_memory, address); 
}

// static uint32_t Mem_get_word_from_seg(SArray_T segment, int index)
// {
//     // assert(mem != NULL);
//     return SArray_get(segment, index);
// } 

/*****************************************************************************/

/* conditional_move
 * Purpose:    Perform the conditional move operation. The value in register C
 *             determines if the other registers get updated. 
 * Parameters: uint32_t *rA_p - a pointer to the value stored in register A 
 *             uint32_t rB_val - the value stored in register B
 *             uint32_t rC_val - the value stored in register C 
 * Returns:    none
 */
static inline void conditional_move(uint32_t *rA_p, uint32_t rB_val,
                                    uint32_t rC_val)
{
    if (rC_val != 0) {
        *rA_p = rB_val;
    }
}

/* map_segment
 * Purpose:    Perform the map segment operation. Create a new segment and 
 *             initializes each value in the segment to be 0. 
 * Parameters: Mem_T main_mem - an instance of Mem_T (must not be NULL)
 *             uint32_t *rB_p - a pointer to the value stored in register B
 *             uint32_t rC_val - the value stored in register C
 * Returns:    none
 */
static void map_segment(Mem_T main_mem, uint32_t *rB_p, uint32_t rC_val)
{
    Mem_Address address = Mem_create_segment(main_mem, rC_val);
    
    /* Initializes each value in the segment to be 0 */
    SArray_T segment = Seq_get(main_mem->main_memory, address); 
    for (unsigned i = 0; i < rC_val; i++) {
        // Mem_update_word(main_mem, address, i, 0);
        *SArray_at(segment, i) = 0;
    }
    *rB_p = address;
}

/* get_input
 * Purpose:    Performs the input operation. Reads in 1 byte at a time 
 *             and stores the value in register C. 
 * Parameters: uint32_t  *rC_p - a pointer to the value stored in register C
 * Returns:    none
 */
static void get_input(uint32_t *rC_p) {
    int byte = getchar();

    /* If the end of input have been signal, then register C is loaded with 
       a 32-bit word where every bit is 1 */
    if (byte == EOF) {
        *rC_p = ~0; 
        return;
    }
    *rC_p = byte;
}

/* load_program
 * Purpose:    Perform the load program operation. Duplicates a segment of the 
 *             address that stores in register B and replaces segment 0 with 
 *             the duplicated segment. 
 * Parameters: Mem_T main_mem - an instance of Mem_T (must not be NULL)
 *             uint32_t *rB_p - a pointer to the value store in register B
 *             uint32_t rC_val - the value stored in register C
 *             uint32_t *program_pointer - a pointer to the value of the 
 *                                         program pointer
 *             uint32_t *seg_0_len - a pointer to the length of segment 0
 * Returns:    none
 */
static void load_program(Mem_T main_mem, uint32_t *rB_p, uint32_t rC_val,
                         uint32_t *program_pointer, uint32_t *seg_0_len,
                         SArray_T *seg_0_ptr)
{
    if (*rB_p != PROG_ADDRESS) {
        *seg_0_len = Mem_duplicate_segment(main_mem, *rB_p, PROG_ADDRESS);
        *seg_0_ptr = Mem_get_segment(main_mem, PROG_ADDRESS);
        // *curr_segment = *seg_0_ptr;
        // *curr_segment_address = PROG_ADDRESS;
    }
    *program_pointer = rC_val;
}

/* execute_cases
 * Purpose:    Executes the correct instruction based on the specified opcode
 *             value, which must be between 0 and 12. If the opcode value is
 *             not within the range of valid opcodes, no instruction is
 *             executed. This function does not handle opcode value 13.
 * Parameters: Mem_T main_mem - an instance of Mem_T (must not be NULL)
 *             int opcode - a value between 0 and 12 that corresponds directly
 *                          with the intended instruction to execute
 *             uint32_t *rA_p - a pointer to the value stored in register A
 *             uint32_t *rB_p - a pointer to the value stored in register B
 *             uint32_t *rC_p - a pointer to the value stored in register C
 *             uint32_t *program_pointer - a pointer to the value of the 
 *                                         program pointer
 *             uint32_t *seg_0_len - a pointer to the length of segment 0
 * Returns:    none
 */
// static void execute_cases(Mem_T main_mem, int opcode, uint32_t *rA_p,
//                           uint32_t *rB_p, uint32_t *rC_p,
//                           uint32_t *program_pointer, uint32_t *seg_0_len,
//                           UArray_T *seg_0_ptr)
// {
//     UArray_T segment;
//     switch (opcode) {
//         case CMOV:
//             conditional_move(rA_p, *rB_p, *rC_p);
//             break;
//         case SLOAD:
//             //*rA_p = Mem_get_word(main_mem, *rB_p, *rC_p);
//             segment = Seq_get(main_mem->main_memory, *rB_p);
//             *rA_p = *((uint32_t *)UArray_at(segment, *rC_p));
//             break;
//         case SSTORE:
//             //Mem_update_word(main_mem, *rA_p, *rB_p, *rC_p);
//             segment = Seq_get(main_mem->main_memory, *rA_p);
//             *((uint32_t *)UArray_at(segment, *rB_p)) = *rC_p;
//             // MEM_UPDATE_WORD(main_mem, *rA_p, *rB_p, *rC_p);
//             break;
//         case ADD:
//             *rA_p = *rB_p + *rC_p;
//             break;
//         case MUL:
//             *rA_p = *rB_p * *rC_p;
//             break;
//         case DIV:
//             *rA_p = *rB_p / *rC_p;
//             break;
//         case NAND:
//             *rA_p = ~(*rB_p & *rC_p);
//             break;
//         case HALT:
//             Mem_free_memory(&main_mem); 
//             exit(EXIT_SUCCESS); 
//             break;
//         case ACTIVATE:
//             map_segment(main_mem, rB_p, *rC_p); 
//             // *rB_p = Mem_create_segment(main_mem, *rC_p);
//             break;
//         case INACTIVATE:
//             Mem_remove_segment(main_mem, *rC_p);
//             break;
//         case OUT:
//             putchar(*rC_p);
//             break;
//         case IN:
//             get_input(rC_p);
//             break;
//         case LOADP:
//             load_program(main_mem, rB_p, *rC_p, program_pointer, seg_0_len, seg_0_ptr);
//             break;
//     }
// }

/* execute_instructions
 * Purpose:    Executes instructions loaded into the first segment of main
 *             memory, which is passed as a parameter to this function.
 *             Declares and initializes the registers and program pointer
 *             necessary to run any UM program.
 * Parameters: Mem_T main_mem - an instance of Mem_T (must not be NULL)
 *             uint32_t seg_0_len - the number of instructions stored in
 *                                  segment 0 of main memory
 * Returns:    none
 */
static void execute_instructions(Mem_T main_mem, uint32_t seg_0_len)
{
    assert(main_mem != NULL);
    uint32_t registers[NUM_REGISTERS];

    /* Initializes each register to be 0 */
    for (int i = 0; i < NUM_REGISTERS; i++) {
        registers[i] = 0;
    }
    
    uint32_t program_pointer = 0;
    uint32_t curr_instruction;
    int curr_opcode;
    SArray_T seg_0_ptr = Mem_get_segment(main_mem, PROG_ADDRESS);
    // uint32_t curr_segment_address = 0;
    SArray_T curr_segment = seg_0_ptr;
    Seq_T main_memory = main_mem->main_memory;

    // int num_mem_ops = 0;
    // int num_mem_cache_hits = 0;

    /* Interating through segment 0 */
    while (program_pointer < seg_0_len) {
        // curr_instruction = Mem_get_word(main_mem, PROG_ADDRESS,
        //                                 program_pointer);
        // curr_instruction = Mem_get_word_from_seg(seg_0_ptr, program_pointer);
        curr_instruction = SArray_get(seg_0_ptr, program_pointer);
        // curr_opcode = Unpacker_get_opcode(curr_instruction);
        // curr_opcode = Bitpack_getu(curr_instruction, OPCODE_WIDTH, OPCODE_LSB);
        curr_opcode = curr_instruction >> 28;
        /* Update the program pointer before executing the instruction */
        program_pointer++;
        /* Special case for the load-value instruction */
        if (curr_opcode == LV) {
            // Operation_13 op_13 = Unpacker_unpack_13(curr_instruction);
            // int rA = Bitpack_getu(curr_instruction, RA_13_WIDTH, RA_13_LSB);
            int rA = (curr_instruction & 0xe000000) >> 25;
            // printf("rA: %d\n", rA);
            // int value = Bitpack_getu(curr_instruction, VALUE_WIDTH, VALUE_LSB);
            int value = (curr_instruction & 0x1ffffff);
            // printf("value: %d\n", value);
            registers[rA] = value;
        } else {
            //Operation op = Unpacker_unpack(curr_instruction);
            // int rA = Bitpack_getu(curr_instruction, RA_WIDTH, RA_LSB);
            int rA = (curr_instruction & 448) >> 6;
            // int rB = Bitpack_getu(curr_instruction, RB_WIDTH, RB_LSB);
            int rB = (curr_instruction & 56) >> 3;
            // int rC = Bitpack_getu(curr_instruction, RC_WIDTH, RC_LSB);
            int rC = curr_instruction & 7;
            // execute_cases(main_mem, curr_opcode, registers + rA,
            //               registers + rB, registers + rC,
            //               &program_pointer, &seg_0_len, &seg_0_ptr);
            uint32_t *rA_p = registers + rA;
            uint32_t *rB_p = registers + rB;
            uint32_t *rC_p = registers + rC;
            uint32_t rA_val = registers[rA];
            uint32_t rB_val = registers[rB];
            uint32_t rC_val = registers[rC];

            switch (curr_opcode) {
                case CMOV:
                    conditional_move(rA_p, rB_val, rC_val);
                    break;
                case SLOAD:
                    //*rA_p = Mem_get_word(main_mem, *rB_p, *rC_p);
                    // num_mem_ops++;
                    // if (curr_segment_address != rB_val) { 
                    curr_segment = Seq_get(main_memory, rB_val);
                        // curr_segment_address = rB_val;
                    // } else {
                    //     num_mem_cache_hits++;
                    // }
                    *rA_p = SArray_get(curr_segment, rC_val);
                    break;
                case SSTORE:
                    // num_mem_ops++;
                    //Mem_update_word(main_mem, *rA_p, *rB_p, *rC_p);
                    //if (curr_segment_address != rA_val) { 
                    curr_segment = Seq_get(main_memory, rA_val);
                        // curr_segment_address = rA_val;
                    // } else {
                    //     num_mem_cache_hits++; 
                    // }
                    // curr_segment = Seq_get(main_mem->main_memory, rA_val);
                    *SArray_at(curr_segment, rB_val) = rC_val;
                    // MEM_UPDATE_WORD(main_mem, *rA_p, *rB_p, *rC_p);
                    break;
                case ADD:
                    *rA_p = rB_val + rC_val;
                    break;
                case MUL:
                    *rA_p = rB_val * rC_val;
                    break;
                case DIV:
                    *rA_p = rB_val / rC_val;
                    break;
                case NAND:
                    *rA_p = ~(rB_val & rC_val);
                    break;
                case HALT:
                    // printf("\nMemory operations: %d\n", num_mem_ops);
                    // printf("Memory operation cache hits: %d\n", num_mem_cache_hits);
                    Mem_free_memory(&main_mem); 
                    exit(EXIT_SUCCESS); 
                    break;
                case ACTIVATE:
                    map_segment(main_mem, rB_p, rC_val); 
                    // *rB_p = Mem_create_segment(main_mem, *rC_p);
                    break;
                case INACTIVATE:
                    Mem_remove_segment(main_mem, rC_val);
                    break;
                case OUT:
                    putchar(rC_val);
                    break;
                case IN:
                    get_input(rC_p);
                    break;
                case LOADP:
                    load_program(main_mem, rB_p, rC_val, &program_pointer,
                                 &seg_0_len, &seg_0_ptr);
                    break;
            }
        }
    }

    /* If the execution loop terminates, there was no halt instruction */
    fprintf(stderr, "Program terminated without a halt instruction.\n");
    Mem_free_memory(&main_mem);
    exit(EXIT_FAILURE);
}

/* read_instructions
 * Purpose:    Opens the given file, retrieves information from the file to
 *             bitpack 32-bit word instructions, and stores all 32-bit word
 *             instructions in segment 0 of main memory. 
 * Parameters: Mem_T main_mem - an instance of Mem_T (must not be NULL)
 *             char *filename - a string representing the name of the file to
 *                              process instructions from
 *             int num_words - the number of instructions in the specified file
 * Returns:    none
 */
static void read_instructions(Mem_T main_mem, char *filename, int num_words)
{
    assert(main_mem != NULL && filename != NULL);
    FILE *fp = fopen(filename, "r"); 
    if (fp == NULL) {
        fprintf(stderr, "Could not open file.\n");
        exit(EXIT_FAILURE); 
    }
    uint32_t curr_word = 0;
    int curr_byte;
    SArray_T segment_0 = Seq_get(main_mem->main_memory, PROG_ADDRESS);

    /* Read through the file 4 bytes at a time */
    for (int i = 0; i < num_words; i++) {
        for (int j = 3; j >= 0; j--) {
            curr_byte = getc(fp);
            /* File is improper if EOF is encountered prematurely */
            if (curr_byte == EOF) {
                fprintf(stderr, "Could not read contents of file.\n");
                fclose(fp);
                exit(EXIT_FAILURE);
            }
            /* Progressively build the instruction with Bitpack_newu */
            curr_word = Bitpack_newu(curr_word, 8, 8 * j, curr_byte);
        }
        /* Store each 32 bit word instruction in segment 0 */
        // Mem_update_word(main_mem, PROG_ADDRESS, i, curr_word);
        // MEM_UPDATE_WORD(main_mem, PROG_ADDRESS, i, curr_word);
        //segment = Seq_get(main_mem->main_memory, *rA_p);
        *(SArray_at(segment_0, i)) = curr_word;
    }
    fclose(fp);
}

/* run_program
 * Purpose:    Initializes the UM by creating main memory, parsing instructions
 *             from the specified input file, and executing said instructions.
 * Parameters: char *filename - a string representing the name of the file to
 *                              process instructions from
 * Returns:    none
 */
static void run_program(char *filename) 
{
    assert(filename != NULL);
    Mem_T main_mem = Mem_new();
    struct stat buf;
    
    /* Ensure the file size can be determined using the stat function */
    if (stat(filename, &buf) != 0) {
        fprintf(stderr, "Could not determine file size.\n");
        Mem_free_memory(&main_mem);
        exit(EXIT_FAILURE);
    }

    int num_bytes = buf.st_size;

    /* Ensure that file size does not contain truncated 32-bit words */
    if (num_bytes % 4 != 0) {
        fprintf(stderr, "Improper total file size.\n");
        Mem_free_memory(&main_mem);
        exit(EXIT_FAILURE);
    }

    /* Create segment 0, then load/execute instructions */
    Mem_create_segment(main_mem, num_bytes / 4); 
    read_instructions(main_mem, filename, num_bytes / 4);
    execute_instructions(main_mem, num_bytes / 4);
    Mem_free_memory(&main_mem); 
}

/* main
 * Purpose:    Main function for the UM. Parses command-line arguments and
 *             calls appropriate functions to run the UM.
 * Parameters: int argc - number of command-line arguments
 *             char *argv[] - array of strings representing the command-line
 *                            arguments, where the first argument should be the
 *                            executable name and the second should be the name
 *                            of the .um file to execute
 * Returns:    int - the status code for the UM program
 */
int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Improper number of arguments.\n");
        exit(EXIT_FAILURE); 
    }
    run_program(argv[1]);
    return EXIT_SUCCESS;
}