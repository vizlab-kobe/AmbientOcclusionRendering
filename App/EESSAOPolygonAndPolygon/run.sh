#!/bin/bash
PROGRAM=${PWD##*/}

# Input files
FILENAME0=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_B_20190125.kvsml
#FILENAME1=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_V_20190125.kvsml
FILENAME1=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle90_B_20190125.kvsml
#FILENAME1=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_B_20190125.kvsml

TITLE="MHD Simulation"
LABEL0="|B| (angle60)"
LABEL1="|B| (angle90)"

# Initial parameters
REPEATS=20
RADIUS=0.5
POINTS=256
EDGE=1.0

./$PROGRAM \
    -title "$TITLE" \
    -file0 $FILENAME0 \
    -file1 $FILENAME1 \
    -label0 "$LABEL0" \
    -label1 "$LABEL1" \
    -ao \
    -lod \
    -r $REPEATS \
    -radius $RADIUS \
    -points $POINTS \
