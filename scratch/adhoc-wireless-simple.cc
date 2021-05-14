#include <stdlib.h>

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("AdHocWirelessNetworksSimple");

static double TRACE_START_TIME = 0.05;

static void
RttTracer (Ptr<OutputStreamWrapper> stream,
           Time oldval, Time newval)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () <<
               " Rtt from " << oldval.GetMilliSeconds () <<
               " to " << newval.GetMilliSeconds ());

  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << newval.GetMilliSeconds () << std::endl;
}

static void
TraceRtt (Ptr<OutputStreamWrapper> rttStream)
{
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/RTT",
                                 MakeBoundCallback (&RttTracer, rttStream));
}

int
main (int argc, char *argv[])
{
  AsciiTraceHelper asciiTraceHelper;

  /* Start by setting default variables. Feel free to play with the input
   * arguments below to see what happens.
   */
  int bwNet = 2; // Mbps
  int delay = 1; // milliseconds
  int time = 300; // seconds
  int chainlen = 10; // length of the chain
  std::string transport_prot = "TcpNewReno";

  CommandLine cmd (__FILE__);
  cmd.AddValue ("bwNet", "Bandwidth of bottleneck (network) link (Mb/s)", bwNet);
  cmd.AddValue ("delay", "Link propagation delay (ms)", delay);
  cmd.AddValue ("time", "Duration (sec) to run the experiment", time);
  cmd.AddValue ("chainlen", "Length of chain of nodes", chainlen);
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, "
                "TcpLinuxReno, TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, "
                "TcpScalable, TcpVeno, TcpBic, TcpYeah, TcpIllinois, "
                "TcpWestwood, TcpLedbat, TcpLp, TcpDctcp",
                transport_prot);
  cmd.Parse (argc, argv);

  /* NS-3 is great when it comes to logging. It allows logging in different
   * levels for different component (scripts/modules). You can read more
   * about that at https://www.nsnam.org/docs/manual/html/logging.html.
   */
  LogComponentEnable("AdHocWirelessNetworksSimple", LOG_LEVEL_DEBUG);

  std::string bwNetStr = std::to_string(bwNet) + "Mbps";
  std::string delayStr = std::to_string(delay) + "ms";
  std::string chainlenStr = std::to_string(chainlen);
  transport_prot = std::string ("ns3::") + transport_prot;

  NS_LOG_DEBUG("Bufferbloat Simulation for:" <<
               " bwNet=" << bwNetStr <<
               " delay=" << delayStr << " time=" << time << "sec" <<
               " chainlen=" << chainlenStr << " protocol=" << transport_prot);

  /******** Declare output files ********/
  /* Traces will be written on these files for postprocessing. */
  std::string dir = "outputs/";

  std::string rttStreamName = dir + "simple-rtt.tr";
  Ptr<OutputStreamWrapper> rttStream;
  rttStream = asciiTraceHelper.CreateFileStream (rttStreamName);

  //source: https://www.nsnam.org/docs/release/3.7/tutorial/tutorial_27.html

  NodeContainer wifiNodes;
  wifiNodes.Create(chainlen);

  // In the simple simulation, the first node is always the sender
  // and the last node is always the receiver.
  Ptr<Node> source = wifiNodes.Get(0);
  Ptr<Node> sink = wifiNodes.Get(chainlen - 1);

  // The next bit of code constructs the wifi devices and the
  // interconnection channel between these wifi nodes. First,
  // we configure the PHY and channel helpers: 
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper ();
  phy.SetChannel (channel.Create ());

  // Once the PHY helper is configured, we can focus on the MAC
  // layer. Here we choose to work with non-Qos MACs so we use a
  // NqosWifiMacHelper object to set MAC parameters. 
  WifiHelper wifi = WifiHelper();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  //note: tutorial used something called  NqosWifiMacHelper
  NqosWaveMacHelper mac = NqosWaveMacHelper::Default ();

  // This code first creates an 802.11 service set identifier (SSID)
  // object that will be used to set the value of the “Ssid” Attribute
  // of the MAC layer implementation. The particular kind of MAC layer
  // is specified by Attribute as being of the "ns3::NqstaWifiMac" type.
  // This means that the MAC will use a “non-QoS station” (nqsta) state
  // machine. Finally, the “ActiveProbing” Attribute is set to false.
  // This means that probe requests will not be sent by MACs created
  // by this helper.
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::NqstaWifiMac",
    "Ssid", SsidValue (ssid),
    "ActiveProbing", BooleanValue (false));

  NetDeviceContainer wifiDevices;
  wifiDevices = wifi.Install (phy, mac, wifiNodes);

  // set the 2d grid for wifi nodes
  // https://www.nsnam.org/doxygen/classns3_1_1_grid_position_allocator.html
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
    "MinX", DoubleValue (0.0),
    "MinY", DoubleValue (0.0),
    "DeltaX", DoubleValue (20.0), //units ??
    "DeltaY", DoubleValue (0.0),
    "GridWidth", UintegerValue (chainlen), //we want all the nodes in a straight line
    "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiNodes);

  InternetStackHelper stack;
  stack.Install (wifiNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (wifiDevices);

  /* Install the sink application on the last node */
  uint16_t receiverPort = 5001;
  AddressValue sinkAddr (InetSocketAddress (interfaces.GetAddress(chainlen - 1),
                                                   receiverPort));
  PacketSinkHelper receiverHelper ("ns3::TcpSocketFactory",
                                   sinkAddr.Get());
  receiverHelper.SetAttribute ("Protocol",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()));
  ApplicationContainer receiverApp = receiverHelper.Install (sink);
  receiverApp.Start (Seconds (0.0));
  receiverApp.Stop (Seconds ((double)time));

  /* Install the source application on the first node */
  BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
  ftp.SetAttribute ("Remote", sinkAddr);
  // ftp.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize)); needed?

  ApplicationContainer sourceApp = ftp.Install (source);
  sourceApp.Start (Seconds (0.0));
  sourceApp.Stop (Seconds ((double)time));

  /* Start tracing the RTT after the connection is established */
  Simulator::Schedule (Seconds (TRACE_START_TIME), &TraceRtt, rttStream);
  
  /******** Run the Actual Simulation ********/
  NS_LOG_DEBUG("Running the Simulation...");
  Simulator::Stop (Seconds ((double)time));
  // TODO: Up until now, you have only set up the simulation environment and
  //       described what kind of events you want NS3 to simulate. However
  //       you have actually not run the simulation yet. Complete the command
  //       below to run it.
  Simulator::Run();
  Simulator::Destroy ();
  return 0;
}
