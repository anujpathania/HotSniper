TARGET = RAYTRACE

OBJS = bbox.o cr.o env.o fbuf.o geo.o huprn.o husetup.o hutv.o isect.o \
	main.o matrix.o memory.o poly.o raystack.o shade.o sph.o trace.o \
	tri.o workpool.o

x = *

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

$(TARGET).dynamic: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(TARGET).dynamic $(filter-out -static,$(LDFLAGS)) -rdynamic


OBJS_OPT=$(patsubst %.o,%.o-opt,$(OBJS))
$(TARGET).opt: $(OBJS_OPT)
	$(CC) $(OBJS_OPT) $(CFLAGS_OPT) -o $@ $(LDFLAGS)

CFLAGS_OPT=$(CFLAGS) -DUSE_LOCK_INC

%.o-opt: %.c
	$(CC) -c -o $@ $(CFLAGS_OPT) $*.c


M4 = -m4 -Uindex -Ulen
RM = -rm -f

.SUFFIXES:
.SUFFIXES:      .o .c .C .h .H

.H.h:
	$(M4) ${MACROS} $*.H > $*.h

.C.c:
	$(M4) $(MACROS) $*.C > $*.c

.c.o:
	$(CC) -c $(CFLAGS) $*.c

.C.o:
	$(M4) $(MACROS) $*.C > $*.c
	$(CC) -c $(CFLAGS) $*.c

bbox.c: bbox.C rt.h
cr.c:	cr.C rt.h
env.c:	env.C rt.h
fbuf.c: fbuf.C rt.h
geo.c:	geo.C rt.h
huprn.c: huprn.C rt.h
husetup.c: husetup.C rt.h
hutv.c: hutv.C rt.h
isect.c: isect.C rt.h
main.c: main.C rt.h
matrix.c: matrix.C rt.h
memory.c: memory.C rt.h
poly.c: poly.C rt.h
raystack.c: raystack.C rt.h
shade.c: shade.C rt.h
sph.c:	sph.C rt.h
trace.c: trace.C rt.h
tri.c:	tri.C rt.h
workpool.c: workpool.C rt.h


all: $(TARGET) $(TARGET).dynamic $(TARGET).opt

clean:
	rm -f *.c *.h *.o $(TARGET)*
