COMPILER=$(CROSS_COMPILE)$(CC)

CFLAGS += -O0
CFLAGS += -g3

all: aesdsocket

aesdsocket: build/src/main.o build/src/tcp_server.o build/src/tcp_connection.o
	$(COMPILER) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

build/%.o: %.c
	mkdir -p $(@D)
	$(COMPILER) -c $(CPPFLAGS) $(CFLAGS) $^ -o $@

clean:
	rm -rf build
	rm -rf aesdsocket

.PHONY: clean all