#!/bin/bash

set -u
set -e

results_path=$(readlink -f `dirname "$0"`)

#declare -a nodes=("zen1" "zen2" "zen3" "zen4" "zen5" "coffee" "raptor")
# declare -a nodes=("zen3" "zen4" "zen5" "coffee" "raptor")
declare -a nodes=("zen5")
# declare -a nodes=("zen4" "zen5" "coffee" "raptor")

results_dump_path="$results_path/dump"

for node in "${nodes[@]}"; do 
    echo "Get from $node"
    to="$results_dump_path/$node"
    mkdir -p $to
    scp -r "$node:~/unixbench/UnixBench/results" "$to"
done
