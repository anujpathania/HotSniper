all: libhooks_base.a libhooks_base_noroi.a buildscripts

include buildconf.makefile

CC=${HOOKS_CC}
CFLAGS=${HOOKS_CFLAGS} -fPIC

%.o: %.c *.h
	${CC} ${CFLAGS} -c -o $@ $<

%_noroi.o: %.c *.h
	$(CC) $(CFLAGS) -DPARMACS_NO_ROI -c -o $@ $<

lib%_noroi.a: %_noroi.o $(OBJECTS)
	ar crs $@ $^

lib%.a: %.o
	ar crs $@ $^

clean:
	rm -f *.o *.a

buildconf.makefile buildscripts:
	./makebuildscripts.py
