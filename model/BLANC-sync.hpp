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

#ifndef BLANC_SYNCH_DOE_H
#define BLANC_SYNCH_DOE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ndn-synchronizer.hpp"

#include "ns3/BLANC.hpp"

namespace ns3 {
namespace ndn {

class BLANCSync :  public Synchronizer {

public:

  /** \brief  On object construction create sockets and set references.
   */
  BLANCSync();


  virtual void
  syncEvent() override;


  /** \brief Add a string to list of packets that arrived at their distination.
   *  \param Str string with information on the arriving packet.
   */
  void
  onFindReplyPacket( uint32_t node, uint32_t txID, double amount);

  void
  onHoldPacket( uint32_t node, uint32_t txID, bool received);

  void
  onPayPacket( uint32_t node, uint32_t txID);


  void
  injectPackets( ) ;

  /** \brief Add  a reference to the consumer application of a sending node
   *  \param node the node to which the appliaction belongs to
   *      \param sender reference to the appliaction instance
   */
  void
  addRH( int node, Ptr<Blanc> routingHelper );

  void
  addNode( int node, Ptr<Blanc> app );

  void
  addSender( int node);  


  virtual void
  sendSync(){};

  /** \brief Receive data from co-simulators.
   */
  virtual void
  receiveSync(){};

  void setFindTable(uint32_t node, std::string RH, std::string nextHop){
     nodes[node]->setFindTable(RH, nextHop);
  };

  void setStart(uint32_t start){
     m_txID = start;
  };  

  void setNeighborCredit(uint32_t node, std::string name, double amount){
     nodes[node]->setNeighborCredit(name, amount);
  };

  void setAddressTable(int node, std::string name, Ipv4Address address){
      nodes[node]->setNeighbor(name, address);
  };

  void setTxTable(std::unordered_map<int, std::string> map){
   hasMap = true;
   txMap = map;
  };  

  void setLimit(uint32_t l){
  limit = l;
  };  

  std::vector<std::string> 
  SplitString( std::string strLine, char delimiter, int max=0 ) {
   std::string str = strLine;
   std::vector<std::string> result;
   uint32_t i =0;
   std::string buildStr = "";
   int total = 0;

   for ( i = 0; i<str.size(); i++) {
      if ( str[i]== delimiter && (total != max || max == 0)) {
         result.push_back( buildStr );
	      buildStr = "";
         total++;
      }
      else {
   	      buildStr += str[i];
      }
   }

   if(buildStr!="")
      result.push_back( buildStr );

   return result;
};

void
checkFail( uint32_t txID){
    
  if (!FRMap[txID][0] || !FRMap[txID][1]) {
  //if (!HRMap[txID] || !HoldMap[txID]){
    uint32_t node1 = txIDmap[txID][0];
    uint32_t node2 = txIDmap[txID][1];

    nodes[node1]->reset(txID);
    nodes[node2]->findFailed(txID, node1);
  }
}

void
onTxFail(uint32_t txID){
   uint32_t node1 = txIDmap[txID][0];
   uint32_t node2 = txIDmap[txID][1];

   nodes[node1]->reset(txID);
   nodes[node2]->reset(txID);
};

private:

  std::unordered_map<int,Ptr<Blanc>> nodes;
  std::unordered_map<int,Ptr<Blanc>> senders;
  std::unordered_map<int,Ptr<Blanc>> routingHelpers;
  std::unordered_map<uint32_t, std::vector <uint32_t>> txIDmap;
  std::unordered_map<uint32_t, uint32_t> T_SMap;
  std::unordered_map<uint32_t, std::vector <bool>> FRMap;
  std::unordered_map<uint32_t, bool> HRMap;
  std::unordered_map<uint32_t, bool> HoldMap;
  std::unordered_map<int,std::string> txMap;
  bool hasMap;

  int limit = 10000;
  uint32_t m_txID= 1;

};

}
}
#endif
