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
     .SetParent<Application> ()
     .AddConstructor<SpeedyM> ()
     .AddAttribute ("Port", "Port on which we listen for incoming connections.",
                    UintegerValue (7),
                    MakeUintegerAccessor (&SpeedyM::m_local_port),
                    MakeUintegerChecker<uint16_t> ())
     .AddAttribute ("PacketSize",
                   "The size of outbound packet, typically acknowledgement packets from server application. 536 not fragmented on 1500 MTU",
                   UintegerValue (536),
                   MakeUintegerAccessor (&SpeedyM::m_packet_size),
		   MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("RouterHelper",
                   "True if application will run in Router helper mode.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SpeedyM::m_route_helper),
                   MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("Name",
                   "Name of node.",
                   StringValue ("0"),
                   MakeStringAccessor (&SpeedyM::m_name),
                   MakeStringChecker())     
     .AddAttribute ("Method2",
                   "Bool.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SpeedyM::method2),
                   MakeUintegerChecker<uint32_t> ())
      .AddTraceSource ("ReceivedPacketS",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&SpeedyM::m_receivedPacket),
                     "ns3::SpeedyM::ReceivedPacketTraceCallback")
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
   m_socket = 0;
   m_running = false;
 }
 
SpeedyM::~SpeedyM()
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
 }
 
 
void
SpeedyM::StartApplication (void)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
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
   m_onAd(m_name, std::to_string(pType));

 
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
      std::string payload = m_name;
      payload += "| ";
      
      blancHeader packetHeader;
      packetHeader.SetPacketType(Advert);

      if (DEBUG1) std::cout<<"Node "<<m_name<<" sending to"<<it->first<<std::endl;
      forwardPacket(packetHeader, it->first, payload, 0);
      TreeTable[m_name].parent = "";
      TreeTable[m_name].coordinate = " ";
   }
}

void 
SpeedyM::onAdvertPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
{
   std::string payload = readPayload(p);
   if(DEBUG1) std::cout << m_name<<" "<<payload <<"  payload "<<std::endl;

   std::vector<std::string> items = SplitString(payload, '|');
   std::string treeId = items[0];
   std::string senderCoor = items[1];
   std::string sender = findSource(s);


   if(false){
      std::cout << "*****Advertisment packet*****\n" <<m_name <<"  NodeID "<<ns3::Simulator::Now().GetSeconds()<< std::endl;
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
      //if(SplitString(it->first, '|').size()!=1 || it->first == sender) continue;
      std::string payload = treeId;
      payload += "|"+TreeTable[treeId].coordinate;

      blancHeader packetHeader;
      packetHeader.SetPacketType(Advert);

      if (false) std::cout<<"Node "<<m_name<<" sending to"<<it->first<<std::endl;
      forwardPacket(packetHeader, it->first, payload, 0);
   }
}

void 
SpeedyM::sendRoutePayPacket(std::string txID, std::string treeID, std::string address, double amount)
{
   if(std::stoi(txID) == TD) std::cout<<"Pay phase: "<<m_name<<"  "<<address<<"\n\n\n";

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
   payload += m_name;
   forwardPacket(packetHeader, nextHop, payload, 0);
   txidTable[txID].nextHop = nextHop;
//std::cout<<txidTable[txID].nextHop <<" "<<txID<<std::endl;
}

void 
SpeedyM::onRoutePayPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 {

   std::string payload = readPayload(p);
   //   std::cout << payload <<"  payload "<<std::endl;

   std::vector<std::string> items = SplitString(payload, '|');
   std::string txID = items[0];
   std::string treeID = items[1];
   std::string address = items[2];
   double amount = std::stod(items[3]);
   std::string routeList = items[4];
   if(std::stoi(txID) == TD ){   
      std::cout << "Route Pay packet------ "<<m_name <<" NodeID "<<ns3::Simulator::Now().GetSeconds()<<std::endl;
      std::cout << txID <<"  TXiD "<<std::endl;
      std::cout << treeID <<"  treeID "<<std::endl;
      std::cout << address <<"  address "<<std::endl;
      std::cout << TreeTable[treeID].coordinate <<"  node address "<<std::endl;
      std::cout << amount <<"  amount\n"<<std::endl;
      std::cout << routeList <<"  routeList\n"<<std::endl;

   }
   //m_onPayPath(std::stoi(m_name), txID);
   std::string src = findSource(s);
   txidTable[txID].src = s;

   if(TreeTable[treeID].coordinate == address){
      sendRoutePayReply(txID, true);
      return;
   }
   std::string nextHop = findNextHop(address, treeID, amount, routeList);
   //   if(txID == "25_1") std::cout << nextHop <<"  nextHop\n\n "<<std::endl;

   if(nextHop == ""){
      sendNack(txID, 0);
   }
   else{
      sendRoutePayReply(txID, false);
      payload += "/"+m_name;
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
   //TODO Add preimage to packet and check it at onHold
   //TODO add destinations
   if(std::stoi(txID) == TD) std::cout<<"Pay Reply: "<<m_name<<"\n\n";

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

   //if(std::stoi(txID) == TD)
   //   std::cout << "Pay reply "<< m_name  <<"\n"<<payload <<"  TxID\n"<<std::endl;

   //txidTable[txID].replied = true;
   blancHeader packetHeader;
   packetHeader.SetPacketType(RouteReply);

   if(std::stoi( SplitString(txID, '_')[0]) == m_txID && items.size()>1){
      m_successful_routes++;
      if(std::stoi(txID) == TD) std::cout<<txID<<"  "<<m_name<<" Successful routes"<<m_successful_routes<<"  "<<m_treeCount<<std::endl;
      if (m_successful_routes == m_treeCount){
         sendPayments(SplitString(txID, '_')[0], m_treeCount);
      }
   }
   if (txidTable[txID].src != NULL ){
      
      //Forward next pay packet
      //forwardPacket(packetHeader, txidTable[txID].src, payload);
      forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
      //txidTable[txID].src = NULL;
      //txidTable.erase(txID);
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
      std::cout << "Pay packet------ "<<m_name <<" NodeID "<<std::endl;
      std::cout << txID <<"  TXiD "<<std::endl;
   }
   m_onPayPath(std::stoi(m_name), txID);
   if(txidTable[txID].nextHop != ""){
      blancHeader packetHeader;
      packetHeader.SetPacketType(Pay);
      std::string payload = txID;
      txidTable[txID].nextHop;
      forwardPacket(packetHeader, txidTable[txID].nextHop, payload, 0);
      //if( neighborTable[ txidTable[txID].nextHop ].cost == 0){

      //}
   }
   if(txIDPrime == m_txID){
      m_successful_routes++;
      //std::cout<<txID<<"  "<<m_name<<" Successful routes"<<m_successful_routes<<std::endl;
      if (m_successful_routes == m_treeCount){
         m_onPay(std::stoi(m_name), txIDPrime);
         //std::cout<<"transaction Complete\n";
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
   m_txID = txID;
   m_onTx(m_name, txID, true, amount);
   TiP = true;
   m_successful_routes=0;
   m_treeCount = addresslist.size();
   for (int i = 0; i < addresslist.size()-1; i++){
      std::string txID_i = std::to_string(txID) +"_"+ std::to_string(i);
      double amount_i = amount * 0.1;//(double(random() % 100000000)/ 100000000.0);
      //amount -= amount_i;
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
      //std::cout<<txID<<" Interesting\n";
   }
}

void 
SpeedyM::setReceiver(uint32_t txID, double amount)
{
   //Split up transaction and send through all trees
   //std::cout<<txID<<std::endl;
   m_txID = txID;
   TiP = true;
   m_successful_routes=0;
   m_treeCount = TreeTable.size();
   m_onTx(m_name, txID, false, amount);
   Simulator::Schedule(Seconds(1.0), &SpeedyM::checkFail, this, txID);
}

void
SpeedyM::reset(uint32_t txID)
{
   //std::cout<<"Good bye tx "<<m_name<<std::endl;
   //Determine number of segments and create the needed txIDs.
   m_amount = 0;
   m_payer = false;
   //txidTable.erase(txID);
   m_txID = 0;
   nextRH = "";
   TiP = false; 
   m_failed = false;
   //TODO: Loop through all entres and delete them
}

void 
SpeedyM::checkTimeout(){
   for (auto each = txidTable.begin(); each != txidTable.end(); each++ ){

   }
   //ns3::Simulator::Schedule(Seconds(0.001), &SpeedyM::checkTimeout, this);
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
   //std::cout<<"No break "<<TreeTable[treeID].parent<<"   "<<m_name<<" "<<treeID<<std::endl;
   std::string parent_name, parent_coor;
   if (m_name != treeID){
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
      //std::cout<<"No parent\n";
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
      if (Debug && m_name == "4183" ) std::cout<<TreeTable[treeID].coordinate << " " <<neighbor_name<<" "<<neighbor_coor<<" "<<match<<" "<< prefix <<" | ";//<<std::endl;

      if (hops < minHops){ 
         minHops = hops;
         bestHop = neighbor_name;
      }
   }
   if (Debug) std::cout<<std::endl;
   //std::cout<<"Choice: "<<bestHop<<"\n"<<std::endl;  
   return bestHop;
}

} // Namespace ns3


