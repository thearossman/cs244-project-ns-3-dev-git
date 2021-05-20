import argparse
import os
import matplotlib.pyplot as plt

sim_time = 0
per_packet_sz = {}

with open('outputs/simple_results.txt','r') as f:
    for line in f:
        data = line.split()
        packet_sz = int(data[0])
        chain_len = int(data[1])
        sim_time = float(data[2])
        throughput = float(data[3])

        per_chain_len = {}
        if packet_sz in per_packet_sz:
            per_chain_len = per_packet_sz[packet_sz]
        
        per_chain_len[chain_len] = throughput
        per_packet_sz[packet_sz] = per_chain_len

filename = 'outputs/simple_plots.png'

plt.figure()
c = 1 #color value
for packet_sz in per_packet_sz:
    per_chain_len = per_packet_sz[packet_sz]

    chain_lens = per_chain_len.keys()
    throughputs = per_chain_len.values()
    plot_name = "" + str(packet_sz) + " byte packets"
    plt.plot(chain_lens, throughputs, c="C" + str(c), label=plot_name, marker='+')

    print("plotting for ", str(packet_sz))

    c += 1

plt.title('Recreation of Figure 3 for ' + str(sim_time) + 's')
plt.ylabel('Chain Throughput (Mbps)')
plt.xlabel('Chain Length (Number of Nodes)')
plt.grid()
plt.legend()
plt.savefig(filename)
print('Saving ' + filename)
