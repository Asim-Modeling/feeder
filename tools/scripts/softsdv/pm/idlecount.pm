#
#  Copyright (C) 2006 Intel Corporation
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

use asim;
use strict;

package idlecount;

##
## Count idle cycles using the ISAX idlecount module.
##


sub load() {
    ssdv::execute_command("kernel", "vpc_command", "loadmod idlecount");
# Caller must do the following after all ISAX modules are loaded:
#    ssdv::execute_command("kernel", "vpc_command", "vpc.cpu isax_init");
}

sub start() {
    ssdv::execute_command("idlecount", "start");
}

sub stop() {
    ssdv::execute_command("idlecount", "stop");
}

sub resetCounters() {
    ssdv::execute_command("idlecount", "reset");
}

sub idleCycles() {
    return asim::read_data("idlecount", "data", "cycles");
}

sub idleUserCycles() {
    return asim::read_data("idlecount", "data", "user_cycles");
}

sub idleKernelCycles() {
    return asim::read_data("idlecount", "data", "kernel_cycles");
}

1;
