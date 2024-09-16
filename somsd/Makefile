# 
## Linux
CC=gcc
#FLAGS=-march=prescott mcpu=prescott -funroll-loops -malign-double -fno-exceptions -fomit-frame-pointer -O9 -Wall
FLAGS=-march=pentium4 -funroll-loops -malign-double -fno-exceptions -fomit-frame-pointer -O9 -Wall
#FLAGS=-g -Wall
#CC=icc
#FLAGS=-tpp6 -xW -ip -O3 -Wall -mp1 #optimize for P4-CPUs with icc
#FLAGS=-I /opt/intel/compiler50/ia32/include/ -O2 -tpp6 -axiM -ip

MACROS=-D'COPT="$(FLAGS)"' #-DDEBUG
CFLAGS=$(FLAGS) $(MACROS)
LDFLAGS=
LDLIBS=-lm -lpthread -lz

# DEC Alpha/OSF
#
#CC=cc
#CFLAGS=-O2 
#LDFLAGS=-s
#LDLIBS=-lm

## HPUX
##
#CC=c89
#CFLAGS=-O
#LDFLAGS=-s
#LDLIBS=-lm

## SGI
##
#CC=cc
#CFLAGS=-O2 -mips1
#LDFLAGS=-s
#LDLIBS=-lm

OBJS=common.o data.o fileio.o system.o train.o utils.o

all: initsom somsd testsom balance stats

somsd:	somsd.c $(OBJS)
	$(CC) $(LDFLAGS) -o $@ somsd.c $(OBJS) $(LDLIBS) $(FLAGS)

psomsd:	somsd.c threads.c threads.h $(OBJS)
	$(CC) $(LDFLAGS) -o $@ somsd.c threads.c $(OBJS) $(LDLIBS) $(FLAGS) -D_BE_MULTITHREADED

initsom:	initsom.c $(OBJS)
	$(CC) $(LDFLAGS) -o $@ initsom.c $(OBJS) $(LDLIBS) $(FLAGS)

testsom:	testsom.c $(OBJS)
	$(CC) $(LDFLAGS) -o $@ testsom.c $(OBJS) $(LDLIBS) $(FLAGS)

balance:	balance.c $(OBJS)
	$(CC) $(LDFLAGS) -o $@ balance.c $(OBJS) $(LDLIBS) $(FLAGS)

stats:	stats.c utils.o
	$(CC) $(LDFLAGS) -o $@ stats.c utils.o $(LDLIBS) $(FLAGS)


# for making development distribution
dist:
	rm -f somsd1.4.tar.gz
	tar -cvf somsd1.4.tar *.c *.h Makefile tools
	gzip -v -9 somsd1.4.tar

common.o:	common.h utils.h
data.o:	data.h common.h train.h utils.h
fileio.o:	common.h data.h fileio.h system.h utils.h
system.o:	system.h utils.h
train.o:	common.h data.h fileio.h system.h train.h utils.h
utils.o:	utils.h

clean:
	rm -f *.o initsom somsd testsom
