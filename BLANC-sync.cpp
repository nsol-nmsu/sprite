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
   if(senders[0]->getTiP() == true){
      return;
   }
   uint32_t txID = rand()%10000;
   uint32_t secret = rand()%10000;
   std::cout<<"Here I is "<<txID<<" "<<secret<<std::endl;

   txIDmap[txID].push_back(0);
   txIDmap[txID].push_back(6);
   FRMap[txID].push_back(false);
   FRMap[txID].push_back(false);
   HRMap[txID] = false;
   HoldMap[txID] = false;
   std::vector<string> plist;
   plist.push_back("4");

   T_SMap[secret] = txID;

   senders[6]->startTransaction(txID, secret, plist, false);
   plist.clear();
   plist.push_back("2");
   plist.push_back("4");
   senders[0]->startTransaction(txID, secret, plist, true);
}

void
BLANCSync::addSender( int node, Ptr<BLANCpp> sender ) {
   senders[node] = sender;
}



//TODO add amount
void
BLANCSync::onFindReplyPacket( uint32_t node, uint32_t txID, double amount){
   //if (txIDmap[txID][0] == node)
   txID = T_SMap[txID]; 

   if (1 == node)
      FRMap[txID][0] = true;
   //else if (txIDmap[txID][1] == node)
   else if (2 == node)
      FRMap[txID][1] = true;

   if (FRMap[txID][0] == true && FRMap[txID][1] == true){
      uint32_t node1 = txIDmap[txID][0];
      uint32_t node2 = txIDmap[txID][1];
      
      senders[node1]->sendHoldPacket(txID, amount);
      senders[node2]->sendHoldPacket(txID, amount);
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
       senders[node1]->sendPayPacket(txID);
   }

}

void
BLANCSync::onPayPacket( uint32_t node, uint32_t txID){
  if(txIDmap[txID][1] == node){
     uint32_t node1 = txIDmap[txID][0];
     uint32_t node2 = txIDmap[txID][1];

     senders[node1]->reset(txID);
     senders[node2]->reset(txID);

     std::cout<<"All finished\n";
  } 
}

}
}