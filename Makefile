CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99
LFLAGS = 
LIBS = 
SOURCES = main.c sigtrap.c
OBJECTS = $(subst .c,.o,$(SOURCES))
EXE = process main
.PHONY: clean help

.PHONY: debug
debug: CFLAGS += -O0 -g3
debug: $(EXE)

process : sigtrap.o
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@ 

main : main.o
	$(CC) $(CFLAGS) -w $^ $(LIBS) -o $@ 

%.o : %.c
	$(CC) -w $(CFLAGS) -c $< 

all : $(EXE)

clean:
	rm -f $(OBJECTS) $(EXE) *~

help:
	@echo "Valid targets:"
	@echo "  all:    generates all binary files"
	@echo "  debug:  generates debug binary files"
	@echo "  clean:  removes .o and .exe files"
