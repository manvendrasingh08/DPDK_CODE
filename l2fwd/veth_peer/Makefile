APP = main
SRC = main.c

PKGCONF = $(shell pkg-config --cflags --libs libdpdk)

CC = cc
CFLAGS = -O3 -Wall -Wextra

all: $(APP)

$(APP): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(APP) $(PKGCONF)

clean:
	rm -f $(APP)
