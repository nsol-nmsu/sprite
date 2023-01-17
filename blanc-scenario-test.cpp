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
#include "ns3/blanc-app-helper.h"
#include "src/ndnSIM/apps/BLANC-sync.hpp"
#include <unordered_map>
namespace ns3 {

void onFindReply(uint32_t, uint32_t, double);
void onHold(uint32_t, uint32_t, bool);
void onPay(uint32_t, uint32_t);

	
// Vectors to store the various node types
std::vector<int> com_nodes, agg_nodes, phy_nodes;

// Vectors to store source and destination edges to fail
std::vector<int> srcedge, dstedge;

// Vectors to store agg<--->com and phy<--->agg p2p IPs, packets are directed to these IP addresses
std::vector<std::string> com_ips;
std::vector<std::pair<int,std::string>> phy_to_agg_map;
std::string GetIP(int nodeid);
bool HasIP(int nodeid);
// Nodes and their corresponding IPs, used to output namespaces in tracefile
std::vector<std::pair<int,std::string>> node_ip_map;
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

std::ofstream flowfile;

//Gets current time to make packets unique
time_t seconds;

ns3::ndn::BLANCSync sync;

int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Open the configuration file for reading
  std::ifstream configFile ("../topology/interface/blancCase.txt", std::ios::in);
  //std::ifstream configFile ("src/ndnSIM/examples/case39Cyber.txt", std::ios::in);

  std::string strLine;
  bool gettingNodeCount = false, buildingNetworkTopo = false, nonRH = false, RH = false, RHTable = false, costTable = false; 
  bool failLinks = false, injectData = false;
  std::vector<std::string> netParams;

  NodeContainer nodes;
  int nodeCount = 0;

  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("50p"));

  PointToPointHelper p2p;
  NetDeviceContainer devices;
  Ipv4AddressHelper addresses;

  std::pair<int,std::string> node_ip_pair;

  BlancHelper blancHelper;
  ApplicationContainer blancApps;
  sync.setTimeStep(1.0);

  uint16_t wacPort = 5000;
  uint16_t pdcPort = 6000;
  uint16_t bgdPort = 1000;

  Ptr<Ipv4> ipv4node;

  flowfile.open("ip_all_flows.csv", std::ios::out);

  std::vector<int> lastIP = {10, 0, 0, 4};


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
		if(strLine.substr(0,7) == "BEG_004") { RHTable = true; continue; }
                if(strLine.substr(0,7) == "END_004") { RHTable = false; continue; }
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
			int mbps = std::stoi(netParams[2])/10;
			p2p.SetDeviceAttribute( "DataRate", StringValue( std::to_string(mbps)+"Mbps" ) );			
                        p2p.SetChannelAttribute("Delay", StringValue("5ms"));
          		devices = p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
			std::string IP = std::to_string(lastIP[0])+"."+std::to_string(lastIP[1])+"."+std::to_string(lastIP[2])+"."+std::to_string(lastIP[3]);

			addresses.SetBase (Ipv4Address (IP.c_str()), Ipv4Mask("255.255.255.252"));
  			addresses.Assign (devices);

                        lastIP[3]+=4;
                        if(lastIP[3]>=256){
                           lastIP[3] = 4;
                           lastIP[2]+=1;
                        }

                        node_ip_pair.first = std::stoi(netParams[0]);
                        node_ip_pair.second = GetIPFromSubnet(netParams[0], IP, 1);
                        node_ip_map.push_back(node_ip_pair);


                      //  if(!HasIP(std::stoi(netParams[1]))){ 
			node_ip_pair.first = std::stoi(netParams[1]);
                        node_ip_pair.second = GetIPFromSubnet(netParams[1], IP, 2);
                        node_ip_map.push_back(node_ip_pair);
                    //    }
		//	if(!HasIP(std::stoi(netParams[0]))){
                  //      }

                }
		else if(nonRH == true) {
			//Install apps on PDCs and PMUs for data exchange
			netParams = SplitString(strLine);

			//Install non Router Helpers
			//TODO: add in attributes
                	blancHelper.SetAttribute ("PacketSize", UintegerValue (200));
                	blancHelper.SetAttribute ("RouterHelper", UintegerValue (0));
			blancHelper.SetAttribute ("Name", StringValue(netParams[0]));
                	blancApps = blancHelper.Install (nodes.Get (std::stoi(netParams[0])));
			auto apps = nodes.Get( std::stoi( netParams[0] ) )->GetApplication( 0 )->GetObject<ns3::BLANCpp>();
                        sync.addSender( std::stoi( netParams[0] ),apps );


			std::string strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnFindReply";
			Config::ConnectWithoutContext( strcallback, MakeCallback( &onFindReply ) );
			strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnPay";
                        Config::ConnectWithoutContext( strcallback, MakeCallback( &onPay ) );

		}
		else if(RH == true) {
                        //Install apps on WACs and PMUs for data exchange
                        netParams = SplitString(strLine);

                        blancHelper.SetAttribute ("PacketSize", UintegerValue (200));
			blancHelper.SetAttribute ("RouterHelper", UintegerValue (1));
			blancHelper.SetAttribute ("Name", StringValue(netParams[0]));

                        blancApps = blancHelper.Install (nodes.Get (std::stoi(netParams[0])));
			auto apps = nodes.Get( std::stoi( netParams[0] ) )->GetApplication( 0 )->GetObject<ns3::BLANCpp>();
			sync.addSender( std::stoi( netParams[0] ),apps );


			std::string strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnFindReply";
                        Config::ConnectWithoutContext( strcallback, MakeCallback( &onFindReply ) );
			strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnHold";
                        Config::ConnectWithoutContext( strcallback, MakeCallback( &onHold ) );
			strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/OnPay";
                        Config::ConnectWithoutContext( strcallback, MakeCallback( &onPay ) );
                }
		else if(RHTable == true) {
		   netParams = SplitString(strLine);
		   sync.setFindTable(std::stoi(netParams[0]), netParams[2], netParams[1]);
		}
		else if(costTable == true) {
		   netParams = SplitString(strLine);
                   sync.setPathWeight(std::stoi(netParams[0]), netParams[1], std::stod(netParams[2]) );
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
     sync.setAddressTable(std::to_string(i), Ipv4Address(GetIP(i).c_str()) );
  }


  //Populate the routing table
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();


  //Open trace file for writing
  tracefile.open("tcp-case39cyber-trace.csv", std::ios::out);
  tracefile << "nodeid, event, name, payloadsize, time" << std::endl;

  std::string strcallback;

  sync.beginSync();


  //Run actual simulation
  Simulator::Stop (Seconds(10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


void onFindReply(uint32_t node,  uint32_t txid, double amount){
   sync.onFindReplyPacket(node, txid, amount);
};

void onHold(uint32_t node,  uint32_t txid, bool received){
   sync.onHoldPacket(node, txid, received);
};

void onPay(uint32_t node,  uint32_t txid){
   sync.onPayPacket(node, txid);	
};



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
