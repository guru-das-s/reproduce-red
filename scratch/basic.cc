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
#include <set>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TuningRed");

// Delete these temp lines:
Time delta = Seconds(0.05);

typedef struct used_address
{
  uint32_t node;
  uint16_t port;
} used_address_t;

bool operator<(const used_address_t& lhs, const used_address_t& rhs)
{
  return lhs.node*100000 + lhs.port < rhs.node*100000 + rhs.port;
}

typedef struct{
  float prob_value;
  int size_value;
}cdfentry_t;

struct classcomp {
  bool operator() (const cdfentry_t& lhs, const cdfentry_t& rhs) const
  {
    return lhs.prob_value < rhs.prob_value; 
  }
};

std::set<cdfentry_t, classcomp> cdf_consecpages;
std::set<cdfentry_t, classcomp> cdf_filesperpage;
std::set<cdfentry_t, classcomp> cdf_primaryreply;
std::set<cdfentry_t, classcomp> cdf_primaryreq;
std::set<cdfentry_t, classcomp> cdf_secondaryreply;
std::set<cdfentry_t, classcomp> cdf_secondaryreq;
std::set<cdfentry_t, classcomp> cdf_thinktimes;

Ptr<UniformRandomVariable> uv;
std::set<used_address_t> usedAddresses; 
std::vector<int64_t> responseTimes;
NodeContainer browsers;
NodeContainer servers;
Ipv4InterfaceContainer browserInterfaces;
Ipv4InterfaceContainer serverInterfaces;
Ipv4InterfaceContainer csma1Interfaces;
Ipv4InterfaceContainer csma2Interfaces;


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

  while(usedAddresses.find(current) != usedAddresses.end() && usedAddresses.find(next) != usedAddresses.end())
  {
    current.port = (uint16_t) floor(uv->GetValue(1024.0, 60000));
    if(current.port % 2 == 1)
    {
      current.port++;
    }
    next.port = current.port + 1;
  }
  usedAddresses.insert(current);
  usedAddresses.insert(next);
  return current.port;
}

void populate_cdf_data(std::ifstream& file, std::set<cdfentry_t, classcomp>& cdfset)
{
  std::string line;

  if (file.is_open())
  {
    while ( std::getline (file, line) )
    {
      // std::cout << line << '\n';
      size_t found = line.find(",");
      std::string prob = line.substr(0, found);
      std::string sizeval = line.substr(found+1);

      // std::string::size_type sz;     // alias of size_t
      cdfentry_t entry;

      // entry.prob_value = std::stof(line, &sz);
      // entry.size_value = std::stoi(line.substr(sz+1));
      cdfset.insert(entry);
    }
    std::cout<<"\n";
    file.close();
  }
}

int get_size(std::set<cdfentry_t, classcomp>& cdfset, float probval)
{
  cdfentry_t dummy;
  dummy.prob_value = probval;

  return (cdfset.lower_bound(dummy))->size_value;
}

uint32_t generateConsecPageCounter()
{
  // float probval = uv->GetValue(0.0, 1.0);
  // return get_size(cdf_consecpages, probval);
  return 2;
}

uint32_t generatePrimaryRequestSize()
{
  // float probval = uv->GetValue(0.0, 1.0);
  // return get_size(cdf_primaryreq, probval);
  return 2000;
}

uint32_t generatePrimaryResponseSize()
{
  // float probval = uv->GetValue(0.0, 1.0);
  // return get_size(cdf_primaryreply, probval);
  return 1000;
}

uint32_t generateNumSecondaryRequests()
{
  // float probval = uv->GetValue(0.0, 1.0);
  // return get_size(cdf_filesperpage, probval);
  return 5;
}

uint32_t generateSecondaryRequestSize()
{
  // float probval = uv->GetValue(0.0, 1.0);
  // return get_size(cdf_secondaryreq, probval);
  return 300;
}

uint32_t generateSecondaryResponseSize()
{
  // float probval = uv->GetValue(0.0, 1.0);
  // return get_size(cdf_secondaryreply, probval);
  return 3000;
}

uint32_t generateThinkTime()
{
  // float probval = uv->GetValue(0.0, 1.0);
  // return get_size(cdf_thinktimes, probval);
  return 200;
}

void primaryRequest(uint32_t browserNum, InetSocketAddress destServer, uint32_t *consecPageCounter);
void checkPrimaryComplete(uint32_t browserNum, Ptr<NewSendApplication> sendptr, uint32_t* consecPageCounter, Time start);
void secondaryRequest(uint32_t browserNum, InetSocketAddress destServer, uint32_t* consecPageCounter);
void checkSecondaryComplete(uint32_t browserNum, Ptr<NewSendApplication> sendptr, uint32_t* consecPageCounter, uint32_t* secondaryRequestCounter, Time start);

InetSocketAddress generateDestServer()
{
  // pick server node num
  // uint32_t server = 0;
  uint16_t port = 5000;

  // Switch comments later
  // std::cout<<"Destination Address: "<<(tempInterfaces.GetAddress(1))<<std::endl;
  return InetSocketAddress (csma1Interfaces.GetAddress(1), port);
  // return InetSocketAddress(serverInterfaces.GetAddress(server), port);
}

InetSocketAddress ConvertToInetSocketAddress(Address A)
{
  return InetSocketAddress(InetSocketAddress::ConvertFrom(A).GetIpv4 (), InetSocketAddress::ConvertFrom (A).GetPort ());
}
int user_count = 0;
void User(uint32_t browserNum)
{
  std::cout<<"User #"<<user_count++<<std::endl;
  InetSocketAddress destServer = generateDestServer();
  uint32_t* consecPageCounter = new uint32_t();
  *consecPageCounter = generateConsecPageCounter();
  primaryRequest(browserNum, destServer, consecPageCounter);
}

void primaryRequest(uint32_t browserNum, InetSocketAddress destServer, uint32_t *consecPageCounter)
{
  uint16_t port = GeneratePortNum(browserNum);
  uint32_t primaryReqSize = generatePrimaryRequestSize();
  uint32_t primaryRespSize = generatePrimaryResponseSize();
  //Generate primary request

  //Switch comments later
  // std::cout<<"Source Address: "<<tempInterfaces.GetAddress(browserNum)<<", "<<port<<std::endl;
  NewSendHelper reqSender("ns3::TcpSocketFactory",
                         InetSocketAddress (csma2Interfaces.GetAddress(browserNum), port), destServer, primaryRespSize, primaryReqSize);
  // NewSendHelper reqSender("ns3::TcpSocketFactory",
  //                        InetSocketAddress (browserInterfaces.GetAddress(browserNum), port), destServer, primaryRespSize, primaryReqSize);
  ApplicationContainer sourceApps = reqSender.Install (browsers.Get(browserNum));
  //sourceApps.Start(Simulator::Now());
  // std::cout<<"sender app started\n";
  Ptr<NewSendApplication> sendptr = DynamicCast<NewSendApplication> (sourceApps.Get(0));
  // std::cout<<"created pointer\n";
  Simulator::Schedule(Seconds(1), &checkPrimaryComplete, browserNum, sendptr, consecPageCounter, Simulator::Now());
  // std::cout<<"primary check scheduled\n";
}

void checkPrimaryComplete(uint32_t browserNum, Ptr<NewSendApplication> sendptr, uint32_t* consecPageCounter, Time start)
{
  // std::cout<<"In primary check...\n";
  if(sendptr->ResponseComplete())
  {
    // std::cout<<"Response is complete\n";
    responseTimes.push_back(Simulator::Now().GetMilliSeconds() - start.GetMilliSeconds());
    InetSocketAddress destServer = ConvertToInetSocketAddress(sendptr->GetDestinationAddress());
    secondaryRequest(browserNum, destServer, consecPageCounter);
  }
  else
  {
    // std::cout<<"Incomplete! Scheduling another check\n";
    Simulator::Schedule(delta, &checkPrimaryComplete, browserNum, sendptr, consecPageCounter, start);
  }
}

void secondaryRequest(uint32_t browserNum, InetSocketAddress destServer, uint32_t* consecPageCounter)
{
  // std::cout<<"In Secondary request ...\n";
  uint32_t* secondaryRequestCounter = new uint32_t();
  *secondaryRequestCounter = generateNumSecondaryRequests();
  for(uint32_t i = 0; i < *secondaryRequestCounter; i++)
  {
    // std::cout<<"Creating secondary request #"<<*secondaryRequestCounter<<"\n";
    uint32_t secReqSize = generateSecondaryRequestSize();
    uint32_t secRespSize = generateSecondaryResponseSize();
    uint16_t port = GeneratePortNum(browserNum);
    //Generate secondary request

    //Switch comments later
    NewSendHelper reqSender("ns3::TcpSocketFactory",
                         InetSocketAddress (csma2Interfaces.GetAddress(browserNum), port), destServer, secRespSize, secReqSize);
    // NewSendHelper reqSender("ns3::TcpSocketFactory",
    //                      InetSocketAddress (browserInterfaces.GetAddress(browserNum), port), destServer, secRespSize, secReqSize);
    ApplicationContainer sourceApps = reqSender.Install (browsers.Get(browserNum));
    Ptr<NewSendApplication> sendptr = DynamicCast<NewSendApplication> (sourceApps.Get(0));
    // std::cout<<"created pointer\n";
    Simulator::Schedule(Seconds(1), &checkSecondaryComplete, browserNum, sendptr, consecPageCounter, secondaryRequestCounter, Simulator::Now());
    // std::cout<<"Scheduling checks\n";
  }
}

void checkSecondaryComplete(uint32_t browserNum, Ptr<NewSendApplication> sendptr, uint32_t* consecPageCounter, uint32_t* secondaryRequestCounter, Time start)
{

  if(sendptr->ResponseComplete())
  {
    responseTimes.push_back(Simulator::Now().GetMilliSeconds() - start.GetMilliSeconds());
    // std::cout<<"Consec pg ctr"<<*consecPageCounter<<std::endl;
    // std::cout<<"Sec req ctr: "<<*secondaryRequestCounter<<std::endl;  
   (*secondaryRequestCounter)--;
   // std::cout<<"Sec req ctr after --: "<<*secondaryRequestCounter<<std::endl;
   if(*secondaryRequestCounter == 0)  //Page loaded, think and open new one
   {
    // delete secondaryRequestCounter;
    (*consecPageCounter)--;
    uint32_t thinkTime = generateThinkTime();
    if(*consecPageCounter == 0) // all pages for this sever done. open new site
    {
      // delete consecPageCounter;
      Simulator::Schedule(Seconds(thinkTime), &User, browserNum);
      std::cout<<"User done with this site\n";
    }
    else
    {
      InetSocketAddress destServer = ConvertToInetSocketAddress(sendptr->GetDestinationAddress());
      Simulator::Schedule(Seconds(thinkTime), &primaryRequest, browserNum, destServer, consecPageCounter);
    }
   }
  }
  else
    Simulator::Schedule(delta, &checkSecondaryComplete, browserNum, sendptr, consecPageCounter, secondaryRequestCounter, start);
}

int main (int argc, char *argv[])
{
  RngSeedManager::SetSeed (11223344);
  uv = CreateObject<UniformRandomVariable> ();
  uv->SetAttribute("Stream", IntegerValue(6110));

  /* Read CDF data from disk into respective std::set's */
  std::ifstream consecpages_file("cdf_data/p4consecpages.txt");
  std::ifstream filesperpage_file("cdf_data/p4filesperpage.txt");
  std::ifstream primaryreply_file("cdf_data/p4primaryreply.txt");
  std::ifstream primaryreq_file("cdf_data/p4primaryreq.txt");
  std::ifstream secondaryreply_file("cdf_data/p4secondaryreply.txt");
  std::ifstream secondaryreq_file("cdf_data/p4secondaryreq.txt");
  std::ifstream thinktimes_file("cdf_data/p4thinktimes.txt");

  populate_cdf_data(consecpages_file, cdf_consecpages);
  populate_cdf_data(filesperpage_file, cdf_filesperpage);
  populate_cdf_data(primaryreply_file, cdf_primaryreply);
  populate_cdf_data(primaryreq_file, cdf_primaryreq);
  populate_cdf_data(secondaryreply_file, cdf_secondaryreply);
  populate_cdf_data(secondaryreq_file, cdf_secondaryreq);
  populate_cdf_data(thinktimes_file, cdf_thinktimes);

  // NodeContainer p2pNodes1;
  // p2pNodes1.Create(2);
  
  browsers.Create(1);
  servers.Create(1);
  //CREATE NODES
  NodeContainer temp;
  temp.Add(browsers.Get(0));
  temp.Add(servers.Get(0));
  // CREATE ROUTERS
  temp.Create(2);

  // CREATE LINKS
  NodeContainer n1r1 = NodeContainer(temp.Get(0),temp.Get(2));
  NodeContainer r1r2 = NodeContainer(temp.Get(2),temp.Get(3));
  NodeContainer r2n2 = NodeContainer(temp.Get(3),temp.Get(1));
  // Create Switches

  CsmaHelper csma1;
  csma1.SetChannelAttribute("DataRate", StringValue ("100Mbps"));
  csma1.SetChannelAttribute("Delay", StringValue ("10ms"));
  NetDeviceContainer csmaDevices1 = csma1.Install(n1r1);  

  CsmaHelper csma2;
  csma2.SetChannelAttribute("DataRate", StringValue ("100Mbps"));
  csma2.SetChannelAttribute("Delay", StringValue ("10ms"));
  NetDeviceContainer csmaDevices2 = csma2.Install(r2n2);
  
  PointToPointHelper link;
  link.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  link.SetChannelAttribute ("Delay", StringValue ("10ms"));
  //link.SetQueue();
  NetDeviceContainer link1 = link.Install(r1r2);

  InternetStackHelper stack;
  stack.Install(browsers);
  stack.Install(servers);
  stack.Install(r1r2);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");

  csma1Interfaces = address.Assign(csmaDevices1);
  address.SetBase("10.1.2.0", "255.255.255.0");

  csma2Interfaces = address.Assign(csmaDevices2);
  address.SetBase("10.1.3.0", "255.255.255.0");

  Ipv4InterfaceContainer link1Interfaces;
  link1Interfaces = address.Assign(link1);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  uint16_t port = 5000;
  ApplicationContainer sinkApps;
  NewPacketSinkHelper sink ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), port));
  sinkApps.Add (sink.Install(servers.Get(0)));
  Ptr<NewPacketSink> sinkptr = DynamicCast<NewPacketSink> (sinkApps.Get(0));

  for(int i = 0; i < 100; i++)
  {
    Simulator::Schedule(Seconds(10.0 + uv->GetValue(0,1)), &User, 0);
  }
  Simulator::Stop(Seconds(3000));
  Simulator::Run ();
  
  Simulator::Destroy ();
  std::cout<<"Sim ended\n";
  std::cout<<"Total data received by the server"<<sinkptr->GetTotalRx()<<std::endl;

  std::cout<<"Number of response times recorded: "<<responseTimes.size()<<std::endl;
  
}
