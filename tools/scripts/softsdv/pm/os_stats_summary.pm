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

use strict;

package os_stats;

##
## Common summary reporting functions for the OS-dependent summary libraries.
##

############################################################################
##
## total_time --
##     Pass in two arrays with begin and end stats snapshots.
##
sub total_time_cpu($\%\%) {
   my ($cpu, $prev, $cur) = @_;
   return ${$cur}{'user'}[$cpu] + ${$cur}{'system'}[$cpu] + ${$cur}{'idle'}[$cpu] + ${$cur}{'io_wait'}[$cpu] -
          ${$prev}{'user'}[$cpu] - ${$prev}{'system'}[$cpu] - ${$prev}{'idle'}[$cpu] - ${$prev}{'io_wait'}[$cpu];
}

sub total_time(\%\%) {
   my ($prev, $cur) = @_;
   my $t = 0.0;
   foreach my $cpu ( 0 .. $#{${$cur}{'user'}} ) {
       $t += total_time_cpu($cpu, %{$prev}, %{$cur});
   }
   return $t;
}


##
## user_fraction --
##     Returns the fraction of time spent in user space for the region.
##
sub user_fraction_cpu($\%\%) {
   my ($cpu, $prev, $cur) = @_;
   my $time = total_time_cpu($cpu, %{$prev}, %{$cur});
   if ($time == 0) {
       return 0;
   }
   return (${$cur}{'user'}[$cpu] - ${$prev}{'user'}[$cpu]) / $time;
}

sub user_fraction(\%\%) {
   my ($prev, $cur) = @_;
   my $t = 0.0;
   foreach my $cpu ( 0 .. $#{${$cur}{'user'}} ) {
       $t += ${$cur}{'user'}[$cpu] - ${$prev}{'user'}[$cpu];
   }
   my $time = total_time(%{$prev}, %{$cur});
   if ($time == 0) {
       return 0;
   }
   return $t / $time;
}


##
## system_fraction --
##     Returns the fraction of time spent in the kernel for the region.
##
sub system_fraction_cpu($\%\%) {
   my ($cpu, $prev, $cur) = @_;
   my $time = total_time_cpu($cpu, %{$prev}, %{$cur});
   if ($time == 0) {
       return 0;
   }
   return (${$cur}{'system'}[$cpu] - ${$prev}{'system'}[$cpu]) / $time;
}

sub system_fraction(\%\%) {
   my ($prev, $cur) = @_;
   my $t = 0.0;
   foreach my $cpu ( 0 .. $#{${$cur}{'system'}} ) {
       $t += ${$cur}{'system'}[$cpu] - ${$prev}{'system'}[$cpu];
   }
   my $time = total_time(%{$prev}, %{$cur});
   if ($time == 0) {
       return 0;
   }
   return $t / $time;
}


##
## idle_fraction --
##     Returns the fraction of time spent idle for the region.
##
sub idle_fraction_cpu($\%\%) {
   my ($cpu, $prev, $cur) = @_;
   my $time = total_time_cpu($cpu, %{$prev}, %{$cur});
   if ($time == 0) {
       return 0;
   }
   return (${$cur}{'idle'}[$cpu] - ${$prev}{'idle'}[$cpu]) / $time;
}

sub idle_fraction(\%\%) {
   my ($prev, $cur) = @_;
   my $t = 0.0;
   foreach my $cpu ( 0 .. $#{${$cur}{'idle'}} ) {
       $t += ${$cur}{'idle'}[$cpu] - ${$prev}{'idle'}[$cpu];
   }
   my $time = total_time(%{$prev}, %{$cur});
   if ($time == 0) {
       return 0;
   }
   return $t / $time;
}


##
## io_wait_fraction --
##     Returns the fraction of time spent idle waiting for I/O for the region.
##
sub io_wait_fraction_cpu($\%\%) {
   my ($cpu, $prev, $cur) = @_;
   my $time = total_time_cpu($cpu, %{$prev}, %{$cur});
   if ($time == 0) {
       return 0;
   }
   return (${$cur}{'io_wait'}[$cpu] - ${$prev}{'io_wait'}[$cpu]) / $time;
}

sub io_wait_fraction(\%\%) {
   my ($prev, $cur) = @_;
   my $t = 0.0;
   foreach my $cpu ( 0 .. $#{${$cur}{'io_wait'}} ) {
       $t += ${$cur}{'io_wait'}[$cpu] - ${$prev}{'io_wait'}[$cpu];
   }
   my $time = total_time(%{$prev}, %{$cur});
   if ($time == 0) {
       return 0;
   }
   return $t / $time;
}


##
## context_switches --
##    Returns the number of context switches for the region.
##
sub cswitch_cpu($\%\%) {
   my ($cpu, $prev, $cur) = @_;
   return ${$cur}{'ctx'}[$cpu] - ${$prev}{'ctx'}[$cpu];
}

sub cswitch(\%\%) {
   my ($prev, $cur) = @_;
   my $c = 0;
   foreach my $cpu ( 0 .. $#{${$cur}{'ctx'}} ) {
       $c += cswitch_cpu($cpu, %{$prev}, %{$cur});
   }
   return $c;
}


##
## disk_reads --
##     Returns the number of disk read operations in the region.
##
sub disk_reads(\%\%) {
   my ($prev, $cur) = @_;
   my $n = 0;
   foreach my $d ( 0 .. $#{${$cur}{'r_io'}} ) {
       $n += ${$cur}{'r_io'}[$d] - ${$prev}{'r_io'}[$d];
   }
   return $n;
}


##
## disk_writes --
##     Returns the number of disk read operations in the region.
##
sub disk_writes(\%\%) {
   my ($prev, $cur) = @_;
   my $n = 0;
   foreach my $d ( 0 .. $#{${$cur}{'w_io'}} ) {
       $n += ${$cur}{'w_io'}[$d] - ${$prev}{'w_io'}[$d];
   }
   return $n;
}


##
## disk_sector_reads --
##     Returns the number of disk read operations in the region.
##
sub disk_sector_reads(\%\%) {
   my ($prev, $cur) = @_;
   my $n = 0;
   foreach my $d ( 0 .. $#{${$cur}{'r_sectors'}} ) {
       $n += ${$cur}{'r_sectors'}[$d] - ${$prev}{'r_sectors'}[$d];
   }
   return $n;
}


##
## disk_sector_writes --
##     Returns the number of disk read operations in the region.
##
sub disk_sector_writes(\%\%) {
   my ($prev, $cur) = @_;
   my $n = 0;
   foreach my $d ( 0 .. $#{${$cur}{'w_sectors'}} ) {
       $n += ${$cur}{'w_sectors'}[$d] - ${$prev}{'w_sectors'}[$d];
   }
   return $n;
}


##
## disk_read_latency --
##     Returns the average latency of disk reads in seconds.
##
sub disk_read_latency(\%\%) {
   my ($prev, $cur) = @_;

   my $t = 0;
   foreach my $d ( 0 .. $#{${$cur}{'r_time'}} ) {
       $t += ${$cur}{'r_time'}[$d] - ${$prev}{'r_time'}[$d];
   }

   if ($t > 0) {
       return $t / disk_reads(%{$prev}, %{$cur});
   }
   return 0;
}


##
## disk_write_latency --
##     Returns the average latency of disk writes in seconds.
##
sub disk_write_latency(\%\%) {
   my ($prev, $cur) = @_;

   my $t = 0;
   foreach my $d ( 0 .. $#{${$cur}{'w_time'}} ) {
       $t += ${$cur}{'w_time'}[$d] - ${$prev}{'w_time'}[$d];
   }

   if ($t > 0) {
       return $t / disk_writes(%{$prev}, %{$cur});
   }
   return 0;
}


##
## stats_summary --
##     Returns a string summarizing the statistics for the region.
##
sub stats_summary(\%\%) {
   my ($prev, $cur) = @_;

   my $time  = total_time(%{$prev}, %{$cur}) / asim::num_cpus();
   my $user  = user_fraction(%{$prev}, %{$cur}) * 100.0;
   my $sys   = system_fraction(%{$prev}, %{$cur}) * 100.0;
   my $idle  = idle_fraction(%{$prev}, %{$cur}) * 100.0;
   my $io    = io_wait_fraction(%{$prev}, %{$cur}) * 100.0;
   my $ctx   = cswitch(%{$prev}, %{$cur});
   my $rd    = disk_reads(%{$prev}, %{$cur});
   my $rdLat = disk_read_latency(%{$prev}, %{$cur}) * 1000.0;
   my $wr    = disk_writes(%{$prev}, %{$cur});
   my $wrLat = disk_write_latency(%{$prev}, %{$cur}) * 1000.0;

   return sprintf("%.1fu %.1fs %.1fi %.1fw, %dcs, %drd (%.1fms) %dwr (%.1fms), %.4fs",
                  $user, $sys, $idle, $io, $ctx, $rd, $rdLat, $wr, $wrLat, $time);
}


##
## stats_summary_cpu --
##     Returns a string summarizing the statistics on one CPU for the region.
##
sub stats_summary_cpu($\%\%) {
   my ($cpu, $prev, $cur) = @_;

   my $time  = total_time_cpu($cpu, %{$prev}, %{$cur});
   my $user  = user_fraction_cpu($cpu, %{$prev}, %{$cur}) * 100.0;
   my $sys   = system_fraction_cpu($cpu, %{$prev}, %{$cur}) * 100.0;
   my $idle  = idle_fraction_cpu($cpu, %{$prev}, %{$cur}) * 100.0;
   my $io    = io_wait_fraction_cpu($cpu, %{$prev}, %{$cur}) * 100.0;
   my $ctx   = cswitch_cpu($cpu, %{$prev}, %{$cur});

   return sprintf("%.1fu %.1fs %.1fi %.1fw, %dcs, %.4fs",
                  $user, $sys, $idle, $io, $ctx, $time);
}


##
## stats_summary_disk_reads --
##     Returns a string with a details of reads on each disk device.
##
sub stats_summary_disk_reads(\%\%) {
   my ($prev, $cur) = @_;
   my $s = "";

   foreach my $d (sort {${$cur}{'d_name'}[$a] cmp ${$cur}{'d_name'}[$b]} ( 0 .. $#{${$cur}{'w_time'}})) {
       $s .= ", " if ($s ne "");
       $s .= ${$cur}{'d_name'}[$d];

       my $io = ${$cur}{'r_io'}[$d] - ${$prev}{'r_io'}[$d];
       my $sect = ${$cur}{'r_sectors'}[$d] - ${$prev}{'r_sectors'}[$d];
       my $t = ${$cur}{'r_time'}[$d] - ${$prev}{'r_time'}[$d];

       $s .= " $io";
       if ($io != 0) {
           my $lat = 1000.0 * $t / $io;
           $s .= sprintf("/%d (%.1fms)", $sect, $lat);
       }
   }

   return $s;
}


##
## stats_summary_disk_writes --
##     Returns a string with a details of writes on each disk device.
##
sub stats_summary_disk_writes(\%\%) {
   my ($prev, $cur) = @_;
   my $s = "";

   foreach my $d (sort {${$cur}{'d_name'}[$a] cmp ${$cur}{'d_name'}[$b]} ( 0 .. $#{${$cur}{'w_time'}})) {
       $s .= ", " if ($s ne "");
       $s .= ${$cur}{'d_name'}[$d];

       my $io = ${$cur}{'w_io'}[$d] - ${$prev}{'w_io'}[$d];
       my $sect = ${$cur}{'w_sectors'}[$d] - ${$prev}{'w_sectors'}[$d];
       my $t = ${$cur}{'w_time'}[$d] - ${$prev}{'w_time'}[$d];

       $s .= " $io";
       if ($io != 0) {
           my $lat = 1000.0 * $t / $io;
           $s .= sprintf("/%d (%.1fms)", $sect, $lat);
       }
   }

   return $s;
}

1;
