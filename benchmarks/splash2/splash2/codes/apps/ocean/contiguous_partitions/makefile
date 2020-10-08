TARGET = OCEAN
OBJS = main.o jacobcalc.o jacobcalc2.o laplacalc.o \
	slave1.o slave2.o multi.o linkup.o subblock.o 

x = *

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

.SUFFIXES:
.SUFFIXES:	.o .c .C .h .H

.H.h: 
	m4 ${MACROS} $*.H > $*.h

.C.c: 
	m4 $(MACROS) $*.C > $*.c

.c.o: 
	$(CC) -c $(CFLAGS) $*.c

.C.o: 
	m4 $(MACROS) $*.C > $*.c
	$(CC) -c $(CFLAGS) $*.c

decs.h: decs.H
jacobcalc.c: decs.h
linkup.c: decs.h
slave1.c: decs.h
jacobcalc2.c: decs.h
main.c: decs.h
slave2.c: decs.h
laplacalc.c: decs.h
multi.c: decs.h
subblock.c: decs.h


all: $(TARGET)

clean:
	rm -f *.c *.h *.o $(TARGET)
