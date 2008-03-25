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
## Manage creation of SimPoint based checkpoints
##

use strict;
use asim;
use idlecount;

package simpoints;


##
## sample --
##     Load a benchmark, sample the IP and run the SimPoint algorithm to
##     choose representative regions to simulate.  A .pp file is left in
##     the checkpoints directory describing the regions to simulate.  A
##     .bb file is generated with the IP samples as an input to the
##     SimPoints algorithm.  The .bb file is also left in the checkpoints
##     directory.
##
##     This is the first pass of the algorithm.  Call gen_checkpoints
##     to build the actual checkpoints.
##
sub sample() {
    my $tmpChkptDir = "$main::config{'save_name'}_tmp";
    if (-f "$tmpChkptDir/pass2.region") {
        print "***\n";
        print "***\n";
        print "*** Pass 2 was running.  Skipping pass 1.\n";
        print "***\n";
        print "***\n";
        return 0;
    }

    benchmark::load_program_to_sample();
    asim::save("$main::config{'save_name'}_start");

    ##
    ## Use SimPoints algorithm to pick regions to simulate
    ##
    print "Finding SimPoints...\n";

    load_simpoint_sampler();

    my $startICount = main::get_icount();

    ##
    ## Skip over a region the size of warm-up code.  We need to leave this
    ## much as a buffer in case SimPoints chooses the first region.
    ##
    my $warmRegions = POSIX::ceil($main::config{'warmup_chkpt_instrs'} /
                                  $main::config{'simpoint_region_length'});
    print "  Warm-up is $warmRegions regions\n";
    my @m = asim::count_marks($warmRegions * $main::config{'simpoint_region_length'});
    if (defined($m[$asim::asim_mark_end])) {
        too_short();
        return 0;
    }

    print "  Benchmark loaded.  Sampling $main::config{'simpoint_region_length'} cycle regions...\n";

    # Remove old files
    system("rm -f $main::config{'save_name'}.bb* $main::config{'save_name'}.pp");

    my $iter = 0;

    # Create a temporary directory for samples
    system("rm -rf $tmpChkptDir") if (-d "$tmpChkptDir");
    mkdir($tmpChkptDir) or die("Failed to make temp directory");

    # Start the sampling code in SoftSDV
    start_sampling("$tmpChkptDir/samples.bb");

    open(MD, ">> $tmpChkptDir/metadata") or die("Can't open $tmpChkptDir/metadata");
    print MD "\n";
    print MD "# S records show the region number and the start IP for CPU 0\n";
    print MD "#   of every region.  This is useful for verifying start positions\n";
    print MD "#\n";
    print MD "# M records show the region number and the number of instructions\n";
    print MD "#   spent idle in spin loops by each CPU during the interval.\n";

    my $done = 0;
    while (1) {
        # Record the start IP of CPU 0 for each region.  Useful for verification.
        ssdv::set_data("oml", "data", "notifier.view_cpu", 0);
        my $startIP = asim::get_ip();

        @m = asim::count_marks($main::config{'simpoint_region_length'});

        if (defined($m[$asim::asim_mark_end])) {
            print "  Reached end marker...\n";
            $done = 1;
            last;
        }

        last if ($done);
        ssdv::execute_command("simpoint", "end_region");

        #
        # Emit metadata -- start IP and time spent in idle loops
        #
        print MD "S $iter $startIP\n";
        print MD "M $iter";
        for (my $i = 0; $i < asim::num_cpus(); $i++) {
            ssdv::set_data("oml", "data", "notifier.view_cpu", $i);
            print MD " " . idlecount::idleCycles();
        }
        print MD "\n";
        idlecount::resetCounters();
        ssdv::set_data("oml", "data", "notifier.view_cpu", 0);

        ++$iter;
    }

    stop_sampling();
    close(MD);

    ## Is the benchmark unreasonably short for SimPoints?
    if ($iter < 2) {
        too_short();
        return 0;
    }

    ##
    ## Sampling is complete.  Merge all the data into a single BB file.
    ##
    system("cat $tmpChkptDir/samples.bb $tmpChkptDir/metadata > $main::config{'save_name'}.bb") == 0 or
        die("Failed to merge sample data");
    system("rm -f $tmpChkptDir/samples.bb $tmpChkptDir/metadata");

    ##
    ## Run SimPoints on the bb file.
    ##
    my $tmpDir = "/tmp/simpt.$$";
    mkdir($tmpDir) or die("Failed to make temp directory");
    my @args = ("runsimpoint.asim", "$main::config{'save_name'}.bb", $tmpDir);
    system(@args) == 0 or die("runsimpoint.asim failed: $?");

    ##
    ## Read in the regions and weights
    ##
    my $tPath = ${tmpDir} . "/" . main::basename($main::config{'save_name'});
    open(PT, "paste ${tPath}.simpoints ${tPath}.weights |");
    my %simRegions = ();
    my $nRegions = 0;

    my $droppedWeight = 0;
    while (<PT>) {
        chomp;
        if ($_ ne '') {
            my ($region, $weight) = split(/\s+/, $_);
            ##
            ## Ignore insignificant regions (< 0.1%).  Not worth simulating
            ## given that Asim's error is probably larger.
            ##
            if ($weight < 0.001) {
                $droppedWeight += $weight;
                printf "  Dropping region %d with low weight (%0.4f\%)\n",
                       $region,
                       $weight * 100.0;
            }
            else {
                # SimPoint regions are 1 based.  We want 0 based.
                $simRegions{$region-1} = $weight;
                $nRegions += 1;
            }
        }
    }

    ##
    ## Add weight dropped from low weight regions above to the remaining regions
    ## in proportion to the weights of the remaining regions.  This restores
    ## the total weight to 1.
    ##
    if ($droppedWeight > 0) {
        foreach my $r (keys %simRegions) {
            $simRegions{$r} += $droppedWeight * ($simRegions{$r} / (1.0 - $droppedWeight));
        }
    }

    die("No regions chosen!") if ($nRegions == 0);

    ## Old checkpoints are now invalid.
    system("rm -rf $main::config{'save_name'}_r[0-9]*");

    open(PP, "> $main::config{'save_name'}.pp") or die("Failed to open $main::config{'save_name'}.pp for writing");
    print PP "# SimPoints chosen during SoftSDV setup\n\n";
    print PP "#N Number of regions\n";
    print PP "N ${nRegions}\n\n";
    print PP "#I Total represented instrs per CPU\n";
    my $totalInstrs = main::get_icount() - $startICount;
    print PP "I ${totalInstrs}\n\n";
    print PP "#L Region instrs per CPU\n";
    print PP "L $main::config{'simpoint_region_length'}\n\n";
    print PP "#W Leading warm-up regions in checkpoint for preloading caches\n";
    print PP "W ${warmRegions}\n\n";

    print PP "#R <0 based region number> <weight>\n";
    foreach my $r (sort {$a <=> $b} keys %simRegions) {
        print PP "R $r $simRegions{$r}\n";
    }

    print PP "\n#M <0 based region number> <idle instructions per CPU> ...\n";
    foreach my $r (sort {$a <=> $b} keys %simRegions) {
        my $m = `grep "^M $r " "$main::config{'save_name'}.bb"`;
        chomp($m);
        print PP "$m\n";
    }
    close(PP);

    system("gzip $main::config{'save_name'}.bb");
    print "\n";
    system("cat $main::config{'save_name'}.pp");
    print "\n";

    # Mark pass 1 complete
    system("echo 0 0 > $tmpChkptDir/pass2.region");

    return 1;
}


##
## gen_checkpoints --
##     Generate checkpoints given a preexisting pp file in the checkpoint area.
##     This routine does NOT depend on running sample() in the same
##     execution of SoftSDV.  No variables are kept live from the sampling
##     phase.  This allows us to break the sampling and checkpointing into
##     two passes or to reuse pp files if we are daring.
##
sub gen_checkpoints() {
    my $tmpChkptDir = "$main::config{'save_name'}_tmp";

    ## Sampling pass saved a checkpoint at the beginning
    asim::restore("$main::config{'save_name'}_start");

    ##
    ## Did gen_checkpoints stop early due to machine failure?
    ##
    my $rStart = 0;
    my $iStart = 0;

    if (-f "$tmpChkptDir/pass2.region") {
        open(R, "< $tmpChkptDir/pass2.region") or die("Can't read $tmpChkptDir/pass2.region");
        my $d = <R>;
        chomp($d);
        ($rStart, $iStart) = split(" ", $d);
        close(R);

        if ($iStart > 0) {
            print "Resuming checkpointing from region $iStart\n";
            asim::restore("$main::config{'save_name'}_r${iStart}");
        }
    }

    ##
    ## Read the region data from the pp file
    ##
    my $regionInstrs = 0;
    my %regions = ();

    open(PP, "< $main::config{'save_name'}.pp") or die("Failed to open $main::config{'save_name'}.pp for reading");
    while (<PP>) {
        chomp;

        if (/^L/) {
            $regionInstrs = $_;
            $regionInstrs =~ s/L *//;
        }
        if (/^R/) {
            my @rInfo = split(/\s+/, $_);
            # Skip regions for which checkpoints have already been generated
            if (($iStart == 0) || ($rInfo[1] > $rStart)) {
                $regions{$rInfo[1]} = $rInfo[2];
            }
        }
    }
    close(PP);

    if ($regionInstrs == 0) {
        print "\n";
        print "\n";
        print "**************************************************************\n";
        print "No regions defined.  Check profiling session above for errors.\n";
        print "**************************************************************\n";
        print "\n";
        return 0;
    }

    ##
    ## On the sampling pass we stepped forward at the beginning over the warmup
    ## region size.  On this pass we do not since the warm-up code will be
    ## fed into Asim before the simulation begins.
    ##

    my $curRegion = $rStart;
    my $iter = $iStart;
    foreach my $r (sort {$a <=> $b} keys %regions) {
        my $stepCnt = ($r - $curRegion) * $regionInstrs;
        if ($stepCnt > 0) {
            print "  Advancing ${stepCnt} cycles to region ${r}\n";
            main::step($stepCnt);
        }

        $curRegion = $r;
        ++$iter;
        if ($iter == 1) {
            asim::save("$main::config{'save_name'}_r${iter}");
        }
        else {
            asim::save_delta("$main::config{'save_name'}_r1",
                             "$main::config{'save_name'}_r${iter}");
        }

        system("echo $curRegion $iter > $tmpChkptDir/pass2.region");
    }

    system("rm -f $tmpChkptDir/pass2.region");
    return 1;
}



########################################################################
########################################################################
##
## Support code
##
########################################################################
########################################################################

##
## load_simpoint_sampler --
##     Load the SoftSDV module that samples instructions for SimPoints.
##
sub load_simpoint_sampler() {
    print "  Loading SimPoints SoftSDV module...\n";
    idlecount::load();
    ssdv::execute_command("kernel", "vpc_command", "loadmod simpoint");
    ssdv::execute_command("kernel", "vpc_command", "vpc.cpu isax_init");
}


##
## start_sampling --
##     Start taking branch and idle loop samples.
##
sub start_sampling($) {
    my ($s) = @_;
    ssdv::execute_command("simpoint", "start", $s);
    idlecount::start();
}

sub stop_sampling() {
    ssdv::execute_command("simpoint", "stop");
    idlecount::stop();
}



sub too_short() {
    print "\n";
    print "********************************************************\n";
    print "This program is too short for the SimPoints region size!\n";
    print "********************************************************\n";
    print "\n";

    open(PP, "> $main::config{'save_name'}.pp");
    print PP "# This program is too shart for the SimPoints region size!\n";
    close(PP);
}


1;
