#ifndef PCN
#define PCN

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
class PCN_App_Base : public Application
{
public:
  
   struct Neighbor {            // Structure declaration
      Ipv4Address address;
      Ptr<Socket> socket;
      double cost;
      Neighbor() :
         socket(NULL),
         cost(0) {}
   };    

  static TypeId GetTypeId (void);
  PCN_App_Base ();
  virtual ~PCN_App_Base ();

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

  virtual void processPacket(Ptr<Packet> p, Ptr<Socket> s);  

  //Transaction Functions
  bool getTiP(){ return TiP; }; 

  void startTransaction(uint32_t txID, uint32_t secret, std::vector<std::string> peerlist, bool payer);

  void reset(uint32_t txID);
  
  void forwardPacket(blancHeader packetHeader, std::string nextHop){
     forwardPacket(packetHeader,nextHop,"", 0);
  };

  void forwardPacket(blancHeader packetHeader, std::string nextHop, std::string payload, double delay);

  void forwardPacket(blancHeader packetHeader, Ptr<Socket> socket, std::string payload, double delay);  

  //Block Chain Functions
  void updateBCFind(uint32_t txID){};

  void updateBCHold(uint32_t txID){};

  void updateBCHoldRecv(uint32_t txID){};

  void updateBCPay(uint32_t txID){};  

  void updatePathWeight(std::string name, double amount){
      neighborTable[name].cost -= amount;
      m_onPathUpdate(m_name, name, amount);
  };
  
  std::string getName(){
   return m_name;
  };

  void checkFail(uint32_t txID){
      if (TiP && m_txID == txID)
         m_onTxFail(txID, 0, std::stoi(m_name));
  };  

  void setNeighborCredit(std::string name, double amount);

  std::string 
  readPayload(Ptr<Packet> p);

  std::string 
  findSource(Ptr<Socket> s);
  
  void setNeighbor(std::string name, Ipv4Address address);

  void onRegPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s);
  void send(Ptr<Socket> s, Ptr<Packet> p);

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

  void 
  insertTimeout(uint32_t txID, std::string src, uint64_t timeout){};

  Ptr<Socket> 
  getSocket(std::string dest);


  std::vector<std::string> SplitString( std::string strLine, char delimiter ) {
   std::string str = strLine;
   std::vector<std::string> result;
   uint32_t i =0;
   std::string buildStr = "";

   for ( i = 0; i<str.size(); i++) {
      if ( str[i]== delimiter ) {
         result.push_back( buildStr );
	 buildStr = "";
      }
      else {
   	      buildStr += str[i];
      }
   }

   if(buildStr!="")
      result.push_back( buildStr );

   return result;
  };

  virtual void DoDispose (void);

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);
private:

};

} // namespace ns3

#endif /* ICENS_TCP_SERVER */

