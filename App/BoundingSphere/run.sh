#!/bin/bash
PROGRAM=${PWD##*/}

FILENAME=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_B_20190125.kvsml
FILENAME2=~/Work/Data/MHDData/data_from_Ueda_20190125/result4_v_from_Ueda_angle60_V_20190125.kvsml

SCALE=100
REPEATS=40
RADIUS=0.5
POINTS=256
MIN_COORD="0 0 0"
MAX_COORD="250 250 250"

./$PROGRAM $FILENAME $FILENAME2
