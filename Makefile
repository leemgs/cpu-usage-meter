
#-------------- Please specify the your compiler --------------
CC=gcc
#CC=/opt/arm-linux-uclibc/bin/arm-linux-uclibc-gcc

#-------------- Do not modify the below statements ------------
CFLAGS=-g -Wall

TARGET=cpumeter
OBJECTS=cpumeter.o

all: $(TARGET)

clean:
	rm -rf $(TARGET) $(OBJECTS)

cpumeter: cpumeter.o
	$(CC) $(CFLAGS) -o $@ $<


.c.o:
	$(CC) $(CFLAGS) -c $<

