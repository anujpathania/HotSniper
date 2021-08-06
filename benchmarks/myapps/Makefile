
.PHONY: all

include ../tools/hooks/buildconf.makefile

SUBDIRS = $(shell ls -d */)


all:
	make -C tools
	make -C pi
	make -C my_pi
	cd openssl && ./config && make -j8
	make -C aes
	make -C noaes


# all:
# 	for dir in $(SUBDIRS) ; do \
# 		make -C  $$dir ; \
# 	done

clean:
	for dir in $(SUBDIRS) ; do \
		make -C  $$dir clean ; \
	done

