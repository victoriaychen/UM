/******************************************************************************
 *
 *                          instruction_executor.c
 *
 *     Assignment: profiling
 *     Authors:    Ryan Beckwith and Victoria Chen
 *     Date:       12/4/2020
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

/* Named constants for instruction execution */
#define PROG_ADDRESS 0
#define NUM_REGISTERS 8

/* Named constants for malloc */
#define SIZE_OF_UINT32 4
#define SIZE_OF_MEM_ADDRESS 4
#define SIZE_OF_SEGMENT_INFO 8

/* Enumeration for each opcode value */
typedef enum Um_opcode {
    CMOV = 0, SLOAD, SSTORE, ADD, MUL, DIV,
    NAND, HALT, ACTIVATE, INACTIVATE, OUT, IN, LOADP, LV
} Um_opcode;

typedef int Mem_Address;

typedef struct Segment_Info {
    int starting_index; 
    int width; 
} Segment_Info; 

typedef struct Mem_T {
    Segment_Info *seg_info;
    uint32_t *main_memory;
    Mem_Address *deleted_addresses;
    int length;
    int capacity;
    int seg_info_length;
    int seg_info_capacity;
    int deleted_addresses_length;
    int deleted_addresses_capacity;
} *Mem_T;

/* Mem_new
 * Purpose:    Dynamically allocates space for a new Mem_T struct instance and
 *             returns a pointer to that struct. Also initializes its members.
 * Parameters: length - the desired inital length of main memory 
 * Returns:    Mem_T - a pointer to the newly allocated Mem_T struct
 * Notes:      Leaves memory on the heap after this function terminates.
 */
static Mem_T Mem_new(int length)
{
    Mem_T mem = malloc(sizeof *mem);
    /* Ensure that malloc was successful */
    assert(mem != NULL);

    Segment_Info *segment_info = malloc(sizeof *segment_info);
    assert(segment_info != NULL);
    segment_info->starting_index = 0;
    segment_info->width = length;
    mem->seg_info = segment_info;
    mem->seg_info_length = 1;
    mem->seg_info_capacity = 1;
    
    uint32_t *main_memory = malloc(length * SIZE_OF_UINT32);
    assert(main_memory != NULL);
    mem->main_memory = main_memory;
    
    Mem_Address *deleted_addresses = NULL;
    mem->deleted_addresses = deleted_addresses;
    mem->deleted_addresses_length = 0; 
    mem->deleted_addresses_capacity = 0; 
    mem->length = length;
    mem->capacity = length;

    return mem;
}

/* Mem_free_memory
 * Purpose:    Deallocates all heap allocated memory associated with a Mem_T
 *             instance. 
 * Parameters: Mem_T *mem_p - A double pointer to a Mem_T struct instance, 
 *                            which will be freed.
 * Returns:    none
 */
static void Mem_free_memory(Mem_T *mem)
{
    Mem_T memory = *mem;
    free(memory->deleted_addresses);
    free(memory->main_memory);
    free(memory->seg_info);
    free(memory);
}

static void expand_deleted_addresses(Mem_T mem, int new_length)
{
    int deleted_addresses_capacity = mem->deleted_addresses_capacity;
    int new_capacity = deleted_addresses_capacity * 2 + 2;
    while (new_length > new_capacity) {
        new_capacity *= 2;
        new_capacity += 2;
    }
    
    Mem_Address *new_deleted_addresses = malloc(new_capacity *
                                                SIZE_OF_MEM_ADDRESS); 
    assert(new_deleted_addresses  != NULL);
    
    int deleted_addresses_length = mem->deleted_addresses_length;
    Mem_Address *deleted_addresses = mem->deleted_addresses;
    for (int i = 0; i < deleted_addresses_length; i++) {
        new_deleted_addresses[i] = deleted_addresses[i]; 
    }

    free(deleted_addresses);
    mem->deleted_addresses_capacity = new_capacity; 
    mem->deleted_addresses_length = new_length;
    mem->deleted_addresses = new_deleted_addresses;
}

static void Mem_remove_segment(Mem_T mem, Mem_Address address)
{
    int deleted_addresses_length = mem->deleted_addresses_length; 
    int deleted_addresses_capacity = mem->deleted_addresses_capacity; 
    if (deleted_addresses_length >= deleted_addresses_capacity) {
        expand_deleted_addresses(mem, deleted_addresses_length + 1); 
    } else {
        mem->deleted_addresses_length += 1;
    }
    mem->deleted_addresses[deleted_addresses_length] = address;
}

static void expand_main_memory(Mem_T mem, int new_length)
{
    int capacity = mem->capacity;
    int new_capacity = capacity * 2 + 2;
    while (new_length > new_capacity) {
        new_capacity *= 2;
        new_capacity += 2;
    }
    uint32_t *new_main_memory = malloc(new_capacity * SIZE_OF_UINT32); 
    assert(new_main_memory != NULL);
    
    int length = mem->length;
    uint32_t *main_memory = mem->main_memory;
    for (int i = 0; i < length; i++) {
        new_main_memory[i] = main_memory[i]; 
    }

    free(main_memory);
    mem->capacity = new_capacity; 
    mem->length = new_length;
    mem->main_memory = new_main_memory;
}

static void expand_seg_info(Mem_T mem, int new_length)
{
    int capacity = mem->seg_info_capacity;
    int new_capacity = capacity * 2 + 2;
    while (new_length > new_capacity) {
        new_capacity *= 2;
        new_capacity += 2;
    }
    Segment_Info *new_seg_info = malloc(new_capacity * SIZE_OF_SEGMENT_INFO);
    assert(new_seg_info != NULL);

    Segment_Info *seg_info = mem->seg_info;
    int seg_info_length = mem->seg_info_length;
    for (int i = 0; i < seg_info_length; i++) {
        new_seg_info[i] = seg_info[i]; 
    }

    free(seg_info);
    mem->seg_info = new_seg_info;
    mem->seg_info_capacity = new_capacity;
    mem->seg_info_length = new_length;
}

static Mem_Address add_to_seg_info(Mem_T mem, int starting_index,
                                   int seg_length)
{
    if (mem->seg_info_length >= mem->seg_info_capacity) {
        expand_seg_info(mem, mem->seg_info_length + 1);
    } else {
        mem->seg_info_length += 1;
    }
    Segment_Info *seg_info = mem->seg_info;
    Segment_Info *curr_seg = seg_info + (mem->seg_info_length - 1);
    curr_seg->starting_index = starting_index;
    curr_seg->width = seg_length;
    return mem->seg_info_length - 1;
}
/* Mem_create_segment
 * Purpose:    Creates a new segment with the specified length at a free index
 *             in main memory. The index of the new segment is returned to the
 *             client.
 * Parameters: Mem_T mem - an instance of Mem_T (must not be null)
 *             int length - length of the segment
 * Returns:    Mem_Address - the address of the newly instantiated segment
 */
static Mem_Address Mem_create_segment(Mem_T mem, int seg_length)
{
    int del_addresses_len = mem->deleted_addresses_length; 
    if (del_addresses_len == 0) {
        int prev_length = mem->length;
        int new_length = prev_length + seg_length;
        if (new_length > mem->capacity) {
            expand_main_memory(mem, new_length);
        } else {
            mem->length = new_length;
        }
        return add_to_seg_info(mem, prev_length, seg_length);
    } else {
        Mem_Address address = mem->deleted_addresses[del_addresses_len - 1];
        Segment_Info *seg_info = mem->seg_info;
        if (seg_info[address].width >= seg_length) {
            seg_info[address].width = seg_length;
            mem->deleted_addresses_length -= 1;
            return address;
        } else {
            int prev_length = mem->length;
            int new_length = prev_length + seg_length;
            if (new_length > mem->capacity) {
                expand_main_memory(mem, new_length);
            } else {
                mem->length = new_length; 
            }
            return add_to_seg_info(mem, prev_length, seg_length);
        }
    }
    
}

static void replace_seg_0(Mem_T mem, int length_of_seg_to_dup)
{
    Segment_Info *seg_info = mem->seg_info;
    Mem_remove_segment(mem, PROG_ADDRESS);
    if (seg_info[PROG_ADDRESS].width >= length_of_seg_to_dup) {
        seg_info[PROG_ADDRESS].width = length_of_seg_to_dup;
        mem->deleted_addresses_length -= 1;
    } else {
        int prev_length = mem->length;
        int new_length = prev_length + length_of_seg_to_dup;
        if (new_length > mem->capacity) {
            expand_main_memory(mem, new_length);
        } else {
            mem->length = new_length;
        }
        seg_info[PROG_ADDRESS].starting_index = prev_length;
        seg_info[PROG_ADDRESS].width = length_of_seg_to_dup;
    }
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
static void Mem_update_word(Mem_T mem, Mem_Address address, int index,
                            uint32_t value)
{
    int word_index = mem->seg_info[address].starting_index + index; 
    mem->main_memory[word_index] = value;
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
    int word_index = mem->seg_info[address].starting_index + index; 
    return mem->main_memory[word_index];
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
static int Mem_duplicate_segment(Mem_T mem, Mem_Address address_to_dup)
{
    
    Segment_Info *seg_info = mem->seg_info;
    Segment_Info segment_to_dup = seg_info[address_to_dup];
    int segment_to_dup_len = segment_to_dup.width;
    replace_seg_0(mem, segment_to_dup_len); 
    
    /* Populate new segment with 32-bit words from the duplicated segment */
    for (int i = 0; i < segment_to_dup_len; i++) {
        Mem_update_word(mem, PROG_ADDRESS, i,
                        Mem_get_word(mem, address_to_dup, i));
    }
    return segment_to_dup_len;
} 

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
static void map_segment(Mem_T mem, uint32_t *rB_p, uint32_t rC_val)
{
    Mem_Address address = Mem_create_segment(mem, rC_val);

    /* Initializes each value in the segment to be 0 */
    for (unsigned i = 0; i < rC_val; i++) {
        Mem_update_word(mem, address, i, 0);
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
static void load_program(Mem_T mem, uint32_t rB_val, uint32_t rC_val,
                         uint32_t *program_pointer, uint32_t *seg_0_len)
{
    if (rB_val != PROG_ADDRESS) {
        *seg_0_len = Mem_duplicate_segment(mem, rB_val);
    }
    *program_pointer = rC_val;
}

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
static void execute_instructions(Mem_T mem, uint32_t seg_0_len)
{
    uint32_t registers[NUM_REGISTERS];

    /* Initializes each register to be 0 */
    for (int i = 0; i < NUM_REGISTERS; i++) {
        registers[i] = 0;
    }
    
    uint32_t program_pointer = 0;
    uint32_t curr_instruction;
    int curr_opcode;
    
    /* Interating through segment 0 */
    while (program_pointer < seg_0_len) {
        curr_instruction = Mem_get_word(mem, PROG_ADDRESS, program_pointer);
        curr_opcode = curr_instruction >> 28;
        /* Update the program pointer before executing the instruction */
        program_pointer++;
        /* Special case for the load-value instruction */
        if (curr_opcode == LV) {
            int rA = (curr_instruction & 0xe000000) >> 25;
            int value = (curr_instruction & 0x1ffffff);
            registers[rA] = value;
        } else {
            int rA = (curr_instruction & 448) >> 6;
            int rB = (curr_instruction & 56) >> 3;
            int rC = curr_instruction & 7;
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
                    *rA_p = Mem_get_word(mem, rB_val, rC_val);
                    break;
                case SSTORE:
                    Mem_update_word(mem, rA_val, rB_val, rC_val);
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
                    Mem_free_memory(&mem);
                    exit(EXIT_SUCCESS); 
                    break;
                case ACTIVATE:
                    map_segment(mem, rB_p, rC_val);
                    break;
                case INACTIVATE:
                    Mem_remove_segment(mem, rC_val);
                    break;
                case OUT:
                    putchar(rC_val);
                    break;
                case IN:
                    get_input(rC_p);
                    break;
                case LOADP:
                    load_program(mem, rB_val, rC_val, &program_pointer,
                                 &seg_0_len);
                    break;
            }
        }
    }

    /* If the execution loop terminates, there was no halt instruction */
    fprintf(stderr, "Program terminated without a halt instruction.\n");
    Mem_free_memory(&mem);
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
static void read_instructions(Mem_T mem, char *filename, int num_words)
{
    FILE *fp = fopen(filename, "r"); 
    if (fp == NULL) {
        fprintf(stderr, "Could not open file.\n");
        exit(EXIT_FAILURE); 
    }
    uint32_t curr_word = 0;
    int curr_byte;

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
            int shift_amt = 8 * j;
            
            /* Progressively build the instruction by bitpacking */
            uint32_t shifted_byte = curr_byte << shift_amt;
            uint32_t mask = ~(255 << shift_amt);
            curr_word = curr_word & mask;
            curr_word = curr_word | shifted_byte;
        }
        /* Store each 32 bit word instruction in segment 0 */
        Mem_update_word(mem, PROG_ADDRESS, i, curr_word); 
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
    struct stat buf;
    
    /* Ensure the file size can be determined using the stat function */
    if (stat(filename, &buf) != 0) {
        fprintf(stderr, "Could not determine file size.\n");
        exit(EXIT_FAILURE);
    }

    int num_bytes = buf.st_size;

    /* Ensure that file size does not contain truncated 32-bit words */
    if (num_bytes % 4 != 0) {
        fprintf(stderr, "Improper total file size.\n");
        exit(EXIT_FAILURE);
    }
    int num_words = num_bytes / 4;

    Mem_T mem = Mem_new(num_words);
    read_instructions(mem, filename, num_words);
    execute_instructions(mem, num_words);
    Mem_free_memory(&mem);
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
