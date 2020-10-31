CC = gcc
LDFLAGS = -lpthread
CFLAGS = -g -Wall

SRC = $(wildcard *.c)
DEPS = $(wildcard *.h)
OBJS = ${SRC:.c=.o}
EXEC = tiny_webdisk

.PHONEY = clean run

all: $(EXEC)

$(EXEC): $(OBJS) $(DEPS)
	$(CC) $(OBJS) $(LDFLAGS) $(CFLAGS) -o $(EXEC)

clean: 
	rm $(OBJS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

run: $(EXEC)
	./tiny_webdisk
