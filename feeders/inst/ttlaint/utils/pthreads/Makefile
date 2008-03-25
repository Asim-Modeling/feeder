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

AINTDIR=../..
INCDIR=$(AINTDIR)/include

ifeq ($(SITE), CRL)
  CC = gcc-osf4
else
  CC = gcc -g -nostdinc -I/usr/include 
endif

CXX=cxx

AINT_PTHREAD_LIBS = libpthread-aint.a libpthread-null.a

all: $(AINT_PTHREAD_LIBS)

AS = /usr/ucb/as
OSVER = $(shell uname -r | sed -e s/V// -e s/\\.//)

OBJS_libpthreadaint = pthread.o pthread_stubs.o simcontrol-aint.o warn.o
OBJS_libpthreadnull = simcontrol-null.o

DEFINES = -DOSVER=$(OSVER)
CFLAGS = -I$(INCDIR) $(DEFINES) -D_THREAD_SAFE

libpthread-aint.a: $(OBJS_libpthreadaint)
	ar -r libpthread-aint.a $(OBJS_libpthreadaint)

libpthread-null.a: $(OBJS_libpthreadnull)
	ar -r libpthread-null.a $(OBJS_libpthreadnull)

warn.o: warn.c
	$(CXX) -c $(CFLAGS) warn.c -o warn.o
.c.o: $<
	$(CC) -S $(CFLAGS) $(INCLUDES) -o $(*).s $<
	$(AS) -o $(*).o $(*).s
	rm -f $(*).s

clean:
	-rm -f core *.o simple hello $(AINT_PTHREAD_LIBS)
