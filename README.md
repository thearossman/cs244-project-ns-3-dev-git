**Fork of NS-3 repo for use in CS244 final project**

**Reproducing Network Research: Capacity of Ad-Hoc Wireless Networks (Li et. al., 2001).**

*Thea Rossman and Daniel Classon*

-----

This GitHub repository contains our scripts for reproducing two figures from the "Capacity of Ad-Hoc Wireless Networks". As documented in our final report, we replicate the paper's conclusion that 802.11 MAC limits network throughput, beyond the ceiling imposed by the physical layer, for chain-like topologies in which the node originating traffic experiences less contention that subsequent nodes. 

The original paper was written in *ns* with the CMU Wireless Extension, and we did not have access to the original scripts. We based the skeleton of our scripts on the nsnam.org example, "wifi-simple-adhoc-grid.cc", available [here](https://www.nsnam.org/doxygen/wifi-simple-adhoc-grid_8cc.html). 

Below is some justification of our parameters: 

*Physical layer: YansWifiPhy* 

**PCAP Data Link Type**: For tracing. We use DLT_IEEE802_11 to isolate MAC layer information, though DLT_IEEE802_11_RADIO can also be helpful.

As far as we can tell from toggling them, the remaining attributes are either overridden by the default value set by the standard (for example, SIFS is overridden), do not impact our results (e.g., propagation loss model is purely range-based and does not depend on TxGain and RxGain, so toggling these does not impact throughput), are incompatible with 802.11b, or throw errors when we try to configure them. 

*Channel: YansWifiChannel*

**Propagation Delay**: Propagation speed is constant at the speed of light. This is the model used in all of the wireless ns-3 examples that we looked at, and is consistent with what we would expect from radio waves.

**Propagation Loss**: Range-based propagation loss model, set to RANGE = 250m. See documentation of this model, and other options, [here](https://www.nsnam.org/docs/models/html/propagation.html). This sets the effective transmission range to 250m: "Receivers at or within MaxRange meters receive the transmission at the transmit power level. Receivers beyond MaxRange receive at power -1000 dBm (effectively zero)."

A simulation more true to the original would use a model that also simulates interference, such as one of the log-based models, to account for two-hop interference. Our argument is that the same pattern -- and effective isolation of the impacts of MAC -- exists with either propagation loss model. 

One workaround would be to try this experiment with the same propagation model and a 550m maximum range, but hard-code [IPv4 static routing tables](https://www.nsnam.org/doxygen/classns3_1_1_ipv4_static_routing.html) such that packets are still forwarded one hop at a time. Though the power levels would not be exact, this would simulate the impacts of interference. 

*Remote Station Manager*

**Constant Rate WiFi Manager**: The [remote station manager](https://www.nsnam.org/doxygen/classns3_1_1_constant_rate_wifi_manager.html) sets a rate control algorithm, if any. The constant rate WiFi manager disables rate control, ensuring data and control packets are transmitted at a single, constant rate. Our understanding is that this effectively disables adaptive bitrate, further isolating the impacts of MAC. 

**DsssRate2Mbps**: This is how we set the bitrate. The authors try to model the Lucent WaveLAN card, a radio card which used DSSS (direct-sequence spread spectrum), at the 2Mbps data rate. (Our information about the WaveLAN comes from [this Wikipedia page](https://en.wikipedia.org/wiki/WaveLAN) and [this paper](https://onlinelibrary.wiley.com/doi/abs/10.1002/bltj.2069?casa_token=dQehGJYeEmQAAAAA:oL2nbRUqNdcBhDEuDyadPjxrJKOxXzTZEh2pSWsb-0wbrloRg4gcwBis_zKHXiyhRJ3GCnUkRVBIGy4).)

Other attributes: 
- *RtsCtsThreshold*: in the paper, RTS/CTS exchange precedes every packet transmission. Thus, the minimum packet size for which RTS/CTS is required is the default value, 0. 
- *FragmentationThreshold*: the paper did not mention fragmentation, so we disable it by ensuring that this is impossibly large. 
- *MaxSsrc* and *MaxSlrc*: the paper did not mention a ceiling on retransmission attempts, and the 802.11 standard does not specify what this value should be or if it has changed in the past two decades. We could not justify increasing or decreasing this value from the default. 

The remaining attributes are either overridden by the helpers for compatibility (for example, HtProtectionMode is not defined for 802.11b) or do not yield a significant difference in results when we toggle them. (For example, toggling the default transmission power level has no impact on throughput, as long as it is non-zero, because our range-based propagation loss model does not depend on it.) 

*MAC layer: WifiMac* 

**Ad-Hoc Mode**: Self-explanatory. 

**WIFI_STANDARD_80211b**: The authors either use the original, legacy 802.11-1997 or 802.11b. They describe the 802.11 MAC protocol, which is the same for both 802.11 and 802.11b. 

The ssid attribute should not impact our results. 

*L3 and L4*

**OLSR**: Since routing is not a factor in our simulations -- nodes have one option for where to forward traffic -- we use the default. We give it 30 seconds to converge, based on the example that we based our simulation on. 

**UDPSocketFactory**: The authors say that the sender "sends packets as fast as its 802.11 allows." We interpret this as a one-way UDP connection. (When we ran this simulation over TCP, we got very low throughput -- which makes sense, due to the bidirectional communication.) 

We define throughput as (total application layer bytes received) / (total simulation time), though the authors do not specify how they perform their calculation. 

**Packet size**: We subtract approximate IP and UDP header length from packet size before transmitting, because we noticed from PCAP files that packets were being fragmented when we did not, and we could not justify changing the MTU from its default value of 1500. e.g., a 1500-byte packet becomes `1500 - 20 - 8` bytes of UDP payload. 
