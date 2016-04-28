// Main runner (This file) - https://kobra.io/#/e/-KFQ_oHdUaRXudzDGMjp
// Server - https://kobra.io/#/e/-KFQaIJyKVXU4Tye3UEr
// Browser client - https://kobra.io/#/e/-KFQaG7mYTeCm38J1nqR
// Probability Distributions (from http-distributions in GTNets) 

/* 
Name: Deepti Kochar
GT ID: 903064898 (dkochar3)
ECE 6110 Project 2
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/random-variable-stream.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/new-packet-sink-helper.h"
#include "ns3/new-send-helper.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <set>
#include <math.h>
#include <string>

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Project2");
double scheduler_interval = 0.1;
int queue = 0;
  //General Parameters
uint16_t numNodes = 10;
uint16_t segSize = 536; //536 is the default TCP Segment size according to wikipedia
uint16_t maxWin = 65535; // The default value for linux
std::string bottleNeckLinkBw = "10Mbps";
std::string bottleNeckLinkDelay = "30ms"; // TODO tune this to achieve an average minimin RTT of 79ms
uint16_t num_apps = 500; // Tune this to find number of browsers per load

  // Queue Parameters
uint32_t modeBytes = 0;
uint32_t queueBytesLimit = 2000;
uint32_t queuePacketsLimit = 100;
double minTh = 50;
double maxTh = 80;
Ptr< Queue > red_queue;
typedef struct used_address
{
  int node;
  uint16_t port;
} used_address_t;

bool operator<(const used_address_t& lhs, const used_address_t& rhs)
{
  return lhs.node*100000 + lhs.port < rhs.node*100000 + rhs.port;
}

double avg_queue_size = 0;
int num_checks = 0;
std::ofstream fPlotQueue;
std::ofstream fPlotQueueAvg;

void CheckBottleneck()
{
  uint32_t qSize = StaticCast<RedQueue> (red_queue)->GetQueueSize ();
  avg_queue_size += qSize;
  num_checks++;
  fPlotQueue<<queue<<","<<segSize<<","<<maxWin<<","<<bottleNeckLinkBw<<","<<bottleNeckLinkDelay<<","<<num_apps<<","<<modeBytes<<","<<queueBytesLimit<<","<<queuePacketsLimit<<","<<minTh<<","<<maxTh<<",";
  fPlotQueue << Simulator::Now ().GetSeconds () << "," << qSize << std::endl;
  fPlotQueueAvg<<queue<<","<<segSize<<","<<maxWin<<","<<bottleNeckLinkBw<<","<<bottleNeckLinkDelay<<","<<num_apps<<","<<modeBytes<<","<<queueBytesLimit<<","<<queuePacketsLimit<<","<<minTh<<","<<maxTh<<",";
  fPlotQueueAvg << Simulator::Now ().GetSeconds () << "," << avg_queue_size / num_checks << std::endl;
  Simulator::Schedule (Seconds (scheduler_interval), &CheckBottleneck);
}

int main (int argc, char *argv[])
{

  // Output file names
  std::string goodput_file = "goodput.csv";
  std::string queue_plot_file = "queue_size.csv";
  std::string avg_queue_plot_file = "avg_queue_size.csv";

  CommandLine cmd;
  cmd.AddValue("queuetype", "Set Queue type to Droptail <0> or RED <1>", queue);
  cmd.AddValue("nNodes", "Number of nodes", numNodes);
  cmd.AddValue("tcpSegSize", "TCP segment size", segSize);
  cmd.AddValue("maxWin", "TCP receiver window size", maxWin);
  cmd.AddValue("bottleNeckLinkBw", "Bottleneck Link Bandwidth", bottleNeckLinkBw);
  cmd.AddValue("bottleNeckLinkDelay", "Bottleneck Link Delay", bottleNeckLinkDelay);
  cmd.AddValue("numApps", "Number of applications", num_apps);

  cmd.AddValue ("modeBytes", "Set Queue mode to packets <0> or bytes <1>", modeBytes);
  cmd.AddValue("queueBytesLimit", "Queue limit in bytes", queueBytesLimit);
  cmd.AddValue("queuePacketsLimit", "Queue limit in packets", queuePacketsLimit);
  cmd.AddValue ("redMinTh", "RED queue minimum threshold", minTh);
  cmd.AddValue ("redMaxTh", "RED queue maximum threshold", maxTh);
  cmd.Parse (argc,argv);

  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (segSize));
  Config::SetDefault("ns3::TcpSocketBase::MaxWindowSize", UintegerValue(maxWin));
  Config::SetDefault("ns3::TcpSocketBase::WindowScaling", BooleanValue(false));
  if (!modeBytes)
  {
    Config::SetDefault ("ns3::DropTailQueue::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (queuePacketsLimit));
    Config::SetDefault ("ns3::RedQueue::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (queuePacketsLimit));
  }
  else 
    {
      Config::SetDefault ("ns3::DropTailQueue::Mode", StringValue ("QUEUE_MODE_BYTES"));
      Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue (queueBytesLimit));
      Config::SetDefault ("ns3::RedQueue::Mode", StringValue ("QUEUE_MODE_BYTES"));
      Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (queueBytesLimit));
      minTh *= segSize; 
      maxTh *= segSize;
    }

  if(queue == 1)
  {
    fPlotQueue.open(queue_plot_file.c_str(), std::ofstream::out | std::ofstream::app);
    fPlotQueueAvg.open(avg_queue_plot_file.c_str(), std::ofstream::out | std::ofstream::app);
  }
  // Left side
  NodeContainer p2pNodes1;
  p2pNodes1.Create(2);
  NodeContainer csmaNodes1;
  csmaNodes1.Add(p2pNodes1.Get(1));
  csmaNodes1.Create(numNodes);

  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NetDeviceContainer p2pDevices1;
  p2pDevices1 = pointToPoint1.Install(p2pNodes1);

  // Modelling an Ethernet switch
  CsmaHelper csma1;
  csma1.SetChannelAttribute("DataRate", StringValue ("10Mbps"));
  csma1.SetChannelAttribute("Delay", StringValue ("10ms"));

  NetDeviceContainer csmaDevices1;
  csmaDevices1 = csma1.Install(csmaNodes1);

  // Right side
  NodeContainer p2pNodes2;
  p2pNodes2.Create(2);
  NodeContainer csmaNodes2;
  csmaNodes2.Add(p2pNodes2.Get(1));
  csmaNodes2.Create(numNodes);

  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NetDeviceContainer p2pDevices2;
  p2pDevices2 = pointToPoint2.Install(p2pNodes2);

  CsmaHelper csma2;
  csma2.SetChannelAttribute("DataRate", StringValue ("10Mbps"));
  csma2.SetChannelAttribute("Delay", StringValue ("10ms"));

  NetDeviceContainer csmaDevices2;
  csmaDevices2 = csma2.Install(csmaNodes2);

  NodeContainer bottleneckNodes;
  bottleneckNodes.Add(p2pNodes1.Get(0));
  bottleneckNodes.Add(p2pNodes2.Get(0));

  PointToPointHelper pointToPoint3;
  pointToPoint3.SetDeviceAttribute ("DataRate", StringValue (bottleNeckLinkBw));
  pointToPoint3.SetChannelAttribute ("Delay", StringValue (bottleNeckLinkDelay));

  if(queue == 0)
  {
    pointToPoint3.SetQueue ("ns3::DropTailQueue");
  }
  else if(queue == 1)
  {
    pointToPoint3.SetQueue ("ns3::RedQueue",
                              "MinTh", DoubleValue (minTh),
                              "MaxTh", DoubleValue (maxTh),
                               "LinkBandwidth", StringValue (bottleNeckLinkBw),
                              "LinkDelay", StringValue (bottleNeckLinkDelay));
  }
  else
  {
    std::cout<<"Invalid queue option. Enter 0 for Droptail and 1 for RED"<<std::endl;
    return 0;
  }

  NetDeviceContainer bottleneckDevices;
  bottleneckDevices = pointToPoint3.Install(bottleneckNodes);

  InternetStackHelper stack;
  stack.Install(csmaNodes1);
  stack.Install(bottleneckNodes);
  stack.Install(csmaNodes2);
  
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces1;
  p2pInterfaces1 = address.Assign(p2pDevices1);

  address.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces1;
  csmaInterfaces1 = address.Assign(csmaDevices1);

  address.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces2;
  p2pInterfaces2 = address.Assign(p2pDevices2);  

  address.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces2;
  csmaInterfaces2 = address.Assign(csmaDevices2);

  address.SetBase("10.1.5.0", "255.255.255.0");
  Ipv4InterfaceContainer bottleneckInterfaces;
  bottleneckInterfaces = address.Assign(bottleneckDevices);

  Ptr<PointToPointNetDevice> nd = StaticCast<PointToPointNetDevice> (bottleneckDevices.Get (0));
  red_queue = nd->GetQueue ();

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  RngSeedManager::SetSeed (11223344);
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetAttribute("Stream", IntegerValue(6110));
  double stop = 60.0;
  
  ApplicationContainer sinkApps;
  double *start = new double[num_apps];
  double *end = new double[num_apps];
  std::set<used_address_t> used_addresses;                                              
  uint16_t port;
  int source;
  int destination;

  double time_interval = 20.0;
  for(uint16_t i = 0; i < num_apps; i++)
  {
    start[i] = uv->GetValue(0.1, stop-5.0);
    end[i] = uv->GetValue(start[i]+5.0, start[i] + time_interval);
    while(end[i] < start[i])
      end[i] = uv->GetValue(start[i]+1.0, start[i] + time_interval);
    source = floor(uv->GetValue(0.0, numNodes-0.1));
    destination = floor(uv->GetValue(0.0, numNodes-0.1));
    port = (uint16_t) floor(uv->GetValue(1024.0, 49151.0));
    used_address_t current_dest;
    current_dest.node = destination;
    current_dest.port = port;
    while(used_addresses.find(current_dest) != used_addresses.end())
      port = (uint16_t) floor(uv->GetValue(1024.0, 49151));
    current_dest.port = port; 
    used_addresses.insert(current_dest);

    NewPacketSinkHelper sink ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), current_dest.port));
    sinkApps.Add (sink.Install(csmaNodes2.Get(destination)));
    NewSendHelper tcp_source("ns3::TcpSocketFactory",
                         InetSocketAddress (csmaInterfaces2.GetAddress(destination), current_dest.port));
    ApplicationContainer sourceApps = tcp_source.Install (csmaNodes1.Get(source));
    sourceApps.Start(Seconds(start[i]));
    sourceApps.Stop(Seconds (end[i]));
  }

  sinkApps.Start(Seconds(0.0));
  sinkApps.Stop(Seconds(stop));

  if(queue == 1)
    Simulator::Schedule (Seconds (scheduler_interval), &CheckBottleneck);
  Simulator::Stop(Seconds(stop));
  Simulator::Run ();

  std::ofstream fgoodput;
  fgoodput.open(goodput_file.c_str(), std::ofstream::out | std::ofstream::app);
  double average_goodput = 0;
  double average_goodput_tcp;
  double totalRxBytesCounter = 0;
  int index = 0;
  for(int i = 0; i < num_apps; i++)
  {
    Ptr <Application> app = sinkApps.Get (i);
    Ptr <PacketSink> pktSink = DynamicCast <PacketSink> (app);
    double rxBytes = pktSink->GetTotalRx();
    totalRxBytesCounter += rxBytes;
    double goodput = rxBytes/(end[index] - start[index]);
    index++;
    average_goodput_tcp += goodput;
  }
  

  average_goodput = (average_goodput_tcp)/num_apps;

  Simulator::Destroy ();
  fgoodput.close();
  if(queue)
  {
    fPlotQueue.close();
    fPlotQueueAvg.close();
  }
  return 0;
}
