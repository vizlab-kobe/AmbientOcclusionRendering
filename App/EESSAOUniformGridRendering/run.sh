#!/bin/bash
PROGRAM=${PWD##*/}

FILENAME=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_B_20190125.kvsml
#FILENAME=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_V_20190125.kvsml
#FILENAME=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle90_B_20190125.kvsml
#FILENAME=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle90_V_20190125.kvsml

REPEATS=20
RADIUS=0.5
POINTS=256

./$PROGRAM $FILENAME \
    -ao \
    -lod \
    -r $REPEATS \
    -radius $RADIUS \
    -points $POINTS \
