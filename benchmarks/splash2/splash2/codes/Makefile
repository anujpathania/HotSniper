SPLASH_ROOT = $(CURDIR)

include ${SPLASH_ROOT}/../../../tools/hooks/buildconf.makefile

CC = ${HOOKS_CC}
CXX = ${HOOKS_CXX}
LD ?= ${CC}
CFLAGS = -g -O3 -I${SPLASH_ROOT} -I${SPLASH_ROOT}/pthreads ${HOOKS_CFLAGS}
LDFLAGS = -lm -pthread ${HOOKS_LDFLAGS}
MACROS = ${SPLASH_ROOT}/pthreads/c.m4.null.POSIX

all:
	$(MAKE) -C ${HOOKS_DIR}
	for DIR in `find apps kernels -name \[Mm\]akefile|sed -e 's+/[Mm]akefile++'|sort -r`; do CC="$(CC)" AR="$(AR)" LD="$(LD)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" MACROS="$(MACROS)" TARGET_ARCH="" $(MAKE) -C $$DIR all; if [ $$? -eq 0 ]; then echo; else exit -1; fi; done

clean:
	$(MAKE) -C ${HOOKS_DIR} clean
	for DIR in `find apps kernels -name \[Mm\]akefile|sed s+/[Mm]akefile++|sort -r`; do $(MAKE) -C $$DIR clean; done
