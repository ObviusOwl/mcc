CC=     gcc
LD=     ld
CFLAGS= -W -Wall -Werror -std=c99
CFLAGS_DEBUG= -W -Wall -Werror -std=c99 -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unused-parameter
LIBS= -pthread -lm
OBJECTS= args.o mc.o
OBJECTS2= client.o

debug: OPT_FLAGS= -Og -g 
debug: CFLAGS= $(CFLAGS_DEBUG)
debug: $(OBJECTS)
	$(CC) $(OPT_FLAGS) $(CFLAGS) -o mc $(OBJECTS) $(LIBS)

wrapper: OPT_FLAGS= -O3
wrapper: $(OBJECTS)
		$(CC) $(OPT_FLAGS) $(CFLAGS) -o mc $(OBJECTS) $(LIBS)

client: $(OBJECTS2)
	$(CC) $(OPT_FLAGS) $(CFLAGS) -o client $(OBJECTS2) $(LIBS)

.PHONY: clean

%.o: %.c
	$(CC) $(OPT_FLAGS) $(CFLAGS) -c $< $(LIBS)

clean:
	rm -rf mc client *.o

all: debug