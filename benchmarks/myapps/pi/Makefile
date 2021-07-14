include ../../tools/hooks/buildconf.makefile
TARGET = pi

$(TARGET): $(TARGET).o
	$(HOOKS_CC) $(TARGET).o $(HOOKS_LDFLAGS) -fopenmp -o $@

%.o : %.c
	$(HOOKS_CC) $< $(HOOKS_CFLAGS)  -fopenmp -c -o $@

clean :
	rm -f *.o $(TARGET)
