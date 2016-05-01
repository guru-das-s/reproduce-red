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

NS_LOG_COMPONENT_DEFINE ("TuningRed");

// Delete these temp lines:
Time delta = Seconds(0.005);

typedef struct used_address
{
  uint32_t node;
  uint16_t port;
} used_address_t;

bool operator<(const used_address_t& lhs, const used_address_t& rhs)
{
  return lhs.node*100000 + lhs.port < rhs.node*100000 + rhs.port;
}

Ptr<UniformRandomVariable> uv;
std::set<used_address_t> usedAddresses; 
NodeContainer browsers;
NodeContainer servers;
Ipv4InterfaceContainer browserInterfaces;
Ipv4InterfaceContainer serverInterfaces;

uint16_t GeneratePortNum(uint32_t node)
{
  used_address_t current;
  current.node = node;
  current.port = (uint16_t) floor(uv->GetValue(1024.0, 60000.0));
  if(current.port % 2 == 1)
  {
    current.port++;
  }

  used_address_t next;
  next.node = node;
  next.port = current.port + 1;

  while(usedAdresses.find(current) != usedAdresses.end() && usedAdresses.find(next) != usedAdresses.end())
  {
    current.port = (uint16_t) floor(uv->GetValue(1024.0, 60000));
    if(current.port % 2 == 1)
    {
      current.port++;
    }
    next.port = current.port + 1;
  }
  usedAdresses.insert(current);
  usedAdresses.insert(next);
  return current.port;
}

uint32_t generateConsecPageCounter()
{
  return 10;
}

uint32_t generatePrimaryRequestSize()
{
  return 2000;
}

uint32_t generatePrimaryResponseSize()
{
  return 10000;
}

uint32_t generateNumSecondaryRequests()
{
  return 5;
}

uint32_t generateSecondaryRequestSize()
{
  return 3000;
}

uint32_t generateSecondaryResponseSize()
{
  return 15000;
}

uint32_t generateThinkTime()
{
  return 100;
}

InetSocketAddress generateDestServer()
{
  // pick server node num
  uint32_t server = 0;
  uint16_t port = 5000;
  return InetSocketAddress(serverInterfaces.Get(server), port);
}

void User(uint32_t browserNum)
{
  InetSocketAddress destServer = generateDestServer()
  uint32_t* consecPageCounter = new uint32_t();
  *consecPageCounter = generateConsecPageCounter()
  primaryRequest(browserNum, Address destServer, consecPageCounter)
}

void primaryRequest(uint32_t browserNum, InetSocketAddress destServer, uint32_t *consecPageCounter)
{
  uint16_t port = GeneratePortNum(browserNum);
  uint32_t primaryReqSize = generatePrimaryRequestSize();
  uint32_t primaryRespSize = generatePrimaryResponseSize();
  //Generate primary request
  NewSendHelper reqSender("ns3::TcpSocketFactory",
                         InetSocketAddress (browserInterfaces.GetAddress(browserNum), port), destServer, primaryRespSize, primaryReqSize);
  ApplicationContainer sourceApps = reqSender.Install (browsers.Get(browserNum));
  Ptr<NewSendApplication> sendptr = DynamicCast<NewSendApplication> (sourceApps.Get(0));
  Simulator::Schedule(delta, &checkPrimaryComplete, browserNum, sendptr, consecPageCounter);
}

void checkPrimaryComplete(uint32_t browserNum, Ptr<NewSendApplication> sendptr, uint32_t* consecPageCounter)
{
  if(sendptr->ResponseComplete())
  {
    InetSocketAddress destServer = ConvertToInetSocketAddress(sendptr->GetDestinationAddress());
    secondaryRequest(browserNum, destServer, consecPageCounter);
  }
  else:
   Simulator::Schedule(delta, &checkPrimaryComplete, browserNum, sendptr, consecPageCounter);
}

void secondaryRequest(uint32_t browserNum, InetSocketAddress destServer, uint32_t* consecPageCounter)
{
  uint32_t* secondaryRequestCounter = new uint32_t();
  *secondaryRequestCounter = generateNumSecondaryRequests()
  for(int i = 0; i < *secondaryRequestCounter; i++)
  {
    uint32_t secReqSize = generateSecondaryRequestSize();
    uint32_t secRespSize = generateSecondaryResponseSize();
    uint16_t port = GeneratePortNum(browserNum);
    //Generate secondary request
    NewSendHelper reqSender("ns3::TcpSocketFactory",
                         InetSocketAddress (browserInterfaces.GetAddress(browserNum), port), destServer, secRespSize, secReqSize);
    ApplicationContainer sourceApps = reqSender.Install (browsers.Get(browserNum));
    Ptr<NewSendApplication> sendptr = DynamicCast<NewSendApplication> (sourceApps.Get(0));
    Simulator::Schedule(delta, &checkSecondaryComplete, browserNum, sendptr, consecPageCounter, secondaryRequestCounter);
  }
}

void checkSecondaryComplete(uint32_t browserNum, Ptr<NewSendApplication> sendptr, uint32_t* consecPageCounter, uint32_t* secondaryRequestCounter)
{
  if(sendptr->ResponseComplete())
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
      Simulator::Schedule(Seconds(generateThinkTime()), &User)
    }
    else
    {
      InetSocketAddress destServer = ConvertToInetSocketAddress(sendptr->GetDestinationAddress());
      Simulator::Schedule(Seconds(generateThinkTime()), &primaryRequest, browserNum, destServer, consecPageCounter);
    }
   }
  }
  else
    Simulator::Schedule(delta, &checkSecondaryComplete, browserNum, sendptr, consecPageCounter, secondaryRequestCounter);
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
  
  browsers.Create(1);
  servers.Create(1);
  NodeContainer temp;
  temp.Add(browsers.Get(0));
  temp.Add(servers.Get(0));


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
