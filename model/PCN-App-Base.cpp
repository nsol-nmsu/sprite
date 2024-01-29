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
 
 NS_LOG_COMPONENT_DEFINE ("ns3.PCN_App_Base");
 NS_OBJECT_ENSURE_REGISTERED (PCN_App_Base);
 
 TypeId
 PCN_App_Base::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::PCN_App_Base")
     .SetParent<Application> ()
     .AddConstructor<PCN_App_Base> ()                      
   ;
   return tid;
 }
 
 
PCN_App_Base::PCN_App_Base () {
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
   m_running = false;
}
 
PCN_App_Base::~PCN_App_Base() {
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
}
 
void
PCN_App_Base::DoDispose (void) {
   NS_LOG_FUNCTION_NOARGS ();
   Application::DoDispose ();
}
 
void
PCN_App_Base::StartApplication (void) {
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
         MakeCallback (&PCN_App_Base::HandleAccept, this));

}
 
void
PCN_App_Base::StopApplication () {
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
PCN_App_Base::ReceivePacket (Ptr<Socket> s) {
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
 
void 
PCN_App_Base::HandleAccept (Ptr<Socket> s, const Address& from) {
   NS_LOG_FUNCTION (this << s << from);
   s->SetRecvCallback (MakeCallback (&PCN_App_Base::ReceivePacket, this));
   s->SetCloseCallbacks(MakeCallback (&PCN_App_Base::HandleSuccessClose, this),
     MakeNullCallback<void, Ptr<Socket> > () );
}
 
void 
PCN_App_Base::HandleSuccessClose(Ptr<Socket> s) {
   NS_LOG_FUNCTION (this << s);
   NS_LOG_LOGIC ("Client close received");
   s->Close();
   s->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > () );
   s->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),
       MakeNullCallback<void, Ptr<Socket> > () );
}

std::string 
PCN_App_Base::findSource(Ptr<Socket> s){
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

void 
PCN_App_Base::onRegPacket(Ptr<Packet> p, blancHeader ph, Ptr<Socket> s){
   std::string payload = readPayload(p);

   //TODO: Add debug flag
   if(false) std::cout<<"Node "<<m_name<<" registering socket for "<<payload<<std::endl;
    
   if (neighborTable[payload].socket == 0)
      neighborTable[payload].socket = s;
   else 
      neighborTable[payload+"|T"].socket = s;
}

void 
PCN_App_Base::forwardPacket(blancHeader packetHeader, std::string nextHop, std::string payload, double delay)
{

  Ptr<Packet> p;
  p = Create<Packet> (reinterpret_cast<const uint8_t*> (payload.c_str()), payload.size());  
  //p = Create<Packet> (100);
    packetHeader.setPayloadSize(payload.size());

  uint64_t m_bytesSent = 100;
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well

  //Add PCN_App_Base information to packet header and send
  Address peerAddress = neighborTable[nextHop].address;
  Ptr<Socket> socket = getSocket(nextHop);

  p->AddHeader(packetHeader);

  Simulator::Schedule(Seconds(delay), &PCN_App_Base::send, this, socket, p);
  //send(socket, p);
  //socket->Send (p);
  NS_LOG_INFO ("Sent " << 100 << " bytes to " << peerAddress << ":" << m_peerPort);

  //Increase sequence number for next packet
  m_seq = m_seq + 1;
}

void
PCN_App_Base::forwardPacket(blancHeader packetHeader, Ptr<Socket> socket, std::string payload, double delay)
{

  Ptr<Packet> p;
  p = Create<Packet> (reinterpret_cast<const uint8_t*> (payload.c_str()), payload.size());
  packetHeader.setPayloadSize(payload.size()); 

  uint64_t m_bytesSent = 100;

  //Add PCN_App_Base information to packet header and send
  p->AddHeader(packetHeader);
   
  Simulator::Schedule(Seconds(delay), &PCN_App_Base::send, this, socket, p);

  //Increase sequence number for next packet
  m_seq = m_seq + 1;
}

void 
PCN_App_Base::send(Ptr<Socket> s, Ptr<Packet> p){

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

   if(lastSent == Simulator::Now().GetSeconds()){
      Simulator::Schedule(Seconds(0.000001), &PCN_App_Base::send, this, s, p);
      return;
   }

   lastSent = Simulator::Now().GetSeconds();
   s->Send(p);
};

void 
PCN_App_Base::setNeighborCredit(std::string name, double amount){
   neighborTable[name].cost = amount;
}

void
PCN_App_Base::setNeighbor(std::string name, Ipv4Address address){
   neighborTable[name].address = address;
}

Ptr<Socket> 
PCN_App_Base::getSocket(std::string dest)
{
 Ptr<Socket> socket = neighborTable[dest].socket;
	
 if (socket == 0)
 {
   TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");   
   socket = Socket::CreateSocket (GetNode (), tid);
   socket->Bind();
   socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom( neighborTable[dest].address ), m_peerPort));

   socket->SetRecvCallback (MakeCallback (&PCN_App_Base::ReceivePacket, this));

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

std::string 
PCN_App_Base::readPayload(Ptr<Packet> p){
   uint8_t buffer[p->GetSize()];
   p->CopyData (buffer,  p->GetSize());

   std::ostringstream convert;
   for (int a = 0; a < p->GetSize(); a++) {
      convert << (char)buffer[a];
   }
   return convert.str();
}

} // Namespace ns3