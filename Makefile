#
#      Makefile
#      Ryan Beckwith (rbeckw02) and Victoria Chen (vchen05)
#      Date: 11-24-20
#      HW6: um
#
#      Purpose: Makefile for HW6: um. Provides rules for compiling the um
#      executable.
# 
CC = gcc

IFLAGS  = -I/comp/40/build/include -I/usr/sup/cii40/include/cii
CFLAGS  = -g -std=gnu99 -Wall -Wextra -Werror -Wfatal-errors -pedantic \
		  $(IFLAGS)
LDFLAGS = -g -L/comp/40/build/lib -L/usr/sup/cii40/lib64
LDLIBS  = -lbitpack -lm -lcii40 -l40locality
COMPILE = $(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)
INCLUDES = $(shell echo *.h)
EXECS   = um

all: $(EXECS)

um: instruction_executor.o memory.o
	$(COMPILE)

# To get *any* .o file, compile its .c file with the following rule.
%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXECS) *.o