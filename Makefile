#@author GIUSEPPE MUSCHETTA


CC = gcc
CFLAGS = -std=c99 -Wall -pedantic
OPTFLAGS = -g -O3
LIBS = -lpthread

SERVEROBJS = server.o tpool.o
CLIENTOBJ = client.o
SOCKNAME = mysocket
TARGETS = server client


#scrivo .PHONY per dichiarare esplicitamente i target falsi, non verrano creati ad es. files di nome clean o test1
.PHONY: all clean 
.SUFFIXES: .c .h


#target di default
all: 			$(TARGETS)


%.o:			%.c
			$(CC) $(CFLAGS) $(OPTFLAGS) -c $<


server: 		$(SERVEROBJS)
			$(CC) $(CFLAGS) $(OPTFLAGS) $^ -o $@ $(LIBS)


client: 		$(CLIENTOBJ)
			$(CC) $(CFLAGS) $(OPTFLAGS) $^ -o $@ 		
			
		
clean: 		
			@echo "Rimozione files avvenuta con successo!"
			-rm -f $(TARGETS) $(SERVEROBJS) $(CLIENTOBJ) $(SOCKNAME)
			