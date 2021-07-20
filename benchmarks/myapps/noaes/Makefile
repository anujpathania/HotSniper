include ../../tools/hooks/buildconf.makefile
TARGET = noaes
HOOKS_CFLAGS +=-I../openssl/include -I../../../include

$(TARGET):
	g++ -o $(TARGET) $(TARGET).cc $(HOOKS_CFLAGS) ../openssl/libcrypto.a
clean :
	rm -f *.o $(TARGET)




