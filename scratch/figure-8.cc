/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

/* This code is heavily based on the "wifi-simple-adhoc-grid" example 
 * from ns-3, which is available here:
 * https://www.nsnam.org/doxygen/wifi-simple-adhoc-grid_8cc_source.html
 *  
 * The original code is Copyright (c) 2009 University of Washington
 * and released under the GNU General Public License v2. 
 */

// This program configures a chain (default 8) of nodes on an
// 802.11b physical layer, with 802.11b NICs in adhoc mode. 

// The first node in the topology sends packets over a UDP connection, 
// as fast as its protocol allows, to the last node in the topology. 
//
// Layout is like this, on a square lattice: 
//
// n00   n01   n02   n03   n04 ...
// n10   n11   n12   n13   n14 ...
// 
// Traffic patterns are horizontal and parallel, from n*0->n*(n-1).

// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc-grid --help"
//
// The script measures total throughput by calculating the total
// application-layer bytes received by the UDP receiver. 

#include <iostream>
#include <fstream>

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/tcp-header.h"
#include "ns3/core-module.h"
#include "ns3/packet.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/packet-sink.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");


/* Not used unless "printIntermediate" is selected. 
   Useful if we want to re-calculate throughput 
   (e.g., for logging, to observe fluctuations, etc.) over the course of 
   the simulation. */
// uint64_t lastTotalRx = 0; 
// Ptr<PacketSink> sink;
// static void CalculateThroughput ()
// { 
//   // Get the current time 
//   Time now = Simulator::Now ();
//   // Total Mbits received by the UDP receiver so far
//   // Note: GetTotalRx = measured in bytes. 
//   // I think bytes received by the application (e.g., not counting headers). 
//   double totalRx = sink->GetTotalRx () * 8 / 1e6; 
//   // Total Mbits received in the last second. 
//   double cur = totalRx - lastTotalRx; 
//   // Print accounting 
//   std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
//   // Update lastTotalRx
//   lastTotalRx = totalRx;
//   // Call again in a second (should be called every seconds)
//   Simulator::Schedule (Seconds (1), &CalculateThroughput);
// }


// Give OLSR time to converge (e.g., populate routing tables). 
double OLSR_CONVERGE_TIME = 30.0;

// Effective transmission range of the nodes. 
// (Used to set range-based propagation loss model.) 
double RANGE = 250.0;

// Used to set payload size for UDP packets.
int UDP_HDR_BYTES = 8;
int IP_HDR_MIN_BYTES = 20;

int main (int argc, char *argv[])
{

  /*** DECLARE PARAMETERS ***/
  
  std::string phyMode ("DsssRate2Mbps");
  double distance = 200;  // m
  double simTime = 300.0; // seconds
  uint32_t packetSize = 1500; // bytes (including IP and UDP hdrs)
  uint32_t chainLen = 8;
  // bool printIntermediate = false;
  bool verbose = false;
  bool tracing = false;
  std::string dir = "outputs"; 
 
  CommandLine cmd (__FILE__);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("simTime", "length of simulation (seconds)", simTime);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("chainLen", "number of nodes", chainLen);\
  cmd.Parse (argc, argv);

  // Source is always chain start and sink is always chain end
  uint32_t sinkNodes[chainLen];
  uint32_t sourceNodes[chainLen];
  for (uint32_t i = 0; i < chainLen; i++) {
    sinkNodes[i] = i * chainLen;
    sourceNodes[i] = (i + 1) * chainLen - 1;

    std::cout << sinkNodes[i] << " " << sourceNodes[i] << std::endl;
  }

  Ptr<PacketSink> sinkPtrs[chainLen];

  // if (sinkNode <= sourceNode) {
  //   std::cerr << "Chain must be of length at least 2" << std::endl;
  //   return 1;
  // }
  
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));
   
  /*** CREATE NODES ***/
  NodeContainer c;
  c.Create (chainLen * chainLen);

  /*** PUT TOGETHER WIFI HELPERS ***/

  /** WiFi network device helper **/
  // Container for WiFi Net Device objects, which hold together everything 
  // needed for a set of WiFi devices. (address, channel, settings, etc.)
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }

  /** Physical layer WiFi network device helper **/
  // For physical layer objects, modeled using the YANS WiFi framework.
  // (YANS = Yet Another Network Simulator; just offers a specific 
  // propagation model. PHY = physical layer.)
  YansWifiPhyHelper wifiPhy;

  // NOTE: ideally, we want to model what they modeled in the paper - 
  // the Lucent WaveLAN Card (2001). Since little documentation is available 
  // for this card, our parameters may be slightly different than those used
  // in the original paper.  

  // NS--3 supports RadioTap and Prism tracing extensions for 802.11b
  // This is only used if tracing is enabled. 
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);

  /** Channel helper **/
  // Channel means everything involved in getting packets between devices. 
  // The main things we model here are propagation delay and propagation loss. 
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
 
  // This sets the propagation loss model for the WiFi channel.
  // Drop packets outside of the effective transmission range set for each node. 
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(RANGE));
  // Bind the WiFi channel to the physical layer 
  wifiPhy.SetChannel (wifiChannel.Create ());

  /** Upper-level MAC **/ 
  WifiMacHelper wifiMac;
  // Ad-Hoc Mode. 
  wifiMac.SetType ("ns3::AdhocWifiMac");
  
  /** Network device (this was declared earlier. WiFi Helper object.) **/
  // Standard used (802.11b)
  wifi.SetStandard (WIFI_STANDARD_80211b);
  // Rate control algorithm, if any. 
  // Constant Rate = use constant rates for data and RTS (request to send) transmissions
  // (Note that this is also a rate control framework supported by the physical layer.) 
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode), // 2Mbps
                                "ControlMode",StringValue (phyMode)); // 2Mbps
  
  /** Create a collection of network devices, each running the settings we 
      configured above. These are built from the nodes we initialized at the beginning. **/
  // wifi (the WifiHelper) = the standard + rate control shared by all nodes
  // wifiPhy (physical layer model) + wifiMac = what makes up a single WiFi device
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  /** Create the topology. **/
  // Devices can be moving or stationary (in our case, stationary) 
  // We also have relative positions (used to calculate propagation loss)
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (distance), // b/t two consecutive positions
                                 "DeltaY", DoubleValue (distance),
                                 "GridWidth", UintegerValue (chainLen), // Lattice topology.  
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c); // Layout the collection of nodes according to these settings. 

  /** Set up the IP stack **/
  // Enable OLSR (Optimized Link State Routing) 
  // It's an IP routing protocol optimized for mobile and ad-hoc networks; 
  // uses HELLO and topology control packets to discover link state information, 
  // then uses that information to run graph algorithms to populate routing tables.
  OlsrHelper olsr;
  // We want to use static routing
  Ipv4StaticRoutingHelper staticRouting;
  // This just creates a priority list of routing protocols to consult
  // When there's a packet to send, consult routing protocols in the list in 
  // order (e.g., first try static routing, then OLSR), until able to route the packet. 
  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, 10);

  // Install IP stack on each device in the container. 
  InternetStackHelper internet;
  internet.SetRoutingHelper (list); // has effect on the next Install ()
  internet.Install (c);

  // Set up IP addresses for each node. 
  // Routing tables will get populated using OLSR
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);
  // Note: all of these devices are on one network. 

  /*** INSTALL LAYER 4 APPLICATION ***/
  
  for (uint32_t i = 0; i < chainLen; i++) {
    uint32_t sinkNode = sinkNodes[i];
    uint32_t sourceNode = sourceNodes[i];

    // Build UDP receiver application and install it on the sink node 
    uint16_t receiverPort = 5001; 
    AddressValue receiverAddress (InetSocketAddress (interfaces.GetAddress (sinkNode), 
                                                     receiverPort));
    PacketSinkHelper receiverHelper ("ns3::UdpSocketFactory", 
                                     receiverAddress.Get());
    receiverHelper.SetAttribute ("Protocol",
                                 TypeIdValue (UdpSocketFactory::GetTypeId ()));
   
    // Install
    ApplicationContainer receiverApp = receiverHelper.Install (c.Get(sinkNode));
    // Save so that we can call getTotalRx later
    sinkPtrs[i] = StaticCast<PacketSink> (receiverApp.Get (0));
    // Schedule
    receiverApp.Start (Seconds (OLSR_CONVERGE_TIME));
    receiverApp.Stop (Seconds ((double)OLSR_CONVERGE_TIME + simTime));
   
    // Build sender application and install it on the sending node
    OnOffHelper onoffHelper ("ns3::UdpSocketFactory", InetSocketAddress (interfaces.GetAddress (sinkNode),
                                                     receiverPort));
    onoffHelper.SetAttribute ("DataRate", DataRateValue (DataRate ("2Mbps")));
    uint32_t udpPacketSize = packetSize - UDP_HDR_BYTES - IP_HDR_MIN_BYTES; 
    onoffHelper.SetAttribute ("PacketSize", UintegerValue (udpPacketSize));
    // The OnOff application is designed to send packets for an interval ("on"), then stop sending them 
    // for an interval. We "flood" UDP packets as fast as the MAC protocol allows by setting the OffTime 
    // to be 0 and the OnTime to be the duration of the simulation. 
    onoffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
    onoffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string(simTime) + "]"));

    // Install and schedule 
    ApplicationContainer sourceApp = onoffHelper.Install (c.Get(sourceNode));
    sourceApp.Start (Seconds (OLSR_CONVERGE_TIME));
    sourceApp.Stop (Seconds ((double)OLSR_CONVERGE_TIME + simTime));
  }

  AsciiTraceHelper asciiTraceHelper;

  /*** TRACING SETUP ***/
  if (tracing == true)
    {
      wifiPhy.EnableAsciiAll (asciiTraceHelper.CreateFileStream ("wifi-simple-adhoc-grid.tr"));
      wifiPhy.EnablePcap ("wifi-simple-adhoc-grid", devices);
      // Trace routing tables
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.routes", std::ios::out);
      olsr.PrintRoutingTableAllEvery (Seconds (2), routingStream);
      Ptr<OutputStreamWrapper> neighborStream = Create<OutputStreamWrapper> ("wifi-simple-adhoc-grid.neighbors", std::ios::out);
      olsr.PrintNeighborCacheAllEvery (Seconds (2), neighborStream);

      // To do if desired -- enable an IP-level trace that shows forwarding events only
    }

  // Output what we are doing
  NS_LOG_UNCOND ("Testing for lattice size " << chainLen << " by " << chainLen
    << " with grid distance " << distance << " and packet size " << packetSize);

  // Start tracing throughput after we've given time for OLSR to converge 
  // (+ a buffer to allow connection to be established.) 
  // if (printIntermediate) {
  //   Simulator::Schedule (Seconds (OLSR_CONVERGE_TIME + 0.1), &CalculateThroughput); 
  // }

  // Time delay for when simulator should stop
  // We should do this as [time for OLSR to converge] + 
  // [time we want to be testing the UDP flow] 
  Time start = Simulator::Now ();
  Simulator::Stop (Seconds (OLSR_CONVERGE_TIME + simTime)); 
  Simulator::Run ();
  Simulator::Destroy ();

  double throughputs[chainLen];
  double aveThroughput = 0;
  for (uint32_t i = 0; i < chainLen; i++) {
    throughputs[i] = sinkPtrs[i]->GetTotalRx () * 8 / 1e6 / simTime;
    std::cout << "Stream " << i << " throughput: " << throughputs[i] << std::endl;
    aveThroughput += throughputs[i];
  }
  aveThroughput = aveThroughput / chainLen;

  std::cout << "Average: " 
	    << aveThroughput 
	    << "Mb/s (application) received, with:\n Lattice size: " 
	    << chainLen * chainLen
	    << " nodes,\n Packet size: "
	    << packetSize
	    << " bytes (includes IP and UDP headers),\n Simulation time: "
	    << simTime 
	    << " seconds. Finished in " 
      << Simulator::Now().GetSeconds() - start.GetSeconds()
      << "s.\n"
	    << std::endl;
   

  
  std::string filename = "outputs/figure_8_results.txt";
  std::ofstream ofs(filename, std::ios::app); //append to the end of this file
  ofs << std::to_string(packetSize) << " " << std::to_string(chainLen)
    << " " << std::to_string(simTime) << " " << std::to_string(aveThroughput);

  for (uint32_t i = 0; i < chainLen; i++) {
    ofs << " " << std::to_string(throughputs[i]);
  }
  ofs << std::endl;
  ofs.close();

  return 0;
}

