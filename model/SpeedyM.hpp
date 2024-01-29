/* -*- Mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2012
 *
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

#ifndef SPEEDY
#define SPEEDY

#include <unordered_map>
#include <unordered_set>
#include <random>

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/blanc-header.h"
#include "ns3/socket.h"
#include "ns3/packet.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications
 * \defgroup TcpEcho 
 */

/**
 * \ingroup tcpecho
 * \brief A Tcp Echo server
 *
 * Every packet received is sent back to the client.
 */
class SpeedyM : public Application
{
public:

  typedef void (* ReceivedPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, 
		  uint32_t localport, uint32_t packetSize, uint32_t subscription, Ipv4Address localip);
  typedef void (* SentPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport);

  typedef void (* OnFindReplyTraceCallback) (uint32_t nodeid, uint32_t txID, double amount);
  typedef void (* OnHoldTraceCallback) (uint32_t nodeid, uint32_t txID, bool received);
  typedef void (* OnPayTraceCallback) (uint32_t nodeid, uint32_t txID);
  typedef void (* OnPayPathTraceCallback) (uint32_t nodeid, std::string txID);
  typedef void (* OnTxTraceCallback) (std::string nodeid, uint32_t txID, bool payer, double amount);
  typedef void (* OnPathUpdateTraceCallback) (std::string nodeid1, std::string nodeid2, double amount);
  typedef void (* OnTxFailTraceCallback) (uint32_t txID);
  typedef void (* OnTxRetryTraceCallback) (uint32_t txID);
  typedef void (* OnAdTraceCallback) (std::string nodeid, std::string treeId);

  
   struct TreeEntry {            // Structure declaration
      std::string parent; //Structure: Name|coordinate
      std::vector<std::string> neighbors; //Structure: Name|coordinate
      std::string coordinate;
      TreeEntry() :
         parent({}),
         neighbors({}), 
         coordinate("") {}
   };   

   struct TransactionInfo {             // Structure declaration
      double pending;
      Ptr<Socket> src;
      std::string nextHop;
      bool replied;
      TransactionInfo() :
         pending(0.0),
         src(NULL),
         nextHop(""),
         replied(false){}
   };    

  static TypeId GetTypeId (void);
  SpeedyM ();
  virtual ~SpeedyM ();
  
  //Incoming Packet handling 
  enum PacketType { 
      PayRoute =     5,
      Advert =       6,
      Reg =          7,
      RouteReply = 9, 
      Pay =          10,
      Nack =         11,
      parentUpdate = 12
  };

  void processPacket(Ptr<Packet> p, Ptr<Socket> s);

  void onRoutePayPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s);

  void onRoutePayReply(Ptr<Packet> p, blancHeader ph);
    
  void onNack(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s);

  void onAdvertPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s);

  void onParentUpdate(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s);

  //Transaction Functions
  void startTransaction(uint32_t txID, std::vector<std::string> addresslist, double amount);

  void reset(uint32_t txID) overide;
  
  double KeyGen(){
   return keyGen(generator)/1000.0;
  };

  double Sign(){
   return sign(generator)/1000.0;
  };

  double Verify(){
   return verify(generator)/1000.0;
  };

  //Packet fowarding
  void sendAdvertPacket();

  void 
  sendRegPacket(std::string dest);

  void 
  sendRoutePayPacket(std::string txID, std::string treeID, std::string address, double amount);

  void 
  sendRoutePayReply(std::string txID, bool complete);
 
  void
  sendPayments(std::string txID, int m_treeCount);

  void sendParentUpdate(std::string treeID, std::string address);

  void
  onPay(Ptr<Packet> p, blancHeader ph, Ptr<Socket> );

  void 
  sendNack(std::string txID, double delay);

  //Utility Functions

  void 
  insertTimeout(uint32_t txID, std::string src, std::string payload, double te1, bool sender){} overide;

  void 
  checkTimeout();

  std::vector<std::string>
  getAddressList(){
   std::vector<std::string> a_list = {};
   for(auto i = TreeTable.begin(); i != TreeTable.end(); i++){
      a_list.push_back(i->first+"|"+i->second.coordinate);
   }
   return a_list;
  };

  uint32_t 
  getHighestAmount(std::string dest);

  std::string 
  findNextHop(std::string dest, std::string treeID, double amount, std::string routeList);

   //TODO:: Revist this function for optimization
  std::string 
  createPath(std::string dest, double amount);

  int
  getCommonPrefixsize(std::string dest_coor, std::string cand_coor){
      std::vector<std::string> dest_coor_list = SplitString(dest_coor, ',');
      std::vector<std::string> cand_coor_list = SplitString(cand_coor, ',');

      int i;
      for(i = 0; i < dest_coor_list.size() && i < cand_coor_list.size(); i++){
         if (dest_coor_list[i] != cand_coor_list[i]) break;
      }
      return i;
  }
   
void 
setReceiver(uint32_t txID, double amount);

virtual void StartApplication (void);

private:

  uint16_t m_local_port;
  bool m_running;
  Ipv4Address m_local_ip;
  Ptr<Socket> m_socket;
  uint32_t m_packet_size;
  uint16_t m_peerPort = 5017;
  uint32_t m_seq;
  double m_amount;
  bool m_payer;
  int m_maxRetires = 4;//TODO:: add as varaible


  //SpeedyM Atributes
  bool m_route_helper;
  bool method2 = false;
  std::string m_name;
  std::string nextRH;
  uint32_t m_txID = 10000000000000000000;
  double lastSent = 0;
  int m_successful_routes;
  int m_treeCount;
  bool m_failed = false;
   bool Debug = false;
  std::unordered_map<std::string, TreeEntry> TreeTable;

  std::unordered_map<std::string, TransactionInfo> txidTable; 
  std::unordered_map<std::string, Neighbor>  neighborTable; 
  std::unordered_map<std::string, double>  sendOverlapTable; 

  //Transaction Pair atributes
  bool TiP = false;


  //Callbacks
  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t, uint32_t, Ipv4Address> m_receivedPacket;
  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t> m_sentPacket;
  TracedCallback<uint32_t, uint32_t, double> m_onFindReply;
  TracedCallback<uint32_t, uint32_t, bool> m_onHold;
  TracedCallback<uint32_t, uint32_t> m_onPay;
  TracedCallback<uint32_t, std::string> m_onPayPath;
  TracedCallback<std::string,std::string> m_onAd;
  TracedCallback<std::string, uint32_t, bool, double> m_onTx;
  TracedCallback<std::string, std::string, double> m_onPathUpdate;
  TracedCallback<uint32_t> m_onTxFail;
  TracedCallback<uint32_t> m_onTxRetry;

  std::default_random_engine generator;
  std::normal_distribution<double> keyGen = std::normal_distribution<double>(4.046,1.461831891);
  std::normal_distribution<double> sign = std::normal_distribution<double>(2.056333333,0.248690915);
  std::normal_distribution<double> verify = std::normal_distribution<double>(2.164,0.823274245);


};

} // namespace ns3

#endif /* ICENS_TCP_SERVER */
