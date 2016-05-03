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


//<<<<<<< HEAD
//uint32_t maxNodes = 1;
//=======
uint32_t maxNodes = 49;
//>>>>>>> custom

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

// Function prototypes
uint16_t GeneratePortNum(uint32_t node);
void populate_cdf_data(std::ifstream& file, std::set<cdfentry_t, classcomp>& cdfset);


// typedef struct request_param 
// {
//   uint32_t type;                          // 0 for primary request, 1 for secondary
//   uint32_t browserNum;
//   uint32_t* consecPageCounter;
//   Time start;
//   InetSocketAddress destServer;
//   uint32_t* secondaryRequestCounter;  // Only for secondary request
// } request_param_t;

void initDistributions()
{
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

}


Ptr<UniformRandomVariable> uv;
std::set<used_address_t> usedAddresses; 
std::vector<int64_t> responseTimes;
NodeContainer browsers;
NodeContainer servers;
std::vector<Ipv4Address> browserInterfaces(maxNodes*sizeof(Ipv4Address));
std::vector<Ipv4Address> serverInterfaces(maxNodes*sizeof(Ipv4Address));
double browserRx = 0;

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

      std::string::size_type sz;     // alias of size_t
      cdfentry_t entry;

      entry.prob_value = std::stof(line, &sz);
      entry.size_value = std::stoi(line.substr(sz+1));
      cdfset.insert(entry);
    }
    //std::cout<<"\n";
    file.close();
  }
}

std::vector<uint32_t> populate_delays(std::ifstream& file)
{
  std::string line;
  std::vector<uint32_t> delays;

  if (file.is_open())
  {
    while ( std::getline (file, line) )
    {
      //std::cout << line << '\n';
      std::istringstream iss(line);
      uint32_t n;
      iss>>n;
      delays.push_back(n);
    }
    //std::cout<<"\n";
    file.close();
  }
  return delays;
}


int get_size(std::set<cdfentry_t, classcomp>& cdfset, float probval)
{
  cdfentry_t dummy;
  dummy.prob_value = probval;

  return (cdfset.upper_bound(dummy))->size_value;
}

uint32_t generateConsecPageCounter()
{
  float probval = uv->GetValue(0.0, 1.0);
  return get_size(cdf_consecpages, probval);
  // return 2;
}

uint32_t generatePrimaryRequestSize()
{
  float probval = uv->GetValue(0.0, 1.0);
  return get_size(cdf_primaryreq, probval);
  // return 2000;
}

uint32_t generatePrimaryResponseSize()
{
  float probval = uv->GetValue(0.0, 1.0);
  return get_size(cdf_primaryreply, probval);
  // return 1000;
}

uint32_t generateNumSecondaryRequests()
{
  float probval = uv->GetValue(0.0, 1.0);
  return get_size(cdf_filesperpage, probval);
  // return 5;
}

uint32_t generateSecondaryRequestSize()
{
  float probval = uv->GetValue(0.0, 1.0);
  return get_size(cdf_secondaryreq, probval);
  // return 300;
}

uint32_t generateSecondaryResponseSize()
{
  float probval = uv->GetValue(0.0, 1.0);
  return get_size(cdf_secondaryreply, probval);
  // return 0;
}

uint32_t generateThinkTime()
{
  float probval = uv->GetValue(0.0, 1.0);
  // std::cout<<"Probval: "<<probval<<std::endl;
  return get_size(cdf_thinktimes, probval);
  // return 200;
}

void primaryRequest(uint32_t browserNum, InetSocketAddress destServer, uint32_t *consecPageCounter);
static void checkPrimaryComplete(request_param_t param);
void secondaryRequest(uint32_t browserNum, InetSocketAddress destServer, uint32_t* consecPageCounter);
void checkSecondaryComplete(request_param_t param);

InetSocketAddress generateDestServer()
{
  // pick server node num
  // uint32_t server = 0;
  uint16_t port = 5000;

  // Switch comments later
  // std::cout<<"Destination Address: "<<(serverInterfaces[(int) uv->GetValue(0,maxNodes)])<<std::endl;
  return InetSocketAddress (serverInterfaces[(int) uv->GetValue(0,maxNodes)], port);
  // return InetSocketAddress(serverInterfaces.GetAddress(server), port);

}

InetSocketAddress ConvertToInetSocketAddress(Address A)
{
  return InetSocketAddress(InetSocketAddress::ConvertFrom(A).GetIpv4 (), InetSocketAddress::ConvertFrom (A).GetPort ());
}
int user_count = 0;
void User(uint32_t browserNum)
{
  // std::cout<<"User #"<<user_count++<<std::endl;
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
  request_param_t param;
  //param.type = 0;
  param.browserNum = browserNum;
  param.destServer = destServer;
  param.consecPageCounter = consecPageCounter;
  param.start = Simulator::Now();
  param.secondaryRequestCounter = NULL;
  param.func = &checkPrimaryComplete;
  // std::cout<<"function ptr: "<<(param.func)<<std::endl;
  NewSendHelper reqSender("ns3::TcpSocketFactory",
                         InetSocketAddress (browserInterfaces[browserNum], port), destServer, primaryRespSize, primaryReqSize, param);
  // NewSendHelper reqSender("ns3::TcpSocketFactory",
  //                        InetSocketAddress (browserInterfaces.GetAddress(browserNum), port), destServer, primaryRespSize, primaryReqSize, param);
  ApplicationContainer sourceApps = reqSender.Install (browsers.Get(browserNum));
  //sourceApps.Start(Simulator::Now());
  // std::cout<<"sender app started\n";
  // Ptr<NewSendApplication> sendptr = DynamicCast<NewSendApplication> (sourceApps.Get(0));
  // std::cout<<"created pointer\n";
  //Simulator::Schedule(Seconds(1), &checkPrimaryComplete, browserNum, sendptr, consecPageCounter, Simulator::Now());
  // std::cout<<"primary check scheduled\n";
}

static void checkPrimaryComplete(request_param_t param)
{
  //std::cout<<"In check primary complete"<<std::endl;
  responseTimes.push_back(param.end.GetMilliSeconds() - param.start.GetMilliSeconds());
  browserRx += double(param.resp_size);
  secondaryRequest(param.browserNum, param.destServer, param.consecPageCounter);
}

void secondaryRequest(uint32_t browserNum, InetSocketAddress destServer, uint32_t* consecPageCounter)
{
  //std::cout<<"In Secondary request ...\n";
  uint32_t* secondaryRequestCounter = new uint32_t();
  *secondaryRequestCounter = generateNumSecondaryRequests();
  for(uint32_t i = 0; i < *secondaryRequestCounter; i++)
  {
    // std::cout<<"Creating secondary request #"<<*secondaryRequestCounter<<"\n";
    uint32_t secReqSize = generateSecondaryRequestSize();
    uint32_t secRespSize = generateSecondaryResponseSize();
    uint16_t port = GeneratePortNum(browserNum);

    request_param_t param;
    //param.type = 1;
    param.browserNum = browserNum;
    param.destServer = destServer;
    param.consecPageCounter = consecPageCounter;
    param.secondaryRequestCounter = secondaryRequestCounter;
    param.func = &checkSecondaryComplete;
    //Generate secondary request

    //Switch comments later
    NewSendHelper reqSender("ns3::TcpSocketFactory",
                          InetSocketAddress (browserInterfaces[browserNum], port), destServer, secRespSize, secReqSize, param);
    ApplicationContainer sourceApps = reqSender.Install (browsers.Get(browserNum));
    // Ptr<NewSendApplication> sendptr = DynamicCast<NewSendApplication> (sourceApps.Get(0));
    // std::cout<<"created pointer\n";
    //Simulator::Schedule(Seconds(1.0+ uv->GetValue(0,10)), &checkSecondaryComplete, browserNum, sendptr, consecPageCounter, secondaryRequestCounter, Simulator::Now());
    // std::cout<<"Scheduling checks\n";
  }
}

void checkSecondaryComplete(request_param_t param)
{
  //std::cout<<"In check sec complete"<<std::endl;
  responseTimes.push_back(param.end.GetMilliSeconds() - param.start.GetMilliSeconds());
  browserRx += double(param.resp_size);
  // std::cout<<"Consec pg ctr"<<*consecPageCounter<<std::endl;
  // std::cout<<"Sec req ctr: "<<*secondaryRequestCounter<<std::endl;  
  uint32_t* secondaryRequestCounter = param.secondaryRequestCounter;
 (*secondaryRequestCounter)--;
 // std::cout<<"Sec req ctr after --: "<<*secondaryRequestCounter<<std::endl;
 if(*secondaryRequestCounter == 0)  //Page loaded, think and open new one
 {
  // delete secondaryRequestCounter;
  uint32_t *consecPageCounter = param.consecPageCounter;
  (*consecPageCounter)--;
  uint32_t thinkTime = generateThinkTime();
  if(*consecPageCounter == 0) // all pages for this sever done. open new site
  {
    // delete consecPageCounter;
    Simulator::Schedule(Seconds(thinkTime), &User, param.browserNum);
    // std::cout<<"User done with this site\n";
  }
  else
  {
    Simulator::Schedule(Seconds(thinkTime), &primaryRequest, param.browserNum, param.destServer, consecPageCounter);
  }
 }

}

int main (int argc, char *argv[])
{
  CommandLine cmd;
  int numUsers = 1;
  int queueType = 1; // Red 1, Droptail - 0
  double minTh = 30;
  double maxTh = 90;
  uint32_t qlen = 480;
  double qw = 0.002;
  double maxp = 10;
  std::string bottleneck = "100Mbps";

  cmd.AddValue("numUsers", "Number of users", numUsers);
  cmd.AddValue("queuetype", "Set Queue type to Droptail <0> or RED <1>", queueType);
  cmd.AddValue ("redMinTh", "RED queue minimum threshold", minTh);
  cmd.AddValue ("redMaxTh", "RED queue maximum threshold", maxTh);
  cmd.AddValue("qlen", "Queue limit in packets", qlen);
  cmd.AddValue("qw", "Queue weight related to the exponential weighted moving average", qw);
  cmd.AddValue("maxp", "Max probability of dropping a packet", maxp);
  cmd.AddValue("bottleneck", "Bottleneck bandwidth", bottleneck);
  cmd.Parse (argc,argv);
  RngSeedManager::SetSeed (11223344);
  uv = CreateObject<UniformRandomVariable> ();
  uv->SetAttribute("Stream", IntegerValue(6110));

  initDistributions();
  std::ifstream delay_file("cdf_data/delays.txt");
  std::vector<uint32_t> delays = populate_delays(delay_file);
  browsers.Create(maxNodes);
  servers.Create(maxNodes);
  NodeContainer routers;
  routers.Create(2);
  PointToPointHelper link;
  PointToPointHelper link2;

  InternetStackHelper stack; 
  stack.Install(browsers);
  stack.Install(servers);
  stack.Install(routers);
  NetDeviceContainer netDevice;
  NetDeviceContainer netDevice2;
  Ipv4AddressHelper ipv4;
  for(uint32_t i = 0; i < maxNodes; i++)  {
    link.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    std::ostringstream delay;
    delay << delays[i]<<"ms";
    link.SetChannelAttribute ("Delay", StringValue (delay.str()));
    NodeContainer leftLink = NodeContainer(browsers.Get(i), routers.Get(0));
    netDevice = link.Install(leftLink);

    std::ostringstream subnet;
    subnet << "10.1." << i+1 << ".0";
    ipv4.SetBase(subnet.str().c_str(), "255.255.255.0"); 
    Ipv4InterfaceContainer iface = ipv4.Assign(netDevice);
    browserInterfaces[i] = iface.GetAddress(0);

    link2.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
    link2.SetChannelAttribute ("Delay", StringValue ("1ms"));
    NodeContainer rightLink = NodeContainer (routers.Get(1), servers.Get(i));
    netDevice2 = link2.Install(rightLink);
    
    std::ostringstream subnet2;
    subnet2 << "10.2." << i+1 << ".0";
    ipv4.SetBase(subnet2.str().c_str(), "255.255.255.0");
    Ipv4InterfaceContainer iface2 = ipv4.Assign(netDevice2);
    serverInterfaces[i] = iface2.GetAddress(1);
  }


  if(queueType == 0)
  {
    Config::SetDefault ("ns3::DropTailQueue::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (qlen));
    link.SetQueue ("ns3::DropTailQueue");
  }
  else if(queueType == 1)
  {
    Config::SetDefault ("ns3::RedQueue::Mode", StringValue ("QUEUE_MODE_PACKETS"));
    Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (qlen));
    link.SetQueue ("ns3::RedQueue",
                        "MinTh", DoubleValue (minTh),
                        "MaxTh", DoubleValue (maxTh),
                        "QW", DoubleValue(qw),
                        "LInterm", DoubleValue(maxp));
  }
  link.SetDeviceAttribute ("DataRate", StringValue (bottleneck));
  link.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NodeContainer midLink = NodeContainer(routers.Get(0),routers.Get(1));
  netDevice = link.Install(midLink);


  ipv4.SetBase("10.3.1.0", "255.255.255.0");
  ipv4.Assign(netDevice);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  uint16_t port = 5000;
  ApplicationContainer sinkApps;
  NewPacketSinkHelper sink ("ns3::TcpSocketFactory",
                          InetSocketAddress (Ipv4Address::GetAny (), port));
  for(uint32_t i = 0; i < maxNodes; i++)  {
    sinkApps.Add(sink.Install(servers.Get(i)));
  } 
  for(uint32_t browserNum = 0; browserNum < maxNodes; browserNum++)
    for(int i = 0; i < numUsers; i++)
      Simulator::Schedule(Seconds(10.0 + uv->GetValue(0,10)), &User, browserNum);

  Simulator::Stop(Seconds(2000));
  Simulator::Run ();
  Simulator::Destroy ();
  //std::cout<<"Sim ended\n";

  uint32_t total = 0;
  for(uint32_t i = 0; i < maxNodes; i++) {
    Ptr<NewPacketSink> sinkptr = DynamicCast<NewPacketSink> (sinkApps.Get(i));
    total = total + sinkptr->GetTotalRx ();
  }
  //std::cout<<"Total data received at browsers: "<<browserRx<<std::endl;
  std::cout<<"Load generated: "<<(browserRx+total)*8/2000<<" bps"<<std::endl;
  std::cout<<"Number of response times recorded: "<<responseTimes.size()<<std::endl;

  for (int ind=0; ind<responseTimes.size(); ind++)
     std::cout<<responseTimes[ind]<<"\n";
  
}
