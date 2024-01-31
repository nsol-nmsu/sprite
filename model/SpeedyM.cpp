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
 #include "SpeedyM.hpp"
 #define DEBUG1 0
 #define DEBUG2 0
 #define DEBUG3 0
 #define TD -1
 #define BALANCE 1

 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("ns3.SpeedyM");
 NS_OBJECT_ENSURE_REGISTERED (SpeedyM);
 
 TypeId
 SpeedyM::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::SpeedyM")
     .SetParent<PCN_App_Base> ()
     .AddConstructor<SpeedyM> ()
     .AddAttribute ("RouterHelper",
                   "True if application will run in Router helper mode.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SpeedyM::m_route_helper),
                   MakeUintegerChecker<uint32_t> ())   
     .AddAttribute ("Method2",
                   "Bool.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SpeedyM::method2),
                   MakeUintegerChecker<uint32_t> ())
     .AddTraceSource ("SentPacketS",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&SpeedyM::m_sentPacket),
                     "ns3::SpeedyM::SentPacketTraceCallback")
     .AddTraceSource ("OnFindReply",
                     "A findReply packet has been received",
                     MakeTraceSourceAccessor (&SpeedyM::m_onFindReply),
                     "ns3::SpeedyM::OnFindReplyTraceCallback")
     .AddTraceSource ("OnHold",
                     "A hold packet has been recived",
                     MakeTraceSourceAccessor (&SpeedyM::m_onHold),
                     "ns3::SpeedyM::OnHoldTraceCallback")
     .AddTraceSource ("OnPay",
                     "A pay packet has been received",
                     MakeTraceSourceAccessor (&SpeedyM::m_onPay),
                     "ns3::SpeedyM::OnPayTraceCallback")
     .AddTraceSource ("OnPayPath",
                     "A pay packet has been received",
                     MakeTraceSourceAccessor (&SpeedyM::m_onPayPath),
                     "ns3::SpeedyM::OnPayPathTraceCallback")                     
     .AddTraceSource ("OnTx",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&SpeedyM::m_onTx),
                     "ns3::SpeedyM::OnTxTraceCallback")  
     .AddTraceSource ("OnTxFail",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&SpeedyM::m_onTxFail),
                     "ns3::SpeedyM::OnTxFailTraceCallback")   
     .AddTraceSource ("OnPathUpdate",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&SpeedyM::m_onPathUpdate),
                     "ns3::SpeedyM::OnPathUpdateTraceCallback") 
     .AddTraceSource ("OnTxRetry",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&SpeedyM::m_onTxRetry),
                     "ns3::SpeedyM::OnTxRetryTraceCallback")    
     .AddTraceSource ("OnAd",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&SpeedyM::m_onAd),
                     "ns3::SpeedyM::OnAdTraceCallback")                                                                                                          
   ;
   return tid;
 }
 
 
SpeedyM::SpeedyM ()
 {
   NS_LOG_FUNCTION_NOARGS ();
 }
 
SpeedyM::~SpeedyM()
 {
   NS_LOG_FUNCTION_NOARGS ();
 }
 
 
void
SpeedyM::StartApplication (void)
 {
   NS_LOG_FUNCTION_NOARGS ();
   PCN_App_Base::StartApplication();
   if (m_route_helper){
      double time = 0;
      Simulator::Schedule(Seconds(time), &SpeedyM::sendAdvertPacket, this);
   }
   checkTimeout();
 }
 
//SECTION: Incoming Packet handling 
void 
SpeedyM::processPacket(Ptr<Packet> p, Ptr<Socket> s){
   blancHeader packetHeader;
   p->RemoveHeader(packetHeader);
   uint32_t pType = packetHeader.GetPacketType();
   uint32_t extraSize = p->GetSize() - packetHeader.GetPayloadSize();
   m_onAd(getName(), std::to_string(pType));

 
   Ptr<Packet> pkt  = p->Copy();
   p->RemoveAtEnd(extraSize);
   	  
   if (p->GetSize() == 0) return;
   switch(pType){
      case PayRoute:
         onRoutePayPacket(p, packetHeader, s);
         break;
      case RouteReply:
         onRoutePayReply(p, packetHeader);
         break;
      case Advert:
         onAdvertPacket(p, packetHeader, s);
         break;            
      case Reg:
         onRegPacket(p, packetHeader, s);
         break;      
      case Nack:
         onNack(p, packetHeader, s);
         break;
      case Pay:
         onPay(p, packetHeader, s);
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
SpeedyM::sendAdvertPacket()
{
   for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      if(SplitString(it->first, '|').size()!=1) continue;
      std::string payload = getName();
      payload += "| ";
      
      blancHeader packetHeader;
      packetHeader.SetPacketType(Advert);

      if (DEBUG1) std::cout<<"Node "<<getName()<<" sending to"<<it->first<<std::endl;
      forwardPacket(packetHeader, it->first, payload, 0);
      TreeTable[getName()].parent = "";
      TreeTable[getName()].coordinate = " ";
   }
}

void 
SpeedyM::onAdvertPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
{
   std::string payload = readPayload(p);
   if(DEBUG1) std::cout << getName()<<" "<<payload <<"  payload "<<std::endl;

   std::vector<std::string> items = SplitString(payload, '|');
   std::string treeId = items[0];
   std::string senderCoor = items[1];
   std::string sender = findSource(s);


   if(false){
      std::cout << "*****Advertisment packet*****\n" <<getName() <<"  NodeID "<<ns3::Simulator::Now().GetSeconds()<< std::endl;
      std::cout << treeId <<"  Tree ID "<<std::endl;
      std::cout << sender <<"  sender ID"<<std::endl;
      std::cout << "( "+senderCoor <<" )  sender coordinates"<<std::endl;
   }

   if(TreeTable.find(treeId) == TreeTable.end()){
      TreeTable[treeId].parent = sender+"|"+senderCoor;
      if(senderCoor == " ") senderCoor = "";//Makes appending next sequences more streamlined      
      else senderCoor += ",";
      TreeTable[treeId].coordinate = senderCoor+std::to_string(rand()%1048576);
   }
   else{
      TreeTable[treeId].neighbors.push_back(sender+"|"+senderCoor);
      return;
   }

   for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      std::string payload = treeId;
      payload += "|"+TreeTable[treeId].coordinate;

      blancHeader packetHeader;
      packetHeader.SetPacketType(Advert);

      if (false) std::cout<<"Node "<<getName()<<" sending to"<<it->first<<std::endl;
      forwardPacket(packetHeader, it->first, payload, 0);
   }
}

void 
SpeedyM::sendRoutePayPacket(std::string txID, std::string treeID, std::string address, double amount)
{
   if(std::stoi(txID) == TD) std::cout<<"Pay phase: "<<getName()<<"  "<<address<<"\n\n\n";

   std::string nextHop = findNextHop(address, treeID, amount, "");
   if (nextHop == ""){
      m_failed = true;
      return;
   }
      

   blancHeader packetHeader;
   packetHeader.SetPacketType(PayRoute);
   std::string payload = txID + "|";
   payload += treeID + "|";
   payload += address + "|";
   payload += std::to_string(amount) + "|";
   payload += getName();
   forwardPacket(packetHeader, nextHop, payload, 0);
   txidTable[txID].nextHop = nextHop;
}

void 
SpeedyM::onRoutePayPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 {

   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   std::string txID = items[0];
   std::string treeID = items[1];
   std::string address = items[2];
   double amount = std::stod(items[3]);
   std::string routeList = items[4];
   if(std::stoi(txID) == TD ){   
      std::cout << "Route Pay packet------ "<<getName() <<" NodeID "<<ns3::Simulator::Now().GetSeconds()<<std::endl;
      std::cout << txID <<"  TXiD "<<std::endl;
      std::cout << treeID <<"  treeID "<<std::endl;
      std::cout << address <<"  address "<<std::endl;
      std::cout << TreeTable[treeID].coordinate <<"  node address "<<std::endl;
      std::cout << amount <<"  amount\n"<<std::endl;
      std::cout << routeList <<"  routeList\n"<<std::endl;

   }
   std::string src = findSource(s);
   txidTable[txID].src = s;

   if(TreeTable[treeID].coordinate == address){
      sendRoutePayReply(txID, true);
      return;
   }
   std::string nextHop = findNextHop(address, treeID, amount, routeList);

   if(nextHop == ""){
      sendNack(txID, 0);
   }
   else{
      sendRoutePayReply(txID, false);
      payload += "/"+getName();
      blancHeader packetHeader;
      packetHeader.SetPacketType(PayRoute);      
      forwardPacket(packetHeader, nextHop, payload, 0);
      txidTable[txID].nextHop = nextHop;
      updatePathWeight(nextHop, amount, true);
      if(BALANCE) updatePathWeight(src, -1*amount, true);      
   }
   Debug = false;
 }

void 
SpeedyM::sendRoutePayReply(std::string txID, bool complete)
{
   //TODO add destinations
   if(std::stoi(txID) == TD) std::cout<<"Pay Reply: "<<getName()<<"\n\n";

   blancHeader packetHeader;
   packetHeader.SetPacketType(RouteReply);

   std::string payload = txID;
   if(complete) payload += "|Complete";

   forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
}

void 
SpeedyM::onRoutePayReply(Ptr<Packet> p, blancHeader ph){

   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   std::string txID = items[0];

   if (txidTable.find(txID) == txidTable.end()) return;

   blancHeader packetHeader;
   packetHeader.SetPacketType(RouteReply);

   if(std::stoi( SplitString(txID, '_')[0]) == getTxID() && items.size()>1){
      m_successful_routes++;
      if(std::stoi(txID) == TD) std::cout<<txID<<"  "<<getName()<<" Successful routes"<<m_successful_routes<<"  "<<m_treeCount<<std::endl;
      if (m_successful_routes == m_treeCount){
         sendPayments(SplitString(txID, '_')[0], m_treeCount);
      }
   }
   if (txidTable[txID].src != NULL ){
      //Forward next pay packet
      forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
   }
}

void 
SpeedyM::sendNack(std::string txID, double delay)
{
   blancHeader packetHeader;
   packetHeader.SetPacketType(Nack);

   std::string payload = txID+"|No Route";
   forwardPacket(packetHeader, txidTable[txID].src, payload, delay);
   txidTable.erase(txID);
}

void 
SpeedyM::onNack(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s){
   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   std::string txID = items[0];

   std::string src = findSource(s);

   updatePathWeight(src, (-1 * txidTable[txID].pending), true);

   if (txidTable[txID].src != NULL ){
      blancHeader packetHeader;
      packetHeader.SetPacketType(Nack);   
      forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
      std::string nextHop = findSource( txidTable[txID].src );
      if(BALANCE) updatePathWeight(nextHop, txidTable[txID].pending, true);
   }
   txidTable.erase(txID);
}

void
SpeedyM::sendPayments(std::string txID, int m_treeCount){
   for (int i = 0; i < m_treeCount; i++){
      std::string txID_i = txID +"_"+ std::to_string(i);
      blancHeader packetHeader;
      packetHeader.SetPacketType(Pay);
      std::string payload = txID_i;
      
      forwardPacket(packetHeader, txidTable[txID_i].nextHop, payload, 0);
      txidTable.erase(txID_i) ;
   }
}


void
SpeedyM::onPay(Ptr<Packet> p, blancHeader ph, Ptr<Socket> ){
  std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   std::string txID = items[0];
   uint32_t txIDPrime = std::stoi(SplitString(txID, '_')[0]);

   if(txIDPrime == TD){   
      std::cout << "Pay packet------ "<<getName() <<" NodeID "<<std::endl;
      std::cout << txID <<"  TXiD "<<std::endl;
   }
   m_onPayPath(std::stoi(getName()), txID);
   if(txidTable[txID].nextHop != ""){
      blancHeader packetHeader;
      packetHeader.SetPacketType(Pay);
      std::string payload = txID;
      txidTable[txID].nextHop;
      forwardPacket(packetHeader, txidTable[txID].nextHop, payload, 0);
   }
   if(txIDPrime == getTxID()){
      m_successful_routes++;
      if (m_successful_routes == m_treeCount){
         m_onPay(std::stoi(getName()), txIDPrime);
      }
   }
   txidTable.erase(txID);
}

//Transaction Functions
void 
SpeedyM::startTransaction(uint32_t txID, std::vector<std::string> addresslist, double amount)
{
   //Split up transaction and send through all trees
   if(txID == TD) std::cout<<txID<<std::endl;
   setTxID(txID);
   m_onTx(getName(), txID, true, amount);
   setTiP(true);
   m_successful_routes=0;
   m_treeCount = addresslist.size();
   for (int i = 0; i < addresslist.size()-1; i++){
      std::string txID_i = std::to_string(txID) +"_"+ std::to_string(i);
      double amount_i = amount * 0.1;
      std::string treeID = SplitString(addresslist[i], '|')[0];
      std::string address = SplitString(addresslist[i], '|')[1];
      sendRoutePayPacket(txID_i, treeID, address, amount_i);
   }
   std::string txID_i = std::to_string(txID) + "_" +std::to_string(addresslist.size()-1);
   if(txID == TD) std::cout<<txID_i<<std::endl;;
   std::string treeID = SplitString(addresslist.back(), '|')[0];
   std::string address = SplitString(addresslist.back(), '|')[1];
   sendRoutePayPacket(txID_i, treeID, address, amount*0.1);
   if (m_failed) {
      m_onTxFail(txID);
      reset(txID);   
   }
}

void 
SpeedyM::setReceiver(uint32_t txID, double amount)
{
   //Split up transaction and send through all trees
   setTxID(txID);
   setTiP(true);
   m_successful_routes=0;
   m_treeCount = TreeTable.size();
   m_onTx(getName(), txID, false, amount);
   Simulator::Schedule(Seconds(1.0), &SpeedyM::checkFail, this, txID);
}

void
SpeedyM::reset(uint32_t txID)
{
   //Determine number of segments and create the needed txIDs.
   m_amount = 0;
   m_payer = false;
   setTxID(0);
   nextRH = "";
   setTiP(false);
   m_failed = false;
   //TODO: Loop through all entres and delete them
}

void 
SpeedyM::checkTimeout(){
   for (auto each = txidTable.begin(); each != txidTable.end(); each++ ){

   }
}

std::string 
SpeedyM::findNextHop(std::string dest, std::string treeID, double amount, std::string routeList){
   std::vector<std::string> seenNodes;
   if(routeList != "") seenNodes = SplitString(routeList, '/');
   std::unordered_map<std::string, bool> seenNode_map;
   for (int i = 0; i < seenNodes.size(); i++){
      seenNode_map[ seenNodes[i] ] = true;
   }

   int minHops = 10000000;
   std::string bestHop = "";
   std::string parent_name, parent_coor;
   if (getName() != treeID){
      parent_name = SplitString(TreeTable[treeID].parent, '|')[0];
      parent_coor = SplitString(TreeTable[treeID].parent, '|')[1];
      if (Debug) std::cout<<"Coors: "<<parent_coor<<std::endl;

      if(neighborTable[parent_name].cost >= amount && !seenNode_map[parent_name]) {
         int commonSize = getCommonPrefixsize(dest, parent_coor);
         minHops = SplitString(dest,',').size() - commonSize;
         minHops += SplitString(parent_coor,',').size() - commonSize;
         bestHop = parent_name;
      }
   }
   else {
      minHops =  SplitString(dest,',').size() - getCommonPrefixsize(dest, " ");
      if (minHops == 0) return "";
   }

   if (Debug) std::cout<<"Parent: "<<bestHop<<std::endl;

   std::vector<std::string> neighbor_list = TreeTable[treeID].neighbors;
   for (auto it = neighbor_list.begin(); it != neighbor_list.end(); it++){
      std::string neighbor_name = SplitString(*it, '|')[0];
      if(neighborTable[neighbor_name].cost < amount || seenNode_map[neighbor_name]) continue;

      std::string neighbor_coor = SplitString(*it, '|')[1];
      int commonSize = getCommonPrefixsize(dest, neighbor_coor);
      int hops = SplitString(dest,',').size() - commonSize;
      hops += SplitString(neighbor_coor,',').size() - commonSize;
      int prefix = SplitString(TreeTable[treeID].coordinate,',').size();
      int match = getCommonPrefixsize(TreeTable[treeID].coordinate, neighbor_coor);
      if (Debug ) std::cout<<TreeTable[treeID].coordinate << " " <<neighbor_name<<" "<<neighbor_coor<<" "<<match<<" "<< prefix <<" | ";//<<std::endl;

      if (hops < minHops){ 
         minHops = hops;
         bestHop = neighbor_name;
      }
   }
   if (Debug) std::cout<<std::endl;
   return bestHop;
}

} // Namespace ns3


