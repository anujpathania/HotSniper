TARGET = FFT
OBJS = fft.o

x = *

all: $(TARGET) $(TARGET)-O0 $(TARGET)-O1 $(TARGET)-O2 $(TARGET)-O3

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

$(TARGET)-O0: fft.c
	$(CC) fft.c $(CFLAGS) -o $@ $(LDFLAGS) -O0
$(TARGET)-O1: fft.c
	$(CC) fft.c $(CFLAGS) -o $@ $(LDFLAGS) -O1
$(TARGET)-O2: fft.c
	$(CC) fft.c $(CFLAGS) -o $@ $(LDFLAGS) -O2
$(TARGET)-O3: fft.c
	$(CC) fft.c $(CFLAGS) -o $@ $(LDFLAGS) -O3

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


all: $(TARGET)

clean:
	rm -f *.c *.h *.o $(TARGET)*
