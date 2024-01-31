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
#include "Sprite-sync.hpp"
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

// On object construction create sockets and set references
SpriteSync::SpriteSync() {

}

void
SpriteSync::syncEvent(){
   Synch::syncEvent();
   if(!m_tablesMade) createTables();
   injectPackets( );
}

bool DYNAMIC = true;
void
SpriteSync::injectPackets( ) {
//Go through list and find two apps not currently in a transaction. 
int sender, reciver;
std::string rh1, rh2;
if (limit==0) return;
double amount = ((double(rand()%5) + 10.0) *  9900000.0)/1000000;
limit--;

if(!hasMap){
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
      if ( count == 0 && nodes[ options[choice] ]->getRH("") != "") {
         rh1 = nodes[ options[choice] ]->getRH("");
         count++;
         pair.push_back(options[choice]);
      }
      else if (rh1 != nodes[ options[choice] ]->getRH("") && 
            nodes[ options[choice] ]->getRH("") != "") {
         rh2 = nodes[ options[choice] ]->getRH("");
         count++;
         pair.push_back(options[choice]);
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
else {
   std::vector<std::string> txInfo = SplitString(txMap[m_txID],'|');
   sender = std::stoi(txInfo[0]);
   reciver = std::stoi(txInfo[1]);
   amount = std::stod(txInfo[2]);
   if(m_txID % 10 == 0) 
      amount = amount *10;
   rh1 = nodes[ sender ]->getRH("");   
   rh2 = nodes[ reciver ]->getRH(rh1);
   if(nodes[sender]->getTiP() || nodes[reciver]->getTiP() || rh1 == rh2
         || rh1 == "" || rh2 == ""){
      limit++;
      return;
   }
}

   int txID = m_txID;
   m_txID++;
   uint32_t secret = rand()%10000000;
   txIDmap[txID].push_back(sender);
   txIDmap[txID].push_back(reciver);
   FRMap[txID].push_back(false);
   FRMap[txID].push_back(false);
   HRMap[txID] = false;
   HoldMap[txID] = false;
   std::vector<string> plist;
   plist.push_back(rh2);

   nodes[reciver]->startTransaction(txID, plist, false, amount);
   plist.clear();
   plist.push_back(rh1);
   plist.push_back(rh2);
   nodes[sender]->startTransaction(txID, plist, true, amount);
}

void
SpriteSync::addNode( int node, Ptr<Sprite> app ) {
   nodes[node] = app;
}

void
SpriteSync::addSender( int node){
   senders[node] = nodes[node];
}

void
SpriteSync::addRH( int node, Ptr<Sprite> routingHelper ){
   routingHelpers[node] = routingHelper;
}

void
SpriteSync::createTables( ) {
   m_tablesMade = true;
   std::unordered_map<std::string, std::vector<std::string>> reachableMap;
   for(auto each = routingHelpers.begin(); each != routingHelpers.end(); each++){
      std::string node = std::to_string(each->first);
      reachableMap[node] = each->second->getReachableRHs();
   }
   std::unordered_map<std::string, std::vector<std::string>> reachableMap1;
   for(auto each = routingHelpers.begin(); each != routingHelpers.end(); each++){
      std::string node = std::to_string(each->first);
      reachableMap1[node] = each->second->matchUpNonces(reachableMap);
   }   
   for(auto each = routingHelpers.begin(); each != routingHelpers.end(); each++){
      each->second->setRHTable(reachableMap1);
   }
}

void
SpriteSync::onHoldPacket( uint32_t node, uint32_t txID, bool received){
   uint32_t node1 = txIDmap[txID][1];
   nodes[node1]->sendPayPacket(txID);
}

void
SpriteSync::onPayPacket( uint32_t node, uint32_t txID){
  if(txIDmap[txID][0] == node){
     uint32_t node1 = txIDmap[txID][0];
     uint32_t node2 = txIDmap[txID][1];

     nodes[node1]->reset(txID);
     nodes[node2]->reset(txID);
  } 
}

void
SpriteSync::onTxFail(uint32_t txID){
   uint32_t node1 = txIDmap[txID][0];
   uint32_t node2 = txIDmap[txID][1];

   nodes[node1]->reset(txID);
   nodes[node2]->reset(txID);}

}
