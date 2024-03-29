#!/bin/bash
PROGRAM=${PWD##*/}

# Input file
#FILENAME=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_B_20190125.kvsml
FILENAME=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle90_V_20190125.kvsml

TITLE="MHD Simulation"
LABEL="Velocity |V|"

# Initial parameters
SCALE=100
REPEATS=20
RADIUS=0.5
POINTS=256
EDGE=1.0
MIN_COORD="0 0 0"
MAX_COORD="250 250 250"
STRIDE="30 30 30"

./$PROGRAM $FILENAME \
    -title "$TITLE" \
    -label "$LABEL" \
    -ao \
    -lod \
    -s $SCALE \
    -r $REPEATS \
    -radius $RADIUS \
    -points $POINTS \
    -edge $EDGE \
    -min_coord $MIN_COORD \
    -max_coord $MAX_COORD \
    -stride $STRIDE
