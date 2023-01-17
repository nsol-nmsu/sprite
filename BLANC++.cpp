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
 #include "ns3/socket.h"
 #include "ns3/simulator.h"
 #include "ns3/socket-factory.h"
 #include "ns3/packet.h"
 #include "ns3/uinteger.h"
 #include "ns3/string.h"
 #include "ns3/trace-source-accessor.h"
 #include "ns3/ipv4.h"
 #include "BLANC++.hpp"
 
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
   ;
   return tid;
 }
 
 
BLANCpp::BLANCpp ()
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
   m_running = false;
   m_subscription = 0;
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

   while (packet = s->RecvFrom (from))
     {
       if (packet->GetSize () > 0)
         {

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

	  processPacket(packet);

	} 
	
      //TODO: Alter
      if (false){ 
     	  NS_LOG_LOGIC ("Sending reply packet " << packet->GetSize ());
	  //Keep original packet to send to trace file and get correct size
          Ptr<Packet> packet_copy = Create<Packet> (m_packet_size);
     	  s->Send (packet_copy);
      }
          // Callback for received packet
    	  m_receivedPacket (GetNode()->GetId(), packet, from, m_local_port, m_subscription, m_local_ip);
      
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


 //SECTION
void BLANCpp::onHoldPacket(Ptr<Packet> p, blancHeader ph)
 {
   //Get all values from packet
   std::string src = "";//REPLACE	 
   std::string dest = std::to_string(ph.GetDest());
   uint32_t oldTxID = ph.GetTIDPrime();
   uint32_t txID = ph.GetTID();
   uint64_t timeout = 10000;
   double amount = ph.GetAmount();
   std::string nextHop = "";
  

   std::cout<<"Holddd\n";

   if (pendingTx.find(txID) != pendingTx.end()) {
      if(hasHoldRecv(oldTxID)) {
         updatePathWeight(src, amount);
         insertTimeout(txID, src, timeout);
	 updateBCHold(txID);
         m_onHold(std::stoi(m_name), txID, false);	 
      }
      return;
   }

   if(m_name != dest){
      nextHop = nextHopTable[oldTxID];
      nextHopTable.erase(oldTxID);
      nextHopTable[txID] = nextHop;
   }

   pendingTx[txID] = amount;

   updatePathWeight(src, amount);
   insertTimeout(txID, src, timeout);

   uint32_t nextTxID = oldTxID;
   //Create next hold packet
   //Ptr<Packet> packet_copy = Create<Packet> (m_packet_size);
   if(m_route_helper && m_name == dest){

      //Update Transaction Table
      //txIDTable[txID] = txIDTable[oldTxID];	   
      nextTxID = txIDTable[oldTxID];
      txIDTable.erase(oldTxID);

      //Update next hop and Router Helper Destination
      nextHop = nextHopTable[nextTxID];
      nextHopTable.erase(nextTxID);
      nextHopTable[txID] = nextHop;
      dest = nextRHTable[nextTxID];
      nextRHTable.erase(nextTxID);
      nextRHTable[txID] = dest;

      //Send out BC message, affirming hold.
      updateBCHold(txID);
   }

   blancHeader packetHeader;
   packetHeader.SetPacketType(1);
   packetHeader.SetDest(std::stoi(dest));
   packetHeader.SetTIDPrime( nextTxID );
   packetHeader.SetTID( txID  );
   packetHeader.SetAmount( amount );

   //Forward next hold packet
   forwardPacket(packetHeader, nextHop);
   

   //Send out BC message, all only do in case of timeout. 
   //updateBCHold(txID);
   
 }

void BLANCpp::onFindPacket(Ptr<Packet> p, blancHeader ph){

   //Get all values from packet
   std::string src = "";//REPLACE
   std::string dest = std::to_string(ph.GetDest());
   std::string nxtDest = std::to_string(ph.GetNxtDest());
   uint32_t txID = ph.GetTID();
   uint32_t secret = ph.GetTIDPrime();
   uint64_t timeout = 10000;
   double amount = ph.GetAmount();
   uint32_t type = ph.GetPacketType();
   std::string nextHop = "";

   if(m_name != dest){
      nextHop = findRHTable[dest];
      nextHopTable[txID] = nextHop;
   }

   insertTimeout(txID, src, timeout);

   //Create next find packet
   //Ptr<Packet> packet_copy = Create<Packet> (m_packet_size);
   if(m_route_helper && m_name == dest){

      if ( txIDTable.find(secret) != txIDTable.end() ){
         txIDTable[txID] = txIDTable[secret];
	 txIDTable[ txIDTable[secret] ] = txID;
	 txIDTable.erase(secret);
      }	  
      else {
         txIDTable[secret] = txID;
      }

      sendFindReply(txID);      
      if (type == 0){

         //Update Transaction Table
         uint32_t nextTxID = createTxID(txID);
   	 txIDTable[txID] = nextTxID;
    	 txID = nextTxID;

   	 type = 10;

   	 //Update next hop and Router Helper Destination  
	 nextHop = findRHTable[nxtDest];
   	 nextHopTable[txID] = nextHop;
   	 dest = nxtDest;
   	 nextRHTable[txID] = dest;
      }
      else {
         //Send out BC message, affirming hold. TODO: Confirm if all do this or only RHs.
         updateBCFind(txID);	     
	 //TODO move to find reply
	 if(amount > costTable[dest]) amount = costTable[dest];
	 if (type == 10)
	    m_onFindReply(1, secret, amount);
	 else
            m_onFindReply(2, secret, amount);

         return;
      }
   }

   blancHeader packetHeader;
   packetHeader.SetPacketType(type);
   packetHeader.SetDest(std::stoi(dest));
   if(type == 0)
      packetHeader.SetNxtDest(std::stoi(nxtDest));
   packetHeader.SetTID( txID  );
   packetHeader.SetTIDPrime( secret );

   if(amount > costTable[dest]) amount = costTable[dest];
   packetHeader.SetAmount( amount );

   //Forward next hold packet
   forwardPacket(packetHeader, nextHop);


   //Send out BC message, affirming hold. TODO: Confirm if all do this or only RHs.
   updateBCFind(txID);
}



void BLANCpp::onHoldRecvPacket(Ptr<Packet> p, blancHeader ph)
 { 
   //Get all values from packet
   std::string src = "";//REPLACE
   std::string dest = std::to_string(ph.GetDest());
   uint32_t oldTxID = ph.GetTIDPrime();
   uint32_t txID = ph.GetTID();
   uint64_t timeout = 10000;
   double amount = ph.GetAmount();
   std::string nextHop = "";

   std::cout<<"HoldRE\n";


   if (pendingTx.find(txID) != pendingTx.end()) return;

   if(m_name != dest){
      nextHop = nextHopTable[oldTxID];
      nextHopTable.erase(oldTxID);
      nextHopTable[txID] = nextHop;
   }

   pendingTx[txID] = amount;

   updatePathWeight(src, amount);
   insertTimeout(txID, src, timeout);

   //Create next hold packet
   //Ptr<Packet> packet_copy = Create<Packet> (m_packet_size);
   if(!m_route_helper || m_name != dest){

      blancHeader packetHeader;
      packetHeader.SetPacketType(2);
      packetHeader.SetDest(std::stoi(dest));
      packetHeader.SetTIDPrime( oldTxID );
      packetHeader.SetTID( txID  );
      packetHeader.SetAmount( amount );

      //Forward next hold packet
      forwardPacket(packetHeader, nextHop);
   }
   else if (m_route_helper && m_name == dest){
      //Send out BC message, affirming hold. 
      updateBCHoldRecv(txID);
      m_onHold(std::stoi(m_name), txID, true);
   }
 }

void BLANCpp::onPayPacket(Ptr<Packet> p, blancHeader ph)
 {
   std::string src = "";//REPLACE
   std::string dest = std::to_string(ph.GetDest());
   uint32_t txID = ph.GetTID();
   uint64_t timeout = 10000;
   double amount = ph.GetAmount();

   if (pendingTx.find(txID) == pendingTx.end()) return;

   pendingTx.erase(txID);
   insertTimeout(txID, src, timeout);

   std::cout<<"Paying up " <<m_name<<std::endl;;


   if (nextHopTable.find(txID) != nextHopTable.end()){
      //Create next pay packet
      if(m_route_helper && m_name == dest ){
         //Update next hop and Router Helper Destination
         dest = nextRHTable[txID];
         nextRHTable.erase(txID);//TODO:Move to after BC confirmation 
	 //Send out BC message, affirming hold.
	 updateBCPay(txID);

      }

      std::string nextHop = nextHopTable[txID];
      nextHopTable.erase(txID);//TODO:Move to after BC confirmation

      //Forward next pay packet
      blancHeader packetHeader;
      packetHeader.SetPacketType(3);
      packetHeader.SetDest(std::stoi(dest));
      packetHeader.SetTID( txID  );
      packetHeader.SetAmount( amount );

      forwardPacket(packetHeader, nextHop);
   }

 }


void 
BLANCpp::forwardPacket(blancHeader packetHeader, std::string nextHop)
{

  Ptr<Packet> p;
  p = Create<Packet> (100);
  uint64_t m_bytesSent = 100;
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  //m_txTrace (p);

  //Add Blanc information to packet header and send
  Address peerAddress = addressTable[nextHop];
  Ptr<Socket> socket = getSocket(nextHop);

  p->AddHeader(packetHeader);

  
  socket->Send (p);
  NS_LOG_INFO ("Sent " << 100 << " bytes to " << peerAddress << ":" << m_peerPort);

  //Increase sequence number for next packet
  m_seq = m_seq + 1;
}


Ptr<Socket> 
BLANCpp::getSocket(std::string dest)
{
 Ptr<Socket> socket = m_sockets[ dest ];
	
 if (socket == 0)
 {
    TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");   
    socket = Socket::CreateSocket (GetNode (), tid);
    socket->Bind();
    socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom( addressTable[dest] ), m_peerPort));

    socket->SetRecvCallback (MakeCallback (&BLANCpp::ReceivePacket, this));
 }
 return socket;
}


bool
BLANCpp::hasHoldRecv(uint32_t txID)
{
   uint32_t txIDprime = txIDTable[txID];
   if ( txIDTable.find(txIDprime) != txIDTable.end() ){
      txIDTable.erase(txIDprime);
      txIDTable.erase(txID);
      return true;
   }
   return false;
}



//Transaction Functions
void 
BLANCpp::startTransaction(uint32_t txID, uint32_t secret, std::vector<std::string> peerlist, bool payer)
{
   //Determine number of segments and create the needed txIDs.
   std::string NH = findRHTable[peerlist[0]];
   m_amount = costTable[NH];
   m_payer = payer;
   txIDTable[txID] = createTxID(txID);
   m_txID = txIDTable[txID];
   nextRHTable[txID] = peerlist[0];
   if (payer) nextRH = peerlist[1];
   sendFindPacket(txID, secret);
   TiP = true;
}


void
BLANCpp::reset(uint32_t txID)
{
   //Determine number of segments and create the needed txIDs.
   m_amount = 0;
   m_payer = false;
   txIDTable.erase(txID);
   m_txID = 0;
   nextRHTable.erase(txID);
   nextRH = "";
   TiP = false;
}


uint32_t
BLANCpp::createTxID(uint32_t txID){
   return txID + rand()%10;
}

/*void 
BLANCpp::sendFindPacket(uint32_t txID, uint32_t secret)
{
   uint32_t txIDPrime = txIDTable[txID];
   std::string dest = nextRHTable[txID];
   std::string nextHop = findRHTable[dest]; //Update based on Some table base

   blancHeader packetHeader;
   packetHeader.SetPacketType(0);
   packetHeader.SetDest(std::stoi(dest));
   packetHeader.SetNxtDest(std::stoi(nextRH));
   packetHeader.SetTID( txIDPrime  );
   packetHeader.SetTIDPrime( secret );
   packetHeader.SetAmount( m_amount );

   forwardPacket(packetHeader, nextHop);
	
}*/

void
BLANCpp::sendFindPacket(uint32_t txID, uint32_t secret)
{
   uint32_t txIDPrime = txIDTable[txID];
   std::string dest = nextRHTable[txID];
   std::string nextHop = findRHTable[dest]; //Update based on Some table base
   nextHopTable[txID] = nextHop;

   blancHeader packetHeader;
   packetHeader.SetPacketType(4);
   if(m_payer)
      packetHeader.SetPacketType(0);
   packetHeader.SetDest(std::stoi(dest));
   if(m_payer)
      packetHeader.SetNxtDest(std::stoi(nextRH));
   packetHeader.SetTID( txIDPrime  );
   packetHeader.SetTIDPrime( secret );
   packetHeader.SetAmount( m_amount );

   forwardPacket(packetHeader, nextHop);

}

void 
BLANCpp::sendHoldPacket(uint32_t txID, double amount)
{
   uint32_t txIDPrime = txIDTable[txID];
   txIDTable.erase(txID);
   std::string dest = nextRHTable[txID];
   std::string nextHop = nextHopTable[txID];

    std::cout<<txID<<" "<<dest<<std::endl;

   blancHeader packetHeader;
   if(m_payer)
      packetHeader.SetPacketType(1);
   else 
      packetHeader.SetPacketType(2);
   packetHeader.SetDest(std::stoi(dest));
   packetHeader.SetTIDPrime( txIDPrime  );
   packetHeader.SetTID( txID );
   packetHeader.SetAmount( amount );

   forwardPacket(packetHeader, nextHop);
   std::cout<<"Holding on to you\n";
}


void 
BLANCpp::sendPayPacket(uint32_t txID)
{
   std::cout<<"Pay up hoe\n";

	
   std::string dest = nextRHTable[txID];
   nextRHTable.erase(txID);
   std::string nextHop = nextHopTable[txID];
   nextHopTable.erase(txID);

   blancHeader packetHeader;
   packetHeader.SetPacketType(3);
   packetHeader.SetDest(std::stoi(dest));
   packetHeader.SetTID( txID );
   packetHeader.SetAmount( m_amount );

   forwardPacket(packetHeader, nextHop);
}


 } // Namespace ns3


