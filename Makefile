CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -g -O2 $(shell pkg-config --cflags fuse3) 
#-Wall -Wextra
LDFLAGS = -L/usr/local/lib -Wl,-R/usr/local/lib
LDLIBS = $(shell pkg-config --libs fuse3)

fat16: main.o fat16_lowlevel.o fat16.o blkio.o

main.o: main.c fat16_lowlevel.h fat16.h blkio.h fat16_structs.h
fat16_lowlevel.o: fat16_lowlevel.c fat16_lowlevel.h blkio.h fat16.h fat16_structs.h
fat16.o: fat16.c fat16.h blkio.h fat16_structs.h
blkio.o: blkio.c blkio.h

clean:
	rm -f *~ *.o fat16

# vim: ts=8 sw=8 noet
#gcc -Wall passthrough_ll.c `pkg-config fuse3 --cflags --libs` -o passthrough_ll -L/usr/local/lib -Wl,-R/usr/local/lib