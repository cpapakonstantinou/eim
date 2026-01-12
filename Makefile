#Target options
TARGET = eim
TARGET_EIM_LOG=eim.csv
TARGET_EIM_IMG=eim.png
TARGET_MODE_LOG=mode2D.csv
TARGET_MODE_IMG=mode2D.png
TARGET_MODE_MODE=TE0
TARGET_MODE_WIDTH=0.5
TARGET_MODE_TRIB=0.22
TARGET_MODE_TSLAB=0
SRC = eim.cc

PREFIX ?= /usr/bin
INSTALLDIR ?= $(PREFIX)

#Unit test options
TEST_TARGET = test_app
TEST_SRC = slab.cc
TEST_EXTRA_OBJ = 
TEST_OUT = 
TEST_LOG=slab.csv
TEST_IMG=slab.png

#Directories
SRCDIR = src
INCDIR = inc
TESTDIR = test

#Toolchains
CXX = g++-11
CXX_SUFFIX = cc
LD = $(CXX)

#Compile Options
ENABLE_PARALLEL=1
OPT=-O3
CXXFLAGS = -std=c++23 $(OPT) -DPARALLEL=$(ENABLE_PARALLEL) -march=native -I$(INCDIR) -I /usr/include/carray
LDFLAGS = 
LDLIBS =
TEST_LDFLAGS = 
TEST_LDLIBS = -ltdd

ifeq ($(ENABLE_PARALLEL), 1)
    LDLIBS += -ltbb
    TEST_LDLIBS += -ltbb
endif

#Shell type
SHELL := /bin/bash

#build rules
OBJ = $(SRC:%.$(CXX_SUFFIX)=$(SRCDIR)/%.o)
TEST_OBJ = $(TEST_SRC:%.$(CXX_SUFFIX)=$(TESTDIR)/%.o)

#text editing
define add_section 
	$(shell sed -i 's|$(1)[ ]*=.*|$(1)="$(2)"|' $(3))
endef

all: $(TARGET)

$(TARGET): $(OBJ) 
	$(LD) -o $@ $(OBJ) $(LDFLAGS) $(LDLIBS)

$(TEST_TARGET): $(TEST_OBJ)
	$(LD) -o $@ $(TEST_OBJ) $(TEST_EXTRA_OBJ) $(TEST_LDFLAGS) $(TEST_LDLIBS)

plot_slab:
	$(call add_section,ifile,$(TEST_LOG),plt/slab.gp)
	$(call add_section,ofile,$(TEST_IMG),plt/slab.gp)
	gnuplot --persist plt/slab.gp

plot_mode:
	$(call add_section,ifile,$(TARGET_MODE_LOG),plt/mode.gp)
	$(call add_section,ofile,$(TARGET_MODE_IMG),plt/mode.gp)
	$(call add_section,mode,$(TARGET_MODE_MODE),plt/mode.gp)
	$(call add_section,width,$(TARGET_MODE_WIDTH),plt/mode.gp)
	$(call add_section,t_rib,$(TARGET_MODE_TRIB),plt/mode.gp)
	$(call add_section,t_slab,$(TARGET_MODE_TSLAB),plt/mode.gp)

	gnuplot --persist plt/mode.gp

plot_eim:
	$(call add_section,ifile,$(TARGET_EIM_LOG),plt/eim.gp)
	$(call add_section,ofile,$(TARGET_EIM_IMG),plt/eim.gp)
	gnuplot --persist plt/eim.gp

clean:
	$(RM) $(SRCDIR)/*.o $(TESTDIR)/*.o $(foreach var,$(filter TARGET_%_IMG,$(.VARIABLES)),$($(var))) $(TEST_IMG)

cleanall: clean
	$(RM) $(TARGET) $(foreach var,$(filter TARGET_%_LOG,$(.VARIABLES)),$($(var))) $(TEST_TARGET) $(TEST_LOG) *.dat


install: $(TARGET)
	install -m 755 $(TARGET) $(INSTALLDIR)

uninstall:
	$(RM) -r $(INSTALLDIR)/$(TARGET)

.PHONY: all clean help

.DEFAULT_GOAL := $(TARGET)
