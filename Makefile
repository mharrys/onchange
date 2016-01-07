CC := gcc
CFLAGS := -std=c99 -pedantic -Wall -O4
LIBS := -lm
C_FILES := $(wildcard *.c)
OBJ_FILES := $(C_FILES:.c=.o)

onchange: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

obj/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -f onchange
	rm -f *.o
