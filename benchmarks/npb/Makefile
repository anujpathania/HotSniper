npb_make: npb_deps
	$(MAKE) -C NPB3.3.1/NPB3.3-OMP suite

npb_deps: NPB3.3.1/Changes.log
NPB3.3.1/Changes.log:
	wget -O - http://www.snipersim.org/packages/NPB3.3.1.tar.gz | tar xz --keep-old-files ; true

clean:
	$(MAKE) -C NPB3.3.1/NPB3.3-OMP clean
	rm -rf NPB3.3.1/NPB3.3-OMP/bin/*

.PHONY: npb_deps npb_make
