COMPILER=$(CROSS_COMPILE)$(CC)

CFLAGS += -O0
CFLAGS += -g3

all: aesdsocket

aesdsocket: src/main.o src/server_mng.o
	$(COMPILER) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

src/main.o: src/main.c
	$(COMPILER) -c $(CPPFLAGS) $(CFLAGS) $^ -o $@

src/server_mng.o: src/server_mng.c
	$(COMPILER) -c $(CPPFLAGS) $(CFLAGS) $^ -o $@

clean:
	rm -f aesdsocket
	rm -f src/*.o

.PHONY: clean all