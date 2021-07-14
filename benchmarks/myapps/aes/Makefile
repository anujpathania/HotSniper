include ../../tools/hooks/buildconf.makefile
TARGET = aes
HOOKS_CFLAGS +=-I../openssl/include -I../../../include

$(TARGET):
	g++ -o $(TARGET) $(TARGET).cc $(HOOKS_CFLAGS) ../openssl/lib/libcrypto.a
clean :
	rm -f *.o $(TARGET)




