/* -*- Mode:C  ; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 
 #include "ns3/log.h"
 #include "ns3/ipv4-address.h"
 #include "ns3/ipv6-address.h"
 #include "ns3/address-utils.h"
 #include "ns3/nstime.h"
 #include "ns3/inet-socket-address.h"
 #include "ns3/inet6-socket-address.h"
 #include "ns3/simulator.h"
 #include "ns3/socket-factory.h"
 #include "ns3/uinteger.h"
 #include "ns3/string.h"
 #include "ns3/trace-source-accessor.h"
 #include "ns3/ipv4.h"
 #include "BLANC.hpp" 
 #define HOPMAX 6
 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("ns3.BLANC");
 NS_OBJECT_ENSURE_REGISTERED (Blanc);
 
 TypeId
 Blanc::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::BLANC")
     .SetParent<Application> ()
     .AddConstructor<Blanc> ()
     .AddAttribute ("Port", "Port on which we listen for incoming connections.",
                    UintegerValue (7),
                    MakeUintegerAccessor (&Blanc::m_local_port),
                    MakeUintegerChecker<uint16_t> ())
     .AddAttribute ("PacketSize",
                   "The size of outbound packet, typically acknowledgement packets from server application. 536 not fragmented on 1500 MTU",
                   UintegerValue (536),
                   MakeUintegerAccessor (&Blanc::m_packet_size),
		   MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("RouterHelper",
                   "True if application will run in Router helper mode.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Blanc::m_route_helper),
                   MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("Name",
                   "Name of node.",
                   StringValue ("0"),
                   MakeStringAccessor (&Blanc::m_name),
                   MakeStringChecker())     
     .AddAttribute ("HopMax",
                   "Hop max.",
                   UintegerValue (6),
                   MakeUintegerAccessor (&Blanc::m_hopMax),
                   MakeUintegerChecker<uint32_t> ())                     
     .AddTraceSource ("ReceivedPacketS",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&Blanc::m_receivedPacket),
                     "ns3::Blanc::ReceivedPacketTraceCallback")
     .AddTraceSource ("SentPacketS",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&Blanc::m_sentPacket),
                     "ns3::Blanc::SentPacketTraceCallback")
     .AddTraceSource ("OnFindReply",
                     "A findReply packet has been received",
                     MakeTraceSourceAccessor (&Blanc::m_onFindReply),
                     "ns3::Blanc::OnFindReplyTraceCallback")
     .AddTraceSource ("OnHold",
                     "A hold packet has been recived",
                     MakeTraceSourceAccessor (&Blanc::m_onHold),
                     "ns3::Blanc::OnHoldTraceCallback")
     .AddTraceSource ("OnPay",
                     "A pay packet has been received",
                     MakeTraceSourceAccessor (&Blanc::m_onPay),
                     "ns3::Blanc::OnPayTraceCallback")
     .AddTraceSource ("OnPayPath",
                     "A pay packet has been received",                     
                     MakeTraceSourceAccessor (&Blanc::m_onPayPath),
                     "ns3::BLANCpp::OnPayPathTraceCallback")                     
     .AddTraceSource ("OnTx",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Blanc::m_onTx),
                     "ns3::BLANCpp::OnTxTraceCallback")  
     .AddTraceSource ("OnFind",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Blanc::m_onFind),
                     "ns3::BLANCpp::OnTxTraceCallback")                       
     .AddTraceSource ("OnTxFail",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Blanc::m_onTxFail),
                     "ns3::BLANCpp::OnTxFailTraceCallback")   
     .AddTraceSource ("OnPathUpdate",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Blanc::m_onPathUpdate),
                     "ns3::BLANCpp::OnPathUpdateTraceCallback") 
     .AddTraceSource ("OnTxRetry",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Blanc::m_onTxRetry),
                     "ns3::BLANCpp::OnTxRetryTraceCallback")    
     .AddTraceSource ("OnAd",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Blanc::m_onAd),
                     "ns3::BLANCpp::OnAdTraceCallback")                         
   ;
   return tid;
 }
 
 
Blanc::Blanc ()
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
   m_running = false;
 }
 
Blanc::~Blanc()
{
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
}

//SECTION: Incoming Packet handling 
void 
Blanc::processPacket(Ptr<Packet> p, Ptr<Socket> s){
   if (p->GetSize() == 0) return;
   blancHeader packetHeader;
   p->RemoveHeader(packetHeader);
   uint32_t pType = packetHeader.GetPacketType();
   uint32_t extraSize = p->GetSize() - packetHeader.GetPayloadSize();
   Ptr<Packet> pkt  = p->Copy();
   p->RemoveAtEnd(extraSize);
   m_onAd(m_name);

   switch(pType){
      case Find:
      case FindRecv:
      case 10:
         onFindPacket(p, packetHeader, s);
         break;
      case FindReply:
	      onFindReply(p, packetHeader, s);
         break;
      case Hold:
         onHoldPacket(p, packetHeader, s);
         break;
      case HoldRecv:
         onHoldRecvPacket(p, packetHeader, s);
         break;
      case Pay:
         onPayPacket(p, packetHeader);
         break;
      case Reg:
         onRegPacket(p, packetHeader, s);
         break;           
      default:
         break;
   }
     if (extraSize > 0){
      pkt->RemoveAtStart( packetHeader.GetPayloadSize() );
      processPacket(pkt, s);
   }
}

void 
Blanc::onHoldPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 {

   uint8_t buffer[p->GetSize()];
   p->CopyData (buffer,  p->GetSize());

   std::ostringstream convert;
   for (int a = 0; a < p->GetSize(); a++) {
      convert << (char)buffer[a];
   }


   std::string payload = convert.str();

   std::vector<std::string> items = SplitString(payload, '|');
   if(items.size() == 0) return;

   uint32_t txID = std::stoul(items[0]);
   uint32_t oldTxID = std::stoul(items[1]);
   std::string dest = txidTable[oldTxID].dest;
   double amount = std::stod(items[2]);
   uint64_t timeout = std::stoi(items[4]);

   if( false){
      std::cout <<"Hold: "<< m_name <<" NodeID "<<std::endl;
      std::cout << txID <<"  TXiD "<<std::endl;
      std::cout << oldTxID <<" TXiDPrime "<<std::endl;   
      std::cout << dest <<std::endl;
      std::cout << amount <<"  amount\n"<<std::endl;
   }

   //Get all values from packet
   std::string src = "";//REPLACE	 
   std::string nextHop = "";

   if(m_route_helper && m_name == dest && txidTable[oldTxID].nextDest == "") {
      m_onHold(std::stoi(m_name), txID, false);	
      return;
   }
   TransactionInfo* entry = NULL;
   if ( txidTable.find(txID) != txidTable.end() && txidTable[txID].pending != 0) {
      if (txidTable.find(oldTxID) == txidTable.end()) 
         return;
      if  ( overlapTable.find(txID) == overlapTable.end() ){
         overlapTable[txID];
         overlapTable[txID].push_back(txidTable[txID]);
      }   
      overlapTable[txID].push_front(txidTable[oldTxID]);
      entry =  &overlapTable[txID].front();
   }
   else entry = &txidTable[txID];

   uint32_t nextTxID = oldTxID;

   //Create next hold packet
   //Update Transaction Table
   nextHop = txidTable[oldTxID].nextHop;
   (*entry) = txidTable[oldTxID];

   if(m_route_helper && m_name == dest){   
      //Update next hop and Router Helper Destination
      nextTxID = txidTable[oldTxID].nextTxID;
      nextHop = entry->nextHop;
      dest = entry->nextDest;

      //Send out BC message, affirming hold.
      updateBCHold(txID);
   }

   txidTable.erase(oldTxID);
   entry->pending = amount;
   entry->nextHop;

   blancHeader packetHeader;
   packetHeader.SetPacketType(Hold);

   payload = std::to_string(txID) + "|";
   payload += std::to_string(nextTxID) + "|";
   payload += std::to_string(amount) + "|";
   payload += "en|";
   payload += "5";
   
   updatePathWeight(nextHop, amount);
   updatePathWeight(findSource(s), -1*amount);
   insertTimeout(txID, nextHop, timeout);

   //Forward next hold packet
   double delay = Sign() + Verify();
   //Simulator::Schedule(Seconds(delay), &Blanc::forwardPacket, this, packetHeader, nextHop, payload);

   forwardPacket(packetHeader, nextHop, payload, delay);
   //std::cout<<"sent "<<nextHop<<std::endl;
   

   //Send out BC message, all only do in case of timeout. 
   //updateBCHold(txID);
   
 }

void 
Blanc::onFindPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s){

   //Get all values from packet
   std::string src = "";//REPLACE
   uint32_t type = ph.GetPacketType();
   std::string nextHop = "";

   uint8_t buffer[p->GetSize()];   
   p->CopyData (buffer,  p->GetSize());

   std::ostringstream convert;
   for (int a = 0; a < p->GetSize(); a++) {
      convert << (char)buffer[a];
   }

   std::string payload = convert.str();
   //std::cout<<"Payload:"<<payload<<"s"<<std::endl;

   std::vector<std::string> items = SplitString(payload, '|');
   if(items.size() == 0) return;

   uint32_t txID = std::stoul(items[0]);
   if ( txidTable.find(txID) != txidTable.end() ){
      return;
   }	       
   std::string path = items[1];
   std::string dest = SplitString(path, ',')[0];
   txidTable[txID].dest = dest;
   double amount = std::stod(items[2]);
   uint32_t secret = std::stoi(items[3]);
   int hopsLeft = std::stoi(items[4]);
   uint64_t timeout = 5;
   Simulator::Schedule(Seconds(2), &Blanc::clearTxIDFind, this, txID);

   if (false){
      std::cout << "Find........\n\n\nNode "<<m_name<<std::endl;
   std::cout << txID <<"  TXiD "<<std::endl;
   std::cout << path <<"  dest "<<std::endl;
   std::cout << amount <<"  amount "<<std::endl;
   std::cout << secret <<"  secret "<<std::endl;
   std::cout << hopsLeft <<"  hopsLeft\n"<<std::endl;
   }
   txidTable[txID].src = s;
   std::string source = findSource(s);
   
   if(m_name != dest){
      //nextHop = findRHTable[dest];
      //txidTable[txID].nextHop = nextHop;
   }    
   //m_onAd(m_name);
   insertTimeout(txID, src, timeout);
   //Create next find packet
   //Ptr<Packet> packet_copy = Create<Packet> (m_packet_size);

   bool doNotSend = false;

   if(m_route_helper && m_name == dest){

      if ( txidTable.find(secret) != txidTable.end() ){
         txidTable[txID].nextTxID = txidTable[secret].nextTxID;
	      txidTable[ txidTable[secret].nextTxID ].nextTxID = txID;
	      txidTable.erase(secret);
      }	    
      else {
         txidTable[secret].nextTxID = txID;
      }

      if (type == Find){

         //Update Transaction Table
         uint32_t nextTxID = createTxID(txID);
   	   txidTable[txID].nextTxID = nextTxID;
    	   //txID = nextTxID;
   	   type = 10;
   	   //Update next hop and Router Helper Destination  
	      dest = SplitString(path, ',')[1];
	      //nextHop = findRHTable[dest];
         //txidTable[txID].nextHop = nextHop;
         txidTable[txID].nextDest = dest;
         txidTable[nextTxID].dest = dest;
         hopsLeft = m_hopMax;

         txID = nextTxID;
	      path = dest;
      }
      else {
         //Send out BC message, affirming hold. TODO: Confirm if all do this or only RHs.
         updateBCFind(txID);	     
	      if (type == 10){
	         sendFindReply(txID, secret, amount);		 
	      }
	      else{
            sendFindReply(txID, secret, amount);		 
	      }
         return;
      }
   }
   else if(m_route_helper ){

      doNotSend = true;
      //return;
   }

   //Send out BC message, affirming hold. TODO: Confirm if all do this or only RHs.
   updateBCFind(txID);
   if (hopsLeft == 0) return;

  //TODO: Remove shortcut
  if( neighborTable.find(dest) !=  neighborTable.end() ) {
      double sendAmount = amount;
      blancHeader packetHeader;
      packetHeader.SetPacketType(type);

      payload = std::to_string(txID) + "|";
      payload += path;
      payload += "|";

      if(sendAmount > neighborTable[dest].cost) sendAmount = neighborTable[dest].cost;

      payload += std::to_string(sendAmount) + "|";
      payload += std::to_string(secret) + "|";
      payload += std::to_string(hopsLeft-1);
      
      forwardPacket(packetHeader, dest, payload, 0);
      return;
  }

   for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      double sendAmount = amount;
      blancHeader packetHeader;
      packetHeader.SetPacketType(type);

      payload = std::to_string(txID) + "|";
      payload += path;
      payload += "|";

      std::string n = it->first;
      if(SplitString(n, '|').size()>1) {
         n = SplitString(n, '|')[0];
      }

      if(it->second.cost == 0 || n == source) continue;

      if(sendAmount > it->second.cost) sendAmount = it->second.cost;

      payload += std::to_string(sendAmount) + "|";

      payload += std::to_string(secret) + "|";
      payload += std::to_string(hopsLeft-1);
      forwardPacket(packetHeader, it->first, payload, 0);
   }
}

void 
Blanc::onHoldRecvPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 { 

   uint8_t buffer[p->GetSize()];
   p->CopyData (buffer,  p->GetSize());

   std::ostringstream convert;
   for (int a = 0; a < p->GetSize(); a++) {
      convert << (char)buffer[a];
   }


   std::string payload = convert.str();

   std::vector<std::string> items = SplitString(payload, '|');
   if(items.size() == 0) return;

   uint32_t txID = std::stoul(items[0]);
   uint32_t oldTxID = std::stoul(items[1]);
   std::string dest = txidTable[oldTxID].dest;
   double amount = std::stod(items[2]);
   uint64_t timeout = std::stoi(items[4]);

   if ( false){

   std::cout <<"Hold Recv:  "<<m_name<<" NodeID "<<std::endl;
   std::cout << txID <<"  TXiD "<<std::endl;
   std::cout << oldTxID <<" TXiDPrime "<<std::endl;
   std::cout << dest << " Dest\n";

   std::cout << amount <<"  amount\n"<<std::endl;
   }

   //Get all values from packet
   std::string src = "";//REPLACE
   std::string nextHop = "";

   if ( txidTable.find(txID) != txidTable.end() ){//&& txidTable[txID].pending != 0) {
      if (txidTable.find(oldTxID) == txidTable.end()){
         return;
      }
      if  ( overlapTable.find(txID) == overlapTable.end() ){
         overlapTable[txID];
         overlapTable[txID].push_back(txidTable[txID]);
      }
   }
   if  ( overlapTable.find(txID) == overlapTable.end() ){
      overlapTable[txID];
   }

   if(m_name != dest){
      nextHop = txidTable[oldTxID].nextHop;
      txidTable.erase(oldTxID);
   }

   txidTable[txID].src = s;
   txidTable[txID].dest = dest;

   txidTable[txID].pending = amount;

   //Create next hold packet
   if(!m_route_helper || m_name != dest){

      blancHeader packetHeader;
      packetHeader.SetPacketType(HoldRecv);

      payload = std::to_string(txID) + "|";
      payload += std::to_string(oldTxID) + "|";
      payload += std::to_string(amount) + "|";
      payload += "en|";
      payload += "5";
      
      updatePathWeight(nextHop, -1*amount);
      updatePathWeight(findSource(s), amount);

      insertTimeout(txID, nextHop, timeout);

      //Forward next hold packet
      double delay = Sign() + Verify();
      //Simulator::Schedule(Seconds(delay), &Blanc::forwardPacket, this, packetHeader, nextHop, payload);      
      forwardPacket(packetHeader, nextHop, payload, delay);
      sendNum +=1;
   }
   else if (m_route_helper && m_name == dest){
      //Send out BC message, affirming hold. 
      updateBCHoldRecv(txID);
      m_onHold(std::stoi(m_name), txID, true);
   }
   sendNum = 0;
 }

void 
Blanc::onFindReply(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 {

   uint8_t buffer[p->GetSize()];
   p->CopyData (buffer,  p->GetSize());

   std::ostringstream convert;
   for (int a = 0; a < p->GetSize(); a++) {
      convert << (char)buffer[a];
   }


   std::string payload = convert.str();

   std::vector<std::string> items = SplitString(payload, '|');
   if(items.size() == 0) return;

   uint32_t txID = std::stoul(items[0]);
   double amount = std::stod(items[1]);
   uint32_t secret = std::stoi(items[2]); 
   std::string src = "";//REPLACE
   //m_onAd(m_name);


   std::string nextHop;
   /*for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      if (it->second.socket == s)
         nextHop = it->first; 
   }*/
   nextHop = findSource(s);
   if (false){
      std::cout << "Find reply "<< m_name  <<"\n"<<txID <<"  TXiD "<<std::endl;
      std::cout << amount <<"  amount"<<std::endl;
      std::cout << secret <<"  Secret"<<std::endl;
      std::cout << nextHop <<"  Next Hop\n"<<std::endl;
   }   

   blancHeader packetHeader;
   packetHeader.SetPacketType(FindReply);
   txidTable[txID].nextHop = nextHop;
   if (txidTable[txID].src != NULL ){
      //Forward next pay packet
      forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
      txidTable[txID].src = NULL;
   }
   else if (m_route_helper){
      for (auto i = txidTable.begin(); i != txidTable.end(); i++){
         //std::cout<<"One of these should be it "<<txID<<"  "<<i->second.nextTxID<<std::endl;
         if (i->second.nextTxID == txID){
            sendFindReply(i->first, secret, amount);
            txidTable[i->first].nextHop = nextHop;
            //txidTable.erase(txID);
	         return;
	      }
      }
   }
   else {
      //Update next hop and Router Helper Destination
      m_onFindReply(std::stoi(m_name), secret, amount);
   }
}

bool
Blanc::hasHoldRecv(uint32_t txID)
{
   uint32_t txIDprime = txidTable[txID].nextTxID;
   if ( txidTable.find(txIDprime) != txidTable.end() ){
      return true;
   }
   return false;
}

//Transaction Functions
void 
Blanc::startTransaction(uint32_t txID, uint32_t secret, std::vector<std::string> peerlist, bool payer)
{
   //Determine number of segments and create the needed txIDs.
   //std::string NH = findRHTable[peerlist[0]];
   m_amount = 10000;//neighborTable[NH].cost;
   m_payer = payer;
   txidTable[txID].nextTxID = createTxID(txID);
   if(!payer) txidTable[txID].nextTxID += 20000;
   m_txID = txidTable[txID].nextTxID;
   txidTable[txID].nextDest = peerlist[0];
   if (payer) nextRH = peerlist[1];
   sendFindPacket(txID, secret);
   TiP = true;
   std::string path = m_name+"|" +peerlist[0];
   if (payer) {
      path += "|" +peerlist[1]; 
   }
   m_onFind(path, txID, payer);
}

void
Blanc::reset(uint32_t txID)
{
   //Determine number of segments and create the needed txIDs.
   m_amount = 0;
   m_payer = false;
   txidTable.erase(txID);
   m_txID = 0;
   nextRH = "";
   TiP = false;
}

uint32_t
Blanc::createTxID(uint32_t txID){
   return (txID+10000);
}

void 
Blanc::sendFindReply(uint32_t txID, uint32_t secret, double amount)
{
   blancHeader packetHeader;
   packetHeader.SetPacketType(FindReply);

   std::string payload = std::to_string(txID) + "|";
   payload += std::to_string(amount) + "|";
   payload += std::to_string(secret);

   forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
   txidTable[txID].src = NULL;
}

void
Blanc::sendFindPacket(uint32_t txID, uint32_t secret)
{
   uint32_t txIDPrime = txidTable[txID].nextTxID;
   std::string dest = txidTable[txID].nextDest;
   std::string nextHop = findRHTable[dest]; //Update based on Some table base
   //txidTable[txID].nextHop = nextHop;
   txidTable[txIDPrime];
   
   int hopMax = m_hopMax;
   for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      blancHeader packetHeader;
      packetHeader.SetPacketType(FindRecv);
      if(m_payer){
         packetHeader.SetPacketType(Find);
      }
      if(it->second.cost == 0) continue;
   if ( false) std::cout<<"Node "<<m_name<<" "<< it->first << " " <<dest<<" "<<it->second.cost<<" sending out find packet\n";

      std::string payload = std::to_string(txIDPrime) + "|";
      payload += dest;
      if(m_payer)
         payload += "," + nextRH;
      payload += "|";
      payload += std::to_string(it->second.cost) + "|";
      payload += std::to_string(secret) + "|";
      payload += std::to_string(hopMax);

      forwardPacket(packetHeader, it->first, payload, 0);
   }
}

void 
Blanc::sendHoldPacket(uint32_t txID, double amount)
{
   uint32_t txIDPrime = txidTable[txID].nextTxID;
   txidTable[txID].nextHop = txidTable[txIDPrime].nextHop;
   if (false ) std::cout<<"Node "<< m_name <<" TxID "<<txID<<" NDest " <<txidTable[txID].nextDest<<" NH "<< txidTable[txIDPrime].nextHop << " Let's see the issue.\n";
   std::string dest = txidTable[txID].nextDest;
   std::string nextHop = txidTable[txIDPrime].nextHop;
   txidTable.erase(txIDPrime);

   m_amount = amount;

   blancHeader packetHeader;
   if(m_payer){
      packetHeader.SetPacketType(Hold);
      m_onTx(m_name, txID, true, amount);
      updatePathWeight(nextHop, amount);
   }
   else { 
      packetHeader.SetPacketType(HoldRecv);
      txidTable[txID].pending = amount;
      m_txID = txID;
      updatePathWeight(nextHop, -1*amount);
   }

   std::string payload = std::to_string(txID) + "|";
   payload += std::to_string(txIDPrime) + "|";
   payload += std::to_string(amount) + "|";
   payload += "ENC|";
   payload += "5";

   int timeout = 5;
   //updatePathWeight(nextHop, amount);
   insertTimeout(txID, nextHop, timeout);

   forwardPacket(packetHeader, nextHop, payload, 0);
   //std::cout<<"Hold phase "<<nextHop<<"\n\n\n";
   Simulator::Schedule(Seconds(1.5), &Blanc::checkFail, this, txID);

}

void 
Blanc::sendPayPacket(uint32_t txID)
{

   std::string nextHop = txidTable[txID].nextHop;
   txidTable.erase(txID);
   if (false) std::cout<<"Pay phase NH "<<nextHop <<" TxID " << txID <<"\n\n\n";

   blancHeader packetHeader;
   packetHeader.SetPacketType(Pay);
   std::string payload = std::to_string(txID) + "|";
   payload += std::to_string(m_amount) + "|";
   payload += "5";

   forwardPacket(packetHeader, nextHop, payload, 0);
}

} // Namespace ns3


