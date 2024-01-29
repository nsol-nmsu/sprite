/*
 * Copyright ( C ) 2020 New Mexico State University
 *
 * George Torres, Anju Kunnumpurathu James
 * See AUTHORS.md for complete list of authors and contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * ( at your option ) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <algorithm>

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include "BLANC-sync.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

using namespace std;

namespace ns3 {
namespace ndn {

// On object construction create sockets and set references
BLANCSync::BLANCSync() {

}

void
BLANCSync::syncEvent(){
   Synchronizer::syncEvent();
   injectPackets( );
}

void
BLANCSync::injectPackets( ) {
//Go through list and find two apps not currently in a transaction. 
int sender, reciver;
std::string rh1 = "", rh2 = "";
if (limit==0) return;

   limit--;
   int txID = m_txID;
   m_txID++;

if(hasMap){
   std::vector<std::string> txInfo = SplitString(txMap[txID],'|');
   sender = std::stoi(txInfo[0]); 
   reciver = std::stoi(txInfo[1]);
   
      int c1 = rand()%routingHelpers.size();
      auto it1 = routingHelpers.begin();
      int c2 = c1;
      while (c2 == c1)
         c2 = rand()%routingHelpers.size();
      auto it2 = routingHelpers.begin();      
      while (c1 > 0){
         it1++;            
         c1--;
      }
      while (c2 > 0){
         it2++;            
         c2--;
      }      
      rh1 = it1->second->getName();
      rh2 = it2->second->getName();

   if(nodes[sender]->getTiP() || nodes[reciver]->getTiP()){
      //std::cout<<sender<<" "<<reciver<<" "<<txID<<std::endl;
      //if(nodes[sender]->getTiP())std::cout<<"Sender\n";
      //if(nodes[reciver]->getTiP())std::cout<<"Reciver\n";
      limit++;
      m_txID--;
      return;
   }
}
else{
      int count = 0;
   vector<int> options;
   for(auto i = senders.begin(); i != senders.end(); i++){
      if (!nodes[i->first]->getTiP() && routingHelpers.find(i->first) == routingHelpers.end()){
         options.push_back(i->first);
      }
   }
   vector<int> pair;
   while (pair.size() < 2 && options.size() > 1){
      uint32_t choice = rand()%options.size();

      if ( count == 0 ) {   
         int c = rand()%routingHelpers.size();
         auto it = routingHelpers.begin();
         while (c > 0){
            it++;            
            c--;
         }
         rh1 = it->second->getName();
         count++;
         pair.push_back(options[choice]);
      }

      else if (rh1 != rh2 && rh2 != "") {
         count++;
         pair.push_back(options[choice]);
      }
      else{
         int c = rand()%routingHelpers.size();
         auto it = routingHelpers.begin();
         while (c > 0){
            it++;            
            c--;
         }         
         rh2 = it->second->getName();
         continue;
      }

      vector<int> copy = options;
      options.clear();
      for (int i = 0; i < copy.size(); i++){
         if(i != choice){
            options.push_back(copy[i]);
         }
      }
      
   }
   if (pair.size() != 2) {
      return;
   }

   sender = pair[0];
   reciver = pair[1];
}

   uint32_t secret = txID + 10000000;
   txIDmap[txID].push_back(sender);
   txIDmap[txID].push_back(reciver);
   FRMap[txID].push_back(false);
   FRMap[txID].push_back(false);
   HRMap[txID] = false;
   HoldMap[txID] = false;
   std::vector<string> plist;
   plist.push_back(rh2);

   T_SMap[secret] = txID;

   nodes[reciver]->startTransaction(txID, secret, plist, false);
   plist.clear();
   plist.push_back(rh1);
   plist.push_back(rh2);
   nodes[sender]->startTransaction(txID, secret, plist, true);
	Simulator::Schedule( Seconds( 5.0 ), &BLANCSync::checkFail, this, txID );
   
}

void
BLANCSync::addNode( int node, Ptr<Blanc> app ) {
   nodes[node] = app;
}

void
BLANCSync::addSender( int node){
   senders[node] = nodes[node];
}

void
BLANCSync::addRH( int node, Ptr<Blanc> routingHelper ){
   routingHelpers[node] = routingHelper;
}



//TODO Change to Phase switching
void
BLANCSync::onFindReplyPacket( uint32_t node, uint32_t txID, double amount){
   txID = T_SMap[txID]; 
   if(FRMap[txID][0] == true && FRMap[txID][1] == true){//Hold is already begun
      return;
   }
   if (txIDmap[txID][0] == node)
      FRMap[txID][0] = true;
   //else if (txIDmap[txID][1] == node)
   else if (txIDmap[txID][1] == node)
      FRMap[txID][1] = true;

   if (FRMap[txID][0] == true && FRMap[txID][1] == true){
      uint32_t node1 = txIDmap[txID][0];
      uint32_t node2 = txIDmap[txID][1];
      if(!hasMap)
         amount = std::min(rand()%14+1, int(amount/2));
      else {
         std::vector<std::string> txInfo = SplitString(txMap[txID],'|');
         amount = std::min(std::stod(txInfo[2]), amount);
      }

      if(amount == 0)amount++;

      nodes[node1]->sendHoldPacket(txID, amount);
      nodes[node2]->sendHoldPacket(txID, amount);
   }
}

void
BLANCSync::onHoldPacket( uint32_t node, uint32_t txID, bool received){
   if (received) 
      HRMap[txID] = true;
   else
      HoldMap[txID] = true;

   if (HRMap[txID] == true && HoldMap[txID] == true){
      uint32_t node1 = txIDmap[txID][0];
       nodes[node1]->sendPayPacket(txID);
   }

}

void
BLANCSync::onPayPacket( uint32_t node, uint32_t txID){
  if(txIDmap[txID][1] == node){
     uint32_t node1 = txIDmap[txID][0];
     uint32_t node2 = txIDmap[txID][1];

     nodes[node1]->reset(txID);
     nodes[node2]->reset(txID);

     //std::cout<<"All finished\n\n\n";
  } 
}

}
}
