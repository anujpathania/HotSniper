all : parsec-build
HOOKS_DIR=../tools/hooks/

parsec-core : parsec-2.1/.parsec_source
parsec-2.1/.parsec_source :
	wget -O - http://parsec.cs.princeton.edu/download/2.1/parsec-2.1-core.tar.gz | tar xz --keep-old-files ; true
	wget -O - -q http://parsec.cs.princeton.edu/download/2.1/bugfixes/parsec-ocreat.patch | patch -p1 -dparsec-2.1
	wget -O - -q http://parsec.cs.princeton.edu/download/2.1/bugfixes/ferret-deadlock.patch | patch -p1 -dparsec-2.1
	wget -O - -q http://parsec.cs.princeton.edu/download/2.1/bugfixes/blackscholes.c.patch | patch -p0 -dparsec-2.1/pkgs/apps/blackscholes/src
	wget -O - -q http://parsec.cs.princeton.edu/download/2.1/bugfixes/blackscholes.tbb.cpp.patch | patch -p0 -dparsec-2.1/pkgs/apps/blackscholes/src
	wget -O - -q http://parsec.cs.princeton.edu/download/2.1/bugfixes/streamcluster_barrier.patch | patch -p1 -dparsec-2.1
	patch -p1 -dparsec-2.1 < patches/ssl-asm.patch
	patch -p1 -dparsec-2.1 < patches/raytrace-unistd-gcc-fix.patch
	patch -p3 -dparsec-2.1 < patches/raytrace-Xext-fix.patch
	patches/fixPODSyntax.sh
	# Once the download succeeded and we applied all patches, mark as done
	touch parsec-2.1/.parsec_source

parsec-input-sim : parsec-2.1/pkgs/apps/blackscholes/inputs/input_simsmall.tar
parsec-2.1/pkgs/apps/blackscholes/inputs/input_simsmall.tar :
	wget -O - http://parsec.cs.princeton.edu/download/2.1/parsec-2.1-sim.tar.gz | tar xz

parsec-input-native : parsec-2.1/pkgs/apps/blackscholes/inputs/input_native.tar
parsec-2.1/pkgs/apps/blackscholes/inputs/input_native.tar :
	wget -O - http://parsec.cs.princeton.edu/download/2.1/parsec-2.1-native.tar.gz | tar xz

parsec-dependencies :
	./checkdependencies.py

parsec-build : parsec-dependencies parsec-core parsec-input-sim
	make -C ${HOOKS_DIR}
	cd parsec-2.1; ./bin/parsecmgmt -a build -p bench -c gcc-sniper
	cd parsec-2.1; ./bin/parsecmgmt -a clean -c gcc-sniper

clean :
	make -C ${HOOKS_DIR} clean
	cd parsec-2.1; ./bin/parsecmgmt -a uninstall -p all -c gcc-sniper
