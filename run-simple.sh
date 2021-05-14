#!/bin/bash
[ ! -d outputs ] && mkdir outputs

TIME=20
BWNET=10
DELAY=10

for CHAINLEN in 2 2; do
    # Run the NS-3 Simulation
    ./waf --run "scratch/adhoc-wireless-simple.cpp --bwNet=$BWNET --delay=$DELAY --time=$TIME --chainlen=$CHAINLEN"
done

echo "Simulations are done! Results can be retrieved via the server"