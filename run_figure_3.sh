#!/bin/bash
[ ! -d outputs ] && mkdir outputs

TIME=300
DISTANCE=200
VERBOSE=0

#write to a single file, rewritten every run
filename="outputs/figure_3_results.txt"
if [ -f $filename ]; then
  rm "$filename"
fi
touch "$filename"

for PACKETSZ in 64 500 1500; do
	for CHAINLEN in 2 3 4 5 6 10; do
	    # Run the NS-3 Simulation
	    ./waf --run "scratch/figure-3.cc --simTime=$TIME --distance=$DISTANCE --packetSize=$PACKETSZ --numNodes=$CHAINLEN --printIntermediate=$VERBOSE"
	done
done

echo "Simulations are done!"

python3 scratch/plot-figure-3.py

echo "Plots are done!"
