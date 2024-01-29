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
 #include "BLANC++.hpp"
 #define DEBUG1 0
 #define DEBUG2 0
 #define DEBUG3 0
 #define DEBUGNACK 0
 #define TD -1
 #define BALANCE 1

 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("ns3.BLANCpp");
 NS_OBJECT_ENSURE_REGISTERED (BLANCpp);
 
 TypeId
 BLANCpp::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::BLANCpp")
     .SetParent<Application> ()
     .AddConstructor<BLANCpp> ()
     .AddAttribute ("Port", "Port on which we listen for incoming connections.",
                    UintegerValue (7),
                    MakeUintegerAccessor (&BLANCpp::m_local_port),
                    MakeUintegerChecker<uint16_t> ())
     .AddAttribute ("PacketSize",
                   "The size of outbound packet, typically acknowledgement packets from server application. 536 not fragmented on 1500 MTU",
                   UintegerValue (536),
                   MakeUintegerAccessor (&BLANCpp::m_packet_size),
		   MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("RouterHelper",
                   "True if application will run in Router helper mode.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BLANCpp::m_route_helper),
                   MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("Name",
                   "Name of node.",
                   StringValue ("0"),
                   MakeStringAccessor (&BLANCpp::m_name),
                   MakeStringChecker())     
     .AddAttribute ("Method2",
                   "Bool.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BLANCpp::method2),
                   MakeUintegerChecker<uint32_t> ())
     .AddAttribute ("HopMax",
                   "Hop max.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&BLANCpp::m_hopMax),
                   MakeUintegerChecker<uint32_t> ())                   
      .AddTraceSource ("ReceivedPacketS",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&BLANCpp::m_receivedPacket),
                     "ns3::BLANCpp::ReceivedPacketTraceCallback")
     .AddTraceSource ("SentPacketS",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&BLANCpp::m_sentPacket),
                     "ns3::BLANCpp::SentPacketTraceCallback")
     .AddTraceSource ("OnFindReply",
                     "A findReply packet has been received",
                     MakeTraceSourceAccessor (&BLANCpp::m_onFindReply),
                     "ns3::BLANCpp::OnFindReplyTraceCallback")
     .AddTraceSource ("OnHold",
                     "A hold packet has been recived",
                     MakeTraceSourceAccessor (&BLANCpp::m_onHold),
                     "ns3::BLANCpp::OnHoldTraceCallback")
     .AddTraceSource ("OnPay",
                     "A pay packet has been received",
                     MakeTraceSourceAccessor (&BLANCpp::m_onPay),
                     "ns3::BLANCpp::OnPayTraceCallback")
     .AddTraceSource ("OnPayPath",
                     "A pay packet has been received",
                     MakeTraceSourceAccessor (&BLANCpp::m_onPayPath),
                     "ns3::BLANCpp::OnPayPathTraceCallback")                     
     .AddTraceSource ("OnTx",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&BLANCpp::m_onTx),
                     "ns3::BLANCpp::OnTxTraceCallback")  
     .AddTraceSource ("OnTxFail",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&BLANCpp::m_onTxFail),
                     "ns3::BLANCpp::OnTxFailTraceCallback")   
     .AddTraceSource ("OnPathUpdate",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&BLANCpp::m_onPathUpdate),
                     "ns3::BLANCpp::OnPathUpdateTraceCallback") 
     .AddTraceSource ("OnTxRetry",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&BLANCpp::m_onTxRetry),
                     "ns3::BLANCpp::OnTxRetryTraceCallback")    
     .AddTraceSource ("OnAd",
                     "A transaction has started",
                     MakeTraceSourceAccessor (&BLANCpp::m_onAd),
                     "ns3::BLANCpp::OnAdTraceCallback")                                                                                                          
   ;
   return tid;
 }
 
 
BLANCpp::BLANCpp ()
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
   m_running = false;
 }
 
BLANCpp::~BLANCpp()
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
 }
 
void
BLANCpp::DoDispose (void)
 {
   NS_LOG_FUNCTION_NOARGS ();
   Application::DoDispose ();
 }
 
void
BLANCpp::StartApplication (void)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   m_running = true;

   if (m_socket == 0)
     {
       TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
       m_socket = Socket::CreateSocket (GetNode (), tid);
       InetSocketAddress listenAddress = InetSocketAddress (Ipv4Address::GetAny (), m_peerPort);
       m_socket->Bind (listenAddress);
       m_socket->Listen();
     }
   m_socket->SetAcceptCallback (
         MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
         MakeCallback (&BLANCpp::HandleAccept, this));
   if (m_route_helper){
      Simulator::Schedule(Seconds(0.0), &BLANCpp::sendAdvertPacket, this);
   }

   checkTimeout();

 }
 
void
BLANCpp::StopApplication ()
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   m_running = false;
 
   if (m_socket != 0)
     {
       m_socket->Close ();
       m_socket->SetAcceptCallback (
             MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
             MakeNullCallback<void, Ptr<Socket>, const Address &> () );
     }
 }
 
void
BLANCpp::ReceivePacket (Ptr<Socket> s)
 {
   NS_LOG_FUNCTION (this << s);
   Ptr<Packet> packet;
   Address from;
   while (packet = s->RecvFrom (from)) {
      if (packet->GetSize () > 0) {

	      //Get the first interface IP attached to this node (this is where socket is bound, true nodes that have only 1 IP)
    	   //Ptr<NetDevice> PtrNetDevice = PtrNode->GetDevice(0);
    	   Ptr <Node> PtrNode = this->GetNode();
    	   Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> (); 
    	   Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);  
    	   m_local_ip = iaddr.GetLocal (); 

     	   NS_LOG_INFO ("Server Received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
		      << ":" << InetSocketAddress::ConvertFrom (from).GetPort ());

	      packet->RemoveAllPacketTags ();
     	   packet->RemoveAllByteTags ();
	      processPacket(packet, s);
	   } 
	
      //TODO: Alter
      if (false){ 
     	  NS_LOG_LOGIC ("Sending reply packet " << packet->GetSize ());
	  //Keep original packet to send to trace file and get correct size
          Ptr<Packet> packet_copy = Create<Packet> (m_packet_size);
     	  s->Send (packet_copy);
      }
          // Callback for received packet
    	  m_receivedPacket (GetNode()->GetId(), packet, from, m_local_port, 0, m_local_ip);
      
     }
 }
 
void BLANCpp::HandleAccept (Ptr<Socket> s, const Address& from)
 {
   NS_LOG_FUNCTION (this << s << from);
   s->SetRecvCallback (MakeCallback (&BLANCpp::ReceivePacket, this));
   s->SetCloseCallbacks(MakeCallback (&BLANCpp::HandleSuccessClose, this),
     MakeNullCallback<void, Ptr<Socket> > () );
 }
 
void BLANCpp::HandleSuccessClose(Ptr<Socket> s)
 {
   NS_LOG_FUNCTION (this << s);
   NS_LOG_LOGIC ("Client close received");
   s->Close();
   s->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > () );
   s->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),
       MakeNullCallback<void, Ptr<Socket> > () );
 }

//SECTION: Incoming Packet handling 
void 
BLANCpp::processPacket(Ptr<Packet> p, Ptr<Socket> s){
   blancHeader packetHeader;
   p->RemoveHeader(packetHeader);
   uint32_t pType = packetHeader.GetPacketType();
   uint32_t extraSize = p->GetSize() - packetHeader.GetPayloadSize();
   
   m_onAd(m_name);

   Ptr<Packet> pkt  = p->Copy();
   p->RemoveAtEnd(extraSize);
   	  
   if (p->GetSize() == 0) return;
   switch(pType){
      case Find:
      case FindRecv:
      case 10:
         onFindPacket(p, packetHeader, s);
         break;
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
BLANCpp::onHoldPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 {
   double delay = Sign()+Verify();
   std::string payload = readPayload(p);
   //std::cout << payload <<"  payload "<<std::endl;

   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]); std::string path = items[1]; double amount = std::stod(items[2]);
   uint64_t hopMax = std::stoi(items[3]); double te1 = std::stod(items[4]), te2 = std::stod(items[5]);// std::string routeList = items[6]; std::string rhList = "";
   //if (items.size() >= 8)
   //   rhList = items[7];

   if(DEBUG2 || txID == TD){
      std::cout << "Holds----- "<<m_name <<"  NodeID\n"<< txID <<" TxID\n" << path <<" path "<<
         amount <<" amount\n"<< hopMax <<" hop max\n"<< te1 <<" te1\n"<< te2 <<" te2\n"<<std::endl;
         std::cout << payload <<"  payload "<<std::endl;         
      //if (items.size() >= 8)
      //   std::cout << "RHList " << rhList <<std::endl;
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
      //if(DEBUG2 || txID == TD) std::cout<<"I suposse this is bad\n"
      if(m_name == path && m_route_helper) {
         updateBCHold(txID); 
         sendProceedPay(txID, delay); sendHoldReply(txID, delay); 
         txidTable[txID].src = s; txidTable[txID].nextHop = src; 
         txidTable[txID].dest = dest; txidTable[txID].replied = true;
         txidTable[txID].onPath = true;
         return;
      }
      needOverlap = checkOverlap(txID, dest, true);
      if (!needOverlap) {
         //if (m_route_helper && m_name == dest && (failedTxs[ items[0] + "|" + SplitString(path, ',')[1] ])){
            sendPreNack(src, txID, te1, OrignalDest, delay, m_name == dest);
         //}
         //else
            return;
      }
   }
   else if(m_name == path && m_route_helper) {
         txidTable[txID].src = s; txidTable[txID].nextHop = src; 
         txidTable[txID].dest = dest; txidTable[txID].replied = true;
         txidTable[txID].pending = amount;
         return;
   }

   TransactionInfo* entry = &txidTable[txID];  
   if (needOverlap){
      if(DEBUG2 || txID == TD) std::cout<<"Overlap alert\n";
      TransactionInfo overlap;
      overlapTable[txID].push_back(overlap);
      entry = &overlapTable[txID].back();
   }
   if(DEBUG2 || txID == TD){
      std::cout<<"Should be here\n";
   }
   entry->src = s;
   entry->nextHop = src; 
   entry->dest = dest;
   entry->pending = amount;
   entry->onPath = false;

   //Create next hold packet
   //Ptr<Packet> packet_copy = Create<Packet> (m_packet_size);
   //TODO: Create return setting
   if(!m_route_helper && m_name == dest){
      //routeList="";
      path = SplitString(path, ',', 1)[1];
      dest = SplitString(path, ',')[0];
   }
   else if(m_route_helper && m_name == dest){
      std::string nh = "";
      //std::string lastRH = SplitString(routeList, '/')[0];
      //updateRHRTableWeight(lastRH, (-1)*(amount));
      if (SplitString(path, ',').size() == 1) {
         updateBCHold(txID);
         insertTimeout(txID, nextHop, payload, te1, te2, true);
         entry->replied = true;
         return;
      }      
      //routeList="";
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
                  sendNack(entry, txID, te1, m_name, delay, true);
               return;
            }            
         }
         if (DEBUG2 || txID == TD) std::cout<<path<<std::endl;
         entry->nextDest = dest;
         //entry->dest = dest;
         entry->onPath = true;
         attempted_paths[txID].push_back(dest);

         //Send out BC message, affirming hold.
         updateBCHold(txID);
         std::string r = findNextHop(dest, amount, true, "");
         nh = SplitString(r, '|')[0];


         if (attempted_paths.find(txID) == attempted_paths.end()){
            attempted_paths[txID];
            //std::vector<std::string> usedRHs = SplitString( rhList, '/');
            /*for (auto i = usedRHs.begin(); i != usedRHs.end(); i++){
               attempted_paths[txID].push_back(*i);
            }*/
         }

         if (nh == "" || r == "|10000") {   
            if(txID == TD) std::cout<<"I failed\n";
            updateRHRTable(dest, entry->pending);         
            continue;
         }
         //rhList += "/" + m_name;         
      }
   }

   std::string result = findNextHop(dest, amount, true, src);
   //if (routeList != "") routeList+="/";
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
   //payload += "|"+routeList + m_name + "|";// + rhList;
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
BLANCpp::onFindPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s){

   
}

void 
BLANCpp::onHoldRecvPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 { 
   double delay = Sign()+Verify();
   std::string payload = readPayload(p);

   //std::cout << payload <<"  payload "<<std::endl;

   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]); std::string path = items[1]; double amount = std::stod(items[2]);
   uint64_t hopMax = std::stoi(items[3]); double te1 = std::stoi(items[4]); double te2 = std::stoi(items[5]); //std::string routeList = items[6];

   if(DEBUG2 || txID == TD){
      std::cout << "HoldR----- "<<m_name <<" NodeID\n"<<txID<<" TxID\n"<<path <<" path\n" << 
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
 if(DEBUG2 || txID == TD){
      std::cout << "I am here\n";
      }      
      if(m_name == path && m_route_helper) {
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
   if(!m_route_helper || m_name != dest){  

      blancHeader packetHeader;
      packetHeader.SetPacketType(HoldRecv);

      payload = std::to_string(txID) + "|";
      payload += path + "|";
      payload += std::to_string(amount) + "|";
      payload += "5|";
      payload += std::to_string(te1) + "|" +std::to_string(te2);
      //payload += "|"+routeList + "/" + m_name;
      
      std::string origin = nextHop;
      std::string result =  findNextHop(dest, amount, false, findSource(s));
      nextHop = SplitString(result, '|')[0];
   if(DEBUG2 || txID == TD){
std::cout<<result<<"\n";
   }
      if (nextHop == "") {
         //std::cout<<"No where to go\n";
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
   else if (m_route_helper && m_name == dest){
      //Send out BC message, affirming hold. 
      updateBCHoldRecv(txID);
      //m_onHold(std::stoi(m_name), txID, true);
      txidTable[txID].onPath = true;
      txidTable[txID].replied = true;
      insertTimeout(txID, nextHop, payload, te1, te2, false);    

      sendHoldReply(txID, delay);
   }
 }

void 
BLANCpp::onHoldReply(Ptr<Packet> p, blancHeader ph)
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
      
   if(DEBUG2|| txID == TD) std::cout<< m_name  << " "<<ns3::Simulator::Now().GetSeconds() <<"\n"<<txID <<"  TxID\n"<<std::endl;

   txidTable[txID].replied = true;

   blancHeader packetHeader;
   packetHeader.SetPacketType(HoldReply);

   if (txidTable[txID].src != NULL ){
      //Forward next pay packet
      forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
   }

   else if (m_txID == txID && items.size() > 1){
      m_onHold(std::stoi(m_name), txID, !m_payer);	 
   }
 }

void 
BLANCpp::onPayReply(Ptr<Packet> p, blancHeader ph){

   std::string payload = readPayload(p);


   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]);

   if (txidTable.find(txID) == txidTable.end()) return;

   std::string src = "";//REPLACE
   if(DEBUG3 || txID == TD)
      std::cout << "Pay reply "<< m_name  <<"\n"<<txID <<"  TxID\n"<<std::endl;

   txidTable[txID].payReplied = true;
   blancHeader packetHeader;
   packetHeader.SetPacketType(PayReply);

  if( overlapTable.find(txID) != overlapTable.end() && overlapTable[txID].size() > 0){
      if(DEBUG3 || txID == TD)
         std::cout<<"Looking out for an overlap "<<std::endl;
      overlapTable[txID].back().pending = txidTable[txID].pending;
      txidTable[txID] = overlapTable[txID].back();
      overlapTable[txID].pop_back();
   }   
   else{
      if(DEBUG3 || txID == TD)
         std::cout<<"Erasing entry"<<std::endl;      
      txidTable.erase(txID);
   }

   if (srcTrail[txID].size() > 0 ){      
      //Forward next pay packet
      //forwardPacket(packetHeader, txidTable[txID].src, payload);
      forwardPacket(packetHeader, srcTrail[txID].front(), payload, 0);
      srcTrail[txID].erase(srcTrail[txID].begin());
      //txidTable[txID].src = NULL;
      //if (srcTrail[txID].size() == 0 ) ;
   }
}

void 
BLANCpp::onPayPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
 {

   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]);
   double amount = std::stod(items[1]);
   uint64_t timeout = std::stoi(items[2]);
      std::string src = "";//REPLACE

  /*if(txidTable[txID].paySend ){
   //if(DEBUG3 ||  txID == TD) std::cout<<"I should be here\n";
      Simulator::Schedule(Seconds(0.0001), &BLANCpp::onPayPacket, this, p, ph, s);
      return;
   }*/

   if ( txidTable.find(txID) == txidTable.end() || txidTable[txID].pending == 0) return;

   if(DEBUG3 ||  txID == TD){   
      std::cout << "Pay packet------ "<<m_name <<" NodeID "<<std::endl;
      std::cout << txID <<"  TXiD "<<std::endl;
      std::cout << amount <<"  amount\n"<<std::endl;
   }
   m_onPayPath(std::stoi(m_name), txID);
 
   if(m_txID == txID && m_payer){
      //Update next hop and Router Helper Destination
      txidTable[txID].src = s;
      sendPayReply(txID);
      m_onPay(std::stoi(m_name), txID);
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
BLANCpp::onAdvertPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
{
   double delay = KeyGenB()+SignB();
   std::string payload = readPayload(p);
   if(DEBUG1) std::cout << m_name<<" "<<payload <<"  payload "<<std::endl;

   std::vector<std::string> items = SplitString(payload, '|');
   std::string src = items[0];
   double sendMax = std::stod(items[1]);
   double recvMax = std::stod(items[2]);
   double hopCount = std::stod(items[3])+1;
   double hopMax = std::stod(items[4]);
   double band = 0;
   std::string route = "";

   //m_onAd(m_name);

   //uint64_t timestamp = std::stoi(items[2]);

   if( m_name == src)
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
      std::cout << "*****Advertisment packet*****\n" <<m_name <<"  NodeID "<<ns3::Simulator::Now().GetSeconds()<< std::endl;
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

   /*if (m_route_helper){
      std::cout << " Routing helper received\n"<<std::endl;
      if (reachable.find(src) == reachable.end())
         reachable.emplace(src);
   }*/


   //hopCount++;

   entry->sendMax = neighborTable[nextHop].costTo;
   if (sendMax < entry->sendMax)
      entry->sendMax = sendMax;
   entry->recvMax = recvMax;
   entry->hopCount = hopCount;

   //entry->expireTime = ;
   //TODO: Add signature verification
   //TODO: Add keyGen
 
   if (method2 && hopCount >= (hopMax-band) &&  hopCount <= hopMax && !m_route_helper){
      delay += SignB();
      Simulator::Schedule(Seconds(delay), &BLANCpp::sendAdvertReply, this, items);
      //sendAdvertReply(items);
   }
   if (hopCount >= hopMax || hopCount >= minHop)
      return;  
   for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      std::string payload = src;
      payload += "|"+ std::to_string(entry->sendMax) + "|";
      payload += std::to_string(std::min(entry->recvMax, neighborTable[it->first].costTo));
      payload += "|" + std::to_string(hopCount) + "|";
      payload += std::to_string(hopMax) + "|";
      if(method2){
         payload += std::to_string(band) + "|";
         payload += route + "," + m_name + "|";      
      }
      //TODO: Add timestamp
      payload += "now";
      
      blancHeader packetHeader;
      packetHeader.SetPacketType(Advert);
      //TODO: Add signature
      if(it->first != nextHop && SplitString(it->first, '|').size()==1){
         forwardPacket(packetHeader, it->first, payload, delay);
         if (DEBUG1 ) std::cout<<m_name<<" sending to " <<it->first<<" from "<<nextHop<<std::endl;
      }
   }
}

void 
BLANCpp::onAdvertReply(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s)
{
   double delay = SignB();
   std::string payload = readPayload(p);
   std::vector<std::string> items = SplitString(payload, '|');
   if(!method2) return;
   //m_onAd(m_name);

   std::string src = items[0];
   //std::cout << m_name << "  "<<payload <<"  payload "<<ns3::Simulator::Now().GetSeconds()<< std::endl;
   double sendMax = std::stod(items[1]);
   double recvMax = std::stod(items[2]);
   double hopCount = std::stod(items[3])+1;
   std::string route = items[4];
   std::string nextRoute = SplitString(route, ',')[ SplitString(route, ',').size()-1 ];

   //uint64_t timestamp = std::stoi(items[2]);
    
   if(false){ 
      std::cout << "Advert Reply------ "<<m_name <<"  NodeID "<<std::endl;
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
   //std::cout<<"Gonna go to "<<nextRoute<<"  "<< ns3::Simulator::Now().GetSeconds()<<std::endl;
   entry->sendMax = std::min(sendMax, neighborTable[entry->nextHop].costTo);
   entry->recvMax = recvMax;
   entry->hopCount = hopCount;
   entry->nonce = true;
   //entry->expireTime = ;
   //TODO: Add signature verification
   //TODO: Add keyGen

   if (nextRoute != m_name && hopCount < minHop){
      std::string payload = src;
      payload += "|"+ std::to_string(entry->sendMax) + "|";
      payload += std::to_string(std::min(entry->recvMax, neighborTable[nextRoute].costTo));
      payload += "|" + std::to_string(hopCount) + "|";
      std::string payloadBase = payload;
      payload += route.substr(0, (route.size() - nextRoute.size()-1)  ) + "|";
      //TODO: Add timestamp
      payload += "now";
      
      blancHeader packetHeader;
      packetHeader.SetPacketType(AdvertReply);
      //TODO: Add signature

      forwardPacket(packetHeader, nextRoute, payload, delay);
      std::string RH = SplitString(route, ',')[0];
      std::unordered_map<std::string, bool> sentTo;
      sentTo[nextRoute] = true;
      /*for (auto it = RoutingTable[RH].begin(); it != RoutingTable[RH].end(); it++){
         if(it->nextHop != nextHop && it->nextHop != nextRoute && (hopCount + it->hopCount) < m_hopMax){
            payload = payloadBase;
            payload += it->route.substr(0, (it->route.size() - it->nextHop.size()-1)  ) + "|";
            payload += "now";
            forwardPacket(packetHeader, it->nextHop, payload, delay);
            sentTo[it->nextHop] = true;
         }
      }*/

   }
}

void 
BLANCpp::onRegPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s){
   std::string payload = readPayload(p);

   if(DEBUG1) std::cout<<"Node "<<m_name<<" registering socket for "<<payload<<std::endl;
    
   if (neighborTable[payload].socket == 0)
      neighborTable[payload].socket = s;
   else 
      neighborTable[payload+"|T"].socket = s;

}

void 
BLANCpp::forwardPacket(blancHeader packetHeader, std::string nextHop, std::string payload, double delay)
{

  Ptr<Packet> p;
  p = Create<Packet> (reinterpret_cast<const uint8_t*> (payload.c_str()), payload.size());  
  //p = Create<Packet> (100);
    packetHeader.setPayloadSize(payload.size());

  uint64_t m_bytesSent = 100;
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well

  //Add Blanc information to packet header and send
  Address peerAddress = neighborTable[nextHop].address;
  Ptr<Socket> socket = getSocket(nextHop);

  p->AddHeader(packetHeader);


  Simulator::Schedule(Seconds(delay), &BLANCpp::send, this, socket, p);

  //socket->Send (p);
  NS_LOG_INFO ("Sent " << 100 << " bytes to " << peerAddress << ":" << m_peerPort);

  //Increase sequence number for next packet
  m_seq = m_seq + 1;
}

void
BLANCpp::forwardPacket(blancHeader packetHeader, Ptr<Socket> socket, std::string payload, double delay)
{

  Ptr<Packet> p;
  p = Create<Packet> (reinterpret_cast<const uint8_t*> (payload.c_str()), payload.size());
  packetHeader.setPayloadSize(payload.size());
  //p = Create<Packet> (100);
  uint64_t m_bytesSent = 100;
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  //m_txTrace (p);

  //Add Blanc information to packet header and send

  p->AddHeader(packetHeader);


  Simulator::Schedule(Seconds(delay), &BLANCpp::send, this, socket, p);
  //socket->Send (p);

  //Increase sequence number for next packet
  m_seq = m_seq + 1;
}

void 
BLANCpp::setNeighborCredit(std::string name, double amountTo, double amountFrom){
   neighborTable[name].costTo = amountTo;
   neighborTable[name].costFrom = amountFrom;
}

void
BLANCpp::setNeighbor(std::string name, Ipv4Address address){
   neighborTable[name].address = address;
}

Ptr<Socket> 
BLANCpp::getSocket(std::string dest)
{
 Ptr<Socket> socket = neighborTable[dest].socket;
	
 if (socket == 0)
 {
   TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");   
   socket = Socket::CreateSocket (GetNode (), tid);
   socket->Bind();
   socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom( neighborTable[dest].address ), m_peerPort));

   socket->SetRecvCallback (MakeCallback (&BLANCpp::ReceivePacket, this));
   
   std::string payload = m_name;     
   blancHeader packetHeader;
   packetHeader.SetPacketType(Reg);
   packetHeader.setPayloadSize(payload.size());

   neighborTable[dest].socket = socket;

   //forwardPacket(packetHeader, socket, payload, 0);
   Ptr<Packet> p = Create<Packet> (reinterpret_cast<const uint8_t*> (payload.c_str()), payload.size());  
   p->AddHeader(packetHeader);

   socket->Send(p);
 }

 return socket;
}

bool
BLANCpp::hasHoldRecv(uint32_t txID)
{
   uint32_t txIDprime = txidTable[txID].nextTxID;
      std::cout << txIDprime<< ' ' << txID <<"  NodeID "<<std::endl;

   if ( txidTable.find(txIDprime) != txidTable.end() ){
      //txidTable.erase(txIDprime);
      //txidTable.erase(txID);
      return true;
   }
   return false;
}

//Transaction Functions
void 
BLANCpp::startTransaction(uint32_t txID, std::vector<std::string> peerlist, bool payer, double amount)
{
   //Determine number of segments and create the needed txIDs.

   m_payer = payer;
   m_txID = txID;
   txidTable[txID].nextDest = peerlist[0];
   txidTable[txID].dest = peerlist[0];
   std::string path = m_name+"|" +peerlist[0];
   if (payer) {
      nextRH = peerlist[1];
      path += "|" +peerlist[1]; 
   }
   else    txidTable[txID].dest = "RH2";
   m_onTx(path, txID, payer, amount);
   ns3::Simulator::Schedule(Seconds(0.0), &BLANCpp::sendHoldPacket, this,txID, amount);
   //sendHoldPacket(txID, amount);
   TiP = true;
}

void
BLANCpp::reset(uint32_t txID)
{
   //std::cout<<"Good bye tx "<<m_name<<std::endl;
   //Determine number of segments and create the needed txIDs.
   m_amount = 0;
   m_payer = false;
   if(DEBUG3 || txID == TD)
      std::cout<<"Reset at " << m_name << "Erasing entry"<<std::endl;   
   txidTable.erase(txID);
   m_txID = 0;
   nextRH = "";
   TiP = false;
}

uint32_t
BLANCpp::createTxID(uint32_t txID){
   return txID + rand()%10;
}

void 
BLANCpp::sendAdvertPacket()
{
   AdvertSetup();
   double delay = KeyGenB()+SignB();
   //TODO: Add as a variable
   //int hopMax = 12;
   //if(method2) hopMax = 5;
   int band = 3;
   for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      if(SplitString(it->first, '|').size()!=1) continue;
      std::string payload = m_name;
      payload += "|10000000000|";
      payload += std::to_string(neighborTable[it->first].costTo) + "|0|";
      payload += std::to_string(m_hopMax) + "|";
      if(method2) {
         payload += std::to_string(band) + "|";
         payload += m_name + "|";      
      }
      //TODO: Add timestamp
      payload += "now";
      
      blancHeader packetHeader;
      packetHeader.SetPacketType(Advert);
      //TODO: Add signature
      if (DEBUG1) std::cout<<"Node "<<m_name<<" sending to"<<it->first<<std::endl;
      forwardPacket(packetHeader, it->first, payload, delay);
   }
}

void 
BLANCpp::sendAdvertReply(std::vector<std::string> items)
{
   if(!method2) return;
   AdvertSetup();

   std::string recvMax = items[1];
   std::string sendMax = items[2];
   std::string route = items[6];
   std::string nextRoute = SplitString(route, ',')[ SplitString(route, ',').size()-1 ];

   if (nextRoute.size()>0){
      std::string payload = m_name;
      payload += "|100000000000|";
      payload += std::to_string(neighborTable[nextRoute].costTo) + "|0|";
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
BLANCpp::sendRegPacket(std::string dest){

}

void
BLANCpp::sendHoldPacket(uint32_t txID, double amount)
{
   double delay = Sign()+Verify();
   //TODO Add preimage to packet and check it at onHold
   //TODO add path via Onion encryption to protect
   m_amount = amount;
   std::string dest = txidTable[txID].nextDest;
   txidTable[txID].pending = amount;
   //debug = (txID == TD);
   std::string nextHop, result = findNextHop(dest, amount, m_payer, "");
   //debug = false;
   nextHop = SplitString(result, '|')[0];
   if (nextHop == "") {
         //std::cout<<"Node "<<m_name<<" to RH "<< txidTable[txID].nextDest <<" failed sending out Hold packet\n";
         m_onTxFail(txID);         
      reset(txID);
      return; 
   }
   double hopCount = std::stod(SplitString(result, '|')[1]);

   std::string te1 = "30", te2 = "100"; //TODO:: replace with class variable

      if(DEBUG2) std::cout<<"Node "<<m_name<<" "<< nextHop <<" sending out Hold packet\n";

   blancHeader packetHeader;
   packetHeader.SetPacketType(HoldRecv);

   if(m_payer){
      packetHeader.SetPacketType(Hold);
      updatePathWeight(nextHop, amount, true);
      m_txID = txID;
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
   //payload += "|"+m_name+"||";


   int timeout = 5;
   insertTimeout(txID, nextHop, payload, std::stod(te1)*hopCount, std::stod(te2)*hopCount, true);

   forwardPacket(packetHeader, nextHop, payload, delay);
   if (DEBUG2) std::cout<<"Hold phase\n\n\n";
}

void 
BLANCpp::sendHoldReply(uint32_t txID, double delay)
{
   blancHeader packetHeader;
   packetHeader.SetPacketType(HoldReply);

   std::string payload = std::to_string(txID);
   forwardPacket(packetHeader, txidTable[txID].src, payload, delay);
}

void 
BLANCpp::sendProceedPay(uint32_t txID, double delay)
{
   blancHeader packetHeader;
   packetHeader.SetPacketType(HoldReply);

   std::string payload = std::to_string(txID)+"|ProceedPay";
   forwardPacket(packetHeader, txidTable[txID].src, payload, delay);
}

void 
BLANCpp::sendPayReply(uint32_t txID)
{
   //TODO Add preimage to packet and check it at onHold
   //TODO add destinations
   if(DEBUG3 || txID == TD) std::cout<<"Sending Pay Reply: "<<m_name<<" "<<txidTable[txID].dest<<"\n\n";

   blancHeader packetHeader;
   packetHeader.SetPacketType(PayReply);

   std::string payload = std::to_string(txID);

   forwardPacket(packetHeader, txidTable[txID].src, payload, 0);
   //txidTable[txID].src = NULL;
}

void 
BLANCpp::sendPayPacket(uint32_t txID)
{
   if(DEBUG3) std::cout<<"Pay phase: "<<m_name<<" "<<txidTable[txID].nextHop<<"\n\n\n";

   std::string nextHop = txidTable[txID].nextHop;
   //txidTable.erase(txID);

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
BLANCpp::sendNack(TransactionInfo* entry, uint32_t txID, double te1, std::string dest, double delay, bool RHreject){
   blancHeader packetHeader;
   packetHeader.SetPacketType(Nack);
   std::string payload;
   if ( dest == m_name || RHreject)
      payload = std::to_string(txID)+"|RHReject|"+std::to_string(te1)+"|"+dest;
   else
      payload = std::to_string(txID)+"|No Route|"+std::to_string(te1)+"|"+dest;

   std::string attempedPaths = txidAttempts[txID];

   payload += "|" + m_name + "/" + attempedPaths;      
   forwardPacket(packetHeader, entry->src, payload, delay);
   
   if ( entry == &txidTable[txID]) { 
      if(DEBUG3 || txID == TD)
      std::cout<<"Sent Nack at " << m_name << "Erasing entry"<<std::endl;   
      txidTable.erase(txID);
   }
   
   else if(overlapTable.find(txID) != overlapTable.end()){
      overlapTable[txID].pop_back();
   }    
}

void 
BLANCpp::sendPreNack(std::string src, uint32_t txID, double te1, std::string dest, double delay, bool RHreject) {
   blancHeader packetHeader;
   packetHeader.SetPacketType(Nack);
   std::string payload;
   if ( dest == m_name || RHreject)
      payload = std::to_string(txID)+"|RHReject|"+std::to_string(te1)+"|"+dest;
   else
      payload = std::to_string(txID)+"|No Route|"+std::to_string(te1)+"|"+dest;

   std::string attempedPaths = txidAttempts[txID];

   payload += "|" + m_name + "/" + attempedPaths;   
   forwardPacket(packetHeader, src, payload, delay); 
}

void 
BLANCpp::insertTimeout(uint32_t txID, std::string src, std::string payload, double te1, double te2, bool sender){
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
   if(DEBUG2 || txID == TD) std::cout<<txID<<"  " <<m_name<<"   "<<payload<<"  "<< txidTable[txID].dest << std::endl;
   entry->payload = payload; 
   entry->sender = sender;
   entry->te1 = ns3::Simulator::Now().GetSeconds() + te1;
   entry->te2 = ns3::Simulator::Now().GetSeconds() + te2;
}

void 
BLANCpp::checkTimeout(){
   for (auto each = txidTable.begin(); each != txidTable.end(); each++ ){
      if (each->second.te1 <= ns3::Simulator::Now().GetSeconds() && !each->second.replied ) {
         //std::cout<<m_name<<"  "<<each->second.te1<<"  Bruh\n";
         //checkTe1(each->first, each->second.nextHop, each->second.payload, 5, each->second.sender);
      }
      if (each->second.te2 <= ns3::Simulator::Now().GetSeconds() && !each->second.payReplied) {
         //std::cout<<m_name<<"  "<<each->second.te1<<"  .... really?\n";
         //checkTe2(each->first, each->second.nextHop, each->second.sender);
      }
   }
   ns3::Simulator::Schedule(Seconds(0.001), &BLANCpp::checkTimeout, this);
}

void 
BLANCpp::checkTe1(uint32_t txID, std::string src, std::string payload, double te1, bool sender){
   if (txidTable[txID].replied) return;
   return;
   updatePathWeight(src, (-1 * txidTable[txID].pending), sender);
   updateRoutingTable(src, txidTable[txID].pending, sender);
   if(txidTable[txID].onPath){
      //updateRHRTableWeight(txidTable[txID].nextDest, (-1 * txidTable[txID].pending/5));
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
BLANCpp::onNack(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s){
   std::string payload = readPayload(p);

   std::vector<std::string> items = SplitString(payload, '|');
   uint32_t txID = std::stoi(items[0]);
   double te1 = std::stoi(items[2]);
   std::string dest = items[3];

   if(txID == TD || DEBUGNACK){
      std::cout << "Nack----- "<< txID << "  " << m_name <<"  NodeID "<< ns3::Simulator::Now().GetSeconds()<<"\n"<<std::endl<<std::endl;
      std::cout<< "  nextDest: " << dest << " dest: " << txidTable[txID].dest <<std::endl;
   }

   TransactionInfo* entry = NULL;
   if (txidTable[txID].dest == dest || txidTable[txID].dest == m_name) { 
      entry = &txidTable[txID];
   }
   
   else if(overlapTable.find(txID) != overlapTable.end()){
      for (auto each = overlapTable[txID].begin(); each != overlapTable[txID].end(); each++){
         if(each->dest == dest || each->dest == m_name ) {
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
   /*for (auto it = neighborTable.begin(); it != neighborTable.end(); it++){
      if (it->second.socket == s)
         src = it->first; 
   }*/

   //TODO update from nack values
   updatePathWeight(src, (-1 * entry->pending), entry->sender);
   updateRoutingTable(src, entry->pending, entry->sender);
   if(entry->onPath){
      //updateRHRTableWeight(dest, (-1 * entry->pending/5));
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
   //SplitString(entry->payload,'|')[6] + 
   //if (items.size() >= 5)
   //   routeList + items[4];

   if(txID == TD || DEBUGNACK) std::cout<<"No sending to " << routeList <<std::endl;
   std::string result = findNextHop(dest, entry->pending, entry->sender, routeList); 

   std::string nextHop = SplitString(result, '|')[0];

   payload = entry->payload;
   /*std::vector<std::string> pitems = SplitString(entry->payload,'|');
   payload = pitems[0] + "|";
   payload += pitems[1] + "|";
   payload += pitems[2] + "|";
   payload += pitems[3] + "|";
   payload += pitems[4] + "|";
   payload += pitems[5] + "|";
   payload += routeList + "|";
   if ( pitems.size() > 8 )    
      payload += pitems[7];
   */

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
            sendNack(entry, txID, te1, m_name, 0, true);
         return;
      }

         //std::cout<<"Payload: " << entry->payload << std::endl;;
      dest = SplitString(path, ',')[0];
      attempted_paths[txID].push_back(dest);

      //debug = (txID == TD);
      result = findNextHop(dest, entry->pending, entry->sender, ""); 
      //debug = false;
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
      //payload += "|"+m_name;
      //if (items.size() >= 8 ) 
      //   payload += "|" + items[7];      

   }

   if (nextHop == "" || entry->retries >= m_maxRetires) {
      if (dest == m_name) {
         dest = entry->dest; 
      }
      if(m_txID == txID) {
         m_onTxFail(txID);
         //reset(txID);
      }
      else {
         std::string lasHop = findSource(entry->src);
         if(BALANCE) updatePathWeight(lasHop, (entry->pending), entry->sender);
         sendNack(entry, txID, te1, dest, 0, false);
         failedTxs[std::to_string(txID) + "|" + dest ] = true;
         //txidTable.erase(txID);
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
BLANCpp::checkTe2(uint32_t txID, std::string src, bool sender){
   if (txidTable.find(txID) == txidTable.end() ) return;

   updatePathWeight(src, (-1 * txidTable[txID].pending), sender);
   if(DEBUG3 || txID == TD)
      std::cout<<"Timeout at " << m_name << "Erasing entry"<<std::endl;   
   txidTable.erase(txID);
}

void 
BLANCpp::send(Ptr<Socket> s, Ptr<Packet> p){
   //std::cout<<"Lets go "<<ns3::Simulator::Now()<<std::endl;

   Ptr<Packet> pkt  = p->Copy();   
   blancHeader packetHeader;
   pkt->RemoveHeader(packetHeader);
   uint32_t pType = packetHeader.GetPacketType();   
   uint8_t buffer[p->GetSize()];
   pkt->CopyData (buffer,  pkt->GetSize());

   std::ostringstream convert;
   for (int a = 0; a < pkt->GetSize(); a++) {
      convert << (char)buffer[a];
   }


   std::string payload = convert.str();
   //std::cout << pType <<"  "<<payload <<"  payload "<<std::endl;

   if(lastSent  == Simulator::Now().GetSeconds()){
      Simulator::Schedule(Seconds(0.000001), &BLANCpp::send, this, s, p);
      return;
   }

   lastSent  = Simulator::Now().GetSeconds();
   s->Send(p);
};

std::string 
BLANCpp::readPayload(Ptr<Packet> p){
   uint8_t buffer[p->GetSize()];
   p->CopyData (buffer,  p->GetSize());

   std::ostringstream convert;
   for (int a = 0; a < p->GetSize(); a++) {
      convert << (char)buffer[a];
   }
   return convert.str();
}

std::string 
BLANCpp::findSource(Ptr<Socket> s){
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
BLANCpp::checkOverlap(uint32_t txID, std::string dest, bool sender){
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
BLANCpp::getRH(std::string RH1){
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
   //std::cout<<m_name<<":: "<<RH<<"  Hops:" <<minHops<<std::endl;
   return RH;
}

std::vector<std::string> 
BLANCpp::getReachableRHs(){
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
      //std::cout<<"Node " <<m_name<<" "<<i->first <<" creating table...\n";
      RHs.push_back(entry);
   }
   return RHs;
}

void 
BLANCpp::setRHTable(std::unordered_map<std::string, std::vector<std::string>> RHlist){
   if (DEBUG2) std::cout<<"Node " <<m_name<<" creating table...\n";
   for (auto i = RHlist.begin(); i != RHlist.end(); i++){
      if (RoutingTable.find(i->first) != RoutingTable.end() || i->first == m_name) {
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
BLANCpp::matchUpNonces(std::unordered_map<std::string, std::vector<std::string>> RHlist){
   for (auto i = RHlist.begin(); i != RHlist.end(); i++){
      if (RoutingTable.find(i->first) != RoutingTable.end() || i->first == m_name) {
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
               RHlist[m_name].push_back(rh);
               break;
            }
         }
      }
   }
   return RHlist[m_name];
}

std::string 
BLANCpp::findNextHop(std::string dest, double amount, bool send, std::string routeList){
   std::vector<std::string> seenNodes;
   if(routeList != "") seenNodes = SplitString(routeList, '/');
   std::unordered_map<std::string, bool> seenNode_map;
   for (int i = 0; i < seenNodes.size(); i++){
      seenNode_map[ seenNodes[i] ] = true;
   }
   std::vector<RoutingEntry*> options; 
   for (auto i = RoutingTable[dest].begin(); i != RoutingTable[dest].end(); i++){
      if (debug) std::cout<<dest<<" | "<< i->nextHop <<" | " <<i->sendMax<< " | " << i->recvMax <<std::endl;
      //if (i->nextHop == dest && seenNode_map [i->nextHop])  return "|10000";
      if( seenNode_map [i->nextHop] && i->nextHop != dest) continue;
      if (debug) std::cout<<"Not seen"<<std::endl;
      if (send &&   i->sendMax >= amount &&  neighborTable[i->nextHop].costTo >= amount)
         options.push_back(&(*i));
      else if (!send && i->recvMax >= amount &&  neighborTable[i->nextHop].costTo >= amount)
         options.push_back(&(*i));
   }
   if (options.size() == 0){
      if (debug) std::cout<<"Noting was there\n"<<std::endl;
      for (auto i = RoutingTable[dest].begin(); i != RoutingTable[dest].end(); i++){
         if (debug) std::cout<<dest<<" | "<< i->nextHop <<" | " <<neighborTable[i->nextHop].costTo<< " | " << neighborTable[i->nextHop].costFrom <<std::endl;
         if( seenNode_map [i->nextHop] && i->nextHop != dest) 
            continue;
         if (debug) std::cout<<"Not seen"<<std::endl;
         if (send &&  neighborTable[i->nextHop].costTo >= amount  )//&&   i->sendMax >= amount)
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
      /*else if (i->hopCount == leastHops){
         double itemAmount = i->recvMax;
         if (send) itemAmount = i->sendMax;
         if (itemAmount > amount) {
            bestNextHop = i->nextHop;
            bestAmount = itemAmount;
         }
      }*/
   }
   /*if (bNHop != NULL){
      if (send) bNHop->sendMax-=amount;
      else bNHop->recvMax-=amount;
   }*/
   return bestNextHop + "|" + std::to_string(leastHops);
}

//TODO:: Revist this function for optimization
std::string 
BLANCpp::createPath(std::string dest, double amount, std::vector<std::string> attempt_list, bool RH1){
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
      /*if(NonceTable[dest].size() > 0){
         std::cout<<"Yeah I doubt it..\n";
         complete = true;
         path = NonceTable[dest][0] + "," + path;
         continue;
      }*/
      RHRoutingEntry* bestOption = NULL;
      double maxAmount = 0;
      for (auto i = RHRoutingTable[newDest].begin(); i != RHRoutingTable[newDest].end(); i++){
         if ( attempted_map[ i->path ] ||i->path == m_name || i->path == dest)
            continue;
         //if ( i->maxSend >= amount){
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
      //bestOption->maxSend-=(amount/10.0);
      path = bestOption->path + "," + path;
      newDest = bestOption->path;
      //dest = newDest;
   }
      if(DEBUG3) std::cout<<"This is the Path I found "<<path<<std::endl;
   return path;
}

} // Namespace ns3


