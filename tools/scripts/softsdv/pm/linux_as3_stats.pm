#
#  Copyright (C) 2004-2006 Intel Corporation
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

##
## Guest OS statistics summary for RH Linux AS3 2.4.21-3.EL workload.
##
## Usage model:
##    The package is designed for computing resource usage between two
##    samples.  Call get_stats_snapshot at the beginning and end of a
##    range.  Each call returns an array of values representing the state.
##    Functions are defined (e.g. user_fraction) to compute run time,
##    and disk I/O in the region.  A stats_summary() function also exists
##    to generate a string summarizing the region suitable for printing.
##

our $kstat = 0x4be7e00;             ## Physical address of kstat_percpu;
our $runqueues = 0x4a78400;         ## PA of runqueues
our $gendisk_head = 0x4b75090;      ## PA of gendisk_head
our $hz = 1024;                     ## From kernel config

our $timer_tag_va = '0xe000000004432740';
our $idle_tag_va  = '0xe000000004415fb1';

our $pidOffset = 268;               ## Offset of pid in task_struct (sched.h)
our $commOffset = 1828;             ## Offset of comm(ent) in task_struct


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
    asim::set_unix_idle_tag($idle_tag_va);
}


############################################################################
##
## The kstat_percpu structure holds both process timing and disk I/O information.
##
## The base address of kstat_percpu is 0xe000000004be7e00 (0x4be7e00 phys).
##
## It looks like this:
##
## /* We only roll the accounted times into the kstat struct at each timer tick,
##  * this way we can handle both processes with and without accounting enabled
##  * on them.  We use our accumulated accounting time at each tick to subtract
##  * from the number of usecs that *would* be handed out by the timer tick
##  * accounting, that way we don't get accounting errors.
##  */
## struct kernel_stat_tick_times {
##      unsigned long u_usec;           /* user time */
##      unsigned long n_usec;           /* nice user time */
##      unsigned long s_usec;           /* system time */
##      unsigned long irq_usec;
##      unsigned long softirq_usec;
##      unsigned long iowait_usec;
##      unsigned long idle_usec;
## };
## 
## struct kernel_stat_percpu {
##      struct kernel_timeval user, nice, system;
##      struct kernel_timeval irq, softirq, iowait, idle;      // 24 bytes (offset)
##      struct kernel_stat_tick_times accumulated_time;        // 56
##      struct task_struct *unaccounted_task;                  // 112
##      unsigned int dk_drive[DK_MAX_MAJOR][DK_MAX_DISK];      // 120
##      unsigned int dk_drive_rio[DK_MAX_MAJOR][DK_MAX_DISK];  // 1144
##      unsigned int dk_drive_wio[DK_MAX_MAJOR][DK_MAX_DISK];  // 2168
##      unsigned int dk_drive_rblk[DK_MAX_MAJOR][DK_MAX_DISK]; // 3192
##      unsigned int dk_drive_wblk[DK_MAX_MAJOR][DK_MAX_DISK]; // 4216
##      unsigned int pgpgin, pgpgout;                          // 5240
##      unsigned int pswpin, pswpout;                          // 5248
##      unsigned int irqs[NR_IRQS];                            // 5256
## } ____cacheline_aligned;                                    // 6400 bytes total
## 
## extern struct kernel_stat_percpu kstat_percpu[NR_CPUS] ____cacheline_aligned;
##
##
## Context switches are available from sched.c:
##
## The mapping of CPUs to runqueues could be complicated on an SMT machine,
## but not on the machine simulated in SoftSDV:
##
## struct runqueue {
##      spinlock_t lock;
##      unsigned long nr_running, nr_switches, expired_timestamp,
##                      nr_uninterruptible;
##      atomic_t nr_iowait;
##      struct mm_struct *prev_mm;
##      prio_array_t *active, *expired, arrays[2];
##      int prev_cpu_load[NR_CPUS];
##      int nr_cpus;
##      cpu_t cpu[MAX_NR_SIBLINGS];
##} ____cacheline_aligned;
##
##   each array element is 4864 bytes, runqueues is at 0xe000000004a78400:
##
## static struct runqueue runqueues[NR_CPUS] __cacheline_aligned;
##

##
## kstat_cpu --
##     Pass in a CPU number.  Returns the physical address of the start of
##     the kernel_stat_percpu struct for the CPU.
##
sub kstat_cpu($) {
    my ($cpuNum) = @_;
    return $kstat + 6400 * $cpuNum;
}


##
## ktime --
##     Pass in the CPU number and an index for the position of the kernel_timeval
##     in the kernel_stat_percpu.  E.g. the index of user is 0, idle is 6.
##     Returns the time in seconds as an fp value.
##
sub ktime($$) {
    my ($cpu, $idx) = @_;
    my $offset = $idx * 8;
    return asim::read_memory(kstat_cpu($cpu) + $offset, 4) +
           asim::read_memory(kstat_cpu($cpu) + $offset + 4, 4) * 1e-6 +
           asim::read_memory(kstat_cpu($cpu) + 56 + $offset, 8) * 1e-6;
}


##
## context_switches_cpu --
##     Pass in a CPU number.  Returns the number of context switches on
##     the CPU.  Assumes no SMT hardware.
##
sub context_switches_cpu($) {
    my ($cpuNum) = @_;
    return asim::read_memory($runqueues + 16 + 4864 * $cpuNum, 8);
}


############################################################################
##
##
## gendisk and hd_struct hold statistics for disk I/O.  From these statistics
## we can compute the number of I/O operations and their average latency.
##
##
##struct hd_struct {
##  0   unsigned long start_sect;
##  8   unsigned long nr_sects;
## 16   devfs_handle_t de;              /* primary (master) devfs entry  */
##      /* Performance stats: */
## 24   unsigned int ios_in_flight;
## 28   unsigned int io_ticks;
## 32   unsigned int last_idle_time;
## 36   unsigned int last_queue_change;
## 40   unsigned int aveq;
##      
## 44   unsigned int rd_ios;
## 48   unsigned int rd_merges;
## 52   unsigned int rd_ticks;
## 56   unsigned int rd_sectors;
## 60   unsigned int wr_ios;
## 64   unsigned int wr_merges;
## 68   unsigned int wr_ticks;
## 72   unsigned int wr_sectors;        
##};
##
##struct gendisk {
##  0   int major;                      /* major number of driver */
##  8   const char *major_name;         /* name of major driver */
## 16   int minor_shift;                /* number of times minor is shifted to
##                                         get real minor */
## 20   int max_p;                      /* maximum partitions per device */
##
## 24   struct hd_struct *part;         /* [indexed by minor] */
## 32   int *sizes;                     /* [idem], device size in blocks */
## 40   int nr_real;                    /* number of real devices */
##
## 48   void *real_devices;             /* internal use */
## 56   struct gendisk *next;
## 64   struct block_device_operations *fops;
##
## 72   devfs_handle_t *de_arr;         /* one per physical disc */
## 80   char *flags;                    /* one per physical disc */
##};
##
##/* drivers/block/genhd.c */
##extern struct gendisk *gendisk_head;


##
## kv2p --
##     Convert string representation of a kernel virtual address to physical.
##     Depends on Linux mapping the kernel to VA 0xe... and just dropping the
##     high 0xe... to compute the PA.
##
sub kv2p($) {
    my ($a) = @_;
    $a =~ s/0x[eE]0*/0x/;
    return hex $a;
}


##
## get_disk_io_stats --
##     Returns a hash of data about disk I/O.  The hash contains the number
##     of I/O operations, number of sectors per operation and the average
##     latency of operations (in seconds) for reads and writes summed over
##     all the disks.
##
sub get_disk_io_stats() {
    my $sum_rd_sectors = 0;
    my $sum_wr_sectors = 0;
    my $sum_rd_ios = 0;
    my $sum_wr_ios = 0;
    my $sum_rd_time = 0.0;
    my $sum_wr_time = 0.0;
    my %summary;
    my $idx = 0;

    my $gd = kv2p(asim::read_hex_memory($gendisk_head, 8));
    # Loop through all major disk drivers
    while ($gd != 0) {
        # Number of partitions is nr_real << minor_shift
        my $major = asim::read_memory($gd, 4);
        my $minor_shift = asim::read_memory($gd + 16, 4);
        my $nPart = asim::read_memory($gd + 40, 4) << $minor_shift;
        if ($nPart > 0) {
            my $pMask = (1 << $minor_shift) - 1;
            my $pBase = kv2p(asim::read_hex_memory($gd + 24, 8));
            # Look at each partition
            foreach my $minor ( 0 .. $nPart ) {
                #
                # Only look at the whole disk records, not individual partitions.
                # $minor & $pMask is the partition number within the disk.  When
                # 0 it is the whole disk.
                #
                if (($minor & $pMask) == 0) {
                    my $pOffset = $pBase + $minor * 80;
                    my $nr_secs = asim::read_memory($pOffset + 8, 8);
                    if ($nr_secs != 0) {
                        # Finally at the partition data...
                        # First figure out the disk name
                        my $n = "";
                        if (($major == 3) || ($major == 22)) {
                            # IDE bus 0 or 1 supported
                            my $unit = ($minor != 0);
                            $unit += 2 if ($major != 3);
                            $n = "hd" . chr(ord('a') + $unit);
                        }
                        if ($major == 8) {
                            # One SCSI controller supported
                            $n = "sd" . chr(ord('a') + int($minor / 16));
                        }
                        if ($n eq "") {
                            $n = "<$major, $minor>";
                        }

                        $summary{'d_name'}[$idx] =  $n;

                        # Get statistics
                        $summary{'r_sectors'}[$idx] = asim::read_memory($pOffset + 56, 4);
                        $summary{'r_io'}[$idx] = asim::read_memory($pOffset + 44, 4);
                        $summary{'r_time'}[$idx] = asim::read_memory($pOffset + 52, 4) / $hz;
                        $summary{'w_sectors'}[$idx] = asim::read_memory($pOffset + 72, 4);
                        $summary{'w_io'}[$idx] = asim::read_memory($pOffset + 60, 4);
                        $summary{'w_time'}[$idx] = asim::read_memory($pOffset + 68, 4) / $hz;

                        $idx += 1;
                    }
                }
            }
        }
        $gd = kv2p(asim::read_hex_memory($gd + 56, 8));
    }
 
    return %summary;
}

############################################################################
##
## get_stats_snapshot() --
##     Returns a 6 element array with a snapshot of the current user,
##     system, idle and io wait times as well as disk blocks read and written.
##
sub get_stats_snapshot() {
##    my $user = 0.0;
##    my $system = 0.0;
##    my $idle = 0.0;
##    my $io = 0.0;
    my %summary;

    foreach my $cpu ( 0 .. asim::num_cpus()-1 ) {
        $summary{'user'}[$cpu] = ktime($cpu, 0) + ktime($cpu, 1);
        $summary{'system'}[$cpu] = ktime($cpu, 2) + ktime($cpu, 3) + ktime($cpu, 4);
        $summary{'idle'}[$cpu] = ktime($cpu, 6);
        $summary{'io_wait'}[$cpu] = ktime($cpu, 5);
        $summary{'ctx'}[$cpu] = context_switches_cpu($cpu);
    }

    my %disk = get_disk_io_stats();

    # Merge disk I/O stats into summary
    @summary{keys %disk} = values %disk;

    return %summary;
}


1;
