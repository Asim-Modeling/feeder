rem
rem Copyright (C) 2003-2006 Intel Corporation
rem 
rem This program is free software; you can redistribute it and/or
rem modify it under the terms of the GNU General Public License
rem as published by the Free Software Foundation; either version 2
rem of the License, or (at your option) any later version.
rem 
rem This program is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem GNU General Public License for more details.
rem 
rem You should have received a copy of the GNU General Public License
rem along with this program; if not, write to the Free Software
rem Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
rem 

rem
rem  This is the script that runs in Windows just after booting.  It
rem  copies in some tools from the host and sends an SSB mark.  After
rem  the mark it loads a workload-specific script and runs it.
rem

rem Copy tools to c:\asim_softsdv

mkdir c:\asim_softsdv
set PATH=c:\asim_softsdv;%PATH%
ssc_guest host:XXROOTXX/softsdv_XXARCHXX/scripts/client/win32/force_exit.exe c:\asim_softsdv\.
ssc_guest host:XXROOTXX/softsdv_XXARCHXX/scripts/client/win32/mark_send.exe c:\asim_softsdv\.
ssc_guest host:XXROOTXX/softsdv_XXARCHXX/scripts/client/win32/sleep.exe c:\asim_softsdv\.
ssc_guest host:XXROOTXX/softsdv_XXARCHXX/scripts/client/win32/unzip.exe c:\asim_softsdv\.

mark_send fast_clock
rem Sleeping 300 (fast) seconds while machine quiets down...
sleep 300
mark_send slow_clock
rem Sleeping 2 (slow) seconds while machine quiets down...
sleep 2

rem Done.  Save a checkpoint.
mark_send booted

echo off

rem SoftSDV bug sometimes misses the first mark.  Repeat boot.
mark_send booted

rem Asim expects benchmark to ask what to do.  This script ignores the answer.
mark_send num

rem Get and invoke the workload.
mkdir c:\asim
cd \asim
ssc_guest host:AUTO/benchmark.cmd .
ssc_guest host:AUTO/benchmark.zip .
unzip benchmark.zip

mark_send start
cmd /q /c benchmark.cmd
mark_send end

echo on

rem We are done.  Stop simulating!
mark_send end
mark_send done

sleep 5
force_exit
