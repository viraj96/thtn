#!/bin/bash

ROOTDIR="$HOME/5_requests/*"
for file in $ROOTDIR
do
    echo $file
    cp $file ./experiments/thtn/12_blocks_5_requests.hddl
    cd build
    ./planner ../experiments/thtn/timeline_domain.hddl ../experiments/thtn/12_blocks_5_requests.hddl --max-recursive-depth 2 --attempts -1
    cd ..
done
