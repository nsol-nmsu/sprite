#ifndef BLANC_O
#define BLANC_O

#include <unordered_map>
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
class Blanc : public Application
{
public:

  typedef void (* ReceivedPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, 
		  uint32_t localport, uint32_t packetSize, uint32_t subscription, Ipv4Address localip);
  typedef void (* SentPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport);

  typedef void (* OnFindReplyTraceCallback) (uint32_t nodeid, uint32_t txID, double amount);
  typedef void (* OnHoldTraceCallback) (uint32_t nodeid, uint32_t txID, bool received);
  typedef void (* OnPayTraceCallback) (uint32_t nodeid, uint32_t txID);
  typedef void (* OnPayPathTraceCallback) (uint32_t nodeid, uint32_t txID);
  typedef void (* OnTxTraceCallback) (std::string nodeid, uint32_t txID, bool payer, double amount);
  typedef void (* OnFindTraceCallback) (std::string nodeid, uint32_t txID, bool payer);
  typedef void (* OnPathUpdateTraceCallback) (std::string nodeid1, std::string nodeid2, double amount);
  typedef void (* OnTxFailTraceCallback) (uint32_t txID,uint32_t node1,uint32_t node2);
  typedef void (* OnTxRetryTraceCallback) (uint32_t txID);
  typedef void (* OnAdTraceCallback) (std::string nodeid);
  
   struct RoutingEntry {            // Structure declaration
      std::string nextHop;
      double sendMax;
      double recvMax;
      int hopCount;
      double expireTime;
      RoutingEntry() :
         nextHop(""),
         sendMax(0),
         recvMax(0),
         hopCount(0),
         expireTime(0) {}
   }; 

   struct TransactionInfo {             // Structure declaration
      uint32_t nextTxID;         // Member (int variable)
      std::string nextHop;   // Member (string variable)
      std::string nextDest;
      double pending;
      Ptr<Socket> src;
      std::string dest;
      TransactionInfo* overlap;
      TransactionInfo() :
         nextTxID(0),
         nextHop(""),
         nextDest(""),
         pending(0),
         src(NULL),
         dest(""),  
         overlap(NULL)
         {}
   };    

  static TypeId GetTypeId (void);
  Blanc ();
  virtual ~Blanc ();
  
  //Incoming Packet handling 
  enum PacketType { 
      Find =      0, 
      FindRecv =  1,
      FindReply = 2,
      Hold =      3,
      HoldRecv =  4,
      Pay =       5,
      Advert =    6,
      Reg =       7
  };

  void processPacket(Ptr<Packet> p, Ptr<Socket> s);

  void onHoldPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s);

  void onHoldRecvPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s);

  void onFindPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s);

  void onFindReply(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s);

  void onPayPacket(Ptr<Packet> p, blancHeader ph);
  

  //Transaction Functions
  void startTransaction(uint32_t txID, uint32_t secret, std::vector<std::string> peerlist, bool payer);

  void findFailed(uint32_t txID, uint32_t node1){
   reset(txID);
   m_onTxFail(txID, node1, std::stoi(m_name));
  };

  void reset(uint32_t txID);

  uint32_t createTxID(uint32_t txID);
  
  void AdvertSetup(){};

  void sendFindPacket(uint32_t txID, uint32_t secret);

  void sendHoldPacket(uint32_t txID, double amount);

  void sendPayPacket(uint32_t txID);

  void sendFindReply(uint32_t txID, uint32_t secret, double amount);

   std::string getName(){
   return m_name;
  };
  //Routing Functions
  void sendRoutingInfo();

  void setFindTable(std::string RH, std::string nextHop){
     findRHTable[RH] = nextHop;
  };

   void clearTxIDFind(uint32_t txID){
     txidTable.erase(txID);
  };

  //See if tansaction has ben seen before, or if packet has had a corresponding hold or not. 
  TransactionInfo* checkTransaction(uint32_t txID, uint32_t oldTxID, std::string dest){
   TransactionInfo* trans = &txidTable[txID];
   bool found = false;
   while (trans != NULL ) {
      if ((dest == trans->nextDest && oldTxID == trans->nextTxID) || dest ==  m_name)
         found = true;
      trans = trans->overlap;
   }
   if(found ) {
      if (hasHoldRecv(oldTxID)) {
         updateBCHold(txID);
         m_onHold(std::stoi(m_name), txID, false);	
      } 
      return NULL;
   }
   return &txidTable[txID];
  }

  //Utility Functions
  bool hasHoldRecv(uint32_t txID);

  void insertTimeout(uint32_t txID, std::string src, uint64_t timeout){};

  double Sign(){
   return sign(generator)/1000.0;
  };

  double Verify(){
   return verify(generator)/1000.0;
  };

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


  //BLANCpp Atributes
  bool m_route_helper;
  std::string m_name;
  std::string nextRH;
  uint32_t m_txID = 10000000000000000000;
  int m_hopMax;  
  std::unordered_map<std::string, std::string> findRHTable;
  std::unordered_map<std::string, std::vector<RoutingEntry>> RoutingTable;

  std::unordered_map<uint32_t, TransactionInfo> txidTable; 
  std::unordered_map<uint32_t, std::list<TransactionInfo>> overlapTable; 
  std::unordered_map<std::string, Neighbor>  neighborTable; 

  //Transaction Pair atributes
  bool TiP = false;
  double lastSent = 0;
  double sendNum = 0;


  //Callbacks
  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t, uint32_t, Ipv4Address> m_receivedPacket;
  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t> m_sentPacket;
  TracedCallback<uint32_t, uint32_t, double> m_onFindReply;
  TracedCallback<uint32_t, uint32_t, bool> m_onHold;
  TracedCallback<uint32_t, uint32_t> m_onPay;
  TracedCallback<uint32_t, uint32_t> m_onPayPath;
  TracedCallback<std::string> m_onAd;
  TracedCallback<std::string, uint32_t, bool> m_onFind;
  TracedCallback<std::string, uint32_t, bool, double> m_onTx;
  TracedCallback<std::string, std::string, double> m_onPathUpdate;
  TracedCallback<uint32_t,uint32_t,uint32_t> m_onTxFail;
  TracedCallback<uint32_t> m_onTxRetry;
  std::default_random_engine generator;
  std::normal_distribution<double> keyGen = std::normal_distribution<double>(4.046,1.461831891);
  std::normal_distribution<double> sign = std::normal_distribution<double>(2.056333333,0.248690915);
  std::normal_distribution<double> verify = std::normal_distribution<double>(2.164,0.823274245);
};

} // namespace ns3

#endif /* ICENS_TCP_SERVER */

