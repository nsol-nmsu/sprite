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
#include "SpeedyM-sync.hpp"
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
SpeedySync::SpeedySync() {

}

void
SpeedySync::syncEvent(){
   Synch::syncEvent();
   injectPackets( );
}

void
SpeedySync::injectPackets( ) {
//Go through list and find two apps not currently in a transaction. 
int sender, reciver;
if (limit==0) return;
double amount = rand()%14+1;

if(hasMap){
   std::vector<std::string> txInfo = SplitString(txMap[m_txID],'|');
   sender = std::stoi(txInfo[0]);
   reciver = std::stoi(txInfo[1]);
   amount = std::stod(txInfo[2]); 
   if(nodes[sender]->getTiP() || nodes[reciver]->getTiP()){
      return;
   }
}   
else{
   int count = 0;
   vector<int> options;
   for(auto i = senders.begin(); i != senders.end(); i++){
      if (!nodes[i->first]->getTiP()){
         options.push_back(i->first);
      }
   }
   vector<int> pair;
   while (pair.size() < 2 && options.size() > 1){
      uint32_t choice = rand()%options.size();

      count++;
      pair.push_back(options[choice]);

      std::vector<int> copy = options;
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
   limit--;



   int txID = m_txID;
   m_txID++;
   txIDmap[txID].push_back(sender);
   txIDmap[txID].push_back(reciver);
   std::vector<string> plist = nodes[reciver]->getAddressList();

   nodes[sender]->startTransaction(txID, plist, amount);
   nodes[reciver]->setReceiver(txID, amount);

}

void
SpeedySync::addNode( int node, Ptr<SpeedyM> app ) {
   nodes[node] = app;
}

void
SpeedySync::addSender( int node){
   senders[node] = nodes[node];
}

void
SpeedySync::addRH( int node, Ptr<SpeedyM> routingHelper ){
   routingHelpers[node] = routingHelper;
}

void
SpeedySync::onPayPacket( uint32_t node, uint32_t txID){
  if(txIDmap[txID][1] == node){
     uint32_t node1 = txIDmap[txID][0];
     uint32_t node2 = txIDmap[txID][1];

     nodes[node1]->reset(txID);
     nodes[node2]->reset(txID);
  } 
}

void
SpeedySync::onTxFail(uint32_t txID){
   uint32_t node1 = txIDmap[txID][0];
   uint32_t node2 = txIDmap[txID][1];

   nodes[node1]->reset(txID);
   nodes[node2]->reset(txID);}
}
