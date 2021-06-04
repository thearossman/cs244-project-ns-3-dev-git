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

for DATARATE in "320Kbps" "340Kbps" "350Kbps" "400Kbps" "420Kbps" "430Kbps" "450Kbps" "500Kbps" "550Kbps"; do
	    # Run the NS-3 Simulation
	    ./waf --run "scratch/figure-4.cc --simTime=$TIME --dataRate=$DATARATE --printIntermediate=$VERBOSE"
done

echo "Simulations are done!"

python3 scratch/plot-figure-4.py

echo "Plots are done!"
