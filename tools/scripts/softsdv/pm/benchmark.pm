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

##
## benchmark is loaded by the main setup scripts.  Individual workloads
## may provide a file that will be named benchmark.pm in the working
## directory where SoftSDV runs.  Workloads that don't provide their
## own benchmark.pm wind up running this one.
##

##
## A benchmark module must define the public functions run and setup.
## The functions take no arguments since there may be a lot of packages
## named benchmark to handle different workloads and changing the
## interface could be difficult.  State is generally passed in using
## the %config hash generated by setup-softsdv.
##

use POSIX;
use File::Basename;
use asim;
use simpoints;

use strict;

package benchmark;


##
## Run a benchmark either from its checkpoint or by loading a workload
## into a base-boot checkpoint.
##
sub run() {
    my $sv = $main::config{'save_name'};
    if ($sv eq '') {
        $sv = $main::config{'base_save_name'};
    }
    asim::restore($sv);

    if ($main::config{'save_name'} eq $main::config{'base_save_name'}) {
        #
        # This only happens for OS base boots.  User probably just wants
        # to run the OS for a while and experiment in a shell.
        #
        print("Running a shell in simulated machine...\n");
        asim::request_benchmark(1);
        main::cont();
        return 1;
    }

    ##
    ## If this is not a benchmark-specific checkpoint then load the benchmark
    ##
    if ($main::config{'run_loads_image'}) {
        asim::request_benchmark(1);
        asim::wait_for_tag($asim::asim_mark_start, $main::config{'start_count'});

        if ($asim::asim_mark_start == $asim::asim_default_mark_start &&
            $main::config{'start_count'} == 1)
        {
            ##
            ## Vanilla benchmark without markers.  The only mark we receive is from
            ## the script that starts the benchmarks.  Skip some instructions at
            ## the beginning to ignore some of the shell and loader.
            ##
            main::step(100000);
        }
    }

    ##
    ## Does the checkpoint begin with warm-up instructions?
    ##
    asim::manage_warmup();

    asim::print_all_ips();
    my $stime = asim::start_simulation();

    if (defined($main::config{'sim_loop_trips'})) {
        print "Running in performance mode for $main::config{'sim_loop_trips'} loop iterations...\n";
        asim::wait_for_tag($main::config{'loop_trip_marker'},
                           $main::config{'sim_loop_trips'});
    }
    else {
        print "Running in performance mode for $main::config{'sim_instrs'} instructions...\n";
        asim::step_uneven($main::config{'sim_instrs'});
    }

    asim::end_simulation($stime);
    print "Done...\n";

    return 1;
}


##
## Boot an OS and generate a checkpoint just before the OS asks whether it
## should load a workload from the host.
##
sub boot() {
    return asim::standard_boot();
}


############################################################################
#
#      S E T U P
#
############################################################################

##
## load_program_to_sample --
##     Load a program to sample assuming we are currently right after a
##     boot.
##
sub load_program_to_sample() {
    asim::request_benchmark(1);
    asim::wait_for_tag($asim::asim_mark_start, $main::config{'start_count'});

    if ($asim::asim_mark_start == $asim::asim_default_mark_start &&
        $main::config{'start_count'} == 1)
    {
        ##
        ## Vanilla benchmark without markers.  The only mark we receive is from
        ## the script that starts the benchmarks.  Skip some instructions at
        ## the beginning to ignore some of the shell and loader.
        ##
        main::step(100000);
    }
}


##
## Set up a workload, continuing from the result of the boot() function.
##
sub setup() {
    if ($main::config{'simpoint_region_length'}) {
        simpoints::gen_checkpoints();
    }
    else {
        if ($main::config{'save_name'} eq $main::config{'base_save_name'}) {
            ##
            ## Either this is an OS base boot or the workload has no
            ## checkpoint name defined.  Do nothing.
            ##
            return 0;
        }

        ##
        ## Load the workload
        ##
        my $oldTimeSlice = asim::time_slice();
        asim::set_time_slice($oldTimeSlice * 100);
        print "Setting time slice to " . asim::time_slice() . " for setup...\n";

        asim::request_benchmark(1);
        asim::wait_for_tag($asim::asim_mark_start, $main::config{'start_count'});

        asim::set_time_slice($oldTimeSlice);
        print "Restoring time slice to " . asim::time_slice() . "...\n";

        if ($asim::asim_mark_start == $asim::asim_default_mark_start &&
            $main::config{'start_count'} == 1)
        {
            ##
            ## Vanilla benchmark without markers.  The only mark we receive is from
            ## the script that starts the benchmarks.  Skip some instructions at
            ## the beginning to ignore some of the shell and loader.
            ##
            main::step(100000);
        }

        asim::save("$main::config{'save_name'}");
        print "Done\n";
    }

    return 0;
}


1;
