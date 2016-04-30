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
#include <set>

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Project1");

typedef struct used_address
{
  int node;
  uint16_t port;
} used_address_t;

bool operator<(const used_address_t& lhs, const used_address_t& rhs)
{
  return lhs.node*100000 + lhs.port < rhs.node*100000 + rhs.port;
}

Ptr<UniformRandomVariable> uv;
std::set<used_address_t> used_addresses; 

uint16_t generate_random_port(int node)
{
  used_address_t current;
  current.node = node;
  current.port = (uint16_t) floor(uv->GetValue(1024.0, 49151.0));
  while(used_addresses.find(current) != used_addresses.end())
  {
    current.port = (uint16_t) floor(uv->GetValue(1024.0, 49151));
  }
  used_addresses.insert(current);
  return current.port;
}

int main (int argc, char *argv[])
{
  RngSeedManager::SetSeed (11223344);
  uv = CreateObject<UniformRandomVariable> ();
  uv->SetAttribute("Stream", IntegerValue(6110));

  NodeContainer p2pNodes1;
  p2pNodes1.Create(2);

  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NetDeviceContainer p2pDevices1;
  p2pDevices1 = pointToPoint1.Install(p2pNodes1);
  std::cout<<"Created p2p\n";

  InternetStackHelper stack;
  stack.Install(p2pNodes1);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces1;
  p2pInterfaces1 = address.Assign(p2pDevices1);

  // uint16_t port = 10;
  // ApplicationContainer sinkApps;
  // NewPacketSinkHelper sink ("ns3::TcpSocketFactory",
  //                         InetSocketAddress (Ipv4Address::GetAny (), port));
  // sinkApps.Add (sink.Install(p2pNodes1.Get(1)));
  // std::cout<<"Created sink\n";

  // NewSendHelper sender("ns3::TcpSocketFactory",
  //                        InetSocketAddress (p2pInterfaces1.GetAddress(0), port), InetSocketAddress (p2pInterfaces1.GetAddress(1), port), 500, 5000);
  // //sender.SetAttribute ("MaxBytes", UintegerValue (5000));
  // ApplicationContainer sourceApps = sender.Install (p2pNodes1.Get(0));
  // std::cout<<"created source\n";

  // sinkApps.Start(Seconds(0));
  // sinkApps.Stop(Seconds(30));
  // sourceApps.Start(Seconds(0));
  // std::cout<<"App started\n";
  // sourceApps.Stop(Seconds (30));

  // Ptr<NewSendApplication> sendptr = DynamicCast<NewSendApplication> (sourceApps.Get(0));
  // //std::cout<<"Status of response: "<<sendptr->ResponseComplete()<<std::endl;
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop(Seconds(30));
  Simulator::Run ();
  // std::cout<<"Status of response: "<<sendptr->ResponseComplete()<<std::endl;
  
  Simulator::Destroy ();
  std::cout<<"Sim ended\n";
}
