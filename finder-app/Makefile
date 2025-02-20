CC=gcc
COMPILER=$(CROSS_COMPILE)$(CC)

all: writer

writer: writer.o
	$(COMPILER) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

writer.o: writer.c
	$(COMPILER) -c $(CPPFLAGS) $(CFLAGS) $^ -o $@

clean:
	rm -f writer
	rm -f writer.o

.PHONY: all clean