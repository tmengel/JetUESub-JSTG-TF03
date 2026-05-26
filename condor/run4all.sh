#!/bin/bash

FLOW_FLAG=3
INSTALLDIR="/sphenix/user/tmengel/JetUESub-JSTG-TF03/install"
MACRODIR="/sphenix/user/tmengel/JetUESub-JSTG-TF03/macros/sim"
TODAY="$(date +%Y%m%d)"
OUTDIR="/sphenix/tg/tg01/jets/tmengel/jstgtf03/sim/${TODAY}/flow${FLOW_FLAG}"
mkdir -p "$OUTDIR"

MACRO="Fun4All_Sim.C"

HOSTNUM=$(hostname | grep -oP 'sphnxuser\K[0-9]+' || echo 0)
HOSTNUM=$((10#$HOSTNUM))
MYNUM=${1:-$HOSTNUM}
echo "Running on host: $(hostname), MYNUM: $MYNUM"

mkdir -p "$OUTDIR/test${MYNUM}/"

Executable="$(pwd)/driver.sh"
logDir="$(pwd)/logs/test${MYNUM}"
mkdir -p "$logDir"

condor_submit <<EOF
Universe        = vanilla
Executable      = ${Executable}
request_memory  = 3GB
Priority        = 20
PeriodicHold    = (NumJobStarts >= 1 && JobStatus == 1)

Log             = /tmp/condorjob-${USER}.log
Output          = ${logDir}/\$(Process).out
Error           = ${logDir}/\$(Process).err

batch_name      = test${MYNUM}

Arguments       = "\$(Process) ${MYNUM} ${FLOW_FLAG} ${INSTALLDIR} ${MACRODIR} ${MACRO} ${OUTDIR}/test${MYNUM}"

Queue 1
EOF