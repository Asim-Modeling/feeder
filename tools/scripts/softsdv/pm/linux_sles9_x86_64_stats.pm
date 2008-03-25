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
use os_stats_summary;
use strict;

package os_stats;

############################################################################
#
# OS dependent addresses and offsets
#
############################################################################

our $hz = 1024;                     ## From kernel config

##
## Guest OS statistics summary for SuSE Linux Enterprise Server 9
##

our $timer_interrupt_va = '0xffffffff80117200';
our $local_timer_interrupt_va = '0xffffffff8011d5e0';

##
## VA in __switch_to immediately after %gs:0 is updated.  This is
## cpu_pda[].pcurrent.
##
our $switch_to_ip = '0xffffffff8010f106';

##
## PDA -- the base of the array of per processor data.  %gs points
## to an entry in the cpu_pda[] array in the kernel.  When not in the
## kernel it is stored in the KERNEL_GS_BASE MSR.  There is a small
## window on entry and exit from ring 0 where %gs may point to the
## user space %gs.  The MSR and %gs are toggled with a swapgs on
## entry and exit from the kernel.  Knowing the address of cpu_pda
## can help detect the transition.
##
our $cpu_pda_va = '0xffffffff80562580';
our $cpu_pda_size = 0x80;       # sizeof(cpu_pda[0])
our $cpu_pda_pcurrent = 0;      # cpu_pda[].pcurrent (struct task_struct *)
our $cpu_pda_data_offset = 8;   # cpu_pda[].data_offset (per-CPU data)

##
## Offsets in per-CPU data from cpu_pda[<cpu number>].data_offset.
##
##   **** Don't put 64 bit numbers here.  32 bit perl will turn them
##   **** into floating point!
our $per_cpu__kstat_offset = -0x7fa5c200;      # 0xffffffff805a3e00
our $per_cpu__runqueues_offset = -0x7fa5de00;  # 0xffffffff805a2200

our $runqueues_nr_switches = 0x18;  ## Offset to nr_switches in runqueues

our $pidOffset = 0xf4;              ## Offset of pid in task_struct (sched.h)
our $commOffset = 0x479;            ## Offset of comm(ent) in task_struct


##
## Block subsys has a list of block devices, including the disk drives.
##
our $block_subsys_va = '0xffffffff803f8c00';
our $block_subsys_list_offset = 0x10;

our $gendisk_list_next_offset = 0xc8;    ## Pointer to next gendisk
our $gendisk_major_offset = 0;           ## major disk number
our $gendisk_minors_offset = 0x8;        ## max minor disk numbers
our $gendisk_disk_name_offset = 0xc;
our $gendisk_part_offset = 0x30;         ## partition table
our $gendisk_dkstats_offset = 0x120;     ## dkstats

our $dkstats_read_sectors_offset = 0;
our $dkstats_reads_offset = 0x8;
our $dkstats_read_merges_offset = 0x10;
our $dkstats_read_ticks_offset = 0x18;
our $dkstats_write_sectors_offset = 0x4;
our $dkstats_writes_offset = 0xc;
our $dkstats_write_merges_offset = 0x14;
our $dkstats_write_ticks_offset = 0x1c;

##
## Timers
##
our $tick_nsec = '0xffffffff803dc810';
my $tick_seconds = undef;               # Filled in by first call to ktime

############################################################################
############################################################################

# Forward declarations of helper functions
sub kv2p($);
sub per_cpu_data($);

############################################################################
############################################################################

##
## init --
##     General initialization and tell Asim about locations of some Linux
##     kernel objects.
##
sub init($) {
    my ($osName) = @_;

    asim::set_unix_context_offsets($osName, $pidOffset, $commOffset);
    asim::set_unix_pda_info($cpu_pda_va, $switch_to_ip);;
    asim::set_unix_timer_tag($timer_interrupt_va);
    asim::set_unix_local_timer_tag($local_timer_interrupt_va);
}


##
## get_stats_snapshot() --
##     Returns a 6 element array with a snapshot of the current user,
##     system, idle and io wait times as well as disk blocks read and written.
##
sub get_stats_snapshot() {
    my %summary;

    foreach my $cpu ( 0 .. asim::num_cpus()-1 ) {
        my $kstat = kstat_pa($cpu);

        # user + nice
        $summary{'user'}[$cpu] = ktime($kstat, 0) + ktime($kstat, 1);

        # system + softirq + irq
        $summary{'system'}[$cpu] = ktime($kstat, 2) + ktime($kstat, 3) + ktime($kstat, 4);

        # idle
        $summary{'idle'}[$cpu] = ktime($kstat, 5);

        # I/O wait
        $summary{'io_wait'}[$cpu] = ktime($kstat, 6);

        # Number of context switches
        $summary{'ctx'}[$cpu] = context_switches_cpu($cpu);
    }

    my %disk = get_disk_io_stats();

    # Merge disk I/O stats into summary
    @summary{keys %disk} = values %disk;

    return %summary;
}


############################################################################
############################################################################

##
## Private functions
##

##
## kv2p --
##     Convert string representation of a kernel virtual address to physical.
##     We want to be able to do the conversion even when not in the kernel.
##     Use knowledge of the kernel's physical mapping instead of looking
##     it up.
##
sub kv2p($) {
    my ($a) = @_;
    if ($a =~ /^0x0+$/) {
        # NULL
        $a = 0;
    }
    elsif ($a =~ /^0x[fF]+8/) {
        # 0xffffffff8vvvvvvv -> 0xvvvvvvv
        $a =~ s/^0x[fF]+8/0x/;
    }
    elsif ($a =~ /^0x0*1/) {
        # 0x000001vvvvvvvvvv -> 0xvvvvvvvvvv
        $a =~ s/^0x0*1/0x/;
    }
    else {
        print STDERR "kv2p: Unexpected kernel VA (${a})\n";
    }
    return hex $a;
}


##
## ktime --
##     Pass in the PA of the kernel statistics struct and an index for
##     the position of the counter in the kernel_stat.  E.g. the index
##     of user is 0, idle is 5.
##
##     Returns the time in seconds as an fp value.
##
sub ktime($$) {
    my ($kstat, $idx) = @_;

    if (! defined($tick_seconds)) {
        $tick_seconds = asim::read_memory(kv2p($tick_nsec), 8) * 1e-9;
    }

    return asim::read_memory($kstat + $idx * 8, 8) * $tick_seconds;
}


##
## context_switches --
##     Find the number of context switches for the CPU.
##
sub context_switches_cpu($) {
    my ($cpu) = @_;
    my $runq = per_cpu_data_pa($cpu) + $per_cpu__runqueues_offset;

    return asim::read_memory($runq + $runqueues_nr_switches, 8);
}


##
## kstat_pa --
##     Return the PA of a CPU's kernel_stat from the per-CPU section.
##
sub kstat_pa($) {
    my ($cpu) = @_;
    return per_cpu_data_pa($cpu) + $per_cpu__kstat_offset;
}


##
## runqueues_pa --
##     Return the PA of a CPU's run queues from the per-CPU section.
##
sub runqueues_pa($) {
    my ($cpu) = @_;
    return per_cpu_data_pa($cpu) + $per_cpu__runqueues_offset;
}


##
## per_cpu_data_pa --
##     Return the PA of a CPU's per-CPU data section.
##
sub per_cpu_data_pa($) {
    my ($cpu) = @_;

    return 0 if ($cpu > asim::num_cpus());

    my $pda = kv2p($cpu_pda_va);

    # Get the cpu_pda indexed by $cpu
    $pda += $cpu * $cpu_pda_size;

    # Read and translate the data_offset pointer
    my $data = asim::read_hex_memory($pda + $cpu_pda_data_offset, 8);
    return kv2p($data);
}


##
## get_disk_io_stats --
##     Read disk I/O statistics from the kernel.
##
sub get_disk_io_stats() {
    my %summary;

    # Disks are all on a list headed by block_subsys
    my $head = kv2p($block_subsys_va) + $block_subsys_list_offset;
    my $p = kv2p(asim::read_hex_memory($head, 8));
    my $idx = 0;
    while ($p != $head) {
        my $gd = $p - $gendisk_list_next_offset;
        # Only disks have partition tables
        my $part = kv2p(asim::read_hex_memory($gd + $gendisk_part_offset, 8));
        if ($part != 0) {
            my $name = get_string_from_pa($gd + $gendisk_disk_name_offset);
            $summary{'d_name'}[$idx] = $name;

            # ~dkstats (NOTE INVERSION!) is a pointer to an array of pointers
            # to per-CPU disk_stats
            my $dkstats_arr = asim::read_hex_memory($gd + $gendisk_dkstats_offset, 8);
            $dkstats_arr = kv2p(invert_64($dkstats_arr));

            $summary{'r_sectors'}[$idx] = 0;
            $summary{'r_io'}[$idx] = 0;
            $summary{'r_time'}[$idx] = 0;
            $summary{'w_sectors'}[$idx] = 0;
            $summary{'w_io'}[$idx] = 0;
            $summary{'w_time'}[$idx] = 0;

            for (my $c = 0; $c < asim::num_cpus(); $c++) {
                my $dkstats = kv2p(asim::read_hex_memory($dkstats_arr + $c * 8, 8));

                $summary{'r_sectors'}[$idx] += asim::read_memory($dkstats + $dkstats_read_sectors_offset, 4);
                $summary{'r_io'}[$idx] += asim::read_memory($dkstats + $dkstats_reads_offset, 4) +
                                          asim::read_memory($dkstats + $dkstats_read_merges_offset, 4);
                # Disk I/O time is measured in "jiffies".  A jiffy is on HZ, as
                # defined in the kernel config.
                $summary{'r_time'}[$idx] += (asim::read_memory($dkstats + $dkstats_read_ticks_offset, 4) / $hz);

                $summary{'w_sectors'}[$idx] += asim::read_memory($dkstats + $dkstats_write_sectors_offset, 4);
                $summary{'w_io'}[$idx] += asim::read_memory($dkstats + $dkstats_writes_offset, 4) +
                                          asim::read_memory($dkstats + $dkstats_write_merges_offset, 4);
                $summary{'w_time'}[$idx] += (asim::read_memory($dkstats + $dkstats_write_ticks_offset, 4) / $hz);
            }

            $idx += 1;
        }
        $p = kv2p(asim::read_hex_memory($p, 8));    # next
    }

    return %summary;
}


sub get_string_from_pa($) {
    my ($pa) = @_;
    my $s = "";
    while (1) {
        my $c = asim::read_memory($pa, 1);
        last if ($c == 0);
        $s .= chr($c);
        $pa += 1;
    }
    return $s;
}


##
## invert_64 --
##     Takes a 64 bit hex STRING as input.  Returns a bit inverted hex string.
##
sub invert_64($) {
    my ($s) = @_;

    $s =~ s/^0x//;

    # This code works on 32 and 64 bit versions of Perl
    if (length($s) <= 8) {
        return sprintf "0xffffffff%08x", ((~ hex $s) & 0xffffffff);
    }
    else {
        my $low = substr($s, -8, 8);
        my $high = substr($s, 0, length($s) - 8);
        return sprintf "0x%08x%08x",
            ((~ hex $high) & 0xffffffff),
            ((~ hex $low) &  0xffffffff);
    }
}

1;
