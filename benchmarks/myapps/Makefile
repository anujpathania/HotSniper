
.PHONY: all

include ../tools/hooks/buildconf.makefile

SUBDIRS = $(shell ls -d */)


all:
	make -C pi
	make -C my_pi
	make -C openssl -j8
	make -C aes

# all:
# 	for dir in $(SUBDIRS) ; do \
# 		make -C  $$dir ; \
# 	done

clean:
	for dir in $(SUBDIRS) ; do \
		make -C  $$dir clean ; \
	done

