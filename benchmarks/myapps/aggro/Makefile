include ../../tools/hooks/buildconf.makefile
TARGET = aggro
HOOKS_CFLAGS += -std=c++11 -I../openssl/include -I../../../include

$(TARGET): obj
	g++ -o $(TARGET) $(TARGET).o  ../openssl/libcrypto.a

obj:
	g++ -c $(TARGET).cc $(HOOKS_CFLAGS)
clean :
	rm -f *.o $(TARGET)



