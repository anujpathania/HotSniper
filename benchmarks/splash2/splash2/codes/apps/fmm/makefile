TARGET = FMM
OBJS = defs.o memory.o particle.o box.o partition_grid.o \
       cost_zones.o construct_grid.o interactions.o fmm.o 

x = *

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

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

defs.c: defs.h
box.c: box.h
construct_grid.c: construct_grid.h
cost_zones.c: cost_zones.h
interactions.c: interactions.h
memory.c: memory.h
particle.c: particle.h
partition_grid.c: partition_grid.h

fmm.o: fmm.C defs.h memory.h particle.h box.h \
        partition_grid.h cost_zones.h construct_grid.h
interactions.o: interactions.C defs.h memory.h \
        particle.h box.h partition_grid.h interactions.h
shell.o: shell.C defs.h memory.h particle.h box.h \
        partition_grid.h interactions.h
construct_grid.o: construct_grid.C defs.h memory.h \
        particle.h box.h partition_grid.h construct_grid.h
cost_zones.o: cost_zones.C defs.h memory.h box.h \
        partition_grid.h cost_zones.h
orb.o: orb.C defs.h memory.h box.h partition_grid.h orb.h
partition_grid.o: partition_grid.C defs.h memory.h box.h \
        partition_grid.h
box.o: box.C defs.h memory.h particle.h box.h
particle.o: particle.C defs.h memory.h particle.h
memory.o: memory.C defs.h memory.h
defs.o: defs.C defs.h memory.h
memory.h: defs.h particle.h box.h



all: $(TARGET)

clean:
	rm -f *.c *.h *.o $(TARGET)
