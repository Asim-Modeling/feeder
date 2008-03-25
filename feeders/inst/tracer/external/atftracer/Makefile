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


#CXX=cxx
CXX=g++
#CDEFS=-DHOST_FREEBSD_X86
CDEFS=-DHOST_DUNIX -I../../ -I../../../atf -I../../../../../asim-core/base
#CFLAGS=-O2 ${CDEFS}
CFLAGS=-g ${CDEFS}

TARGET=atftracer
ATOBJS=atftracer.o atf.o atfutil.o disasm.o

.PHONY: all
all : $(TARGET)

$(TARGET) : $(ATOBJS)
	$(CXX) $(ATOBJS) -o atftracer

%.o : %.cpp
	$(CXX) $(MKDEPEND) -c -o $@ $(CFLAGS) $(INCDIRS) $<

#atftracer.o : mpipe.h

