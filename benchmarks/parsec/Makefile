# We download the parsec 2.1 files from Figshare
PARSEC_2.1_CORE_URL=https://uvaauas.figshare.com/ndownloader/files/45183391
PARSEC_2.1_SIMSMALL_URL=https://uvaauas.figshare.com/ndownloader/files/45183310

all : parsec-build
HOOKS_DIR=../tools/hooks/

parsec-core : parsec-2.1/.parsec_source
parsec-2.1/.parsec_source :
<<<<<<< HEAD
	wget -O - $(PARSEC_2.1_CORE_URL) | tar xz --keep-old-files ; true
	patch -p1 -dparsec-2.1 < patches/bugfixes/parsec-ocreat.patch
	patch -p1 -dparsec-2.1 < patches/bugfixes/ferret-deadlock.patch 
	patch -p0 -dparsec-2.1/pkgs/apps/blackscholes/src < patches/bugfixes/blackscholes.c.patch 
	patch -p0 -dparsec-2.1/pkgs/apps/blackscholes/src < patches/bugfixes/blackscholes.tbb.cpp.patch 
	patch -p1 -dparsec-2.1 < patches/bugfixes/streamcluster_barrier.patch 
=======
	tar xzf parsec-2.1-core.tar.gz --keep-old-files ; true

	# parsec patches
	patch -p1 -dparsec-2.1 < patches/parsec-ocreat.patch
	patch -p0 -dparsec-2.1/pkgs/apps/blackscholes/src < patches/blackscholes.c.patch
	patch -p1 -dparsec-2.1 < patches/streamcluster_barrier.patch

	# updated parsec patches
	patch -p0 < patches/ferret-deadlock.patch 
	patch -p0 < patches/blackscholes.tbb.cpp.patch 

	# hotsniper patches
>>>>>>> main
	patch -p1 -dparsec-2.1 < patches/ssl-asm.patch
	patch -p1 -dparsec-2.1 < patches/raytrace-unistd-gcc-fix.patch
	patch -p3 -dparsec-2.1 < patches/raytrace-Xext-fix.patch
	patch -p1 -dparsec-2.1 < patches/heartbeat.patch
	patches/fixPODSyntax.sh
	# Once the download succeeded and we applied all patches, mark as done
	touch parsec-2.1/.parsec_source

parsec-input-sim : parsec-2.1/pkgs/apps/blackscholes/inputs/input_simsmall.tar
parsec-2.1/pkgs/apps/blackscholes/inputs/input_simsmall.tar :
<<<<<<< HEAD
	wget -O - $(PARSEC_2.1_SIMSMALL_URL) | tar xz

parsec-input-native : parsec-2.1/pkgs/apps/blackscholes/inputs/input_native.tar
parsec-2.1/pkgs/apps/blackscholes/inputs/input_native.tar :
	echo "native inputs are not available"; false
=======
	tar xzf parsec-2.1-sim.tar.gz
>>>>>>> main

parsec-dependencies :
	./checkdependencies.py

parsec-build : parsec-dependencies parsec-core parsec-input-sim
	make -C ${HOOKS_DIR}
	cd parsec-2.1; ./bin/parsecmgmt -a build -p bench -c gcc-sniper
	cd parsec-2.1; ./bin/parsecmgmt -a clean -c gcc-sniper

clean :
	make -C ${HOOKS_DIR} clean
	cd parsec-2.1; ./bin/parsecmgmt -a uninstall -p all -c gcc-sniper
