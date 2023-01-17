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

#ifndef ICENS_TCP_SERVER
#define ICENS_TCP_SERVER

#include <unordered_map>


#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/blanc-header.h"

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
class BLANCpp : public Application
{
public:

  typedef void (* ReceivedPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, 
		  uint32_t localport, uint32_t packetSize, uint32_t subscription, Ipv4Address localip);
  typedef void (* SentPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport);

  typedef void (* OnFindReplyTraceCallback) (uint32_t nodeid, uint32_t txID, double amount);
  typedef void (* OnHoldTraceCallback) (uint32_t nodeid, uint32_t txID, bool received);
  typedef void (* OnPayTraceCallback) (uint32_t nodeid, uint32_t txID);

  static TypeId GetTypeId (void);
  BLANCpp ();
  virtual ~BLANCpp ();

  /**
   *
   * Receive the packet from client echo on socket level (layer 4), 
   * handle the packet and return to the client.
   *
   * \param socket TCP socket.
   *
   */
  void ReceivePacket(Ptr<Socket> socket);
  
  /**
  *
  * Handle packet from accept connections.
  *
  * \parm s TCP socket.
  * \parm from Address from client echo.
  */
  void HandleAccept (Ptr<Socket> s, const Address& from);
  
  /**
  *
  * Handle successful closing connections.
  *
  * \parm s TCP socket.
  *
  */
  void HandleSuccessClose(Ptr<Socket> s);

  //BLANCpp Function
  void processPacket(Ptr<Packet> p){
     blancHeader packetHeader;
     p->RemoveHeader(packetHeader);
     uint32_t pType = packetHeader.GetPacketType();
	  
     if (pType == 0 || pType == 4  || pType == 10)
        onFindPacket(p, packetHeader);
     if (pType == 1)
        onHoldPacket(p, packetHeader);
     if (pType == 2)
	onHoldRecvPacket(p, packetHeader);
     if (pType == 3)
	onPayPacket(p, packetHeader);
     
  };

  void onHoldPacket(Ptr<Packet> p, blancHeader ph);

  void onHoldRecvPacket(Ptr<Packet> p, blancHeader ph);

  void onFindPacket(Ptr<Packet> p, blancHeader ph);

  void onPayPacket(Ptr<Packet> p, blancHeader ph);


  //Transaction Functions
  bool getTiP(){ return TiP; }; 
  void startTransaction(uint32_t txID, uint32_t secret, std::vector<std::string> peerlist, bool payer);

  void reset(uint32_t txID);

  void sendFindPacket(uint32_t txID, uint32_t secret);

  void sendHoldPacket(uint32_t txID, double amount);

  void sendPayPacket(uint32_t txID);

  void sendFindReply(uint32_t txID){};

  uint32_t
  createTxID(uint32_t txID);

  void forwardPacket(blancHeader packetHeader, std::string nextHop);

  bool hasHoldRecv(uint32_t txID);

  void sendRoutingInfo();

  void updateBCFind(uint32_t txID){};

  void updateBCHold(uint32_t txID){};

  void updateBCHoldRecv(uint32_t txID){};

  void updateBCPay(uint32_t txID){};  

  void updatePathWeight(std::string src, double amount){
     costTable[src] -= amount;
  };

  void insertTimeout(uint32_t txID, std::string src, uint64_t timeout){};

  void setFindTable(std::string RH, std::string nextHop){
     findRHTable[RH] = nextHop;
  };

  void setPathWeight(std::string nextHop, double amount){
     costTable[nextHop] = amount;
  };

  void setAddressTable(std::string name, Ipv4Address address){
     addressTable[name] = address;
  };

  Ptr<Socket> getSocket(std::string dest);


protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);

  uint16_t m_local_port;
  bool m_running;
  uint32_t m_subscription; //!< Subscription value of packet
  Ipv4Address m_local_ip;
  Ptr<Socket> m_socket;
  uint32_t m_packet_size;
  uint16_t m_peerPort = 5017;
  uint32_t m_seq;
  double m_amount;
  bool m_payer;


  //BLANCpp Atributes
  bool m_route_helper;
  std::string m_name;
  std::string nextRH;
  uint32_t m_txID = 0;
  std::unordered_map<std::string, Ipv4Address> addressTable;
  std::unordered_map<std::string, Ptr<Socket>> m_sockets;
  std::unordered_map<std::string, double> costTable;
  std::unordered_map<uint32_t, std::string> nextHopTable; //Given a sub_tx_ID, find the Dest
  std::unordered_map<std::string, std::string> findRHTable;
  std::unordered_map<uint32_t, uint32_t> txIDTable;
  std::unordered_map<uint32_t, std::string> nextRHTable;
  std::unordered_map<uint32_t, bool> pendingTx;

  //Transaction Pair atributes
  bool TiP = false;


  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t, uint32_t, Ipv4Address> m_receivedPacket;
  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t> m_sentPacket;
  TracedCallback<uint32_t, uint32_t, double> m_onFindReply;
  TracedCallback<uint32_t, uint32_t, bool> m_onHold;
  TracedCallback<uint32_t, uint32_t> m_onPay;
  

};

} // namespace ns3

#endif /* ICENS_TCP_SERVER */

