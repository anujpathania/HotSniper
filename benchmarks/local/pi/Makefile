include ../../tools/hooks/buildconf.makefile
TARGET = pi

$(TARGET): pi.o
	$(HOOKS_CC) pi.o $(HOOKS_LDFLAGS) -fopenmp -o $@

%.o : %.c
	$(HOOKS_CC) $< $(HOOKS_CFLAGS) -fopenmp -c -o $@

clean :
	rm -f *.o $(TARGET)
