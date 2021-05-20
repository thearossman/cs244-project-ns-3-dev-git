#!/bin/bash
[ ! -d outputs ] && mkdir outputs

TIME=300
DISTANCE=200
VERBOSE=0

#write to a single file, rewritten every run
filename="outputs/simple_results.txt"
if [ -f $filename ]; then
  rm "$filename"
fi
touch "$filename"

for PACKETSZ in 64 500 1500; do
	for CHAINLEN in 2 3 4 5 6 10; do
	    # Run the NS-3 Simulation
	    ./waf --run "scratch/wifi-simple-adhoc-grid.cc --simTime=$TIME --distance=$DISTANCE --packetSize=$PACKETSZ --numNodes=$CHAINLEN --printIntermediate=$VERBOSE"
	done
done

echo "Simulations are done!"

python3 scratch/plot-simple.py

echo "Plots are done!"
