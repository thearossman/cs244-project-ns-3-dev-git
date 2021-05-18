/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//
// This program configures a grid (default 5x5) of nodes on an
// 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to node 1.
//
// Layout is like this, on a chain: 
//
// n0   n1   n2   n3   n4 ...
//
// the layout is affected by the parameters given to GridPositionAllocator;
// by default, GridWidth is 5 and numNodes is 25..
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc-grid --help"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the ns-3 documentation.
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when distance increases beyond
// the default of 500m.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc-grid --distance=500"
// ./waf --run "wifi-simple-adhoc-grid --distance=1000"
// ./waf --run "wifi-simple-adhoc-grid --distance=1500"
//
// The source node and sink node can be changed like this:
//
// ./waf --run "wifi-simple-adhoc-grid --sourceNode=20 --sinkNode=10"
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc-grid --verbose=1"
//
// By default, trace file writing is off-- to enable it, try:
// ./waf --run "wifi-simple-adhoc-grid --tracing=1"
//
// When you are done tracing, you will notice many pcap trace files
// in your directory.  If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-grid-0-0.pcap -nn -tt
//

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
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-header.h"
#include "ns3/core-module.h"
#include "ns3/packet.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/packet-sink.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhocGrid");


uint64_t lastTotalRx = 0; 
Ptr<PacketSink> sink;
static void CalculateThroughput ()
{ 
  Time now = Simulator::Now ();
  double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}


int main (int argc, char *argv[])
{

  /*** DECLARE PARAMETERS ***/
  
  std::string phyMode ("DsssRate2Mbps");
  double distance = 100;  // m
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 1;
  uint32_t numNodes = 4;  // by default, 5x5 --> CHANGE THIS
  uint32_t sinkNode = 3;
  uint32_t sourceNode = 0;
  // double interval = 1.0; // seconds
  bool verbose = false;
  bool tracing = false;
  std::string transport_prot = "ns3::TcpNewReno";
  std::string dir = "outputs"; 
 
  // Give OLSR time to converge (e.g., populate routing tables). 
  double OLSR_CONVERGE_TIME = 30.0;
  double REMAINING_SIMULATION_TIME = 300.0;   

  CommandLine cmd (__FILE__);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("distance", "distance (m)", distance);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  // cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("tracing", "turn on ascii and pcap tracing", tracing);
  cmd.AddValue ("numNodes", "number of nodes", numNodes);
  cmd.AddValue ("sinkNode", "Receiver node number", sinkNode);
  cmd.AddValue ("sourceNode", "Sender node number", sourceNode);
  cmd.Parse (argc, argv);
  
  // Convert to time object
  // **I THINK WE WILL GET RID OF THIS**
  // Time interPacketInterval = Seconds (interval);

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));
  
  /*** CREATE NODES ***/
  NodeContainer c;
  c.Create (numNodes);

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
  // the Lucent WaveLAN Card (2001). 
  // TODO, then, we should tune the physical layer parameters to match 
  // the hardware we want to simulate.  

  // Reception gain (dB). 
  // NOTE: Not sure what to set this as. 
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (-10) );
  
  // NS--3 supports RadioTap and Prism tracing extensions for 802.11b
  // TODO, set this correctly. We may not need this at all, since I 
  // believe that it's just for tracing.
  // For now, I set it to DLT_IEEE802_11_RADIO 
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);

  /** Channel helper **/
  // Channel means everything involved in getting packets between devices. 
  // The main things we model here are propagation delay and propagation loss. 
  YansWifiChannelHelper wifiChannel;
  // TODO: Again, we may need to tune these parameters.
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  // We need to bind the WiFi channel to the physical layer 
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
                                 "GridWidth", UintegerValue (1), // Chain topology.  
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
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
  // Note: all of these devices are on one network. 

  /*** INSTALL LAYER 4 APPLICATION ***/
  // TCP variant  
  TypeId tid = TypeId::LookupByName ( transport_prot ); 
  
  // Build receiver application and install it on the sink node 
  uint16_t receiverPort = 5001; 
  AddressValue receiverAddress (InetSocketAddress (i.GetAddress (sinkNode), 
                                                   receiverPort));
  PacketSinkHelper receiverHelper ("ns3::TcpSocketFactory",
                                   receiverAddress.Get());
  receiverHelper.SetAttribute ("Protocol",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()));
 
  // Installation  
  ApplicationContainer receiverApp = receiverHelper.Install (c.Get(sinkNode));
  // Save so that we can call getTotalRx later
  sink = StaticCast<PacketSink> (receiverApp.Get (0));
  // Schedule
  receiverApp.Start (Seconds (OLSR_CONVERGE_TIME));
  receiverApp.Stop (Seconds ((double)OLSR_CONVERGE_TIME + REMAINING_SIMULATION_TIME));
 
  // Build sender application and install it on the sending node
  BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
  ftp.SetAttribute ("Remote", receiverAddress);
  uint32_t tcpSegmentSize = packetSize; // TODO: CHANGE TO ACCOUNT FOR OVERHEAD
  ftp.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize));

  // Installation 
  ApplicationContainer sourceApp = ftp.Install (c.Get(sourceNode));
  sourceApp.Start (Seconds (OLSR_CONVERGE_TIME));
  sourceApp.Stop (Seconds ((double)OLSR_CONVERGE_TIME + REMAINING_SIMULATION_TIME));

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

      // To do-- enable an IP-level trace that shows forwarding events only
    }

  // Output what we are doing
  NS_LOG_UNCOND ("Testing from node " << sourceNode << " to " << sinkNode << " with grid distance " << distance);

  // Start tracing throughput after we've given time for OLSR to converge 
  // (+ a buffer to allow connection to be established.) 
  std::string throughputStreamName = dir + "throughput.tr";
  Ptr<OutputStreamWrapper> throughputStream;
  throughputStream = asciiTraceHelper.CreateFileStream (throughputStreamName);
  Simulator::Schedule (Seconds (OLSR_CONVERGE_TIME + 0.05), &CalculateThroughput); 


  // Time delay for when simulator should stop
  // We should do this as [time for OLSR to converge] + 
  // [time we want to be testing the TCP flow] 
  Simulator::Stop (Seconds (OLSR_CONVERGE_TIME + REMAINING_SIMULATION_TIME)); 
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

