#!/bin/bash
[ ! -d outputs ] && mkdir outputs

TIME=300
DISTANCE=200
VERBOSE=0

#write to a single file, rewritten every run
filename="outputs/figure_8_results.txt"
if [ -f $filename ]; then
  rm "$filename"
fi
touch "$filename"

for PACKETSZ in 500 1500 64; do
	for CHAINLEN in 3 4 5 6 7; do
	    # Run the NS-3 Simulation
	    ./waf --run "scratch/figure-8.cc --simTime=$TIME --distance=$DISTANCE --packetSize=$PACKETSZ --chainLen=$CHAINLEN"
	done
done

echo "Simulations are done!"

python3 scratch/plot-figure-8.py

echo "Plots are done!"
