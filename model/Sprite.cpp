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
 #include "Sprite.hpp"
 #define DEBUG1 0
 #define DEBUG2 0
 #define DEBUG3 0
 #define DEBUGNACK 0
 #define TD -1
 #define BALANCE 1

 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("ns3.Sprite");
 NS_OBJECT_ENSURE_REGISTERED (Sprite);
 
 TypeId
 Sprite::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::Sprite")
     .SetParent<PCN_App_Base> ()
     .AddConstructor<Sprite> ()
     .AddAttribute ("RouterHelper",
                   "True if application will run in Router helper mode.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Sprite::m_route_helper),
                   MakeUintegerChecker<uint32_t> ()) 
     .AddAttribute ("Method2",
                   "Bool.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&Sprite::method2),
                   MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("HopMax",
                   "Hop max.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&Sprite::m_hopMax),
                   MakeUintegerChecker<uint32_t> ())                   
     .AddTraceSource ("SentPacketS",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&Sprite::m_sentPacket),
                     "ns3::Sprite::SentPacketTraceCallback")
     .AddTraceSource ("OnHold",
                     "A hold packet has been recived",
                     MakeTraceSourceAccessor (&Sprite::m_onHold),
                     "ns3::Sprite::OnHoldTraceCallback")
     .AddTraceSource ("OnPay",
                     "A pay packet has been received",
                     MakeTraceSourceAccessor (&Sprite::m_onPay),
                     "ns3::Sprite::OnPayTraceCallback")
     .AddTraceSource ("OnPayPath",
                     "A pay packet has been received",
                     MakeTraceSourceAccessor (&Sprite::m_onPayPath),
                     "ns3::Sprite::OnPayPathTraceCallback")                     
     .AddTraceSource ("OnTx",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Sprite::m_onTx),
                     "ns3::Sprite::OnTxTraceCallback")  
     .AddTraceSource ("OnTxFail",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Sprite::m_onTxFail),
                     "ns3::Sprite::OnTxFailTraceCallback")   
     .AddTraceSource ("OnPathUpdate",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Sprite::m_onPathUpdate),
                     "ns3::Sprite::OnPathUpdateTraceCallback") 
     .AddTraceSource ("OnTxRetry",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Sprite::m_onTxRetry),
                     "ns3::Sprite::OnTxRetryTraceCallback")    
     .AddTraceSource ("OnAd",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&Sprite::m_onAd),
                     "ns3::Sprite::OnAdTraceCallback")                                                                                                          
   ;
   return tid;
 }
 
 
Sprite::Sprite ()
 {
   NS_LOG_FUNCTION_NOARGS ();
 }
 
Sprite::~Sprite()
 {
   NS_LOG_FUNCTION_NOARGS ();
 }
 
void
Sprite::StartApplication (void)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   PCN_App_Base::StartApplication();
   if (m_route_helper){
      Simulator::Schedule(Seconds(0.0), &Sprite::sendAdvertPacket, this);
   }
   checkTimeout();
 }

//SECTION: Incoming Packet handling 
void 
Sprite::processPacket(Ptr<Packet> p, Ptr<Socket> s){
   blancHeader packetHeader;
   p->RemoveHeader(packetHeader);
   uint32_t pType = packetHeader.GetPacketType();
   uint32_t extraSize = p->GetSize() - packetHeader.GetPayloadSize();
   
   m_onAd(getName());

   Ptr<Packet> pkt  = p->Copy();
   p->RemoveAtEnd(extraSize);
   	  
   if (p->GetSize() == 0) return;
   switch(pType){
      case Find:
      case FindRecv:
      case HoldReply:
	      onHoldReply(p, packetHeader);
         break;
      case Hold:
         onHoldPacket(p, packetHeader, s);
         break;
      case HoldRecv:
         onHoldRecvPacket(p, packetHeader, s);
         break;
      case Pay:
         onPayPacket(p, packetHeader, s);
         break;
      case PayReply:
         onPayReply(p, packetHeader);
         break;
      case Advert:
         onAdvertPacket(p, packetHeader, s);
         break;  
      case AdvertReply:
         onAdvertReply(p, packetHeader, s);
         break;            
      case Reg:
         onRegPacket(p, packetHeader, s);
         break;      
      case Nack:
         onNack(p, packetHeader, s);
      default:
         break;
   }

   if (extraSize > 0){
      pkt->RemoveAtStart( packetHeader.GetPayloadSize() );
      processPacket(pkt, s);
   }
}

void 
Sprite::onHoldPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 {
   double delay = Sign()+Verify();
   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]); std::string path = items[1]; double amount = std::stod(items[2]);
   uint64_t hopMax = std::stoi(items[3]); double te1 = std::stod(items[4]), te2 = std::stod(items[5]);

   if(DEBUG2 || txID == TD){
      std::cout << "Holds----- "<<getName() <<"  NodeID\n"<< txID <<" TxID\n" << path <<" path "<<
         amount <<" amount\n"<< hopMax <<" hop max\n"<< te1 <<" te1\n"<< te2 <<" te2\n"<<std::endl;
         std::cout << payload <<"  payload "<<std::endl;         
   }

   std::string dest = SplitString(path, ',')[0];
   std::string OrignalDest = dest;
   std::string src = findSource(s), nextHop = "";

   if (messageHist[txID]){
      sendPreNack(src, txID, te1, OrignalDest, delay, true);
      return;
   }

   if (failedTxs[ items[0] + "|" + OrignalDest ]) {      
      sendPreNack(src, txID, te1, OrignalDest, delay, false);
      return; 
   } 

   bool needOverlap = false;
   
   if ( txidTable.find(txID) != txidTable.end() && txidTable[txID].pending != 0) {
      if(getName() == path && m_route_helper) {
         updateBCHold(txID); 
         sendProceedPay(txID, delay); sendHoldReply(txID, delay); 
         txidTable[txID].src = s; txidTable[txID].nextHop = src; 
         txidTable[txID].dest = dest; txidTable[txID].replied = true;
         txidTable[txID].onPath = true;
         return;
      }
      needOverlap = checkOverlap(txID, dest, true);
      if (!needOverlap) {
         sendPreNack(src, txID, te1, OrignalDest, delay, getName() == dest);
         return;
      }
   }
   else if(getName() == path && m_route_helper) {
         txidTable[txID].src = s; txidTable[txID].nextHop = src; 
         txidTable[txID].dest = dest; txidTable[txID].replied = true;
         txidTable[txID].pending = amount;
         return;
   }

   TransactionInfo* entry = &txidTable[txID];  
   if (needOverlap){
      TransactionInfo overlap;
      overlapTable[txID].push_back(overlap);
      entry = &overlapTable[txID].back();
   }

   entry->src = s;
   entry->nextHop = src; 
   entry->dest = dest;
   entry->pending = amount;
   entry->onPath = false;

   //Create next hold packet
   //TODO: Create return setting
   if(!m_route_helper && getName() == dest){
      path = SplitString(path, ',', 1)[1];
      dest = SplitString(path, ',')[0];
   }
   else if(m_route_helper && getName() == dest){
      std::string nh = "";
      if (SplitString(path, ',').size() == 1) {
         updateBCHold(txID);
         insertTimeout(txID, nextHop, payload, te1, te2, true);
         entry->replied = true;
         return;
      }      
      std::string og_path = SplitString(path, ',', 1)[1];
      messageHist[txID] = true;
      if ( SplitString(og_path, ',')[0] == "RH1" ){
         og_path = SplitString(og_path, ',', 1)[1];
         entry->RH1 = true;
      }
      while(nh==""){

         //Update next hop and Router Helper Destination
         sendHoldReply(txID, delay); 
         //TODO add path via Onion encryption to protect
         path = og_path;
         std::string dst = SplitString(path, ',')[0];
         dest = dst;
         if (DEBUG2 || txID == TD) std::cout<<path<<" || "<<dst<<std::endl;
         if(RoutingTable.find(dst) == RoutingTable.end() ||  attempted_paths.find(txID) != attempted_paths.end()){
            path = createPath(dst, amount, attempted_paths[txID], entry->RH1) + path;
            dest = SplitString(path, ',')[0];
            if(path == og_path){
               if (entry->RH1)
                  m_onTxFail(txID);         
               else 
                  sendNack(entry, txID, te1, getName(), delay, true);
               return;
            }            
         }
         if (DEBUG2 || txID == TD) std::cout<<path<<std::endl;
         entry->nextDest = dest;
         entry->onPath = true;
         attempted_paths[txID].push_back(dest);

         //Send out BC message, affirming hold.
         updateBCHold(txID);
         std::string r = findNextHop(dest, amount, true, "");
         nh = SplitString(r, '|')[0];


         if (attempted_paths.find(txID) == attempted_paths.end()){
            attempted_paths[txID];
         }

         if (nh == "" || r == "|10000") {   
            updateRHRTable(dest, entry->pending);         
            continue;
         }
      }
   }

   std::string result = findNextHop(dest, amount, true, src);
   nextHop = SplitString(result, '|')[0];

   if (nextHop == "" ) {      
      if(DEBUG2 || txID == TD) std::cout<<"No path: "<<result<<"  "<<dest<<"  "<<amount<<std::endl;
      sendNack(entry, txID, te1, OrignalDest, delay, false);
      failedTxs[items[0] + "|" + OrignalDest] = true;
      return; 
   }  
   double hopCount = std::stod(SplitString(result, '|')[1]);   

   blancHeader packetHeader;
   packetHeader.SetPacketType(Hold);

   payload = std::to_string(txID) + "|";
   payload += path + "|";
   payload += std::to_string(amount) + "|";
   payload += "5|";
   payload += std::to_string(te1) + "|" +std::to_string(te2);
   updatePathWeight(nextHop, amount, true);
   if(BALANCE) updatePathWeight(src, -1*amount, true);
   insertTimeout(txID, nextHop, payload, te1*hopCount, te2*hopCount, true);

   delay += Sign()+Verify();
   //Forward next hold packet
   if(DEBUG2 || txID == TD)   std::cout<<"Sending to " <<nextHop<<std::endl;
   forwardPacket(packetHeader, nextHop, payload, delay);
   if (txidAttempts.find(txID) == txidAttempts.end())
      txidAttempts[txID] = src;
   else 
      txidAttempts[txID] += "/" + src;   
   if(DEBUG2 || txID == TD)   std::cout<<"Dont send to " <<txidAttempts[txID]<<std::endl;

 }

void 
Sprite::onHoldRecvPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 { 
   double delay = Sign()+Verify();
   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]); std::string path = items[1]; double amount = std::stod(items[2]);
   uint64_t hopMax = std::stoi(items[3]); double te1 = std::stoi(items[4]); double te2 = std::stoi(items[5]); //std::string routeList = items[6];

   if(DEBUG2 || txID == TD){
      std::cout << "HoldR----- "<<getName() <<" NodeID\n"<<txID<<" TxID\n"<<path <<" path\n" << 
         amount <<"  amount\n"<<hopMax<<" hop max\n"<<te1<<" te1\n"<<te2<<" te2"<<
         ns3::Simulator::Now().GetSeconds()<<"\n"<<std::endl;
   }

   std::string nextHop = findSource(s);
   if (failedTxs[items[0] + "|RH2"]) {      
      sendPreNack(nextHop, txID, te1, "RH2", delay, false);
      return; 
   }  

   std::string dest = SplitString(path, ',')[0];
   if ( txidTable.find(txID) != txidTable.end() && txidTable[txID].pending != 0) {
      if(getName() == path && m_route_helper) {
         sendProceedPay(txID, delay); sendHoldReply(txID, delay); 
         txidTable[txID].dest = "RH2"; txidTable[txID].replied = true;
         return;
      }
      if(!checkOverlap(txID, "RH2", false)){
         sendPreNack(nextHop, txID, te1, "RH2", delay, false);
         return;
      }
   }

   if(overlapTable.find(txID) == overlapTable.end()){
      overlapTable[txID] = {};
   }
   txidTable[txID].src = s;
   txidTable[txID].dest = "RH2";   

   txidTable[txID].pending = amount;
   txidTable[txID].payload = "";

   //Create next hold packet
   if(!m_route_helper || getName() != dest){  

      blancHeader packetHeader;
      packetHeader.SetPacketType(HoldRecv);

      payload = std::to_string(txID) + "|";
      payload += path + "|";
      payload += std::to_string(amount) + "|";
      payload += "5|";
      payload += std::to_string(te1) + "|" +std::to_string(te2);
      
      std::string origin = nextHop;
      std::string result =  findNextHop(dest, amount, false, findSource(s));
      nextHop = SplitString(result, '|')[0];
      if(DEBUG2 || txID == TD){
         std::cout<<result<<"\n";
      }
      if (nextHop == "") {
         sendNack(&txidTable[txID], txID, te1, "RH2", delay, false);
         failedTxs[items[0] + "|RH2"] = true;
         return; 
      }  
      double hopCount = std::stod(SplitString(result, '|')[1]);       
      txidTable[txID].nextHop = nextHop;       
    
      updatePathWeight(origin, amount, false);
      if(BALANCE) updatePathWeight(nextHop, -1*amount, false);

      insertTimeout(txID, origin, payload, te1*hopCount, te2*hopCount, false);    

      delay += Sign() + Verify();
      //Forward next hold packet
      forwardPacket(packetHeader, nextHop, payload, 0);
   }
   else if (m_route_helper && getName() == dest){
      //Send out BC message, affirming hold. 
      updateBCHoldRecv(txID);
      txidTable[txID].onPath = true;
      txidTable[txID].replied = true;
      insertTimeout(txID, nextHop, payload, te1, te2, false);    

      sendHoldReply(txID, delay);
   }
 }

void 
Sprite::onHoldReply(Ptr<Packet> p, blancHeader ph)
 {

   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]);

   std::string src = "";//REPLACE

   if (items.size() == 1){
      if (txidTable[txID].replied) return;
      if(DEBUG2|| txID == TD) std::cout << "Hold reply ";
   }
   else if(DEBUG2|| txID == TD)
      std::cout << "ProceedPay ";      
      
   if(DEBUG2|| txID == TD) std::cout<< getName()  << " "<<ns3::Simulator::Now().GetSeconds() <<"\n"<<txID <<"  TxID\n"<<std::endl;

   txidTable[txID].replied = true;

   blancHeader packetHeader;
   packetHeader.SetPacketType(HoldReply);

   if (txidTable[txID].src != NULL ){
      //Forward next pay packet
      forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
   }

   else if (getTxID() == txID && items.size() > 1){
      m_onHold(std::stoi(getName()), txID, !m_payer);	 
   }
 }

void 
Sprite::onPayReply(Ptr<Packet> p, blancHeader ph){

   std::string payload = readPayload(p);


   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]);

   if (txidTable.find(txID) == txidTable.end()) return;

   std::string src = "";//REPLACE
   if(DEBUG3 || txID == TD)
      std::cout << "Pay reply "<< getName()  <<"\n"<<txID <<"  TxID\n"<<std::endl;

   txidTable[txID].payReplied = true;
   blancHeader packetHeader;
   packetHeader.SetPacketType(PayReply);

  if( overlapTable.find(txID) != overlapTable.end() && overlapTable[txID].size() > 0){
      overlapTable[txID].back().pending = txidTable[txID].pending;
      txidTable[txID] = overlapTable[txID].back();
      overlapTable[txID].pop_back();
   }   
   else{    
      txidTable.erase(txID);
   }

   if (srcTrail[txID].size() > 0 ){      
      //Forward next pay packet
      forwardPacket(packetHeader, srcTrail[txID].front(), payload, 0);
      srcTrail[txID].erase(srcTrail[txID].begin());
   }
}

void 
Sprite::onPayPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 {

   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]);
   double amount = std::stod(items[1]);
   uint64_t timeout = std::stoi(items[2]);
   std::string src = "";//REPLACE   

   if ( txidTable.find(txID) == txidTable.end() || txidTable[txID].pending == 0) return;

   if(DEBUG3 ||  txID == TD){   
      std::cout << "Pay packet------ "<<getName() <<" NodeID "<<std::endl;
      std::cout << txID <<"  TXiD "<<std::endl;
      std::cout << amount <<"  amount\n"<<std::endl;
   }
   m_onPayPath(std::stoi(getName()), txID);
 
   if(getTxID() == txID && m_payer){
      //Update next hop and Router Helper Destination
      txidTable[txID].src = s;
      sendPayReply(txID);
      m_onPay(std::stoi(getName()), txID);
      //Send out BC message, affirming hold.
      updateBCPay(txID);  

     return; 
   }

   if(txidTable[txID].nextHop == ""){
      txidTable[txID] = overlapTable[txID].back();
      txidTable[txID].replied = true;
      overlapTable[txID].pop_back();
   }

   TransactionInfo* entry = &txidTable[txID];

   if (txidTable[txID].paySend ){
      int i = overlapTable[txID].size()-1;
      while (overlapTable[txID][i].paySend){
         i--;
      }
      entry = &overlapTable[txID][i];
   }
   entry->paySend = true;
   entry->src = s;
   std::string nextHop = entry->nextHop;

   //Create next pay packet
   if (m_route_helper && txidTable[txID].onPath ){
      //Send out BC message, affirming hold. 
      sendPayReply(txID);
   }
   else {
      srcTrail[txID].push_back(s);
   }

   if (txID == TD) std::cout << nextHop<<std::endl;

   //Forward next pay packet
   blancHeader packetHeader;
   packetHeader.SetPacketType(Pay);

   forwardPacket(packetHeader, nextHop, payload, 0);

   //TODO verify with algo
   if(DEBUG3 ||  txID == TD){   
      std::cout<<"Size of Overlap "<<overlapTable[txID].size() <<std::endl;
   }

 }

void 
Sprite::onAdvertPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
{
   double delay = KeyGenB()+SignB();
   std::string payload = readPayload(p);
   if(DEBUG1) std::cout << getName()<<" "<<payload <<"  payload "<<std::endl;

   std::vector<std::string> items = SplitString(payload, '|');
   std::string src = items[0];
   double sendMax = std::stod(items[1]);
   double recvMax = std::stod(items[2]);
   double hopCount = std::stod(items[3])+1;
   double hopMax = std::stod(items[4]);
   double band = 0;
   std::string route = "";

   if( getName() == src)
      return;
   std::string nextHop = findSource(s);

   RoutingEntry* entry = NULL;
   int minHop = 10000;
   if (RoutingTable.find(src) == RoutingTable.end()){
      RoutingTable[src].push_back(RoutingEntry());
      entry = &RoutingTable[src][0];
      entry->nextHop = nextHop;
   }
   else {
      for (int i = 0; i < RoutingTable[src].size(); i++){
         if (RoutingTable[src][i].nextHop == nextHop){
            entry = &RoutingTable[src][i];
            if(entry->hopCount <= hopCount){
               return;
            }
         }
         if(RoutingTable[src][i].hopCount < minHop)
            minHop = RoutingTable[src][i].hopCount;
      }
      if (entry == NULL){
         RoutingTable[src].push_back(RoutingEntry());
         entry = &RoutingTable[src][RoutingTable[src].size()-1];
         entry->nextHop = nextHop;
      }
   }
   if(method2) {
      band = std::stod(items[5]);
      route = items[6];   
      entry->route = route;
   }
   if(false ){
      std::cout << "*****Advertisment packet*****\n" <<getName() <<"  NodeID "<<ns3::Simulator::Now().GetSeconds()<< std::endl;
      std::cout << src <<"  RH-Advert Source "<<std::endl;
      std::cout << nextHop <<"  Recieved from "<<std::endl;
      std::cout << sendMax <<"  send max"<<std::endl;
      std::cout << recvMax <<"  recv max"<<std::endl;
      std::cout << hopCount <<"  hop count"<<std::endl;
      std::cout << hopMax <<"  hop max\n"<<std::endl;
   
      if(method2) {
         std::cout << band <<"  band"<<std::endl;
         std::cout << route <<"  route\n"<<std::endl;
      }   
    }

   entry->sendMax = neighborTable[nextHop].cost;
   if (sendMax < entry->sendMax)
      entry->sendMax = sendMax;
   entry->recvMax = recvMax;
   entry->hopCount = hopCount;
 
   if (method2 && hopCount >= (hopMax-band) &&  hopCount <= hopMax && !m_route_helper){
      delay += SignB();
      Simulator::Schedule(Seconds(delay), &Sprite::sendAdvertReply, this, items);
   }
   if (hopCount >= hopMax || hopCount >= minHop)
      return;  
   for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      std::string payload = src;
      payload += "|"+ std::to_string(entry->sendMax) + "|";
      payload += std::to_string(std::min(entry->recvMax, neighborTable[it->first].cost));
      payload += "|" + std::to_string(hopCount) + "|";
      payload += std::to_string(hopMax) + "|";
      if(method2){
         payload += std::to_string(band) + "|";
         payload += route + "," + getName() + "|";      
      }
      //TODO: Add timestamp
      payload += "now";
      
      blancHeader packetHeader;
      packetHeader.SetPacketType(Advert);
      if(it->first != nextHop && SplitString(it->first, '|').size()==1){
         forwardPacket(packetHeader, it->first, payload, delay);
         if (DEBUG1 ) std::cout<<getName()<<" sending to " <<it->first<<" from "<<nextHop<<std::endl;
      }
   }
}

void 
Sprite::onAdvertReply(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
{
   double delay = SignB();
   std::string payload = readPayload(p);
   std::vector<std::string> items = SplitString(payload, '|');
   if(!method2) return;

   std::string src = items[0];
   double sendMax = std::stod(items[1]);
   double recvMax = std::stod(items[2]);
   double hopCount = std::stod(items[3])+1;
   std::string route = items[4];
   std::string nextRoute = SplitString(route, ',')[ SplitString(route, ',').size()-1 ];
    
   if(false){ 
      std::cout << "Advert Reply------ "<<getName() <<"  NodeID "<<std::endl;
      std::cout << src <<" Nonce "<<std::endl;
      std::cout << sendMax <<" send max"<<std::endl;
      std::cout << recvMax <<" recv max"<<std::endl;
      std::cout << route <<" route"<<std::endl;
      std::cout << hopCount <<" hop count\n"<<std::endl;
   }

   std::string nextHop = findSource(s);
   RoutingEntry* entry = NULL;
   int minHop = 10000;
   if (RoutingTable.find(src) == RoutingTable.end()){
      RoutingTable[src].push_back(RoutingEntry());
      entry = &RoutingTable[src][0];
      entry->nextHop = nextHop;
   }
   else {
      for (int i = 0; i < RoutingTable[src].size(); i++){
         if (RoutingTable[src][i].nextHop == nextHop){
            entry = &RoutingTable[src][i];
            if(entry->hopCount <= hopCount){
               return;
            }
         }
         if(RoutingTable[src][i].hopCount < minHop)
            minHop = RoutingTable[src][i].hopCount;
      }
      if (entry == NULL){
         RoutingTable[src].push_back(RoutingEntry());
         entry = &RoutingTable[src][RoutingTable[src].size()-1];
         entry->nextHop = nextHop;
      }
   }
   entry->sendMax = std::min(sendMax, neighborTable[entry->nextHop].cost);
   entry->recvMax = recvMax;
   entry->hopCount = hopCount;
   entry->nonce = true;

   if (nextRoute != getName() && hopCount < minHop){
      std::string payload = src;
      payload += "|"+ std::to_string(entry->sendMax) + "|";
      payload += std::to_string(std::min(entry->recvMax, neighborTable[nextRoute].cost));
      payload += "|" + std::to_string(hopCount) + "|";
      std::string payloadBase = payload;
      payload += route.substr(0, (route.size() - nextRoute.size()-1)  ) + "|";
      //TODO: Add timestamp
      payload += "now";
      
      blancHeader packetHeader;
      packetHeader.SetPacketType(AdvertReply);
      forwardPacket(packetHeader, nextRoute, payload, delay);
      std::string RH = SplitString(route, ',')[0];
      std::unordered_map<std::string, bool> sentTo;
      sentTo[nextRoute] = true;
   }
}

void 
Sprite::setNeighborCredit(std::string name, double amountTo, double amountFrom){
   neighborTable[name].cost = amountTo;
   neighborTable[name].costFrom = amountFrom;
}

bool
Sprite::hasHoldRecv(uint32_t txID)
{
   uint32_t txIDprime = txidTable[txID].nextTxID;
      std::cout << txIDprime<< ' ' << txID <<"  NodeID "<<std::endl;

   if ( txidTable.find(txIDprime) != txidTable.end() ){
      return true;
   }
   return false;
}

//Transaction Functions
void 
Sprite::startTransaction(uint32_t txID, std::vector<std::string> peerlist, bool payer, double amount)
{
   //Determine number of segments and create the needed txIDs.
   m_payer = payer;
   setTxID(txID);
   txidTable[txID].nextDest = peerlist[0];
   txidTable[txID].dest = peerlist[0];
   std::string path = getName()+"|" +peerlist[0];
   if (payer) {
      nextRH = peerlist[1];
      path += "|" +peerlist[1]; 
   }
   else    txidTable[txID].dest = "RH2";
   m_onTx(path, txID, payer, amount);
   ns3::Simulator::Schedule(Seconds(0.0), &Sprite::sendHoldPacket, this,txID, amount);
   setTiP(true);
}

void
Sprite::reset(uint32_t txID)
{
   //Determine number of segments and create the needed txIDs.
   m_amount = 0;
   m_payer = false;
   if(DEBUG3 || txID == TD)
      std::cout<<"Reset at " << getName() << "Erasing entry"<<std::endl;   
   txidTable.erase(txID);
   setTxID(0);
   nextRH = "";
   setTiP(false);
}

uint32_t
Sprite::createTxID(uint32_t txID){
   return txID + rand()%10;
}

void 
Sprite::sendAdvertPacket()
{
   AdvertSetup();
   double delay = KeyGenB()+SignB();
   //TODO: Add as a variable
   int band = 3;
   for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      if(SplitString(it->first, '|').size()!=1) continue;
      std::string payload = getName();
      payload += "|10000000000|";
      payload += std::to_string(neighborTable[it->first].cost) + "|0|";
      payload += std::to_string(m_hopMax) + "|";
      if(method2) {
         payload += std::to_string(band) + "|";
         payload += getName() + "|";      
      }
      //TODO: Add timestamp
      payload += "now";
      
      blancHeader packetHeader;
      packetHeader.SetPacketType(Advert);
      //TODO: Add signature
      if (DEBUG1) std::cout<<"Node "<<getName()<<" sending to"<<it->first<<std::endl;
      forwardPacket(packetHeader, it->first, payload, delay);
   }
}

void 
Sprite::sendAdvertReply(std::vector<std::string> items)
{
   if(!method2) return;
   AdvertSetup();

   std::string recvMax = items[1];
   std::string sendMax = items[2];
   std::string route = items[6];
   std::string nextRoute = SplitString(route, ',')[ SplitString(route, ',').size()-1 ];

   if (nextRoute.size()>0){
      std::string payload = getName();
      payload += "|100000000000|";
      payload += std::to_string(neighborTable[nextRoute].cost) + "|0|";
      payload += route.substr(0, (route.size() - nextRoute.size()-1)  ) + "|";

      //TODO: Add timestamp
      payload += "now";

      blancHeader packetHeader;
      packetHeader.SetPacketType(AdvertReply);
      //TODO: Add signature

      forwardPacket(packetHeader, nextRoute, payload, 0);
   }
}

void 
Sprite::sendRegPacket(std::string dest){

}

void
Sprite::sendHoldPacket(uint32_t txID, double amount)
{
   double delay = Sign()+Verify();
   //TODO Add preimage to packet and check it at onHold
   //TODO add path via Onion encryption to protect
   m_amount = amount;
   std::string dest = txidTable[txID].nextDest;
   txidTable[txID].pending = amount;
   std::string nextHop, result = findNextHop(dest, amount, m_payer, "");
   nextHop = SplitString(result, '|')[0];
   if (nextHop == "") {
         m_onTxFail(txID);         
      reset(txID);
      return; 
   }
   double hopCount = std::stod(SplitString(result, '|')[1]);

   std::string te1 = "30", te2 = "100"; //TODO:: replace with class variable

      if(DEBUG2) std::cout<<"Node "<<getName()<<" "<< nextHop <<" sending out Hold packet\n";

   blancHeader packetHeader;
   packetHeader.SetPacketType(HoldRecv);

   if(m_payer){
      packetHeader.SetPacketType(Hold);
      updatePathWeight(nextHop, amount, true);
      setTxID(txID);
   }
   else {
      txidTable[txID].nextHop = nextHop;
   if(overlapTable.find(txID) == overlapTable.end()){
      overlapTable[txID] = {};
   }      
   }

   std::string payload = std::to_string(txID) + "|";
   payload += dest;
   if(m_payer)
      payload += ",RH1," + nextRH;//Fix this
   payload += "|";
   payload += std::to_string(amount) + "|";
   payload += "5|";
   payload += te1 + "|" + te2;

   int timeout = 5;
   insertTimeout(txID, nextHop, payload, std::stod(te1)*hopCount, std::stod(te2)*hopCount, true);

   forwardPacket(packetHeader, nextHop, payload, delay);
   if (DEBUG2) std::cout<<"Hold phase\n\n\n";
}

void 
Sprite::sendHoldReply(uint32_t txID, double delay)
{
   blancHeader packetHeader;
   packetHeader.SetPacketType(HoldReply);

   std::string payload = std::to_string(txID);
   forwardPacket(packetHeader, txidTable[txID].src, payload, delay);
}

void 
Sprite::sendProceedPay(uint32_t txID, double delay)
{
   blancHeader packetHeader;
   packetHeader.SetPacketType(HoldReply);

   std::string payload = std::to_string(txID)+"|ProceedPay";
   forwardPacket(packetHeader, txidTable[txID].src, payload, delay);
}

void 
Sprite::sendPayReply(uint32_t txID)
{
   //TODO Add preimage to packet and check it at onHold
   //TODO add destinations
   if(DEBUG3 || txID == TD) std::cout<<"Sending Pay Reply: "<<getName()<<" "<<txidTable[txID].dest<<"\n\n";

   blancHeader packetHeader;
   packetHeader.SetPacketType(PayReply);

   std::string payload = std::to_string(txID);

   forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
}

void 
Sprite::sendPayPacket(uint32_t txID)
{
   if(DEBUG3) std::cout<<"Pay phase: "<<getName()<<" "<<txidTable[txID].nextHop<<"\n\n\n";
   std::string nextHop = txidTable[txID].nextHop;

   blancHeader packetHeader;
   packetHeader.SetPacketType(Pay);
   std::string payload = std::to_string(txID) + "|";
   payload += std::to_string(m_amount) + "|";
   payload += "5";

   forwardPacket(packetHeader, nextHop, payload, 0);
   if(overlapTable.find(txID) != overlapTable.end() && overlapTable[txID].size() > 0){
      txidTable[txID] = overlapTable[txID].back();
      txidTable[txID].replied = true;
      overlapTable[txID].pop_back();
   }   
}

void 
Sprite::sendNack(TransactionInfo* entry, uint32_t txID, double te1, std::string dest, double delay, bool RHreject){
   blancHeader packetHeader;
   packetHeader.SetPacketType(Nack);
   std::string payload;
   if ( dest == getName() || RHreject)
      payload = std::to_string(txID)+"|RHReject|"+std::to_string(te1)+"|"+dest;
   else
      payload = std::to_string(txID)+"|No Route|"+std::to_string(te1)+"|"+dest;

   std::string attempedPaths = txidAttempts[txID];

   payload += "|" + getName() + "/" + attempedPaths;      
   forwardPacket(packetHeader, entry->src, payload, delay);
   
   if ( entry == &txidTable[txID]) { 
      if(DEBUG3 || txID == TD)
      std::cout<<"Sent Nack at " << getName() << "Erasing entry"<<std::endl;   
      txidTable.erase(txID);
   }
   
   else if(overlapTable.find(txID) != overlapTable.end()){
      overlapTable[txID].pop_back();
   }    
}

void 
Sprite::sendPreNack(std::string src, uint32_t txID, double te1, std::string dest, double delay, bool RHreject) {
   blancHeader packetHeader;
   packetHeader.SetPacketType(Nack);
   std::string payload;
   if ( dest == getName() || RHreject)
      payload = std::to_string(txID)+"|RHReject|"+std::to_string(te1)+"|"+dest;
   else
      payload = std::to_string(txID)+"|No Route|"+std::to_string(te1)+"|"+dest;

   std::string attempedPaths = txidAttempts[txID];

   payload += "|" + getName() + "/" + attempedPaths;   
   forwardPacket(packetHeader, src, payload, delay); 
}

void 
Sprite::insertTimeout(uint32_t txID, std::string src, std::string payload, double te1, double te2, bool sender){
   TransactionInfo* entry = NULL;
   if (txidTable[txID].payload == "" && txidTable[txID].dest != "") { 
      entry = &txidTable[txID];
   }
   
   else if(overlapTable.find(txID) != overlapTable.end()){
      for (auto each = overlapTable[txID].begin(); each != overlapTable[txID].end(); each++){
         if(each->payload == "") {
            entry = &(*each);
         }
      }     
   }   
   if(DEBUG2 || txID == TD) std::cout<<txID<<"  " <<getName()<<"   "<<payload<<"  "<< txidTable[txID].dest << std::endl;
   entry->payload = payload; 
   entry->sender = sender;
   entry->te1 = ns3::Simulator::Now().GetSeconds() + te1;
   entry->te2 = ns3::Simulator::Now().GetSeconds() + te2;
}

void 
Sprite::checkTimeout(){
   for (auto each = txidTable.begin(); each != txidTable.end(); each++ ){
      if (each->second.te1 <= ns3::Simulator::Now().GetSeconds() && !each->second.replied ) {
         //checkTe1(each->first, each->second.nextHop, each->second.payload, 5, each->second.sender);
      }
      if (each->second.te2 <= ns3::Simulator::Now().GetSeconds() && !each->second.payReplied) {
         //checkTe2(each->first, each->second.nextHop, each->second.sender);
      }
   }
   ns3::Simulator::Schedule(Seconds(0.001), &Sprite::checkTimeout, this);
}

void 
Sprite::checkTe1(uint32_t txID, std::string src, std::string payload, double te1, bool sender){
   if (txidTable[txID].replied) return;
   return;
   updatePathWeight(src, (-1 * txidTable[txID].pending), sender);
   updateRoutingTable(src, txidTable[txID].pending, sender);
   if(txidTable[txID].onPath){
      updateRHRTable(txidTable[txID].nextDest, txidTable[txID].pending);
   }

   std::string nextHop = findNextHop(txidTable[txID].nextDest, txidTable[txID].pending, sender, ""); 
   
   if (nextHop == "" || txidTable[txID].retries >= m_maxRetires) {
      sendNack(&txidTable[txID], txID, te1, txidTable[txID].nextDest, 0, false);
      return; 
   }
   txidTable[txID].retries++;
   txidTable[txID].nextHop = nextHop; 

   blancHeader packetHeader;
   packetHeader.SetPacketType(Hold);
 
   updatePathWeight(nextHop, txidTable[txID].pending, sender);
   txidTable[txID].te1 = ns3::Simulator::Now().GetSeconds() + te1;

   //Forward next hold packet
   forwardPacket(packetHeader, nextHop, payload, 0);
}

void 
Sprite::onNack(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s){
   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]);
   double te1 = std::stoi(items[2]);
   std::string dest = items[3];

   if(txID == TD || DEBUGNACK){
      std::cout << "Nack----- "<< txID << "  " << getName() <<"  NodeID "<< ns3::Simulator::Now().GetSeconds()<<"\n"<<std::endl<<std::endl;
      std::cout<< "  nextDest: " << dest << " dest: " << txidTable[txID].dest <<std::endl;
   }

   TransactionInfo* entry = NULL;
   if (txidTable[txID].dest == dest || txidTable[txID].dest == getName()) { 
      entry = &txidTable[txID];
   }
   
   else if(overlapTable.find(txID) != overlapTable.end()){
      for (auto each = overlapTable[txID].begin(); each != overlapTable[txID].end(); each++){
         if(each->dest == dest || each->dest == getName() ) {
            entry = &(*each);
         }
      }     
   } 
   if (entry == NULL || entry->replied) {
      m_onTxFail(txID);         
      return;
   }   
   dest = entry->dest;

   std::string src = findSource(s);
   entry->sender;

   //TODO update from nack values
   updatePathWeight(src, (-1 * entry->pending), entry->sender);
   updateRoutingTable(src, entry->pending, entry->sender);
   if(entry->onPath){
      updateRHRTable(dest, entry->pending);
   }

   if("RHReject" == items[1] && !m_route_helper && !entry->RH1){
         sendNack(entry, txID, te1, dest, 0, true);
         return;
   }

   if (txidAttempts.find(txID) == txidAttempts.end())
      txidAttempts[txID] = src;
   else 
      txidAttempts[txID] += "/" + src;   
   std::string routeList = txidAttempts[txID];

   if(txID == TD || DEBUGNACK) std::cout<<"No sending to " << routeList <<std::endl;
   std::string result = findNextHop(dest, entry->pending, entry->sender, routeList); 

   std::string nextHop = SplitString(result, '|')[0];

   payload = entry->payload;

   if(txID == TD || DEBUGNACK) std::cout << entry->payload <<"  payload "<<std::endl;


   while(entry->onPath && nextHop==""){
      std::vector<std::string> items = SplitString(entry->payload, '|');

      uint32_t txID = std::stoi(items[0]); std::string path = items[1]; double amount = std::stod(items[2]);
      uint64_t hopMax = std::stoi(items[3]); double te1 = std::stod(items[4]), te2 = std::stod(items[5]);
      path = SplitString(path, ',')[SplitString(path, ',').size()-1];
      dest = path;

      std::string newPath = createPath(dest, entry->pending, attempted_paths[txID], entry->RH1);
      path = newPath + path;
      if(newPath == ""){
         if (entry->RH1)
            m_onTxFail(txID);         
         else 
            sendNack(entry, txID, te1, getName(), 0, true);
         return;
      }

      dest = SplitString(path, ',')[0];
      attempted_paths[txID].push_back(dest);

      result = findNextHop(dest, entry->pending, entry->sender, ""); 
      nextHop = SplitString(result, '|')[0];     
      if (txID == TD ) std::cout<<"nextHop: " << result<< "  path "<<path<< " dest "<<dest<<std::endl;;

      if(nextHop == "") {
         updateRHRTable(dest, entry->pending);    
         continue;
      }

      payload = std::to_string(txID) + "|";
      payload += path + "|";
      payload += std::to_string(amount) + "|";
      payload += "5|";
      payload += std::to_string(te1) + "|" +std::to_string(te2);
   }

   if (nextHop == "" || entry->retries >= m_maxRetires) {
      if (dest == getName()) {
         dest = entry->dest; 
      }
      if(getTxID() == txID) {
         m_onTxFail(txID);
      }
      else {
         std::string lasHop = findSource(entry->src);
         if(BALANCE) updatePathWeight(lasHop, (entry->pending), entry->sender);
         sendNack(entry, txID, te1, dest, 0, false);
         failedTxs[std::to_string(txID) + "|" + dest ] = true;
      }
      return; 
   }

   entry->retries++;
   m_onTxRetry(txID);
   
   txidAttempts[txID] += "/" + nextHop;

   blancHeader packetHeader;
   packetHeader.SetPacketType(Hold);
 
   updatePathWeight(nextHop, entry->pending, entry->sender);
   entry->te1 = ns3::Simulator::Now().GetSeconds() + te1;

   double delay = Sign()+Verify();
   //Forward next hold packet
   forwardPacket(packetHeader, nextHop, payload, delay);
}

void 
Sprite::checkTe2(uint32_t txID, std::string src, bool sender){
   if (txidTable.find(txID) == txidTable.end() ) return;

   updatePathWeight(src, (-1 * txidTable[txID].pending), sender);
   if(DEBUG3 || txID == TD)
      std::cout<<"Timeout at " << getName() << "Erasing entry"<<std::endl;   
   txidTable.erase(txID);
}

std::string 
Sprite::findSource(Ptr<Socket> s){
   std::string src = "";
   for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      if (it->second.socket == s)
         src = it->first; 
   }
   if(SplitString(src, '|').size()>1) {
      src = SplitString(src, '|')[0];
   }
   return src;
}

bool
Sprite::checkOverlap(uint32_t txID, std::string dest, bool sender){
   if (txidTable[txID].dest == dest) { 
      return false;
   }
   
   if(overlapTable.find(txID) == overlapTable.end()){
      TransactionInfo tailEntry = txidTable[txID];
      overlapTable[txID].push_back(tailEntry);
      txidTable[txID].dest = "";
      txidTable[txID].nextHop = "";
   }
   if (sender){
      //Ensure this is not a duplicate by looping through overlap table entry.
      for (auto each = overlapTable[txID].begin(); each != overlapTable[txID].end(); each++){
         if(each->dest == dest) return false;
      }      
   }
   return true;
}

std::string 
Sprite::getRH(std::string RH1){
   std::string RH = "";
   int minHops = 10000;
   for (auto i = RoutingTable.begin(); i != RoutingTable.end(); i++){
      double sendMax = 0, recvMax = 0, hops = 10000;
      for (auto each = i->second.begin(); each != i->second.end(); each++){
         if (each->nonce || RH1 == i->first) continue;
         if(sendMax < each->sendMax) sendMax = each->sendMax;
         if(recvMax < each->recvMax) recvMax = each->recvMax;
         if(hops > each->hopCount) hops = each->hopCount;
      }
      if ( hops < minHops ) {
         minHops = hops;
         RH =  i->first;
      }
   }
   return RH;
}

std::vector<std::string> 
Sprite::getReachableRHs(){
   std::vector<std::string> RHs;
   for (auto i = RoutingTable.begin(); i != RoutingTable.end(); i++){
      std::string entry = i->first + "|";
      double sendMax = 0, recvMax = 0;
      bool nonce = false;
      for (auto each = i->second.begin(); each != i->second.end(); each++){
         if(each->nonce) nonce = true;
         if(sendMax < each->sendMax) sendMax = each->sendMax;
         if(recvMax < each->recvMax) recvMax = each->recvMax;
      }
      if(nonce) entry = "N"+ entry;
      entry += std::to_string(sendMax) + "|" + std::to_string(recvMax);
      RHs.push_back(entry);
   }
   return RHs;
}

void 
Sprite::setRHTable(std::unordered_map<std::string, std::vector<std::string>> RHlist){
   if (DEBUG2) std::cout<<"Node " <<getName()<<" creating table...\n";
   for (auto i = RHlist.begin(); i != RHlist.end(); i++){
      if (RoutingTable.find(i->first) != RoutingTable.end() || i->first == getName()) {
         if (DEBUG2) std::cout<<"\tReaches routing helper " <<i->first<<" directly\n";
         //continue;
      }
      if (DEBUG2) std::cout<<"\tFor router helper " <<i->first<<".\n";
      for (auto each = i->second.begin(); each != i->second.end(); each++){
         RHRoutingEntry entry;
         bool nonce = false;
         entry.path = SplitString(*each, '|')[0];

         if(entry.path[0] == 'N' ) {
            nonce = true;
            entry.path = entry.path.substr(1);
         }
         entry.maxSend =  std::stod(SplitString(*each, '|')[1]);
         entry.maxRecv = std::stod(SplitString(*each, '|')[2]);

         if(RoutingTable.find(entry.path) != RoutingTable.end()){
            entry.complete = true;
            if(nonce)
               NonceTable[i->first].push_back(entry.path);
         }
         else if (nonce) continue;
         RHRoutingTable[i->first].push_back(entry);
         if (DEBUG2) std::cout<<"\t\tGo to " <<entry.path<<" with sizes " <<entry.maxSend<<" and " <<entry.maxRecv<<".\n";
      }
   }
}

std::vector<std::string>  
Sprite::matchUpNonces(std::unordered_map<std::string, std::vector<std::string>> RHlist){
   for (auto i = RHlist.begin(); i != RHlist.end(); i++){
      if (RoutingTable.find(i->first) != RoutingTable.end() || i->first == getName()) {
         continue;
      }
      for (auto each = i->second.begin(); each != i->second.end(); each++){
         bool nonce = false;
         std::string path = SplitString(*each, '|')[0];
         std::string send =  SplitString(*each, '|')[1];
         std::string recv = SplitString(*each, '|')[2];
         if(path[0] == 'N' ) {
            nonce = true;
            path = path.substr(1);
         }
         if(RoutingTable.find(path) != RoutingTable.end()){
            if(nonce) {
               std::string rh = i->first +"|" +send + "|" + recv;
               RHlist[getName()].push_back(rh);
               break;
            }
         }
      }
   }
   return RHlist[getName()];
}

std::string 
Sprite::findNextHop(std::string dest, double amount, bool send, std::string routeList){
   std::vector<std::string> seenNodes;
   if(routeList != "") seenNodes = SplitString(routeList, '/');
   std::unordered_map<std::string, bool> seenNode_map;
   for (int i = 0; i < seenNodes.size(); i++){
      seenNode_map[ seenNodes[i] ] = true;
   }
   std::vector<RoutingEntry*> options; 
   for (auto i = RoutingTable[dest].begin(); i != RoutingTable[dest].end(); i++){
      if (debug) std::cout<<dest<<" | "<< i->nextHop <<" | " <<i->sendMax<< " | " << i->recvMax <<std::endl;
      if( seenNode_map [i->nextHop] && i->nextHop != dest) continue;
      if (debug) std::cout<<"Not seen"<<std::endl;
      if (send &&   i->sendMax >= amount &&  neighborTable[i->nextHop].cost >= amount)
         options.push_back(&(*i));
      else if (!send && i->recvMax >= amount &&  neighborTable[i->nextHop].cost >= amount)
         options.push_back(&(*i));
   }
   if (options.size() == 0){
      if (debug) std::cout<<"Noting was there\n"<<std::endl;
      for (auto i = RoutingTable[dest].begin(); i != RoutingTable[dest].end(); i++){
         if (debug) std::cout<<dest<<" | "<< i->nextHop <<" | " <<neighborTable[i->nextHop].cost<< " | " << neighborTable[i->nextHop].costFrom <<std::endl;
         if( seenNode_map [i->nextHop] && i->nextHop != dest) 
            continue;
         if (debug) std::cout<<"Not seen"<<std::endl;
         if (send &&  neighborTable[i->nextHop].cost >= amount  )//&&   i->sendMax >= amount)
            options.push_back(&(*i));
         else if (!send && neighborTable[i->nextHop].costFrom >= amount)// && i->recvMax >= amount)
            options.push_back(&(*i));
      }
   }
   int leastHops = 10000;
   std::string bestNextHop = "";
   RoutingEntry* bNHop = NULL;

   double bestAmount = 0;
   for (auto i = options.begin(); i != options.end(); i++){
      if (debug) std::cout<<(*i)->nextHop<<std::endl;
      if ((*i)->hopCount < leastHops){
         leastHops = (*i)->hopCount;
         bestNextHop = (*i)->nextHop;
         bNHop = (*i);
         bestAmount = (*i)->recvMax;
         if (send) bestAmount = (*i)->sendMax;
      }

   }
   return bestNextHop + "|" + std::to_string(leastHops);
}

//TODO:: Revist this function for optimization
std::string 
Sprite::createPath(std::string dest, double amount, std::vector<std::string> attempt_list, bool RH1){
   bool complete = false;
   std::string path = "";
   std::unordered_map<std::string, bool> attempted_map;
   for (int i = 0; i < attempt_list.size(); i++){
      attempted_map[ attempt_list[i] ] = true;
   }   
   if ( !RH1 ){
      for (auto i = NonceTable[dest].begin(); i != NonceTable[dest].end(); i++){
         if(!attempted_map[ *i ])
            return *i + "," ;
      }
   
      return "";
   }
   std::string newDest = dest;
   while (!complete){
      RHRoutingEntry* bestOption = NULL;
      double maxAmount = 0;
      for (auto i = RHRoutingTable[newDest].begin(); i != RHRoutingTable[newDest].end(); i++){
         if ( attempted_map[ i->path ] ||i->path == getName() || i->path == dest)
            continue;
         if(bestOption == NULL ) {
            bestOption = &(*i);
            maxAmount = i->maxSend;
         }
         if ((!complete && i->complete) || (maxAmount < i->maxSend  && ( !complete || i->complete)) ) {
            complete = true;
            maxAmount = i->maxSend;
            bestOption = &(*i);    
         }
      }
      //}
      if(bestOption == NULL) return path;
      attempted_map[ bestOption->path ] = true;
      path = bestOption->path + "," + path;
      newDest = bestOption->path;
   }
      if(DEBUG3) std::cout<<"This is the Path I found "<<path<<std::endl;
   return path;
}

} // Namespace ns3


