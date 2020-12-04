/*
 * umlab.c
 *
 * Functions to generate UM unit tests. Once complete, this module
 * should be augmented and then linked against umlabwrite.c to produce
 * a unit test writing program.
 *  
 * A unit test is a stream of UM instructions, represented as a Hanson
 * Seq_T of 32-bit words adhering to the UM's instruction format.  
 * 
 * Any additional functions and unit tests written for the lab go
 * here. 
 *  
 */


#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <seq.h>
#include <bitpack.h>


typedef uint32_t Um_instruction;
typedef enum Um_opcode {
        CMOV = 0, SLOAD, SSTORE, ADD, MUL, DIV,
        NAND, HALT, ACTIVATE, INACTIVATE, OUT, IN, LOADP, LV
} Um_opcode;


/* Functions that return the two instruction types */

Um_instruction three_register(Um_opcode op, int ra, int rb, int rc)
{
        Um_instruction word = 0;
        word = Bitpack_newu(word, 4, 28, op);
        word = Bitpack_newu(word, 3, 6, ra); 
        word = Bitpack_newu(word, 3, 3, rb);  
        word = Bitpack_newu(word, 3, 0, rc);  
        return word; 
}

Um_instruction loadval(unsigned ra, unsigned val)
{
        Um_instruction word = 0;
        word = Bitpack_newu(word, 4, 28, LV);
        word = Bitpack_newu(word, 3, 25, ra);
        word = Bitpack_newu(word, 25, 0, val);
        return word;
}


/* Wrapper functions for each of the instructions */

static inline Um_instruction halt(void) 
{
        return three_register(HALT, 0, 0, 0);
}

typedef enum Um_register { r0 = 0, r1, r2, r3, r4, r5, r6, r7 } Um_register;

static inline Um_instruction add(Um_register a, Um_register b, Um_register c) 
{
        return three_register(ADD, a, b, c);
}

Um_instruction output(Um_register c)
{
        return three_register(OUT, 0, 0, c);
}

static inline Um_instruction conditional_move(Um_register a, Um_register b,
                                              Um_register c) 
{
        return three_register(CMOV, a, b, c);
}

static inline Um_instruction segmented_load(Um_register a, Um_register b,
                                            Um_register c) 
{
        return three_register(SLOAD, a, b, c);
}

static inline Um_instruction segmented_store(Um_register a, Um_register b,
                                             Um_register c) 
{
        return three_register(SSTORE, a, b, c);
}

static inline Um_instruction mul(Um_register a, Um_register b,
                                 Um_register c) 
{
        return three_register(MUL, a, b, c);
}

static inline Um_instruction div(Um_register a, Um_register b,
                                 Um_register c) 
{
        return three_register(DIV, a, b, c);
}

static inline Um_instruction nand(Um_register a, Um_register b,
                                  Um_register c) 
{
        return three_register(NAND, a, b, c);
}

static inline Um_instruction map_segment(Um_register b, Um_register c) 
{
        return three_register(ACTIVATE, 0, b, c);
}

static inline Um_instruction unmap_segment(Um_register c) 
{
        return three_register(INACTIVATE, 0, 0, c);
}

static inline Um_instruction input(Um_register c) 
{
        return three_register(IN, 0, 0, c);
}

static inline Um_instruction load_program(Um_register b, Um_register c) 
{
        return three_register(LOADP, 0, b, c);
}


/* Functions for working with streams */

static inline void append(Seq_T stream, Um_instruction inst)
{
        assert(sizeof(inst) <= sizeof(uintptr_t));
        Seq_addhi(stream, (void *)(uintptr_t)inst);
}

const uint32_t Um_word_width = 32;

void Um_write_sequence(FILE *output, Seq_T stream)
{
        assert(output != NULL && stream != NULL);
        int stream_length = Seq_length(stream);
        for (int i = 0; i < stream_length; i++) {
                Um_instruction inst = (uintptr_t)Seq_remlo(stream);
                for (int lsb = Um_word_width - 8; lsb >= 0; lsb -= 8) {
                        fputc(Bitpack_getu(inst, 8, lsb), output);
                }
        }
      
}

/* Helper functions */

/* Uses rB, does not modify rA */
void output_digit(Seq_T stream, Um_register rA, Um_register rB)
{
        append(stream, loadval(rB, 48));
        append(stream, add(rB, rA, rB));
        append(stream, output(rB));
}

/* Stores max value in rB */
void load_max_val(Seq_T stream, Um_register rA, Um_register rB, int offset)
{
        append(stream, loadval(rA, 65535));
        append(stream, loadval(rB, 65536));
        append(stream, mul(rB, rA, rB));
        append(stream, loadval(rA, 65535 - offset));
        append(stream, add(rB, rA, rB));
}

// void output_number(Seq_T stream, Um_register rB,
//                    Um_register rC, uint32_t value_in_rA)
// {
//         if (value_in_rA == 0) {
//                 return;
//         }

//         output_number(stream, rB, rC, value_in_rA / 10);
//         append(stream, loadval(rC, value_in_rA % 10));
//         output_digit(stream, rC, rB);
// }

/* Unit tests for the UM */

void build_halt_test(Seq_T stream)
{
        append(stream, halt());
}

void build_output_test(Seq_T stream)
{
        append(stream, output(r1));
        append(stream, output(r1));
        append(stream, output(r2));
        append(stream, output(r3));
        append(stream, output(r4));
        append(stream, output(r5));
        append(stream, output(r6));
        append(stream, output(r7));
        append(stream, halt());
}

void build_load_value_test(Seq_T stream)
{
        append(stream, loadval(r1, 'a'));
        append(stream, output(r1));
        append(stream, loadval(r2, 'b'));
        append(stream, output(r2));
        append(stream, loadval(r3, 'c'));
        append(stream, output(r3));
        append(stream, loadval(r4, 'd'));
        append(stream, output(r4));
        append(stream, loadval(r5, 'e'));
        append(stream, output(r5));
        append(stream, loadval(r6, 'f'));
        append(stream, output(r6));
        append(stream, loadval(r7, 'g'));
        append(stream, output(r7));
        append(stream, halt());
}

void build_verbose_halt_test(Seq_T stream)
{
        append(stream, halt());
        append(stream, loadval(r1, 'B'));
        append(stream, output(r1));
        append(stream, loadval(r1, 'a'));
        append(stream, output(r1));
        append(stream, loadval(r1, 'd'));
        append(stream, output(r1));
        append(stream, loadval(r1, '!'));
        append(stream, output(r1));
        append(stream, loadval(r1, '\n'));
        append(stream, output(r1));
}

void build_add_test(Seq_T stream)
{
        append(stream, loadval(r1, 1));
        append(stream, loadval(r2, 2));
        append(stream, loadval(r3, 3));
        append(stream, add(r1, r2, r3));
        output_digit(stream, r1, r4);
        append(stream, halt());
}

void build_print_six_test(Seq_T stream)
{
        append(stream, loadval(r1, 48));
        append(stream, loadval(r2, 6));
        append(stream, add(r3, r1, r2));
        append(stream, output(r3));
        append(stream, halt());
}


void build_add_mod_test(Seq_T stream)
{
        append(stream, loadval(r1, 0));
        append(stream, loadval(r2, 65536));
        for (int i = 0; i < 65536; i++) {
                append(stream, add(r1, r1, r2));
        }
        // append(stream, loadval(r3, 10));
        // append(stream, add(r1, r1, r3));
        output_digit(stream, r1, r4);
        append(stream, halt());
}

void build_mul_test(Seq_T stream)
{
        append(stream, loadval(r1, 1));
        append(stream, loadval(r2, 2));
        append(stream, loadval(r3, 3));
        append(stream, mul(r1, r2, r3));
        output_digit(stream, r1, r4);
        append(stream, halt());
}

void build_mul_mod_test(Seq_T stream)
{
        append(stream, loadval(r1, 65536));
        append(stream, mul(r1, r1, r1));
        output_digit(stream, r1, r2);
        append(stream, halt());
}

void build_div_test(Seq_T stream)
{
        append(stream, loadval(r1, 1));
        append(stream, loadval(r2, 2));
        append(stream, loadval(r3, 3));
        append(stream, div(r1, r2, r3));
        output_digit(stream, r1, r4);
        append(stream, div(r1, r3, r2));
        output_digit(stream, r1, r4);
        
        // Divide 2^24 by 2 24 times
        append(stream, loadval(r4, 16777216));
        append(stream, loadval(r5, 2));
        for (int i = 0; i < 24; i++) {
                append(stream, div(r4, r4, r5));
        }
        output_digit(stream, r4, r6);
        append(stream, halt());
}

void build_nand_test(Seq_T stream)
{
        // append(stream, loadval(r1, ~0));
        load_max_val(stream, r7, r1, 0);
        // append(stream, loadval(r2, ~0));
        load_max_val(stream, r7, r2, 0);
        append(stream, nand(r3, r2, r1));
        output_digit(stream, r3, r4); // should be 0
        // append(stream, loadval(r1, ~0 - 1));
        load_max_val(stream, r7, r1, 1);
        // append(stream, loadval(r2, ~0 - 2));
        load_max_val(stream, r7, r2, 2);
        append(stream, nand(r3, r2, r1));
        output_digit(stream, r3, r4); // should be 3
        append(stream, halt());
}


// void build_arithmetic_test(Seq_T stream)
// {
//         append(stream, loadval(r1, 253));
//         append(stream, loadval(r7, 10));

//         // Stores r1 % 10 in r2 (3)
//         append(stream, div(r2, r1, r7));
//         append(stream, mul(r2, r2, r7));
//         append(stream, sub(r2, r1, r2));

//         // r1 now stores 25
//         append(stream, div(r1, r1, 10));

//         // Stores r1 % 10 in r3 (5)
//         append(stream, div(r3, r1, r7));
//         append(stream, mul(r3, r3, r7));
//         append(stream, sub(r3, r1, r3));
        
//         // r1 now stores 2
//         append(stream, div(r1, r1, 10));

//         // swap r1 and r3
//         append(stream, add(r7, r1, 0));
//         append(stream, add(r1, r3, 0));
//         append(stream, add(r3, r7, 0));

//         // Should print 253
//         append(stream, output(r3));
//         append(stream, output(r2));
//         append(stream, output(r1));
// }

void build_cmov_test(Seq_T stream)
{
        append(stream, loadval(r1, 'a'));
        append(stream, loadval(r2, 'b'));
        append(stream, loadval(r3, 0));
        append(stream, conditional_move(r1, r2, r3));
        append(stream, output(r1));
        append(stream, output(r2));
        append(stream, loadval(r3, 10));
        append(stream, conditional_move(r1, r2, r3));
        append(stream, output(r1));
        append(stream, output(r2));
        append(stream, halt());
}

void build_sload_test(Seq_T stream)
{
        append(stream, conditional_move(r1, r2, r3)); // word is equiv to 'S'
        append(stream, conditional_move(r1, r2, r4)); // word is equiv to 'T'
        append(stream, loadval(r1, 0));
        append(stream, loadval(r2, 0));
        append(stream, loadval(r3, 0));
        append(stream, segmented_load(r1, r2, r3));
        append(stream, output(r1));
        append(stream, loadval(r1, 0));
        append(stream, loadval(r2, 0));
        append(stream, loadval(r3, 1));
        append(stream, segmented_load(r1, r2, r3));
        append(stream, output(r1));
        append(stream, halt());
}

void build_sstore_test(Seq_T stream)
{ 
        append(stream, conditional_move(r1, r2, r3)); // word is equiv to 'S'
        append(stream, conditional_move(r1, r2, r4)); // word is equiv to 'T'
        append(stream, loadval(r1, 0));
        append(stream, loadval(r2, 0));
        append(stream, loadval(r3, 0));

        // Gets first instruction word and stores in r3
        append(stream, segmented_load(r3, r1, r2));

        append(stream, loadval(r2, 1));
        // Overwrites second instruction with first instruction 
        append(stream, segmented_store(r1, r2, r3));

        // Load second instruction word and store in r4
        append(stream, segmented_load(r4, r1, r2));

        // Should output 'S'
        append(stream, output(r4));
        append(stream, halt());
}

void build_map_segment_test(Seq_T stream)
{
        append(stream, loadval(r1, 4));
        // append(stream, loadval(r2, 0));
        append(stream, map_segment(r2, r1)); 
        output_digit(stream, r2, r7); // should print 1
        for (int i = 0; i < 4; i++) {
                append(stream, loadval(r3, i));
                append(stream, segmented_load(r4, r2, r3));
                // append(stream, output(r4)); // should print 0 each time
                output_digit(stream, r4, r7);
        }
        append(stream, loadval(r1, 0));
        append(stream, map_segment(r5, r1));
        output_digit(stream, r5, r7); // should print 2
        append(stream, halt());
}

void build_map_and_store_test(Seq_T stream)
{
        /* Map a new segment, store values in it, and retrieve/output the value */
        append(stream, loadval(r1, 3));
        append(stream, map_segment(r2, r1));
        append(stream, loadval(r3, 83));
        append(stream, loadval(r4, 0));
        append(stream, segmented_store(r2, r4, r3));
        append(stream, segmented_load(r6, r2, r4));
        append(stream, output(r6)); // should print 'S'
        append(stream, halt());

}

void build_unmap_segment_test(Seq_T stream)
{
        append(stream, loadval(r1, 4));
        // append(stream, loadval(r2, 0));
        append(stream, map_segment(r2, r1));
        // output_digit(stream, r2, r7); // should print 1
        append(stream, unmap_segment(r2)); 
        append(stream, map_segment(r2, r1));
        append(stream, unmap_segment(r2)); 
        // output_digit(stream, r2, r7); // should print 1
        append(stream, halt());
}

void build_map_empty_seg_test(Seq_T stream)
{
        append(stream, loadval(r1, 0));
        append(stream, map_segment(r2, r1));
        append(stream, unmap_segment(r2));
        append(stream, halt());
}

void build_input_test(Seq_T stream)
{
        for (int i = 0; i < 9; i++) {     
                append(stream, input(r1));
                append(stream, output(r1));
        }
        append(stream, halt());
}

void build_input_eof_test(Seq_T stream)
{    
        /* Input is null, so EOF is obtained as an input value */
        append(stream, input(r1));
        append(stream, loadval(r2, 1));
        append(stream, add(r1, r1, r2));
        output_digit(stream, r1, r7); // should print 0
        append(stream, halt());
}

void build_load_seg_0_test(Seq_T stream)
{
        // check if loading 0 segment is good
        append(stream, loadval(r1, 0));
        append(stream, loadval(r2, 4));
        append(stream, load_program(r1, r2));
        append(stream, halt());
        append(stream, loadval(r1, 'a'));
        append(stream, loadval(r2, 'b'));
        append(stream, output(r1));
        append(stream, output(r2));
        append(stream, halt());
}

void build_load_program_test(Seq_T stream)
{
        // Add two cmov operations to a new segment of size 2
        append(stream, loadval(r1, 4));
        append(stream, map_segment(r2, r1));
        append(stream, loadval(r3, 83));
        append(stream, loadval(r4, 0));
        append(stream, segmented_store(r2, r4, r3));
        append(stream, loadval(r3, 84));
        append(stream, loadval(r4, 1));
        append(stream, segmented_store(r2, r4, r3));
        // r1:2 r2:1 r3:84 r4:1

        // Add the output(r1) instruction to the new segment
        append(stream, loadval(r5, 6637));
        append(stream, loadval(r6, 3041));
        append(stream, loadval(r7, 133));
        append(stream, mul(r5, r5, r6));
        append(stream, mul(r5, r5, r7));
        append(stream, loadval(r4, 2));
        append(stream, segmented_store(r2, r4, r5));

        // Add halt instruction
        append(stream, loadval(r5, 16384));
        append(stream, loadval(r6, 7));
        append(stream, mul(r5, r5, r5));
        append(stream, mul(r5, r5, r6));
        append(stream, loadval(r4, 3));
        append(stream, segmented_store(r2, r4, r5));

        // Set values in registers before running new program
        append(stream, loadval(r1, 'a'));
        append(stream, conditional_move(r6, r2, r1));
        append(stream, loadval(r2, 'b'));
        append(stream, loadval(r3, 1));
        append(stream, loadval(r4, 1));

        // Replace segment 0 with new segment
        append(stream, loadval(r5, 0));
        append(stream, load_program(r6, r5)); // should output 'b'

        append(stream, halt());
}

void build_performance_test(Seq_T stream)
{
        for (int i = 1; i < 50000; i++) {
                append(stream, loadval(r1, i));
                //append(stream, map_segment(r2, r1));
                append(stream, loadval(r3, i - 1));
                append(stream, loadval(r6, 25000));
                append(stream, div(r5, r1, r6));
                append(stream, conditional_move(r4, r3, r5));
                // append(stream, segmented_store(r2, r3, r4));
                // append(stream, segmented_load(r7, r2, r3));
                append(stream, nand(r1, r1, r5));
                append(stream, add(r7, r1, r6));
                append(stream, mul(r5, r0, r2));
                append(stream, add(r7, r1, r6));
                append(stream, mul(r5, r0, r2));
                // append(stream, unmap_segment(r2));
        }
        append(stream, halt());
}

// void build_no_halt_test(Seq_T stream)
// {
//         append(stream, loadval(r1, 4));
//         // append(stream, loadval(r2, 0));
//         append(stream, map_segment(r2, r1));
//         output_digit(stream, r2, r7); // should print 1
//         append(stream, unmap_segment(r2)); 
//         append(stream, map_segment(r2, r1));
//         output_digit(stream, r2, r7); // should print 1
// }

void build_memory_test(Seq_T stream)
{
        // Load segment to store 4 addresses
        int num_segments = 10;
        int seg_length = 1000;
        append(stream, loadval(r1, num_segments));
        // append(stream, loadval(r2, 0));
        append(stream, map_segment(r2, r1));

        append(stream, loadval(r3, seg_length));
        for (int i = 0; i < num_segments; i++) {
                append(stream, loadval(r0, i));
                append(stream, map_segment(r4, r3));
                append(stream, segmented_store(r2, r0, r4));
        }

        // Get address of last segment
        append(stream, segmented_load(r5, r2, r0));
        append(stream, loadval(r7, 9));

        // Store 9 in every position of the segment
        for (int i = 0; i < seg_length; i++) {
                append(stream, loadval(r6, i));
                append(stream, segmented_store(r5, r6, r7));
        }
        
        append(stream, segmented_load(r0, r5, r6));
        output_digit(stream, r0, r1); // should print 9
        append(stream, halt());
}

void build_add_and_remove_test(Seq_T stream)
{
        int num_segments = 10000;
        int seg_length = 1000;
        append(stream, loadval(r1, num_segments));
        // append(stream, loadval(r2, 0));
        append(stream, map_segment(r2, r1));

        append(stream, loadval(r3, seg_length));
        for (int i = 0; i < num_segments; i++) {
                append(stream, loadval(r0, i));
                append(stream, map_segment(r4, r3));
                append(stream, segmented_store(r2, r0, r4));
        }

        // Unmap all segments
        for (int i = 0; i < num_segments; i++) {
                append(stream, loadval(r0, i));
                append(stream, segmented_load(r1, r2, r0));

                // append(stream, loadval(r3, 1000 * i + 10));
                append(stream, unmap_segment(r1));
        }

        for (int i = 0; i < num_segments; i++) {
                append(stream, loadval(r0, i));
                append(stream, map_segment(r4, r3));
                append(stream, segmented_store(r2, r0, r4));
        }

        // Get address of last segment
        append(stream, segmented_load(r5, r2, r0));
        append(stream, loadval(r7, 9));

        // Store 9 in every position of the segment
        for (int i = 0; i < seg_length; i++) {
                append(stream, loadval(r6, i));
                append(stream, segmented_store(r5, r6, r7));
        }
        
        append(stream, segmented_load(r0, r5, r6));
        output_digit(stream, r0, r1); // should print 9
        append(stream, halt());
}

