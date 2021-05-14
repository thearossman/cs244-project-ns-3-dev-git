#!/bin/bash
[ ! -d outputs ] && mkdir outputs

TIME=20
BWNET=10
DELAY=10
CHAINLEN=2

# for CHAINLEN in 2 10; do
    # Run the NS-3 Simulation
    ./waf --run "scratch/adhoc-wireless-simple.cc --bwNet=$BWNET --delay=$DELAY --time=$TIME --chainlen=$CHAINLEN"
# done

echo "Simulations are done! Results can be retrieved via the server"