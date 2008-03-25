#!/bin/sh

#
#  Copyright (C) 2003-2006 Intel Corporation
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
## Mount a workload CD, send an SSB mark to wait for the command to begin
## and invoke the workload.
##

##
## Environment variable allows programs to behave differently under simulation.
## Some of the SSB mark instructions fail outside the simulator.
##
SOFTSDV_GUEST=t
export SOFTSDV_GUEST

asimworkdir="/tmp_user/asimwork"
benchmarkname="benchmark"

##
## This script can run at most once on the machine
##
if [ -f /tmp/asim_got_workload ]; then
    exit 0
fi
touch /tmp/asim_got_workload

echo ""

##
## These could have been removed from the boot disk, but others already
## depend on that disk and changing it will invalidate checkpoints.
##
echo "Stopping some services that can get in the way..."
if [ -f /etc/rc.d/init.d/atd ]; then
    /etc/rc.d/init.d/atd stop
    /etc/rc.d/init.d/crond stop
    /etc/rc.d/init.d/cups stop
    /etc/rc.d/init.d/nfs stop
    /etc/rc.d/init.d/sshd stop
    /etc/rc.d/init.d/sendmail stop
    /etc/rc.d/init.d/xfs stop
else
    /etc/rc.d/atd stop
    /etc/rc.d/cron stop
    /etc/rc.d/cups stop
    /etc/rc.d/nfs stop
    /etc/rc.d/sshd stop
    /etc/rc.d/sendmail stop
    /etc/rc.d/xfs stop
fi

echo ""

echo "Adding swap and tmp_user..."
echo '0,' | sfdisk -L -q /dev/hdc
mkfs -t ext3 /dev/hdc1
mkdir /tmp_user
mount /dev/hdc1 /tmp_user

echo '0,,S' | sfdisk -L -q /dev/hdb
mkswap /dev/hdb1
echo /dev/hdb1 swap swap defaults 0 0 >>/etc/fstab
swapon -a
swapon -s

echo ""

echo "Copying tools from host..."
rm -rf /tmp_user/asim
mkdir /tmp_user/asim
(cd /tmp_user/asim; ssc_guest host:XXROOTXX/softsdv_XXARCHXX/scripts/client/asim_linux_tools.tar.gz - | tar xzvmf -)
chown root /tmp_user/asim/set_parent_priority
chmod u+s /tmp_user/asim/set_parent_priority
chown root /tmp_user/asim/affinity
chmod u+s /tmp_user/asim/affinity
chown root /tmp_user/asim/rr
chmod u+s /tmp_user/asim/rr
# iostat and mpstat are missing from some SoftSDV workload disks
if [ -f /tmp_user/asim/iostat ]; then
    mv /tmp_user/asim/iostat /usr/bin
fi
if [ -f /tmp_user/asim/mpstat ]; then
    mv /tmp_user/asim/mpstat /usr/bin
fi

PATH=/tmp_user/asim:$PATH
export PATH

##
## Add the oracle user here since the tar file loaded for oracle benchmarks
## later needs to load files owned by oracle
##
echo "Adding oracle user and dba group"
groupadd -g 502 dba
useradd -g dba -G dba,root -u 502 -d /tmp_user/oracle -s /bin/bash oracle

# Figure out the number of processors on the machine
NCPUS=`grep ^processor /proc/cpuinfo | wc -l | sed 's/ //g'`
export NCPUS
echo "$NCPUS CPUs"

# SpecOMP needs this
OMP_NUM_THREADS=$NCPUS
export OMP_NUM_THREADS

mark_send fast_clock
echo "Sleeping 300 (fast) seconds while machine quiets down..."
sleep 300
mark_send slow_clock
echo "Sleeping 2 (slow) seconds while machine quiets down..."
sleep 2

echo "Boot complete..."
##
## Send boot message and get response of what to do.  By default, do nothing.
## Sometimes the first boot mark doesn't get through so send 2.
##
mark_send booted
mark_send booted

## Ask SoftSDV host script what to do now
mark_send -r num
action=$?
## mark_send -r (SscSimulAppMarkRet) has been broken in so many SoftSDV releases
## I gave up.  Just force the action to 1 -- load a benchmark using ssc_guest.
action=1

rm -rf ${asimworkdir}
mkdir ${asimworkdir}
cd ${asimworkdir}
bmargs=""
bmofiles=""

case $action in
  1)
    ##
    ## Use ssc_guest to get the workload
    ##
    echo "Copying ${benchmarkname} from AUTO/${benchmarkname} using ssc_guest..."
    ssc_guest host:AUTO/${benchmarkname} - > ${asimworkdir}/${benchmarkname}
    chmod +x ${benchmarkname}

    ##
    ## Grab workload files and arguments for the benchmark from another file.
    ## Also get a list of output files requested.
    ##
    ssc_guest host:AUTO/${benchmarkname}.tar.gz - | tar xzvmf -
    bmargs=`ssc_guest host:AUTO/${benchmarkname}.args -`
    bmofiles=`ssc_guest host:AUTO/${benchmarkname}.ofiles -`
    ;;
  *)
    exit 0
    ;;
esac

echo
echo "Invoking: ${benchmarkname} ${bmargs}"
mark_send start
eval ./${benchmarkname} ${bmargs}
mark_send end

##
## Copy requested output files back to the host
##
if [ "${bmofiles}" != "" ]; then
    ssc_guest ${bmofiles} host:OUTPUT/.
fi

##
## Send end message again in case script wanted to copy output files to host
##
echo
echo "${benchmarkname} exited..."
mark_send end
mark_send done

##
## Benchmark isn't running -- exit.  The sleep 5 gives us a few seconds
## to hit ^c and get the shell back.
##
echo "Forcing simulator to stop in 5 seconds..."
sleep 5
force_exit
