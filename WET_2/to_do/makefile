# Makefile for the smash program
CC = g++
CFLAGS = -g -Wall -std=c++11
CCLINK = $(CC)
OBJS = cache.o cacheSim.o
RM = rm -f
# Creating the  executable
cacheSim: $(OBJS)
	$(CCLINK) -o cacheSim $(CFLAGS) $(OBJS)
# Creating the object files
cache.o :cache.cpp cache.hpp
	$(CC) -c $(CFLAGS) cache.cpp -o cache.o
cacheSim.o: cacheSim.cpp 
	$(CC) -c $(CFLAGS) cacheSim.cpp -o cacheSim.o

# Cleaning old files before new make
clean:
	$(RM) $(TARGET) *.o *~ "#"* core.* cacheSim


