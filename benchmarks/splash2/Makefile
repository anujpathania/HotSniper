HOOKS_DIR=../tools/hooks/

all : splash2-build

splash2-build :
	make -C ${HOOKS_DIR}
	cd splash2/codes; make

clean :
	make -C ${HOOKS_DIR} clean
	cd splash2/codes; make clean
