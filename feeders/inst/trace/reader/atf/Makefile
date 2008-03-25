#
#  Copyright (C) 2001-2006 Intel Corporation
#  
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#  
#

##########################################################################
#
# Standalong atf Makefile. This file is NOT under ASIM builder control. 
# This Makefile is for local atf feeder testing purposes only.
#
# Author: Pritpal Ahuja and Artur Klauser
#
##########################################################################

#-----------------------------------------------------------------------------
# set defaults for make configuration
#-----------------------------------------------------------------------------
GNU = 1
OPT = 0
DEBUG = 1
STATIC = 0
WARN = 1
PAR = 0

#-----------------------------------------------------------------------------
# top-level make configuration
#-----------------------------------------------------------------------------
include Makefile.config

#-----------------------------------------------------------------------------
# includes and defines
#-----------------------------------------------------------------------------
ifndef ASIM_INCDIRS
$(error You need to define ASIM_INCDIRS for this Makefile to work)
endif
FEEDDIR=.

#-----------------------------------------------------------------------------
# TARGET and required components
#-----------------------------------------------------------------------------
ATF_TEST=atf_test

#-----------------------------------------------------------------------------
# paths to sources
#-----------------------------------------------------------------------------
VPATH=$(FEEDDIR)

#-----------------------------------------------------------------------------
# build rules
#-----------------------------------------------------------------------------
.PHONY: all
all : dirs $(ATF_TEST)

INCDIRS = -I$(FEEDDIR) $(ASIM_INCDIRS)

ATF_TEST_OBJ = atf_test.o atf.o atfutil.o disasm_test.o
ATF_TEST_OOBJS := $(addprefix obj/, $(ATF_TEST_OBJ))

$(ATF_TEST): $(ATF_TEST_OOBJS)
	@rm -f $(ATF_TEST)
	$(CXX) -o $(ATF_TEST) $(ATF_TEST_OOBJS)

# generate both .o files and .d files for #include dependencies;
# we move the .d files into a separate directory, however, for clarity;
obj/%.o : %.cpp
	$(CXX) $(MKDEPEND) -c -o $@ $(CFLAGS) $(INCDIRS) $<
	@mv -f $(DEPEND_FILE) .depend/$(notdir $(basename $@)).d

#-----------------------------------------------------------------------------
# Utility rules.
#-----------------------------------------------------------------------------
.PHONY: dirs
dirs: .depend obj
.depend:
	@mkdir .depend
obj:
	@mkdir obj

.PHONY: clean
clean :
	@rm -rf obj
	@rm -rf .depend
	@rm -rf cxx_repository

.PHONY: realclean
realclean : clean
	rm -f $(ATF_TEST)

# include all generated dependencies
-include .depend/*.d
