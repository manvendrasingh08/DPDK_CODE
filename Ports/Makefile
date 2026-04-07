# List of source files
SRC = available_ports.c ports.c

CC ?= gcc
CFLAGS += -O3 -Wall -Wextra
PKGCONF = $(shell pkg-config --cflags --libs libdpdk)

# Generate executable names by removing .c
APP = $(SRC:.c=)

# Default target: build all executables
all: $(APP)

# Pattern rule: build each executable from its .c
%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(PKGCONF)

# Clean all executables
clean:
	rm -f $(APP)
