#!/bin/bash

source /opt/sphenix/core/bin/sphenix_setup.sh -n new
export INSTALLDIR="/sphenix/user/tmengel/PPG14/install"
source $OPT_SPHENIX/bin/setup_local.sh $INSTALLDIR

MACRODIR="/sphenix/user/tmengel/PPG14/macros/truthjet/fun4all_sim"

TODATE="april30"
OUTBASE="/sphenix/tg/tg01/jets/tmengel/mdc2/${TODATE}"
mkdir -p "$OUTBASE"

PROC="${1}"

RUNS=(31)
PTJETS=(10 20 30)
CALOSCALING=(0)
FLOWFLAGS=(0 1 2)
for RUN in "${RUNS[@]}"; do
    for PTJET in "${PTJETS[@]}"; do
        for SCALE_CALO in "${CALOSCALING[@]}"; do
            for FLOWFLAG in "${FLOWFLAGS[@]}"; do

                cd "$MACRODIR" || exit 1
                echo "Running with RUN = $RUN, PTJET = $PTJET and SCALE_CALO = $SCALE_CALO"
                
                OUTDIR="${OUTBASE}/run${RUN}_$( [ $PTJET -eq 0 ] && echo "mb" || echo "jet${PTJET}" )_$( [ $SCALE_CALO -eq 1 ] && echo "scaled_${TODATE}" || echo "${TODATE}" )_flow${FLOWFLAG}"
                mkdir -p "$OUTDIR"
                OUTFILE="${OUTDIR}/$( [ $PTJET -eq 0 ] && echo "mb" || echo "jet${PTJET}" )_flow${FLOWFLAG}-$(printf "%05d" $PROC).root"
                # root -l -b -q "Fun4All_Sim.C(-1, ${RUN}, $PROC, \"$OUTFILE\", \"MDC2\", $PTJET, $SCALE_CALO, $FLOWFLAG)"
                # EXITCODE=$?
                # if [ $EXITCODE -ne 0 ]; then
                #     echo "Error: Fun4All_Run2AuAu.C failed with exit code $EXITCODE"
                #     continue
                # fi
                    
                OUTDIR_MATCHED="${OUTDIR}_matched_v2"
                mkdir -p "$OUTDIR_MATCHED"
                MATCHED_FILE="${OUTDIR_MATCHED}/$( [ $PTJET -eq 0 ] && echo "mb" || echo "jet${PTJET}" )_flow${FLOWFLAG}-$(printf "%05d" $PROC).root"
                cd "/sphenix/user/tmengel/PPG14/macros/truthjet/" || exit 1
                root -l -b -q "match_mb.C(\"$OUTFILE\", \"$MATCHED_FILE\")"
                EXITCODE=$?
                if [ $EXITCODE -ne 0 ]; then
                    echo "Error: match.C failed with exit code $EXITCODE"
                    continue
                fi
                echo "Execution completed successfully for RUN = $RUN, PTJET = $PTJET and SCALE_CALO = $SCALE_CALO"
            done
        done
    done
done

echo "All executions completed successfully"
