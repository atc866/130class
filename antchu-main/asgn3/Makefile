EXECBINS  = queue rwlock
SOURCES  = $(wildcard *.c)
OBJECTS  = $(SOURCES:%.c=%.o)
FORMATS  = $(SOURCES:%.c=%.fmt)

CC       = clang
FORMAT   = clang-format
CFLAGS   = -Wall -pedantic -Werror -Wextra 

.PHONY: all clean format

all : queue.o rwlock.o

queue.o : queue.c
	$(CC) $(CFLAGS) -c queue.c


rwlock.o : rwlock.c
	$(CC) $(CFLAGS) -c rwlock.c

clean:
	rm -f $(EXECBINS) $(OBJECTS)

format: $(FORMATS)

%.fmt: %.c
	$(FORMAT) -i $<
	touch $@
