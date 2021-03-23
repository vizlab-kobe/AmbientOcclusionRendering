#!/bin/bash
PROGRAM=${PWD##*/}

# Input files
FILENAME1=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_B_20190125.kvsml
FILENAME2=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_V_20190125.kvsml

TITLE="MHD Simulation"
LABEL1="Isosurfaces |B|"
LABEL2="Streamlines |V|"

# Initial parameters
SCALE=100
REPEATS=20
RADIUS=0.5
POINTS=256
EDGE=1.0
MIN_COORD="0 0 0"
MAX_COORD="250 250 250"
STRIDE="30 30 30"

./$PROGRAM \
    -title "$TITLE" \
    -isosurface_file $FILENAME1 \
    -streamline_file $FILENAME2 \
    -isosurface_label "$LABEL1" \
    -streamline_label "$LABEL2" \
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
