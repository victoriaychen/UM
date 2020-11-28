#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assert.h"
#include "fmt.h"
#include "seq.h"

extern void Um_write_sequence(FILE *output, Seq_T instructions);

extern void build_halt_test(Seq_T instructions);
extern void build_output_test(Seq_T instructions);
extern void build_load_value_test(Seq_T instructions);
extern void build_verbose_halt_test(Seq_T instructions);
extern void build_add_test(Seq_T instructions);
extern void build_add_mod_test(Seq_T instruction); 
extern void build_mul_test(Seq_T instructions);
extern void build_mul_mod_test(Seq_T instruction); 
extern void build_div_test(Seq_T instructions);
extern void build_nand_test(Seq_T instructions);
extern void build_print_six_test(Seq_T instructions);
extern void build_cmov_test(Seq_T instructions);
extern void build_sload_test(Seq_T instructions);
extern void build_sstore_test(Seq_T instructions);
extern void build_map_segment_test(Seq_T instructions);
extern void build_unmap_segment_test(Seq_T instructions);
extern void build_input_test(Seq_T instructions);
extern void build_input_eof_test(Seq_T instructions);
extern void build_map_and_store_test(Seq_T instructions);
extern void build_load_program_test(Seq_T instructions);
extern void build_load_seg_0_test(Seq_T instructions);
extern void build_map_empty_seg_test(Seq_T instructions);
extern void build_performance_test(Seq_T instructions);
//extern void build_no_halt_test(Seq_T instructions);
// extern void build_arithmetic_test(Seq_T instructions);

/* The array `tests` contains all unit tests for the lab. */

static struct test_info {
        const char *name;
        const char *test_input;          /* NULL means no input needed */
        const char *expected_output; 
        /* writes instructions into sequence */
        void (*build_test)(Seq_T stream);
} tests[] = {
        { "halt",          NULL,         "",                build_halt_test },
        { "output",        NULL,         "",                build_output_test },
        { "load-value",    NULL,         "abcdefg",         build_load_value_test },
        { "halt-verbose",  NULL,         "",                build_verbose_halt_test },
        { "add",           NULL,         "5",               build_add_test },
        { "add-mod",       NULL,         "0",               build_add_mod_test },
        { "mul",           NULL,         "6",               build_mul_test },
        { "mul-mod",       NULL,         "0",               build_mul_mod_test },
        { "div",           NULL,         "011",             build_div_test },
        { "nand",          NULL,         "03",              build_nand_test },
        { "print-six",     NULL,         "6",               build_print_six_test },
        { "cmov",          NULL,         "abbb",            build_cmov_test },
        { "sload",         NULL,         "ST",              build_sload_test },
        { "sstore",        NULL,         "S",               build_sstore_test },
        { "map-segment",   NULL,         "100002",          build_map_segment_test },
        { "unmap-segment", NULL,         "",                build_unmap_segment_test },
        { "input",         "abcde\nabc", "abcde\nabc",      build_input_test },
        { "input-eof",     NULL,         "0",               build_input_eof_test },
        { "load-program",  NULL,         "b",               build_load_program_test },
        { "map-and-store", NULL,         "S",               build_map_and_store_test },
        { "load-seg-0",    NULL,         "ab",              build_load_seg_0_test },
        { "map-empty-seg", NULL,          "",               build_map_empty_seg_test },
        { "performance",   NULL,          "",               build_performance_test },
        //{ "no-halt",       NULL,         "11",              build_no_halt_test },
        // { "arithmetic",   NULL, "253",        build_arithmetic_test },
        
};

  
#define NTESTS (sizeof(tests)/sizeof(tests[0]))

/*
 * open file 'path' for writing, then free the pathname;
 * if anything fails, checked runtime error
 */
static FILE *open_and_free_pathname(char *path);

/*
 * if contents is NULL or empty, remove the given 'path', 
 * otherwise write 'contents' into 'path'.  Either way, free 'path'.
 */
static void write_or_remove_file(char *path, const char *contents);

static void write_test_files(struct test_info *test);


int main (int argc, char *argv[])
{
        bool failed = false;
        if (argc == 1)
                for (unsigned i = 0; i < NTESTS; i++) {
                        printf("***** Writing test '%s'.\n", tests[i].name);
                        write_test_files(&tests[i]);
                }
        else
                for (int j = 1; j < argc; j++) {
                        bool tested = false;
                        for (unsigned i = 0; i < NTESTS; i++)
                                if (!strcmp(tests[i].name, argv[j])) {
                                        tested = true;
                                        write_test_files(&tests[i]);
                                }
                        if (!tested) {
                                failed = true;
                                fprintf(stderr,
                                        "***** No test named %s *****\n",
                                        argv[j]);
                        }
                }
        return failed; /* failed nonzero == exit nonzero == failure */
}


static void write_test_files(struct test_info *test)
{
        FILE *binary = open_and_free_pathname(Fmt_string("%s.um", test->name));
        Seq_T instructions = Seq_new(0);
        test->build_test(instructions);
        Um_write_sequence(binary, instructions);
        Seq_free(&instructions);
        fclose(binary);

        write_or_remove_file(Fmt_string("%s.0", test->name),
                             test->test_input);
        write_or_remove_file(Fmt_string("%s.1", test->name),
                             test->expected_output);
}


static void write_or_remove_file(char *path, const char *contents)
{
        if (contents == NULL || *contents == '\0') {
                remove(path);
        } else {
                FILE *input = fopen(path, "wb");
                assert(input != NULL);

                fputs(contents, input);
                fclose(input);
        }
        free(path);
}


static FILE *open_and_free_pathname(char *path)
{
        FILE *fp = fopen(path, "wb");
        assert(fp != NULL);

        free(path);
        return fp;
}
