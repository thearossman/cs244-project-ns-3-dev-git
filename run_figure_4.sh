#!/bin/bash
[ ! -d outputs ] && mkdir outputs

TIME=300
VERBOSE=0

#write to a single file, rewritten every run
filename="outputs/figure_4_results.txt"
if [ -f $filename ]; then
  rm "$filename"
fi
touch "$filename"
#0.32 0.34 0.35 0.37 0.39 0.4 0.41 0.42 0.43 0.45 0.5 0.55
for DATARATE in 0.3 0.4 0.5 0.6 0.7 0.8 0.9; do
	    # Run the NS-3 Simulation
	    ./waf --run "scratch/figure-4.cc --simTime=$TIME --dataRate=$DATARATE --printIntermediate=$VERBOSE"
done

echo "Simulations are done!"

python3 scratch/plot-figure-4.py

echo "Plots are done!"
