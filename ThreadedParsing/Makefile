APP = threaded_parsing
SRC = main.c parser.c rx.c tx.c

PKGCONF = $(shell pkg-config --cflags --libs libdpdk)

CC = cc
CFLAGS = -O3 -Wall -Wextra

all: $(APP)

$(APP): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(APP) $(PKGCONF)

clean:
	rm -f $(APP)
