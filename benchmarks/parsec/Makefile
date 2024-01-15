all : parsec-build
HOOKS_DIR=../tools/hooks/

parsec-core : parsec-2.1/.parsec_source
parsec-2.1/.parsec_source :
	tar xzf parsec-2.1-core.tar.gz --keep-old-files ; true

	# parsec patches
	patch -p1 -dparsec-2.1 < patches/parsec-ocreat.patch
	patch -p0 -dparsec-2.1/pkgs/apps/blackscholes/src < patches/blackscholes.c.patch
	patch -p1 -dparsec-2.1 < patches/streamcluster_barrier.patch

	# this patch will fail to apply	
	patch -p0 < patches/ferret-deadlock.patch 
	
	# updated patch files
	patch -p0 < patches/blackscholes.tbb.cpp.patch 

	# hotsniper patches
	patch -p1 -dparsec-2.1 < patches/ssl-asm.patch
	patch -p1 -dparsec-2.1 < patches/raytrace-unistd-gcc-fix.patch
	patch -p3 -dparsec-2.1 < patches/raytrace-Xext-fix.patch
	patch -p1 -dparsec-2.1 < patches/heartbeat.patch
	patches/fixPODSyntax.sh
	# Once the download succeeded and we applied all patches, mark as done
	touch parsec-2.1/.parsec_source

parsec-input-sim : parsec-2.1/pkgs/apps/blackscholes/inputs/input_simsmall.tar
parsec-2.1/pkgs/apps/blackscholes/inputs/input_simsmall.tar :
	tar xzf parsec-2.1-sim.tar.gz

parsec-dependencies :
	./checkdependencies.py

parsec-build : parsec-dependencies parsec-core parsec-input-sim
	make -C ${HOOKS_DIR}
	cd parsec-2.1; ./bin/parsecmgmt -a build -p bench -c gcc-sniper
	cd parsec-2.1; ./bin/parsecmgmt -a clean -c gcc-sniper

clean :
	make -C ${HOOKS_DIR} clean
	cd parsec-2.1; ./bin/parsecmgmt -a uninstall -p all -c gcc-sniper
