#!/bin/bash

SEGMENT="${1}"
TEST_ID="${2}"
FLOW_FLAG="${3}"
INSTALLDIR="${4}"
MACRODIR="${5}"
MACRO="${6}"
OUTDIR_BASE="${7}"
echo "Running on host: $(hostname), SEGMENT: $SEGMENT, TEST_ID: $TEST_ID, FLOW_FLAG: $FLOW_FLAG, INSTALLDIR: $INSTALLDIR, MACRODIR: $MACRODIR, MACRO: $MACRO OUTDIR_BASE: $OUTDIR_BASE"

# src env
source /opt/sphenix/core/bin/sphenix_setup.sh -n new
source $OPT_SPHENIX/bin/setup_local.sh $INSTALLDIR

cd "$MACRODIR" || exit 1

RUN=31
JETSAMPS=(10 20 30)
OUTCODE="All executions completed successfully"
ERRCODE=""
for JET in "${JETSAMPS[@]}"; do
    echo "Running with RUN = $RUN and JET = $JET"
    OUTDIR="${OUTDIR_BASE}/run${RUN}_$( [ $JET -eq 0 ] && echo "mb" || echo "jet${JET}" )_flow${FLOW_FLAG}"
    mkdir -p "$OUTDIR"
    OUTFILE="${OUTDIR}/$( [ $JET -eq 0 ] && echo "mb" || echo "jet${JET}" )_flow${FLOW_FLAG}-$(printf "%05d" $SEGMENT).root"
    root -l -b -q "Fun4All_Sim.C(10, ${RUN}, ${SEGMENT}, ${JET}, ${FLOW_FLAG}, ${TEST_ID}, \"${OUTFILE}\")"
    EXITCODE=$?
    if [ $EXITCODE -ne 0 ]; then
        echo "Error: Fun4All_Sim.C failed with exit code $EXITCODE"
        ERRCODE="${ERRCODE}Fun4All_Sim.C failed for RUN=${RUN}, JET=${JET} with exit code ${EXITCODE}\n"
        continue    
    fi
done
if [ -n "$ERRCODE" ]; then
    echo -e "Some executions failed:\n$ERRCODE"
    exit 1
else
    echo "$OUTCODE"
    exit 0
fi
