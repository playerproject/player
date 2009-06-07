C_ARGS = `pkg-config --cflags playerc`
C_LINK = `pkg-config --libs playerc`

OBJECTS = position2d ir camera blinkenlight
all: $(OBJECTS)

clean:
	rm -f $(OBJECTS)

%: %.c
	$(CC) $(C_ARGS) $(C_LINK) $^ -o $@
