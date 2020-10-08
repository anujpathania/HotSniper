TARGET = rltotiff

TIFF_LIBDIR = ./libtiff
TIFF_INCDIR = ./libtiff
OBJS = rltotiff.o tiff_rgba_io.o
LDFLAGS = -L$(TIFF_LIBDIR) -lc_s -ltiff
CFLAGS = -I$(TIFF_INCDIR)

x = *

$(TARGET): $(OBJS)
	cc $(OBJS) $(CFLAGS) -o $(TARGET) $(LDFLAGS)


M4 = -m4 -Uindex -Ulen
RM = -rm -f

.SUFFIXES:
.SUFFIXES:      .o .c

.c.o:
	cc -c $(CFLAGS) $*.c

