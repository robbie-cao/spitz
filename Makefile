all: target
FORCE: ;
.PHONY: FORCE

program = spitz-tailq spitz-sqlite

target: $(program)

CC = gcc

VPATH = .
IPATH = .

CFLAGS  = -O2 --std=c99 -Wall -Wextra

# DEBUG option, set before make, eg 'DEBUG=1 make'
ifeq ($(DEBUG),1)
	CFLAGS+=-DDEBUG=1
endif

LDFLAGS = -L /usr/lib/x86_64-linux-gnu/ -L /usr/local/lib -lev -lpthread -lcurl

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

spitz-tailq: spitz-tailq.o
	$(CC) $^  $(LDFLAGS) -o $@

spitz-sqlite: spitz-sqlite.o
	$(CC) $^  $(LDFLAGS) -o $@

clean:
	rm -f *.o $(program)
