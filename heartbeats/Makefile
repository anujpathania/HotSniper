CXX = /usr/bin/gcc
CXXFLAGS = -fPIC -Wall -Wno-unknown-pragmas -Iinc -Llib -O6
DBG = -g
DEFINES ?=
LDFLAGS = -shared -lpthread -lrt -lm

DOCDIR = doc
BINDIR = bin
LIBDIR = lib
INCDIR = ./inc
SCRATCH = ./scratch
OUTPUT = ./output
SRCDIR = ./src
ROOTS = application system tp lat core-allocator
TEST_ROOTS = test1 test2
BINS = $(ROOTS:%=$(BINDIR)/%)
TESTS = $(TEST_ROOTS:%=$(BINDIR)/%)
OBJS = $(ROOTS:%=$(BINDIR)/%.o)
TEST_OBJS = $(TEST_ROOTS:%=$(BINDIR)/%.o)
DEFAULT_ENERGY_LIBS = -Llib -lhb-energy-dummy
#DEFAULT_ENERGY_LIBS = -Llib -lhb-energy-msr -lm
#DEFAULT_ENERGY_LIBS = -Llib -lhb-energy-wattsup -lwattsup
#DEFAULT_ENERGY_LIBS = -Llib -lhb-energy-odroidxue -lpthread

all: $(BINDIR) $(LIBDIR) $(SCRATCH) shared $(OUTPUT) $(BINS) shared-accuracy energy shared-accuracy-power

$(BINDIR):
	-mkdir -p $(BINDIR)

$(LIBDIR):
	-mkdir -p $(LIBDIR)

$(SCRATCH):
	-mkdir -p $(SCRATCH)

$(OUTPUT):
	-mkdir -p $(OUTPUT)


$(BINDIR)/%.o : $(SRCDIR)/%.c
	$(CXX) -c $(CXXFLAGS) $(DEFINES) $(DBG) -o $@ $<


$(BINS) : $(OBJS)

$(BINS) : % : %.o
	$(CXX) $(CXXFLAGS) -o $@ $< -Llib -lhb-shared -lhrm-shared -lpthread -lrt -lm

$(TESTS) : $(TEST_OBJS)

$(TESTS) : % : %.o
	$(CXX) $(CXXFLAGS) -o $@ $< -Llib -lhb-shared -lhrm-shared -lpthread -lrt -lm


bench-tp:
	$(MAKE) clean
	$(MAKE) all DEFINES="-DSHARED"
	./bin/tp 1000 "" > $(OUTPUT)/tp_shmem_based.out
	cat $(OUTPUT)/tp_shmem_based.out
	#$(MAKE) clean
	#$(MAKE) all DEFINES="-DFILEBASED"
	#./bin/tp 1000 $(OUTPUT)/tp_file_based.log > $(OUTPUT)/tp_file_based.out
	#cat $(OUTPUT)/tp_file_based.out

bench-lat:
	$(MAKE) clean
	$(MAKE) all
	ls $(SCRATCH) | $(BINDIR)/lat 1000 $(OUTPUT)/log > $(OUTPUT)/lat_shmem_based.out
	cat $(OUTPUT)/lat_shmem_based.out

#test:
#	$(MAKE) clean
#	$(MAKE) $(BINDIR) $(SCRATCH) $(OUTPUT) $(BINS) $(TESTS)

# Power/energy monitors
energy: $(LIBDIR)/libhb-energy.so $(LIBDIR)/libhb-energy-dummy.so $(LIBDIR)/libhb-energy-msr.so $(LIBDIR)/libhb-energy-odroidxue.so $(BINDIR)/calculate-idle-power

$(LIBDIR)/libhb-energy.so: $(SRCDIR)/hb-energy-dummy.c $(SRCDIR)/hb-energy-msr.c $(SRCDIR)/hb-energy-odroidxue.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libhb-energy-dummy.so: $(SRCDIR)/hb-energy-dummy.c
	$(CXX) $(CXXFLAGS) -DHB_ENERGY_IMPL $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libhb-energy-msr.so: $(SRCDIR)/hb-energy-msr.c
	$(CXX) $(CXXFLAGS) -DHB_ENERGY_IMPL $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

#$(LIBDIR)/libhb-energy-wattsup.so: $(SRCDIR)/hb-energy-wattsup.c
#	$(CXX) $(CXXFLAGS) -DHB_ENERGY_IMPL $(LDFLAGS) -lwattsup -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libhb-energy-odroidxue.so: $(SRCDIR)/hb-energy-odroidxue.c
	$(CXX) $(CXXFLAGS) -DHB_ENERGY_IMPL $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(BINDIR)/calculate-idle-power: $(SRCDIR)/calculate-idle-power.c
	$(CXX) $(CXXFLAGS) -DHB_ENERGY_IMPL -o $@ $? $(DEFAULT_ENERGY_LIBS) -lrt

# Heartbeat shared memory version
shared: $(LIBDIR)/libhb-shared.so $(LIBDIR)/libhrm-shared.so

shared-accuracy: $(LIBDIR)/libhb-acc-shared.so

shared-accuracy-power: $(LIBDIR)/libhb-acc-pow-shared.so

$(LIBDIR)/libhb-shared.so: $(SRCDIR)/heartbeat-shared.c $(SRCDIR)/heartbeat-util-shared.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libhb-acc-shared.so: $(SRCDIR)/heartbeat-accuracy-shared.c $(SRCDIR)/heartbeat-util-shared.c
	$(CXX) $(CXXFLAGS) -DHEARTBEAT_MODE_ACC $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libhb-acc-pow-shared.so: $(SRCDIR)/heartbeat-accuracy-power-shared.c $(SRCDIR)/heartbeat-util-shared.c
	$(CXX) $(CXXFLAGS) -DHEARTBEAT_MODE_ACC_POW $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

$(LIBDIR)/libhrm-shared.so: $(SRCDIR)/heart_rate_monitor-shared.c
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -Wl,-soname,$(@F) -o $@ $^

# Installation
install: all
	install -m 0644 lib/*.so /usr/local/lib/
	mkdir -p /usr/local/include/heartbeats
	install -m 0644 inc/* /usr/local/include/heartbeats/

uninstall:
	rm -f /usr/local/lib/libhb-*.so
	rm -f /usr/local/lib/libhrm-*.so
	rm -rf /usr/local/include/heartbeats/

## cleaning
clean:
	-rm -rf $(BINDIR) $(LIBDIR) $(SCRATCH) *.log *~ $(SRCDIR)/*~

squeaky: clean uninstall
	-rm -rf $(OUTPUT)

# Documentation
documentation:
	doxygen heartbeats_doc

clean-documentation:
	-rm -rf $(DOCDIR)/html/*
	-rm -rf $(DOCDIR)/latex/*
