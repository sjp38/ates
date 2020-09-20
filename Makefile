.PHONY: clean help

APP	:= test_misc
OBJ	:= ates.o misc.o test_misc.o
DEPS	:= misc.h ates.h

CC	:= gcc
CFLAGS	:= -g -O3 -Wall -Werror -std=gnu99
LIBS	:= -lpthread

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(APP): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

clean:
	rm -fr *.o $(APP)
