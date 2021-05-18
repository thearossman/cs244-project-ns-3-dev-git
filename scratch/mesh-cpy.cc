/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2008,2009 IITP RAS
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
  * Author: Kirill Andreev <andreev@iitp.ru>
  *
  *
  * By default this script creates m_xSize * m_ySize square grid topology with
  * IEEE802.11s stack installed at each node with peering management
  * and HWMP protocol.
  * The side of the square cell is defined by m_step parameter.
  * When topology is created, UDP ping is installed to opposite corners
  * by diagonals. packet size of the UDP ping and interval between two
  * successive packets is configurable.
  * 
  *  m_xSize * step
  *  |<--------->|
  *   step
  *  |<--->|
  *  * --- * --- * <---Ping sink  _
  *  | \   |   / |                ^
  *  |   \ | /   |                |
  *  * --- * --- * m_ySize * step |
  *  |   / | \   |                |
  *  | /   |   \ |                |
  *  * --- * --- *                _
  *  ^ Ping source
  *
  *  See also MeshTest::Configure to read more about configurable
  *  parameters.
  */
 
 #include <iostream>
 #include <sstream>
 #include <fstream>
 #include "ns3/core-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/network-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/mesh-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/mesh-helper.h"
 #include "ns3/command-line.h"
 #include "ns3/config.h"
 #include "ns3/string.h"
 #include "ns3/log.h"
 #include "ns3/yans-wifi-helper.h"
 #include "ns3/ssid.h"
 #include "ns3/mobility-helper.h"
 #include "ns3/on-off-helper.h"
 #include "ns3/yans-wifi-channel.h"
 #include "ns3/mobility-model.h"
 #include "ns3/packet-sink.h"
 #include "ns3/packet-sink-helper.h"
 #include "ns3/internet-stack-helper.h"
 #include "ns3/ipv4-address-helper.h"
 #include "ns3/ipv4-global-routing-helper.h"
 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE ("TestMeshScript"); 
 class MeshTest
 {
 public:
   MeshTest ();
   void Configure (int argc, char ** argv);
   int Run ();
 private:
   int       m_xSize; 
   int       m_ySize; 
   double    m_step; 
   double    m_randomStart; 
   double    m_totalTime; 
   double    m_packetInterval; 
   uint16_t  m_packetSize; 
   uint32_t  m_nIfaces; 
   bool      m_chan; 
   bool      m_pcap; 
   bool      m_ascii; 
   // Ptr<PacketSink> sink;                         /* Pointer to the packet sink application */
   Ptr<UdpServer> sink;                         /* Pointer to the packet sink application */
   uint64_t lastTotalRx = 0;
   std::string dataRate = "100Mbps";                  /* Application layer datarate. */
   std::string tcpVariant = "ns3::TcpNewReno";             /* TCP variant type. */
   std::string phyRate = "HtMcs7";                    /* Physical layer bitrate. */
   std::string m_stack; 
   std::string m_root; 
   NodeContainer nodes;
   NetDeviceContainer meshDevices;
   Ipv4InterfaceContainer interfaces;
   MeshHelper mesh;
 private:
   void CreateNodes ();
   void CalculateThroughput();
   void InstallInternetStack ();
   void InstallApplication ();
 };
 MeshTest::MeshTest () :
   m_xSize (4),
   m_ySize (1),
   m_step (200.0),
   m_randomStart (0.1),
   m_totalTime (20.0),
   m_packetInterval (0.01),
   m_packetSize (64),
   m_nIfaces (1),
   m_chan (true),
   m_pcap (false),
   m_ascii (false),
   m_stack ("ns3::Dot11sStack"),
   m_root ("ff:ff:ff:ff:ff:ff")
 {
 }
 void
 MeshTest::Configure (int argc, char *argv[])
 {
   CommandLine cmd;
   cmd.AddValue ("x-size", "Number of nodes in a row grid", m_xSize);
   cmd.AddValue ("y-size", "Number of rows in a grid", m_ySize);
   cmd.AddValue ("step",   "Size of edge in our grid (meters)", m_step);
   // Avoid starting all mesh nodes at the same time (beacons may collide)
   cmd.AddValue ("start",  "Maximum random start delay for beacon jitter (sec)", m_randomStart);
   cmd.AddValue ("time",  "Simulation time (sec)", m_totalTime);
   cmd.AddValue ("packet-interval",  "Interval between packets in UDP ping (sec)", m_packetInterval);
   cmd.AddValue ("packet-size",  "Size of packets in UDP ping (bytes)", m_packetSize);
   cmd.AddValue ("interfaces", "Number of radio interfaces used by each mesh point", m_nIfaces);
   cmd.AddValue ("channels",   "Use different frequency channels for different interfaces", m_chan);
   cmd.AddValue ("pcap",   "Enable PCAP traces on interfaces", m_pcap);
   cmd.AddValue ("ascii",   "Enable Ascii traces on interfaces", m_ascii);
   cmd.AddValue ("stack",  "Type of protocol stack. ns3::Dot11sStack by default", m_stack);
   cmd.AddValue ("root", "Mac address of root mesh point in HWMP", m_root);
 
   cmd.Parse (argc, argv);
   NS_LOG_DEBUG ("Grid:" << m_xSize << "*" << m_ySize);
   NS_LOG_DEBUG ("Simulation time: " << m_totalTime << " s");
   if (m_ascii)
     {
       PacketMetadata::Enable ();
     }

    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid), "TypeId " << tcpVariant << " not found");
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcpVariant)));
    /* Configure TCP Options */
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (m_packetSize));
 }

 void
 MeshTest::CalculateThroughput ()
 {
   // Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
   // double cur = (sink->GetTotalRx () - lastTotalRx) * (double) 8 / 1e5;      Convert Application RX Packets to MBits. 
   // std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
   // lastTotalRx = sink->GetTotalRx ();
   // Simulator::Schedule (MilliSeconds (100), &MeshTest::CalculateThroughput, this);
   Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
   // double cur = (sink->GetReceived () * m_packetSize - lastTotalRx) * (double) 8 / 1e5;     /* Convert Application RX Packets to MBits. */
   // std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" << std::endl;
   // lastTotalRx = sink->GetReceived () * m_packetSize;
  std::cout << now.GetSeconds () << "s: \t" << sink->GetReceived() << " packets/s" << std::endl;
   Simulator::Schedule (MilliSeconds (100), &MeshTest::CalculateThroughput, this);
 }

 void
 MeshTest::CreateNodes ()
 { 
   /*
    * Create m_ySize*m_xSize stations to form a grid topology
    */
   nodes.Create (m_ySize*m_xSize);
   // Configure YansWifiChannel
   YansWifiPhyHelper wifiPhy;
   YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
   // wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   // wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

   wifiPhy.SetChannel (wifiChannel.Create ());
   /*
    * Create mesh helper and set stack installer to it
    * Stack installer creates all needed protocols and install them to
    * mesh point device
    */
   mesh = MeshHelper::Default ();
   if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
     {
       mesh.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
     }
   else
     {
       //If root is not set, we do not use "Root" attribute, because it
       //is specified only for 11s
       mesh.SetStackInstaller (m_stack);
     }
   if (m_chan)
     {
       mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
     }
   else
     {
       mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
     }

   mesh.SetStandard (WIFI_STANDARD_80211b);
   mesh.SetMacType ("RandomStart", TimeValue (Seconds (m_randomStart)));
   // Set number of interfaces - default is single-interface mesh point
   mesh.SetNumberOfInterfaces (m_nIfaces);
   // Install protocols and return container if MeshPointDevices
   meshDevices = mesh.Install (wifiPhy, nodes);
   // Setup mobility - static grid topology
   MobilityHelper mobility;
   mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue (0.0),
                                  "MinY", DoubleValue (0.0),
                                  "DeltaX", DoubleValue (m_step),
                                  "DeltaY", DoubleValue (m_step),
                                  "GridWidth", UintegerValue (m_xSize),
                                  "LayoutType", StringValue ("RowFirst"));
   mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
   mobility.Install (nodes);
   if (m_pcap)
     wifiPhy.EnablePcapAll (std::string ("mp-"));
   if (m_ascii)
     {
       AsciiTraceHelper ascii;
       wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("mesh.tr"));
     }
 }
 void
 MeshTest::InstallInternetStack ()
 {
   InternetStackHelper internetStack;
   internetStack.Install (nodes);
   Ipv4AddressHelper address;
   address.SetBase ("10.1.1.0", "255.255.255.0");
   interfaces = address.Assign (meshDevices);

   /* Populate routing table */
   // Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 }
 void
 MeshTest::InstallApplication ()
 {
  /* Install TCP Receiver on the last node */
   // int sink_ind = 0;
   // int source_ind = m_xSize*m_ySize-1;
   // PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (interfaces.GetAddress(sink_ind), 9)); //port 9
   // ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get(sink_ind));
   // sink = StaticCast<PacketSink> (sinkApp.Get (0));

   // /* Install TCP/UDP Transmitter on the station */
   // OnOffHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (interfaces.GetAddress(sink_ind), 9)));
   // server.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
   // server.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
   // server.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
   // server.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
   // ApplicationContainer serverApp = server.Install (nodes.Get(source_ind));
 
   // /* Start Applications */
   // sinkApp.Start (Seconds (0.0));
   // serverApp.Start (Seconds (1.0));
   // Simulator::Schedule (Seconds (1.1), &MeshTest::CalculateThroughput, this);
   UdpServerHelper echoServer (9);
   ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));
   sink = DynamicCast<UdpServer> (serverApps.Get(0));
   serverApps.Start (Seconds (0.0));
   serverApps.Stop (Seconds (m_totalTime));
   UdpClientHelper echoClient (interfaces.GetAddress (0), 9);
   echoClient.SetAttribute ("MaxPackets", UintegerValue ((uint32_t)(m_totalTime*(1/m_packetInterval))));
   echoClient.SetAttribute ("Interval", TimeValue (Seconds (m_packetInterval)));
   echoClient.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
   ApplicationContainer clientApps = echoClient.Install (nodes.Get (m_xSize*m_ySize-1));
   clientApps.Start (Seconds (0.0));
   clientApps.Stop (Seconds (m_totalTime));
   Simulator::Schedule (Seconds (1.1), &MeshTest::CalculateThroughput, this);
 }
 int
 MeshTest::Run ()
 {
   CreateNodes ();
   InstallInternetStack ();
   InstallApplication ();
   Simulator::Stop (Seconds (m_totalTime + 1));
   Simulator::Run ();

   // double averageThroughput = ((sink->GetTotalRx () * 8) / (1e6 * m_totalTime));
   double averageThroughput = ((sink->GetReceived () * m_packetSize * 8) / (1e6 * m_totalTime));

   Simulator::Destroy ();
   
   if (averageThroughput < 2)
     {
       NS_LOG_ERROR ("Obtained throughput is not in the expected boundaries!");
       exit (1);
     }
   std::cout << "\nAverage throughput: " << averageThroughput << " Mbit/s" << std::endl;
   return 0;
 }
 int
 main (int argc, char *argv[])
 {
   MeshTest t; 
   t.Configure (argc, argv);
   return t.Run ();
 }
