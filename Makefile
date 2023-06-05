CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -g -pedantic
SFLAGS = -shared
TFLAGS = -pthread
HFILE = reactor.h
LIBFILE = reactor.so
RM = rm -f


.PHONY: all default clean

all: react_server

default: all

#Programs 
react_server: react_server.o $(LIBFILE)
	$(CC) $(CFLAGS) -o $@ $< ./$(LIBFILE) $(TFLAGS)

#Libraries 
$(LIBFILE): reactor.o
	$(CC) $(CFLAGS) $(SFLAGS) -o $@ $^ $(TFLAGS)

reactor.o: reactor.c $(HFILE)
	$(CC) $(CFLAGS) -fPIC -c $<


#Object
%.o: %.c $(HFILE)
	$(CC) $(CFLAGS) -c $<
	
clean:
	$(RM) *.o *.so react_server