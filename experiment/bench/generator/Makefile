CFLAGS=-std=c++11 -g -Wall -pthread -I./
LDFLAGS= -lpthread -ltbb -lhiredis
CC=g++
EXEC=ycsbc

objects = main.o  



edit : $(objects)
	$(CC) -o $(EXEC) $(objects)

main.o : generator.h uniform_generator.h zipfian_generator.h utils.h

.PHONY : clean
clean :
	rm edit $(objects)