**Fork of NS-3 repo for use in CS244 final project**

**Reproducing Network Research: Capacity of Ad-Hoc Wireless Networks (Li et. al., 2001).**

*Thea Rossman and Daniel Classon*

-----

This GitHub repository contains our scripts for reproducing two figures from the "Capacity of Ad-Hoc Wireless Networks". As documented in our final report, we replicate the paper's conclusion that 802.11 MAC limits network throughput, beyond the ceiling imposed by the physical layer, for chain-like topologies in which the node originating traffic experiences less contention that subsequent nodes. 

The original paper was written in *ns* with the CMU Wireless Extension, and we did not have access to the original scripts. We based the skeleton of our scripts on the nsnam.org example, "wifi-simple-adhoc-grid.cc", available [here](https://www.nsnam.org/doxygen/wifi-simple-adhoc-grid_8cc.html). 

Below is a full justification of our parameters: 

*Physical layer* 

DsssRate2Mbps: The authors use the Lucent WaveLAN card at a 2Mbps data rate. Most of what we know about this is from [this Wikipedia page](https://en.wikipedia.org/wiki/WaveLAN) and [this paper](https://onlinelibrary.wiley.com/doi/abs/10.1002/bltj.2069?casa_token=dQehGJYeEmQAAAAA:oL2nbRUqNdcBhDEuDyadPjxrJKOxXzTZEh2pSWsb-0wbrloRg4gcwBis_zKHXiyhRJ3GCnUkRVBIGy4)). We know about this was that it was a radio card which used DSSS (direct-sequence spread spectrum). The authors specify that they use the 2Mbps data rate. 

*MAC layer* 

WIFI_STANDARD_80211b
