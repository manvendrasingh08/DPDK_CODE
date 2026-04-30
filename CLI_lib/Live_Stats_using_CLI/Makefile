APP = live_stats

SRC = main.c \
      stats.c \
      rx.c \
      tx.c \
      cli/cli.c \
      cli/cmd_show.c \
	  cli/cmd_exit.c\
      cli/cmd_set.c

PKGCONF = $(shell pkg-config --cflags --libs libdpdk)

CC = cc
CFLAGS = -O3 -Wall -Wextra

all: $(APP)

$(APP): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(APP) $(PKGCONF)

clean:
	rm -f $(APP)