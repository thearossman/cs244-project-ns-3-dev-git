import argparse
import os
import matplotlib.pyplot as plt
import re

sim_time = 0
per_data_rate = {}

FILENAME = 'outputs/figure_4_results.txt'

with open(FILENAME,'r') as f:
    for line in f:
        data = line.split()
        data_rate_str = data[0]
        data_rate_str = re.search("[0-9]*", data_rate_str).group()
        data_rate = int(data_rate_str)/1000.0 # converet Kbps to Mbps
        sim_time = float(data[1])
        throughput = float(data[2])

        per_data_rate[data_rate] = throughput

filename = 'outputs/figure_4_plots.png'

plt.figure()

data_rates = list(per_data_rate.keys())
throughputs = list(per_data_rate.values())
plt.plot(data_rates, throughputs, c="C2", marker='+')

plt.title('Recreation of Figure 4 for ' + str(sim_time) + 's')
plt.ylabel('Throughput (Mbps)')
plt.xlabel('Offered Load (Mbps)')
plt.grid()
plt.savefig(filename)
print('Saving ' + filename)
