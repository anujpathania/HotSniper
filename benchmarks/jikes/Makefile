include ../tools/hooks/buildconf.makefile

all: jikesrvm-dl data probes advice

JIKES_BUILD:=$(shell mktemp -d)/jikes

.PHONY: jikesrvm jikesrvm-dl probes

jikesrvm-dl: jikesrvm/jksvm.jar
jikesrvm/jksvm.jar:
	wget -O - http://snipersim.org/packages/jikesrvm-3.1.3-magic.tar.bz2 | tar xj

jikesrvm: jikesrvm/JikesRVM
jikesrvm/JikesRVM:
	@echo Building JikesRVM in $(JIKES_BUILD)
	mkdir -p $(JIKES_BUILD)
	wget -O - http://downloads.sourceforge.net/jikesrvm/jikesrvm-3.1.3.tar.bz2 | tar xj -C $(JIKES_BUILD) --strip-components 1
	patch -p1 -d $(JIKES_BUILD) < jikes-3.1.3-magic.patch
	cd $(JIKES_BUILD) && ant -Dhost.name=x86_64-linux -Dconfig.name=production
	rm -rf jikesrvm
	mkdir jikesrvm
	cp -r $(JIKES_BUILD)/dist/production_x86_64-linux/* jikesrvm
	rm -rf $(JIKES_BUILD)

data:
	mkdir -p data
	wget --continue --quiet --directory-prefix=data http://downloads.sourceforge.net/project/dacapobench/9.12-bach/dacapo-9.12-bach.jar
	wget --continue --quiet --directory-prefix=data http://downloads.sourceforge.net/project/dacapobench/archive/2006-10-MR2/dacapo-2006-10-MR2.jar
	wget --continue --quiet --directory-prefix=data http://snipersim.org/packages/dacapo-lucene-2-4-1-bugfree.jar

advice:
	mkdir -p advice
	./genadvice.py advice/

probes: data
	make -C probes
