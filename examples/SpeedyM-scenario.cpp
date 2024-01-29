/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */


#include <fstream>
#include <string>
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <time.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/netanim-module.h"
#include "ns3/blanc-app-helper.hpp"
#include "src/ndnSIM/apps/SpeedyM-sync.hpp"
#include <unordered_map>
namespace ns3 {

void onTx(std::string node,  uint32_t txid, bool payer, double amount);
void onTxFail(uint32_t txid);
void onTxRetry(uint32_t txid);
void onFindReply(uint32_t, uint32_t, double);
void onHold(uint32_t, uint32_t, bool);
void onPay(uint32_t, uint32_t);
void onPayPath(uint32_t, std::string);
void onPathUpdate(std::string, std::string, double);
void onAd(std::string node, std::string treeId);


	
// Vectors to store the various node types
std::vector<int> com_nodes, agg_nodes, phy_nodes;

// Vectors to store source and destination edges to fail
std::vector<int> srcedge, dstedge;

// Vectors to store agg<--->com and phy<--->agg p2p IPs, packets are directed to these IP addresses
std::vector<std::string> com_ips;
std::vector<std::pair<int,std::string>> phy_to_agg_map;
std::string GetIP(int nodeid);
std::string GetLinkIP(int node1, int node2);
bool HasIP(int nodeid);
// Nodes and their corresponding IPs, used to output namespaces in tracefile
std::vector<std::pair<int,std::string>> node_ip_map;
std::vector<std::pair<std::string,std::string>> link_ip_map;
std::vector<std::pair<int,std::string>> wac_ip_map;
std::vector<std::pair<int,std::string>> pdc_ip_map;
std::vector<std::pair<int,std::string>> pmu_ip_map;

// Node and interface pairs to fail, between com and agg
std::vector<std::pair<int,int>> node_link_map_fail;

// Split a line into a vector of strings
std::vector<std::string> SplitString(std::string);

//Store unique IDs to prevent repeated installation of app
bool IsPDCAppInstalled(std::string PDCID);
bool IsWACAppInstalled(std::string WACID);

//Get node IP from subnet defined in topology file
std::string GetIPFromSubnet(std::string nodeid, std::string subnet, int num);

std::string GetPDCIP(int pdcid);
std::string GetWACIP(int wacid);

std::vector<std::string> uniquePDCs;
std::vector<std::string> uniqueWACs;

bool NodeInComm(int);
bool NodeInAgg(int);
bool NodeInPhy(int);
int GetNodeFromIP(std::string);
std::string GetAggIP(int);

//Trace file
std::ofstream tracefile;
std::ofstream tracefile1;

std::ofstream flowfile;

//Gets current time to make packets unique
time_t seconds;

ns3::ndn::SpeedySync sync;

int main (int argc, char *argv[])
{

  int run = 0, Method2 = 0, Lighting = 1;	
  CommandLine cmd;
  cmd.AddValue("Run", "Run", run);  
  cmd.AddValue("Lighting", "Lighting", Lighting);  
  cmd.Parse (argc, argv);

  // Open the configuration file for reading
  //std::ifstream configFile ("../topology/interface/blancPP-TestCase2.txt", std::ios::in);
  std::string prefix = "Blanc";
  std::string TXmapfile = "TXMap.txt";
  if (Lighting) {
        prefix = "LN";
        TXmapfile = "LNTXMap.txt";        
  }
  std::ifstream configFile (prefix+"Topo.txt", std::ios::in);

  std::string strLine;
  bool gettingNodeCount = false, buildingNetworkTopo = false, nonRH = false, RH = false, clients = false, costTable = false; 
  bool failLinks = false, injectData = false;
  std::vector<std::string> netParams;

  unordered_map<int,std::string> txMap;


  std::ifstream txIDFile (TXmapfile, std::ios::in);

  if (txIDFile.is_open ()) {
     while (std::getline(txIDFile, strLine)) {
           std::vector<std::string> params = SplitString(strLine);
           txMap[std::stoi(params[0])] = params[1];
     }
     sync.setTxTable(txMap);
  }


  NodeContainer nodes;
  int nodeCount = 0;

  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("50p"));
  Config::SetDefault("ns3::TcpSocket::SegmentSize", StringValue("1448"));

  PointToPointHelper p2p;
  NetDeviceContainer devices;
  Ipv4AddressHelper addresses;

  std::pair<int,std::string> node_ip_pair;

  SpeedyHelper blancHelper;
  ApplicationContainer blancApps;
  sync.setTimeStep(0.1);

  uint16_t wacPort = 5000;
  uint16_t pdcPort = 6000;
  uint16_t bgdPort = 1000;

  Ptr<Ipv4> ipv4node;

  flowfile.open("ip_all_flows.csv", std::ios::out); 


  //Open trace file for writing
  tracefile.open(prefix+"-Speedy-trace"+ std::to_string(run) +".csv", std::ios::out);
  tracefile << "event,txID,time" << std::endl;

  tracefile1.open(prefix+"-Speedy-path"+ std::to_string(run) +".csv", std::ios::out);
  tracefile1 << "event,amount,time,node1,node2,value" << std::endl;  

  std::vector<int> lastIP = {10, 0, 0, 4};
  unordered_map<int,int> RHs;
  unordered_map<int,int> Cls;
  unordered_map<int,std::vector<double>> costToMap;
  unordered_map<int,std::vector<double>> costFromMap;
  unordered_map<int,std::vector<int>> neighborMap;  
  std::vector<double> normal_links;
  std::vector<double> rh_links;

  if(!Lighting){

     std::ifstream normalLinkFile ("Normal_links.txt", std::ios::in);
     if (normalLinkFile.is_open ()) {
        while (std::getline(normalLinkFile, strLine)) {
                normal_links.push_back(  std::stod(strLine)  );
        }
     }

     std::ifstream rhLinkFile ("RH_links.txt", std::ios::in);
     if (rhLinkFile.is_open ()) {
        while (std::getline(rhLinkFile, strLine)) {
                rh_links.push_back(  std::stod(strLine)  );
        }
     }     
  }

  srand( run );
  int startClient = 10000000000;

  if (configFile.is_open ()) {
     while (std::getline(configFile, strLine)) {

        //Determine what operation is ongoing while reading the config file
        if(strLine.substr(0,7) == "BEG_000") { gettingNodeCount = true; continue; }
        if(strLine.substr(0,7) == "END_000") {
           std::cout<<nodeCount<<std::endl;	
           //Create nodes
           nodeCount++; //Counter increment needed because nodes start from 1 not 0
           gettingNodeCount = false;
           nodes.Create(nodeCount);

           //Install internet protocol stack on nodes
           InternetStackHelper stack1;
           stack1.Install (nodes);

           continue; 
        }
        if(strLine.substr(0,7) == "BEG_001") { buildingNetworkTopo = true; continue; }
        if(strLine.substr(0,7) == "END_001") { buildingNetworkTopo = false; }
        if(strLine.substr(0,7) == "BEG_002") { nonRH = true; continue; }
        if(strLine.substr(0,7) == "END_002") { nonRH = false; continue; }
        if(strLine.substr(0,7) == "BEG_003") { RH = true; continue; }
        if(strLine.substr(0,7) == "END_003") { RH = false; continue; }
        if(strLine.substr(0,7) == "BEG_004") { clients = true; continue; }
        if(strLine.substr(0,7) == "END_004") { clients = false; continue; }
        if(strLine.substr(0,7) == "BEG_005") { costTable = true; continue; }
        if(strLine.substr(0,7) == "END_005") { costTable = false; continue; }
        if(gettingNodeCount == true) {
           //Getting number of nodes to create
           netParams = SplitString(strLine);
           nodeCount = stoi(netParams[1]);
        }
        else if(buildingNetworkTopo == true) {
           //Building network topology
           netParams = SplitString(strLine);
           int mbps = std::stoi(netParams[2]);
           p2p.SetDeviceAttribute( "DataRate", StringValue( std::to_string(mbps)+"Mbps" ) );			
           double delay = 5;//double(rand()%6+4);

           p2p.SetChannelAttribute("Delay", StringValue(std::to_string(delay)+ "ms"));
          
           devices = p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
           std::string IP = std::to_string(lastIP[0])+"."+std::to_string(lastIP[1])+"."+std::to_string(lastIP[2])+"."+std::to_string(lastIP[3]);

           addresses.SetBase (Ipv4Address (IP.c_str()), Ipv4Mask("255.255.255.252"));
           addresses.Assign (devices);

           lastIP[3]+=4;
           if(lastIP[3]>=256){
                lastIP[3] = 4;
                lastIP[2]+=1;
           }
           if(lastIP[2]>=256){
                lastIP[2] = 4;
                lastIP[1]+=1;
           }             
           node_ip_pair.first = std::stoi(netParams[0]);
           node_ip_pair.second = GetIPFromSubnet(netParams[0], IP, 1);
           node_ip_map.push_back(node_ip_pair);

           //  if(!HasIP(std::stoi(netParams[1]))){ 
           node_ip_pair.first = std::stoi(netParams[1]);
           node_ip_pair.second = GetIPFromSubnet(netParams[1], IP, 2);
           node_ip_map.push_back(node_ip_pair);

           std::pair<std::string,std::string> link_ip_pair;
           link_ip_pair.first = netParams[0]+" "+netParams[1];
           link_ip_pair.second = IP;

           link_ip_map.push_back(link_ip_pair );           
        }
        else if(false && nonRH == true) {
           //Install apps on PDCs and PMUs for data exchange
           netParams = SplitString(strLine);

           //Install non Router Helpers
           //TODO: add in attributes
           blancHelper.SetAttribute ("PacketSize", UintegerValue (200));
           blancHelper.SetAttribute ("RouterHelper", UintegerValue (0));
           blancHelper.SetAttribute ("Name", StringValue(netParams[0]));
           blancApps = blancHelper.Install (nodes.Get (std::stoi(netParams[0])));
           auto apps = nodes.Get( std::stoi( netParams[0] ) )->GetApplication( 0 )->GetObject<ns3::SpeedyM>();
           sync.addNode( std::stoi( netParams[0] ),apps );

           std::string strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnFindReply";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onFindReply ) );
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnHold";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onHold ) );  
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnTxRetry";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onTxRetry ) );
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnTxFail";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onTxFail ) );                           
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnPay";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onPay ) );
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnAd";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onAd ) );           

        }
        else if(RH == true) {
           //Install apps on WACs and PMUs for data exchange
           netParams = SplitString(strLine);

           blancHelper.SetAttribute ("PacketSize", UintegerValue (200));
           blancHelper.SetAttribute ("RouterHelper", UintegerValue (1));
           blancHelper.SetAttribute ("Name", StringValue(netParams[0]));

           blancApps = blancHelper.Install (nodes.Get (std::stoi(netParams[0])));
           auto apps = nodes.Get( std::stoi( netParams[0] ) )->GetApplication( 0 )->GetObject<ns3::SpeedyM>();
           sync.addNode( std::stoi( netParams[0] ),apps );
           sync.addRH( std::stoi( netParams[0] ),apps );


           std::string strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnFindReply";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onFindReply ) );
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnHold";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onHold ) );
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnPay";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onPay ) );
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnTxRetry";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onTxRetry ) );
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnTxFail";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onTxFail ) );       
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnPayPath";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onPayPath ) );    
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnPathUpdate";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onPathUpdate ) );   
           strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnAd";
           Config::ConnectWithoutContext( strcallback, MakeCallback( &onAd ) );                                      

           RHs[std::stoi(netParams[0])] = 1;
        }
        else if(clients == true) {
           netParams = SplitString(strLine);
           startClient = std::stoi(netParams[0]);
           sync.addSender( startClient );
           Cls[std::stoi(netParams[0])] = 1;

        }
        else if(costTable == true) {
           netParams = SplitString(strLine);
             netParams = SplitString(strLine);
           double linkTo;
           double linkFrom;
           if(Lighting){
              linkTo = std::stod(netParams[2])/ 1000000;
              linkFrom = std::stod(netParams[3])/ 1000000;
           }
           else {
                linkTo = normal_links[  rand()%normal_links.size()  ];
                linkFrom =  normal_links[  rand()%normal_links.size()  ];             
                if( RHs[std::stoi(netParams[0])] == 1  ||  RHs[std::stoi(netParams[1])] == 1){
                        linkTo = rh_links[  rand()%rh_links.size()  ]/ 1000000;
                        linkFrom = rh_links[  rand()%rh_links.size()  ]/ 1000000;
                }
           }

           if (Cls[ std::stoi(netParams[0]) ] || Cls[ std::stoi(netParams[1]) ])
                linkTo = 100000000000/ 1000000;
                linkFrom = 100000000000/ 1000000;
           onPathUpdate(netParams[0], netParams[1], linkTo);
           onPathUpdate(netParams[1], netParams[0], linkFrom);                   
           costToMap[std::stoi(netParams[0])].push_back(linkTo);
           costFromMap[std::stoi(netParams[0])].push_back(linkFrom);    
           neighborMap[std::stoi(netParams[0])].push_back(std::stoi(netParams[1])); 
        }
        else {
           //std::cout << "reading something else " << strLine << std::endl;
        }
     }
  }
  else {
     std::cout << "Cannot open configuration file!!!" << std::endl;
     exit(1);
  }

  configFile.close();

  for ( int i=0; i<( int )nodes.size()-1; i++ ) {
     if (RHs[i] == 0){
        netParams = SplitString(strLine);
        //Install non Router Helpers
        //TODO: add in attributes
        blancHelper.SetAttribute ("PacketSize", UintegerValue (200));
        blancHelper.SetAttribute ("RouterHelper", UintegerValue (0));
	blancHelper.SetAttribute ("Name", StringValue(std::to_string(i)));
        blancApps = blancHelper.Install (nodes.Get (i));
	auto apps = nodes.Get( i )->GetApplication( 0 )->GetObject<ns3::SpeedyM>();
        sync.addNode( i,apps );

	std::string strcallback = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + "*/OnFindReply";
	Config::ConnectWithoutContext( strcallback, MakeCallback( &onFindReply ) );
        strcallback = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + "*/OnHold";
        Config::ConnectWithoutContext( strcallback, MakeCallback( &onHold ) );
	strcallback = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + "*/OnPay";
        Config::ConnectWithoutContext( strcallback, MakeCallback( &onPay ) );
        strcallback = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + "*/OnTx";
        Config::ConnectWithoutContext( strcallback, MakeCallback( &onTx ) );
        strcallback = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + "*/OnTxRetry";
        Config::ConnectWithoutContext( strcallback, MakeCallback( &onTxRetry ) );
        strcallback = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + "*/OnTxFail";
        Config::ConnectWithoutContext( strcallback, MakeCallback( &onTxFail ) );        
        strcallback = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + "*/OnPayPath";
        Config::ConnectWithoutContext( strcallback, MakeCallback( &onPayPath ) );    
        strcallback = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + "*/OnPathUpdate";
        Config::ConnectWithoutContext( strcallback, MakeCallback( &onPathUpdate ) );      
        strcallback = "/NodeList/" + std::to_string(i) + "/ApplicationList/" + "*/OnAd";
        Config::ConnectWithoutContext( strcallback, MakeCallback( &onAd ) );                       
     }
  }
  std::cout<<"Setting addresses...\n";
  for (auto it = neighborMap.begin(); it != neighborMap.end(); it++) {
     for (int i = 0; i < it->second.size(); i++){
        //std::cout<<"Check this out..." <<it->first<<" "<<it->second[i]<<std::endl;
        std::string baseIP =  GetLinkIP(it->first, it->second[i]);
        std::string IP1 = GetIPFromSubnet(std::to_string(it->first), baseIP, 2);
        std::string IP2 = GetIPFromSubnet(std::to_string(it->second[i]), baseIP, 1);

        sync.setAddressTable(it->first, std::to_string(it->second[i]), Ipv4Address(IP1.c_str()) );
        sync.setAddressTable(it->second[i], std::to_string(it->first), Ipv4Address(IP2.c_str()) );

        sync.setNeighborCredit(it->first, std::to_string(it->second[i]), costToMap[it->first][i] );
        sync.setNeighborCredit(it->second[i], std::to_string(it->first), costFromMap[it->first][i] );
     }
  }

  //Populate the routing table
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  std::cout<<"Finshed\n";

  std::string strcallback;
  sync.setStart(run*10000);
  sync.beginSync(20.0);

  //Run actual simulation
  //Simulator::Stop (Seconds(1.0));
  Simulator::Stop (Seconds(1100.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

void onTx(std::string node,  uint32_t txid, bool payer, double amount){
   if (payer) {
        tracefile << "Start,"<< txid << "," << ns3::Simulator::Now().GetSeconds() << std::endl;
        tracefile1 << "Start,"<< txid << "," << ns3::Simulator::Now().GetSeconds() << "," << node << ",," << std::fixed << amount << std::endl;
   }
   else
        tracefile1 << "Start,"<< txid << "," << ns3::Simulator::Now().GetSeconds() << ",," << node << "," << std::fixed << amount << std::endl;
};

void onFindReply(uint32_t node,  uint32_t txid, double amount){
   //sync.onFindReplyPacket(node, txid, amount);
};

void onHold(uint32_t node,  uint32_t txid, bool received){
   //sync.onHoldPacket(node, txid, received);
};

void onPay(uint32_t node,  uint32_t txid){
   tracefile << "Complete,"<< txid << "," << ns3::Simulator::Now().GetSeconds() << std::endl;
   sync.onPayPacket(node, txid);	
};

void onTxFail(uint32_t txid){
   tracefile << "Fail," << txid<<","<<ns3::Simulator::Now().GetSeconds() << std::endl;
   sync.onTxFail(txid);	
};

void onTxRetry(uint32_t txid){
   tracefile << "Retry," << txid<<","<<ns3::Simulator::Now().GetSeconds() << std::endl;

};

void onPayPath(uint32_t nodeID, std::string txid){
   tracefile1 << "Pay,"<< txid << "," << ns3::Simulator::Now().GetSeconds() << std::endl;

};

void onPathUpdate(std::string nodeID1, std::string nodeID2, double amount){
   tracefile1 << "PathUpdate,"<< std::fixed << amount << "," << ns3::Simulator::Now().GetSeconds() << ","  <<nodeID1 <<"," <<nodeID2<< std::endl;

};

void onAd(std::string node, std::string treeId){
   tracefile << "Ad," << node<<","<<ns3::Simulator::Now().GetSeconds() << std::endl;
}


//Split a string delimited by space
std::vector<std::string> SplitString(std::string strLine) {
        std::string str = strLine;
        std::vector<std::string> result;
        std::istringstream isstr(str);
        for(std::string str; isstr >> str; )
                result.push_back(str);

        return result;
}

//Store unique PDC IDs to prevent repeated installation of app
bool IsPDCAppInstalled(std::string PDCID) {
	for (int i=0; i<(int)uniquePDCs.size(); i++) {
		if(PDCID.compare(uniquePDCs[i]) == 0) {
			return true;
		}
	}

	uniquePDCs.push_back(PDCID);
	return false;
}

//Store unique WAC IDs to prevent repeated installation of app
bool IsWACAppInstalled(std::string WACID) {
        for (int i=0; i<(int)uniqueWACs.size(); i++) {
                if(WACID.compare(uniqueWACs[i]) == 0) {
                        return true;
                }
        }

        uniqueWACs.push_back(WACID);
        return false;
}

//Get node IP from subnet defined in topology file
std::string GetIPFromSubnet(std::string nodeid, std::string subnet, int num) {

	std::string node_p2p_ip = "";
	std::pair<int,std::string> all_nodes_ip_pair;

 	std::size_t dotposition = subnet.find(".",0) + 1;
        node_p2p_ip = node_p2p_ip + subnet.substr(0,dotposition);

        std::string ipsecondtofourthoctet = subnet.substr( dotposition, std::string::npos );
        dotposition = ipsecondtofourthoctet.find(".",0) + 1;
        node_p2p_ip = node_p2p_ip + ipsecondtofourthoctet.substr(0,dotposition);

        std::string ipthirdtofourthoctet = ipsecondtofourthoctet.substr( dotposition, std::string::npos );
        dotposition = ipthirdtofourthoctet.find(".",0) + 1;
        node_p2p_ip = node_p2p_ip + ipthirdtofourthoctet.substr(0,dotposition);

        std::string ipfourthoctet = ipthirdtofourthoctet.substr( (ipthirdtofourthoctet.find(".",0))+1, std::string::npos );
        node_p2p_ip = node_p2p_ip + std::to_string(std::stoi(ipfourthoctet) + num);
	//Store all node and their IPs, used for trace file
	all_nodes_ip_pair.first = std::stoi(nodeid);
	all_nodes_ip_pair.second = node_p2p_ip;
	//node_ip_map.push_back(all_nodes_ip_pair);

	return node_p2p_ip;
}

bool NodeInComm(int nodeid) {
        for (int i=0; i<(int)com_nodes.size(); i++) {
                if (com_nodes[i] == nodeid) {
                        return true;
                }
        }
        return false;
}

bool NodeInAgg(int nodeid) {
        for (int i=0; i<(int)agg_nodes.size(); i++) {
                if (agg_nodes[i] == nodeid) {
                        return true;
                }
        }
        return false;
}

bool NodeInPhy(int nodeid) {
        for (int i=0; i<(int)phy_nodes.size(); i++) {
                if (phy_nodes[i] == nodeid) {
                        return true;
                }
        }
        return false;
}


int GetNodeFromIP(std::string str_ip) {
	for (int i=0; i<(int)node_ip_map.size(); i++) {
		if (node_ip_map[i].second == str_ip) {
			return node_ip_map[i].first;
		}
	}
	std::cout << "IP address " << str_ip << " not mapped to any node!!! Cannot write correct entry in tracefile" << std::endl;
	exit(1);
}


std::string GetAggIP(int nodeid) {
	for (int i=0; i<(int)phy_to_agg_map.size(); i++) {
		if (nodeid == phy_to_agg_map[i].first) {
			return phy_to_agg_map[i].second;
		}
	}
	std::cout << "Node " << nodeid << ", not connected to any aggregation IP!!!!!" << std::endl;
	exit(1);
}

std::string GetPDCIP(int pdcid) {
        for (int i=0; i<(int)pdc_ip_map.size(); i++) {
                if (pdcid == pdc_ip_map[i].first) {
                        return pdc_ip_map[i].second;
                }
        }
        std::cout << "PDC " << pdcid << ", has no corresponding IP!!!!!" << std::endl;
        
	exit(1);
}

std::string GetIP(int nodeid) {
        for (int i=0; i<(int)node_ip_map.size(); i++) {
                if (nodeid == node_ip_map[i].first) {
                        return node_ip_map[i].second;
                }
        }
        std::cout << "Node " << nodeid << ", has no corresponding IP!!!!!" << std::endl;

        exit(1);
}

std::string GetLinkIP(int node1, int node2){
    for (int i=0; i<(int)link_ip_map.size(); i++) {
        std::vector<std::string> nodes = SplitString(link_ip_map[i].first);
        if ((node1 == std::stoi(nodes[0]) || node1 == std::stoi(nodes[1]) ) &&
            (node2 == std::stoi(nodes[0]) || node2 == std::stoi(nodes[1]) ) ) {
                return link_ip_map[i].second;
        }
    }
    std::cout << "Node " << node1 << ", has no corresponding IP!!!!!" << std::endl;
    exit(1);        
}

bool HasIP(int nodeid) {
        for (int i=0; i<(int)node_ip_map.size(); i++) {
                if (nodeid == node_ip_map[i].first) {
                        return true;
                }
        }
        return false;
}


} //namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
