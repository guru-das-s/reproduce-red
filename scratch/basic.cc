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

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Project1");

int main (int argc, char *argv[])
{
  NodeContainer p2pNodes1;
  p2pNodes1.Create(2);

  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NetDeviceContainer p2pDevices1;
  p2pDevices1 = pointToPoint1.Install(p2pNodes1);

  InternetStackHelper stack;
  stack.Install(p2pNodes1);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces1;
  p2pInterfaces1 = address.Assign(p2pDevices1);

  uint16_t port = 10;
  ApplicationContainer sinkApps;
  NewPacketSinkHelper sink ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), port));
  sinkApps.Add (sink.Install(p2pNodes1.Get(1)));
  NewSendHelper sender("ns3::TcpSocketFactory",
                         InetSocketAddress (p2pInterfaces1.GetAddress(1), port));
  ApplicationContainer sourceApps = sender.Install (p2pNodes1.Get(0));
  sourceApps.Start(Seconds(0));
  sourceApps.Stop(Seconds (10));


  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop(Seconds(30));
  Simulator::Run ();
  Simulator::Destroy ();
}