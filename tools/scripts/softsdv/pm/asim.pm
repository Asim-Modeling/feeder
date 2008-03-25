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

use ssdv;
use strict;

package asim;

our $asim_mark_booted        = 32768;        ## Boot complete
our $asim_mark_start         = 32769;        ## Benchmark starting
our $asim_mark_end           = 32770;        ## Benchmark ending
our $asim_mark_req_int       = 32771;        ## Client requested an integer
our $asim_mark_req_str       = 32772;        ## Client requested a string
our $asim_mark_str_ack       = 32773;        ## Client received string
our $asim_mark_str_nack      = 32774;        ## Error receiving string
our $asim_mark_done          = 32775;        ## Emitted at the end of a run
                                             ## only by the invoke_workload script
our $asim_mark_slow_clock    = 32776;        ## Request slow (normal) nsecperinst
our $asim_mark_fast_clock    = 32777;        ## Request fast clock

our $asim_mark_spin_start    = 125;          ## Entering a spin loop
our $asim_mark_spin_body     = 126;          ## Once per iteration
our $asim_mark_spin_end      = 127;          ## Exiting spin loop

our $asim_default_mark_start = $asim_mark_start; ## In case mark_start is changed
our $asim_default_mark_end   = $asim_mark_end;   ## In case mark_end is changed

our $osName = '';
our $pidOffset = 0;
our $pNameOffset = 0;
our $timerTag = 0;        # VA of an instruction in the timer interrupt path
our $idleTag = 0;         # VA of an instruction in the kernel idle loop

my $oldVersion = 0;

sub num_cpus();
sub manage_warmup_instrs();
sub manage_warmup_loops();

############################################################################
#
#      O B J E C T    A C C E S S
#
############################################################################

##
## read_data --
##     Read a simple data value as a decimal value.  Expects the same arguments
##     as ssdv::get_simple_data_value.
##
sub read_data {
    my (@params) = @_;

    my $oldMode = $ssdv::decimal_mode;
    $ssdv::decimal_mode = 1;

    my $result = ssdv::get_simple_data_value(@params);

    $ssdv::decimal_mode = $oldMode;
    return $result;
}


##
## read_hex_data --
##     Read a simple data value as a hex (string) value.  Expects the same
##     arguments as ssdv::get_simple_data_value.
##
sub read_hex_data {
    my ($module, $data, @params) = @_;

    ##
    ## There used to be a single function in ssdv that did this but,
    ## for unknown reasons, they removed it.
    ##
    my $data_handle = ssdv::get_data_handle($module, $data, @params);
    my $result;
    if (defined &sdbg_perl::ssdv_ext_data_item_get_value) {
        ($result) = sdbg_perl::ssdv_ext_data_item_get_value($data_handle);
    }
    else {
        # Old versions
	($result) = dbg_perl::ssdv_ext_get_value($data_handle);
    }
    ssdv::release_data_handle($data_handle);

    return $result;
}


##
## read_memory --
##     Read from physical address requested number of bytes.
##
sub read_memory($$) {
    my ($pa, $bytes) = @_;
    if ($oldVersion) {
        return read_data("cpu", "address", $pa, $bytes);
    }
    else {
        return read_data("cpu", "address", $pa, $bytes, "p");
    }
}


##
## read_hex_memory --
##     Read from physical address requested number of bytes as a hex string.
##
sub read_hex_memory($$) {
    my ($pa, $bytes) = @_;
    if ($oldVersion) {
        return read_hex_data("cpu", "address", $pa, $bytes);
    }
    else {
        return read_hex_data("cpu", "address", $pa, $bytes, "p");
    }
}


##
## read_string --
##     Read a character string from memory
##
sub read_string($) {
    my ($pa) = @_;

    my $s = "";

    while (1) {
        my $c = read_memory($pa++, 1);
        last if ($c == 0);
        $s .= chr($c);
    }

    return $s;
}


##
## get_value --
##     Return value of oml object.
##
sub get_value($) {
    my ($obj) = @_;

    my $oldMode = $ssdv::decimal_mode;
    $ssdv::decimal_mode = 1;

    my $r = read_data("oml", "data", $obj);

    $ssdv::decimal_mode = $oldMode;
    return $r;
}


##
## get_hex_value --
##     Return the value of oml object as a hex string.
##
sub get_hex_value($) {
    my ($obj) = @_;
    return read_hex_data("oml", "data", $obj);
}


##
## get_value_all_cpus --
##     Return the sum of an oml object on all CPUs.
##
sub get_value_all_cpus($) {
    my ($obj) = @_;

    my $oldCpu = get_value("notifier.view_cpu");

    my $sum = 0;
    foreach my $c ( 0 .. num_cpus() - 1 ) {
        ssdv::set_data("oml", "data", "notifier.view_cpu", $c);
        $sum += get_value($obj);
    }

    ssdv::set_data("oml", "data", "notifier.view_cpu", $oldCpu);
    return $sum;
}


############################################################################
#
#      T I M E
#
############################################################################

##
## SoftSDV can't read the value of nsecPerInst correctly because it is
## floating point.  Manage all time changes through these calls.
##
my $currNSecPerInst = 20.0;
sub set_nsec_per_inst($) {
    my ($t) = @_;
    $currNSecPerInst = 1.0 * $t;
    ssdv::set_data("oml", "data", "vpc_kernel.notifier.vpc.time.nsecPerInst", $t);
}

sub get_nsec_per_inst() {
    return $currNSecPerInst;
}

sub get_cycles_per_sec() {
    return 1000000000.0 / get_nsec_per_inst();
}

############################################################################
#
#      S I M P L E    C O M M O N    F U N C T I O N S
#
############################################################################


##
## num_cpus --
##     Returns the number of CPUs simulated.
##
sub num_cpus() {
    if (defined &main::get_cfg_value) {
        return main::get_cfg_value("num_cpus");
    }
    else {
        ## Older versions
        return get_value("notifier.num_of_simulated_cpus");
    }
}


##
## get_ip --
##     Return current IP.
##
sub get_ip() {
    return get_hex_value("data.arch.il");
}


##
## print_all_ips --
##     Print the current IPs of all CPUs.
##
sub print_all_ips() {
    my $oldView = get_value("notifier.view_cpu");

    for (my $c = 0; $c < asim::num_cpus(); ++$c) {
        ssdv::set_data("oml", "data", "notifier.view_cpu", $c);
        my $ip = get_ip();
        print "  CPU ${c} IP:  ${ip}\n";
    }

    ssdv::set_data("oml", "data", "notifier.view_cpu", $oldView);
}


##
## time_slice --
##     Return the current time slice.
##
sub time_slice() {
    return get_value("notifier.time_slice");
}


##
## set_time_slice --
##     Set the current time slice.
##
sub set_time_slice($) {
    my ($tSlice) = @_;
    ssdv::set_data("oml", "data", "notifier.time_slice", $tSlice);
}


##
## save --
##     Save a checkpoint.
##
sub save($) {
    my ($fName) = @_;
    print "Saving checkpoint to ${fName}\n";

    if (! $oldVersion)
    {
        ## Recent versions of SoftSDV -- use built in save
        ssdv::set_data("oml", "data", "notifier.save_mode", 3);
        main::save($fName);
        # SoftSDV gets the proection wrong (no world access)
        chmod(0644, "${fName}.bwa");
    }
    else
    {
        ##
        ## Save using our own compression script.  Michael Adler modified
        ## SoftSDV's srlib to use popen on any file name beginning with |
        ## when save_mode is 0 so we can output through scripts.
        ##
        ssdv::set_data("oml", "data", "notifier.save_mode", 0);

        ##
        ## Treat the save name argument as a directory into which SoftSDV will
        ## store individual files.  We use this save mode because bzip is
        ## faster and compresses better than SoftSDV's internal compression.
        ## This encoding also works well with delta compression (see save_delta).
        ##
        if (! -d $fName) {
            if (! mkdir($fName)) {
                print "Failed to make directory for saving ${fName}\n";
                return 0;
            }
        }
        system("rm -f ${fName}/save*");

        ## Hack -- SoftSDV doesn't accept spaces in file names.  ^ is converted
        ## to space in srlib at the last moment.
        main::save("|checkpoint_save^${fName}/save");
    }

    return 1;
}


##
## save_delta --
##     Save using delta compression.  Only changes from a previous checkpoint
##     are written to disk.
##
sub save_delta($$) {
    my ($baseName, $fName) = @_;
    print "Saving delta checkpoint from ${baseName} to ${fName}\n";

    if (! $oldVersion)
    {
        ## Recent versions of SoftSDV have built in delta compression
        ssdv::set_data("oml", "data", "notifier.save_mode", 3);
        main::save("-base ${baseName} ${fName}");
        # SoftSDV gets the proection wrong (no world access)
        chmod(0644, "${fName}.bwa");
    }
    else
    {
        ## Use our own delta compression
        ssdv::set_data("oml", "data", "notifier.save_mode", 0);

        ##
        ## Treat the save name argument as a directory into which SoftSDV will
        ## store individual files.  We use this save mode because bzip is
        ## faster and compresses better than SoftSDV's internal compression.
        ## We also use delta encoding for multiple checkpoints.
        ##
        if (! -d $fName) {
            if (! mkdir($fName)) {
                print "Failed to make directory for saving ${fName}\n";
                return 0;
            }
        }
        system("rm -f ${fName}/save*");

        ## Hack -- SoftSDV doesn't accept spaces in file names.  ^ is converted
        ## to space in srlib at the last moment.
        main::save("|checkpoint_save_delta^${baseName}^${fName}/save");
    }

    return 1;
}


##
## restore --
##     See save above for discussion of pipes.
##
sub restore($) {
    my ($fName) = @_;
    if (-d ${fName} ) {
        print "Restoring checkpoint from directory ${fName}\n";
        ## Hack -- SoftSDV doesn't accept spaces in file names.  ^ is converted
        ## to space in srlib at the last moment.
        ssdv::set_data("oml", "data", "notifier.save_mode", 0);
        main::restore("|checkpoint_restore^${fName}/save");
    }
    else {
        print "Restoring checkpoint from ${fName}\n";
        ## Standard compressed SoftSDV file
        ssdv::set_data("oml", "data", "notifier.save_mode", 3);
        main::restore(${fName});
    }
}


##
## set_mode_perf --
##     Switch to performance modeling mode.
##
sub set_mode_perf() {
    ssdv::set_data("oml", "data", "notifier.vpc.kernel.simul_mode", 2);
}


##
## set_mode_fast --
##     Switch to fast mode (no Asim connection).
##
sub set_mode_fast() {
    ssdv::set_data("oml", "data", "notifier.vpc.kernel.simul_mode", 4);
}


##
## start_warmup() --
##     Tell Asim to interpret instructions as warm-up data.
##
sub start_warmup() {
    my $t = localtime;
    print "Starting warmup at ${t}\n";
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 start_warmup");
}


##
## end_warmup() --
##     Tell Asim to interpret instructions as normal data.
##
sub end_warmup() {
    my $t = localtime;
    print "Ending warmup at ${t}\n";
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 end_warmup");
}


##
## start_simulation() --
##     Tell Asim simulation is beginning.
##
sub start_simulation() {
    my $t = localtime;
    print "Starting simulation at ${t}\n";
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 start_simulation");
    return time;
}


##
## end_simulation() --
##     Tell Asim simulation is ending.  Pass in the return value of
##     start_simulation().
##
sub end_simulation($) {
    my ($starttime) = @_;
    my $total = time - $starttime;
    my $t = localtime;
    print "Ending simulation at ${t}, wall time (seconds): ${total}\n";
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 end_simulation");
}


##
## start/end_lockstep() --
##     Control lock stepping with another SoftSDV process in performance mode.
##
sub start_lockstep() {
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 start_lockstep");
}

sub end_lockstep() {
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 end_lockstep");
}


##
## force_memory_values() --
##     Force collection of memory values in performance mode.
##
sub force_memory_values() {
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 force_memory_values");
}


##
## enable_eide_latency --
##     Turn on EIDE latencies for all devices on a bus.
##
sub enable_eide_latency($) {
    my ($bus) = @_;
    foreach my $device ( 0 .. 1 ) {
        ssdv::execute_command("kernel",
                              "vpc_command",
                              "vpc.eide latency $bus $device enable");
    }
}


##
## enable_scsi_latency --
##     Turn on SCSI latencies for all devices on a bus.
##
sub enable_scsi_latency($) {
    my ($bus) = @_;
    foreach my $device ( 0 .. 15 ) {
        ssdv::execute_command("kernel",
                              "vpc_command",
                              "scsi.1030 latency $bus $device enable");
    }
}


##
## disable_eide_latency --
##     Turn on EIDE latencies for all devices on a bus.
##
sub disable_eide_latency($) {
    my ($bus) = @_;
    foreach my $device ( 0 .. 1 ) {
        ssdv::execute_command("kernel",
                              "vpc_command",
                              "vpc.eide latency $bus $device disable");
    }
}


##
## disable_scsi_latency --
##     Turn off SCSI latencies for all devices on a bus.
##
sub disable_scsi_latency($) {
    my ($bus) = @_;
    foreach my $device ( 0 .. 15 ) {
        ssdv::execute_command("kernel",
                              "vpc_command",
                              "scsi.1030 latency $bus $device disable");
    }
}


##
## dump_screen_text --
##     Write guest screen to a file.
##
sub dump_screen_text($) {
    my ($fName) = @_;
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "dumper text -file=${fName}")
}


##
## dump_screen_text --
##     Write guest screen to a file.
##
sub dump_screen_vga($) {
    my ($fName) = @_;
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "dumper vga -file=${fName}")
}


##
## start_perf_lockstep --
##     Run two SoftSDV instances in performance mode lock-step using code
##     in Asim's CPUAPI client.  Each SoftSDV working directory must already
##     have an empty file named "lockstep_out".  Each must also have a link
##     named "lockstep_in" to the other's lockstep_out file.
##
sub start_perf_lockstep() {
    start_warmup();
    set_mode_perf();
    force_memory_values();
    start_lockstep();
}

############################################################################
#
#      U N I X    S T A T E
#
############################################################################

##
## set_unix_context_offsets --
##     get_unix_pid() and get_unix_process_name() as well as functions in Asim's
##     CPUAPI module compute process information by knowing the offsets of
##     the pid and the process name from the beginning of the kernel's
##     process context structure.  The offsets must be supplied here for
##     the functions to work.
##
sub set_unix_context_offsets($$$) {
    ($osName, $pidOffset, $pNameOffset) = @_;
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 context_switch_info $osName $pidOffset $pNameOffset");
}

##
## set_unix_pda_info --
##     Used by CPUAPI module for monitoring context switches.  The 64 bit
##     Linux kernel stores per-processor data in the PDA, pointed to
##     by %gs.  The first entry in PDA is a pointer to the task struct.
##     The VA of the PDA array (one entry is allocated in the array per
##     CPU) and the address in the kernel's __switch_to function AFTER
##     which the task struct pointer is updated on a context switch are
##     passed to this routine.
##
sub set_unix_pda_info($$) {
    my ($pdaVA, $switchToIP) = @_;
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 pda_info $pdaVA, $switchToIP");
}

##
## set_unix_timer_tag --
##     Tell Asim about the unix timer interrupt by passing the address of an
##     instruction that is executed once per timer interrupt.
##
sub set_unix_timer_tag($) {
    ($timerTag) = @_;
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 timer_interrupt_info $timerTag");
}

##
## set_unix_local_timer_tag --
##     Tell Asim about the local unix timer interrupt by passing the address
##     of an instruction that is executed once per local timer interrupt.
##
sub set_unix_local_timer_tag($) {
    my ($localTimerTag) = @_;
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 local_timer_interrupt_info $localTimerTag");
}

##
## set_unix_idle_tag --
##     Tell Asim about the unix idle loop by passing the address of an
##     instruction that is executed once per trip of the loop.
##
sub set_unix_idle_tag($) {
    ($idleTag) = @_;
    ssdv::execute_command("kernel",
                          "vpc_command",
                          "vpc.cpu ext 0 1 idle_loop_tag $idleTag");
}

##
## get_unix_pid() --
##     Look through the kernel's process context and find the current pid
##
our $lastGetUnixPidSWC = -1;
our $lastUnixPid = 0;

sub get_unix_pid() {
    return -1 if ($pidOffset == 0);

    # x86 not yet supported
    return -1 if ($main::config{'arch'} =~ /x86/);

    ## AR6 holds the physical address of the current process context
    my $swContext = get_value("data.arch.register.ar.6");

    ## If the process hasn't changed since last time avoid the memory read
    my $pid = 0;
    if ($swContext == $lastGetUnixPidSWC) {
        $pid = $lastUnixPid;
    }
    else {
        ## Get the pid from memory
        $pid = read_memory($swContext + $pidOffset, 4);
        $lastGetUnixPidSWC = $swContext;
        $lastUnixPid = $pid;
    }

    return $pid;
}

##
## get_unix_process_name() --
##     Look through the kernel's process context and find the name of the
##     current process.
##
our $lastGetUnixPNameSWC = -1;
our $lastUnixPName = "";

sub get_unix_process_name() {
    return "<unknown>" if ($pNameOffset == 0);

    # x86 not yet supported
    return -1 if ($main::config{'arch'} =~ /x86/);

    ## AR6 holds the physical address of the current process context
    my $swContext = get_value("data.arch.register.ar.6");

    ## If the process hasn't changed since last time avoid the memory read
    my $pName = "";
    if ($swContext == $lastGetUnixPNameSWC) {
        $pName = $lastUnixPName;
    }
    else {
        foreach my $n (0..15) {
            my $c = read_memory($swContext + $pNameOffset + $n, 1);
            last if ($c == 0);
            $pName .= chr($c);
        }

        $lastGetUnixPNameSWC = $swContext;
        $lastUnixPName = $pName;
    }

    return $pName;
}


############################################################################
#
#      T R A C I N G
#
############################################################################

##
## do_trace --
##     Internal subroutine.  Trace either to stdout or to a file.
##
sub do_trace($$) {
    my ($nInstrs, $oFile) = @_;

    if ($oFile ne "") {
        open(TRACE, ">>$oFile") or die "Can't open ${oFile}: $!";
    }

    while ($nInstrs--) {
        main::step(1);

        my $handle = ssdv::get_data_handle("cpu", "disasm");
        my $d = ssdv::data_handle_to_str($handle);
        ssdv::release_data_handle($handle);

        chomp($d);
        $d =~ s/\n//g;
        $d =~ s/disassembly  address: //;
        $d =~ s/ predicate://;
        $d =~ s/ instruction://;
        ## Shift the non-predicated instrs over so everything lines up
        $d =~ s/([012])  ([^p][^0-9])/$1      $2/;

        my $trc = get_unix_pid() . "/" . get_unix_process_name() . ":  ${d}";
        if ($oFile ne "") {
            print TRACE "${trc}\n";
        }
        else {
            print "${trc}\n";
        }
    }

    close(TRACE) if ($oFile ne "");
}


##
## trace --
##     Trace n instrs and write them to stdout.
##
sub trace($) {
    my ($nInstrs) = @_;
    do_trace($nInstrs, "");
}

##
## trace --
##     Trace n instrs and write them to file name in 2nd argument.
##
sub trace_to_file($$) {
    my ($nInstrs, $oFile) = @_;
    do_trace($nInstrs, $oFile);
}


############################################################################
#
#      F L O W    C O N T R O L    &    M O N I T O R I N G
#
############################################################################

##
## request_benchmark --
##   Wait for host to request a benchmark and tell it whether to load a
##   benchmark script.  Pass 1 to tell the client to load a benchmark
##   from the host and pass 0 to have the client return to a shell prompt.
##
sub request_benchmark($)
{
    my ($response) = @_;

    my $mark_bp = ssdv::set_breakpoint("oml", "event", "event.simul_app.mark");

    ## Put this here because SoftSDV seems not to work well when the response
    ## is set after the mark is reached.
    ssdv::set_data("oml", "data", "data.simul_app.mark_ret", $response);
    
    my $done = 0;
    while ( ! $done ) {
        main::cont();
        if (ssdv::breakpoint_is_hit($mark_bp)) {
            if (get_value("data.simul_app.mark") == $asim_mark_req_int) {
                ssdv::set_data("oml", "data", "data.simul_app.mark_ret", $response);
                $done = 1;
            }
        }
        else {
            # Some other event happened.  ^c?  Just return.
            print "Aborting script...\n";
            ssdv::remove_breakpoint($mark_bp);
            return 0;
        }
    }

    ssdv::remove_breakpoint($mark_bp);

    print "Starting benchmark...\n";
    return 1;
}


##
## wait_for_tag --
##   After calling request_benchmark call this routine to wait until the
##   benchmark hits a mark instruction.  The first argument is the value of
##   the mark from the guest (typically $asim_mark_start) and the second
##   argument is the number of times the mark should be raised before
##   considering the benchmark started.
##
sub wait_for_tag($$)
{
    my ($mark, $hits) = @_;

    my $mark_bp = ssdv::set_breakpoint("oml", "event", "event.simul_app.mark");
    
    while ( $hits > 0 ) {
        main::cont();
        if (ssdv::breakpoint_is_hit($mark_bp)) {
            my $m = get_value("data.simul_app.mark");
            if ($m == $mark) {
                $hits -= 1;
            }
        }
        else {
            # Some other event happened.  ^c?  Just return.
            print "Aborting script...\n";
            ssdv::remove_breakpoint($mark_bp);
            return 0;
        }
    }

    ssdv::remove_breakpoint($mark_bp);

    return 1;
}


##
## wait_for_boot --
##     Wait for an OS to boot and send an SSC mark.
##
sub wait_for_boot()
{
    # Boot script requests fast clock so time passes quickly
    my $result = wait_for_tag($asim_mark_fast_clock, 1);
    return 0 if ($result == 0);
    print "Setting fast clock.\n";
    set_nsec_per_inst(50.0);

    # Boot script requests normal clock
    $result = wait_for_tag($asim_mark_slow_clock, 1);
    return 0 if ($result == 0);
    print "Setting normal clock.\n";
    set_nsec_per_inst(1.0);

    $result = wait_for_tag($asim_mark_booted, 1);
    print "Booted...\n" if ($result);
    return $result;
}


##
## wait_for_exit --
##     Wait for the full simulation to complete.
##
sub wait_for_exit()
{
    ##
    ## Wait for 2 markers.  The first is signalled when the program exits and
    ## the second after output files are copied to the host.
    ##
    my $result = wait_for_tag($asim_mark_done, 1);
    print "Simulation complete.\n" if ($result);
    return $result;
}


##
## count_marks --
##     Count markers in the guest in an interval.  Returns an array of counters
##     indexed by the marker value.  Takes the number of instructions in the
##     interval as an argument.
##
sub count_marks($)
{
    my ($instrs) = @_;

    my $oldView = get_value("notifier.view_cpu");
    ssdv::set_data("oml", "data", "notifier.view_cpu", 0);

    my @marks = ();
    # Define one entry so the error exit case is the only one returning undef
    $marks[0] = 0;
    my $lastIcount = main::get_icount();

    my $markBp = ssdv::set_breakpoint("oml", "event", "event.simul_app.mark");

    while ($instrs > 0) {
        main::step($instrs);

        ssdv::set_data("oml", "data", "notifier.view_cpu", 0);
        my $newIcount = main::get_icount();
        $instrs = $instrs - ($newIcount - $lastIcount);
        $lastIcount = $newIcount;

        if (ssdv::breakpoint_is_hit($markBp)) {
            my $m = get_value("data.simul_app.mark");
            $marks[$m] += 1;
            # Early exit if the guest signals the program's end
            last if ($m == $asim_mark_end);
        }
        else {
            if ($instrs > 0) {
                # Probably a ^c.  Return undef.
                ssdv::remove_breakpoint($markBp);
                ssdv::set_data("oml", "data", "notifier.view_cpu", $oldView);
                return ();
            }
        }
    }

    ssdv::remove_breakpoint($markBp);

    ssdv::set_data("oml", "data", "notifier.view_cpu", $oldView);
    return @marks;
}


##
## standard_boot --
##     The standard boot sequence.  The function is here so workloads can
##     define their own benchmark package without needing to replicate the
##     boot sequence.
##
sub standard_boot() {
    print "SoftSDV time slice is " . asim::time_slice() . "...\n";

    # Run with high nsecPerInst to get through BIOS but switch to real
    # value before kernel starts so timing loops are correct.
    if ($main::config{'arch'} =~ /x86/) {
        # The Twincastle BIOS reads the timestamp counter twice per CPU.
        # After that it is the kernel.
        my $tscBp = ssdv::set_breakpoint("oml", "event", "event.arch.callback_funcs.rdtsc_func");
        for (my $i = 0; $i < 1 + asim::num_cpus() * 2; ++$i) {
            main::cont();
            last if (! ssdv::breakpoint_is_hit($tscBp)); # ^C ?
        }
        ssdv::remove_breakpoint($tscBp);
    }
    else {
        # IPF
        print "Figuring out whether SCSI is available.  You can ignore an error here...\n";
        my $hasSCSI = ssdv::get_data_handle("oml", "data", "vpc.storage.scsi.data.bus.0.enabled");
        my $initCount = 250000000;
        if ($hasSCSI) {
            print "Found SCSI...\n";
            $initCount = 700000000;
            ssdv::release_data_handle($hasSCSI);
        }

        print "Stepping for $initCount cycles with fast clock...\n";
        main::step($initCount);
    }

    # Configure the clock for 1 GHz @ 1 IPC.  The clock must be set before
    # the OS boots so that OS parameters are set correctly.
    set_nsec_per_inst(1.0);

    print "Waiting for OS to boot...\n";
    my $ok = asim::wait_for_boot();
    print "OS booted\n";

    asim::enable_eide_latency(0);
    asim::enable_eide_latency(1);

    if ($ok) {
        asim::save($main::config{'save_name'});
        if ($main::config{'os'} =~ /Linux/) {
            asim::dump_screen_text('screen_output.txt');
        }
        else {
            asim::dump_screen_vga('screen_output.vga');
        }
    }

    return $ok;
}


##
## manage_warmup() --
##     This is a helper function for the run() subroutine in the benchmark
##     package.  Since warm-up is typically the same even when a custom
##     run script is provided that overrides benchmark.pm it is stored here.
##
##     This function checks whether warm-up instructions exist for the workload
##     and runs the warm-up routine.
##
##     *** THIS FUNCTION ALWAYS LEAVES THE SIMULATION IN PERFORMANCE MODE ***
##
sub manage_warmup() {
    if (defined($main::config{'warmup_chkpt_loops'})) {
        # Warm-up counts are loop trip based
        manage_warmup_loops();
    }
    else {
        # Warm-up counts are instruction count based
        manage_warmup_instrs();
    }

    asim::set_mode_perf();
}


##
## manage_warmup_instrs() --
##     Not to be called from outside this module.  Called only by manage_warmup()
##     for instruction count based warm-up.
##
sub manage_warmup_instrs() {
    my $availWarmupInstrs = $main::config{'warmup_chkpt_instrs'};
    if ($availWarmupInstrs > 0) {
        my $warmupInstrs = $availWarmupInstrs;
        
        ##
        ## Are we supposed to send all the warm-up instructions to Asim
        ## or just a subset?  This can be configured either in the .cfg
        ## file, which is pass in main::config or on the run script command
        ## line, which is passed as an environment variable.
        ##
        if (defined($main::config{'warmup_run_instrs'})) {
            $warmupInstrs = $main::config{'warmup_run_instrs'};
        }
        if (defined($ENV{'WARMUP_RUN_INSTRS'})) {
            $warmupInstrs = $ENV{'WARMUP_RUN_INSTRS'};
        }
        if ($warmupInstrs < 0 || $warmupInstrs > $availWarmupInstrs) {
            $warmupInstrs = $availWarmupInstrs;
        }

        my $skipWarmupInstrs = $availWarmupInstrs - $warmupInstrs;
        if ($skipWarmupInstrs) {
            print "Fast-forwarding through $skipWarmupInstrs extra warm-up cycles...\n";
            main::step($skipWarmupInstrs);
        }

        ##
        ## The start_warmup() call will automatically choose either fast
        ## forward or performance mode, depending on whether Asim wants
        ## warm-up data.
        ##
        if ($warmupInstrs > 0) {
            asim::start_warmup();
            if (get_value("notifier.vpc.kernel.simul_mode") != 2) {
                print "Skipping $warmupInstrs warm-up cycles...\n";
            }
            else {
                print "Warming up caches for $warmupInstrs cycles...\n";
            }
            main::step($warmupInstrs);
            asim::end_warmup();
        }
    }
}


##
## manage_warmup_loops() --
##     Not to be called from outside this module.  Called only by manage_warmup()
##     for loop trip based warm-up.
##
sub manage_warmup_loops() {
    my $availWarmupLoops = $main::config{'warmup_chkpt_loops'};
    if ($availWarmupLoops > 0) {
        my $warmupLoops = $availWarmupLoops;
        
        ##
        ## Are we supposed to send all the warm-up instructions to Asim
        ## or just a subset?  This can be configured either in the .cfg
        ## file, which is pass in main::config or on the run script command
        ## line, which is passed as an environment variable.
        ##
        if (defined($main::config{'warmup_run_loops'})) {
            $warmupLoops = $main::config{'warmup_run_loops'};
        }
        if (defined($ENV{'WARMUP_RUN_LOOPS'})) {
            $warmupLoops = $ENV{'WARMUP_RUN_LOOPS'};
        }
        if ($warmupLoops < 0 || $warmupLoops > $availWarmupLoops) {
            $warmupLoops = $availWarmupLoops;
        }

        my $skipWarmupLoops = $availWarmupLoops - $warmupLoops;
        if ($skipWarmupLoops) {
            print "Fast-forwarding through $skipWarmupLoops extra warm-up loop trips...\n";
            wait_for_tag($main::config{'loop_trip_marker'}, $skipWarmupLoops);
        }

        ##
        ## The start_warmup() call will automatically choose either fast
        ## forward or performance mode, depending on whether Asim wants
        ## warm-up data.
        ##
        if ($warmupLoops > 0) {
            asim::start_warmup();
            if (get_value("notifier.vpc.kernel.simul_mode") != 2) {
                print "Skipping $warmupLoops warm-up loop trips...\n";
            }
            else {
                print "Warming up caches for $warmupLoops loop trips...\n";
            }
            wait_for_tag($main::config{'loop_trip_marker'}, $warmupLoops);
            asim::end_warmup();
        }
    }
}


##
## step_uneven --
##     During a performance run the number of instructions stepped on each
##     CPU may be different since the model will make progress at different
##     rates on each CPU.  A normal call to step() would pick one CPU and
##     use it as the reference for the number of instructions.  This function
##     uses the average over all the CPUs.
##
sub step_uneven($)
{
    my ($instrs) = @_;

    ##
    ## Monitor marks so the end of the program can be detected
    ##
    my $markBp = ssdv::set_breakpoint("oml", "event", "event.simul_app.mark");

    my $nCpus = num_cpus();

    my @startICount;
    for (my $c = $nCpus - 1; $c >= 0; --$c) {
        ssdv::set_data("oml", "data", "notifier.view_cpu", $c);
        $startICount[$c] = main::get_icount();
    }

    # Break the requested number of instructions into 100 intervals.  We'll
    # sample the state every interval and figure out whether to continue.
    my $interval = sprintf("%.0f", $instrs / 100);
    my $remainder = $instrs;

    ##
    ## Loop, stepping $interval instructions at a time until the average
    ## number of instructions stepped per CPU is near the requested amount.
    ##
    while ($remainder > 1000) {
        my $lastIcount = main::get_icount();
        my $stepCnt = $interval < $remainder ? $interval : $remainder;
        main::step($stepCnt);

        if (ssdv::breakpoint_is_hit($markBp)) {
            # Mark instruction hit.  Is the program done?
            my $m = get_value("data.simul_app.mark");
            if (($m == $asim_mark_end) ||
                ($m == $asim_mark_done))
            {
                print "Reached end of program.\n";
                $remainder = 0;
                last;
            }
        }
        else {
            if (main::get_icount() < $lastIcount + $stepCnt) {
                # Didn't execute the requested number of instructions.
                # Assume ^c.
                print "Early termination from asim::step_uneven.  ^C hit?\n";
                $remainder = 0;
                last;
            }
        }

        my $nStepped = 0;
        for (my $c = $nCpus - 1; $c >= 0; --$c) {
            ssdv::set_data("oml", "data", "notifier.view_cpu", $c);
            $nStepped += (main::get_icount() - $startICount[$c]);
        }
        $remainder = sprintf("%.0f", $instrs - ($nStepped / $nCpus));
    }

    if ($remainder > 0) {
        main::step($remainder);
    }

    ssdv::remove_breakpoint($markBp);
}


##
## Hack -- older versions of SoftSDV don't use XML config files.
##
if (! -f "cfg.xml") {
    $oldVersion = 1;
}

1;
