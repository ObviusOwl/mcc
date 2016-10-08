CC=     gcc
LD=     ld
CFLAGS= -W -Wall -Werror -std=c99
CFLAGS_DEBUG= -W -Wall -Werror -std=c99 -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unused-parameter
LIBS= -pthread -lm

OBJECTS_WRAPPER= clients.o args.o mc.o
BIN_WRAPPER= minecraft-wrapper

OBJECTS_CLIENT= client.o
BIN_CLIENT= minecraft-console

debug: OPT_FLAGS= -Og -g 
debug: CFLAGS= $(CFLAGS_DEBUG)
debug: $(OBJECTS_WRAPPER)
	$(CC) $(OPT_FLAGS) $(CFLAGS) -o $(BIN_WRAPPER) $(OBJECTS_WRAPPER) $(LIBS)

wrapper: OPT_FLAGS= -O3
wrapper: $(OBJECTS_WRAPPER)
		$(CC) $(OPT_FLAGS) $(CFLAGS) -o $(BIN_WRAPPER) $(OBJECTS_WRAPPER) $(LIBS)

client: $(OBJECTS_CLIENT)
	$(CC) $(OPT_FLAGS) $(CFLAGS) -o $(BIN_CLIENT) $(OBJECTS_CLIENT) $(LIBS)

.PHONY: clean

%.o: %.c
	$(CC) $(OPT_FLAGS) $(CFLAGS) -c $< $(LIBS)

clean:
	rm -f $(BIN_WRAPPER) $(BIN_CLIENT) *.o

all: wrapper