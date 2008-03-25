#
#  Copyright (C) 2005-2006 Intel Corporation
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

package os_stats;

##
## Guest OS statistics summary for SuSE Linux Enterprise Server 9
##

our $timer_tag_va = '0xc010f920';

our $pidOffset = 0x94;              ## Offset of pid in task_struct (sched.h)
our $commOffset = 0x27d;            ## Offset of comm(ent) in task_struct


############################################################################

##
## init --
##     General initialization and tell Asim about locations of some Linux
##     kernel objects.
##
sub init($) {
    my ($osName) = @_;

    asim::set_unix_context_offsets($osName, $pidOffset, $commOffset);
    asim::set_unix_timer_tag($timer_tag_va);
}

1;
