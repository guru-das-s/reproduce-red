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

void User(browserNum)
{
  destServer = generateDestServer()
  uint32_t* consecPageCounter = new uint32_t();
  *consecPageCounter = generateConsecPageCounter()
  primaryRequest(browserNum, address destServer, consecPageCounter)
}

void primaryRequest(browserNum, address destServer, consecPageCounter)
{
  port = PickPort()
  Pick sendSize and respSizes
  //Generate primary request
  A = NewSendApplication(browserNum||port, destServer,  sendSize, respSize)
  Schedule(checkPrimaryComplete(A, consecPageCounter), now()+delta)
}

void checkPrimaryComplete(NewSendApplication* A, uint32_t* consecPageCounter)
{
  address destServer = extractDestAdd(A)
  if(A->ResponseComplete())
    secondaryRequest(destServer, consecPageCounter)
  else:
    Schedule(checkPrimaryComplete(A, consecPageCounter), now()+delta)
}

void secondaryRequest(address destServer, uint32_t* consecPageCounter)
{
  uint32_t* secondaryRequestCounter = new uint32_t();
  *secondaryRequestCounter = generateNumSecondaryRequests()
  for(int i = 0; i < *secondaryRequestCounter; i++ )
  {
    Pick sendSize and respSizes
    port = PickPort()
    //Generate secondary request
    A = NewSendApplication(browserNum||port, destServer,  sendSize, respSize)
    Schedule(checkSecondaryComplete(A, consecPageCounter, secondaryRequestCounter), now()+delta)
  }
}

void checkSecondaryComplete(NewSendApplication*A, uint32_t* consecPageCounter, uint32_t* secondaryRequestCounter)
{
  if(A->ResponseComplete())
  {
   *secondaryRequestCounter--;
   if(*secondaryRequestCounter == 0)  //Page loaded, think and open new one
   {
    delete secondaryRequestCounter;
    *consecPageCounter--;
    uint32_t thinkTime = generateThinkTime();
    if(*consecPageCounter == 0) // all pages for this sever done. open new site
    {
      delete consecPageCounter;
      Schedule(User(browserNum), now()+thinkTime)
    }
    else
    {
      Schedule(primaryRequest(), now()+thinkTime)
    }
   }
  }
  else
    Schedule(checkSecondaryComplete(A, consecPageCounter, secondaryRequestCounter)
}

int main (int argc, char *argv[])
{
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

  uint16_t port = 10;
  ApplicationContainer sinkApps;
  NewPacketSinkHelper sink ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), port));
  sinkApps.Add (sink.Install(p2pNodes1.Get(1)));
  std::cout<<"Created sink\n";

  NewSendHelper sender("ns3::TcpSocketFactory",
                         InetSocketAddress (p2pInterfaces1.GetAddress(0), port), InetSocketAddress (p2pInterfaces1.GetAddress(1), port), 500, 5000);
  //sender.SetAttribute ("MaxBytes", UintegerValue (5000));
  ApplicationContainer sourceApps = sender.Install (p2pNodes1.Get(0));
  std::cout<<"created source\n";

  sinkApps.Start(Seconds(0));
  sinkApps.Stop(Seconds(30));
  sourceApps.Start(Seconds(0));
  std::cout<<"App started\n";
  sourceApps.Stop(Seconds (30));

  Ptr<NewSendApplication> sendptr = DynamicCast<NewSendApplication> (sourceApps.Get(0));
  //std::cout<<"Status of response: "<<sendptr->ResponseComplete()<<std::endl;
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  Simulator::Stop(Seconds(30));
  Simulator::Run ();
  std::cout<<"Status of response: "<<sendptr->ResponseComplete()<<std::endl;
  
  Simulator::Destroy ();
  std::cout<<"Sim ended\n";
}
