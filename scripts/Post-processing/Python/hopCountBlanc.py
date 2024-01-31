from __future__ import division 
import csv 
import sys 
from collections import defaultdict 
import numpy as np 
import matplotlib.pyplot as plt 
import glob
import copy
import networkx as nx

def minPath(edge_list = None, startNode= None, endNode=None, minValue=0.0 ):
    node_list = set()
    distanceTable = {}
    for node in edge_list:
        node_list.add(node) 
        distanceTable[node] = 10000000000
        if startNode == node:
            distanceTable[node] = 0

    while len(node_list) != 0:
        shortestPath = 0
        minNode = ""
        for node in node_list:
            if minNode == "" or distanceTable[node] < shortestPath:
                shortestPath = distanceTable[node]
                minNode = node
        node_list.remove(minNode)
        if shortestPath == 10000000000:
            continue
        for nodeTo in edge_list[minNode]:
            value = edge_list[minNode][nodeTo]
            if value < minValue:
                continue
            value = shortestPath+1
            if value < distanceTable[nodeTo]:
                distanceTable[nodeTo] = value
            if distanceTable[endNode] <= shortestPath:
                return distanceTable[endNode]                
    return distanceTable[endNode]




def main(filepath=None, traceID=None, Speedy=False):

    if filepath is None:
        sys.stderr.write("usage: python3 %s <file-path> <expected-success-number>\n")
        return 1
    filesToProcess = glob.glob(filepath+"*" )
    print (filesToProcess)
    hopCountR = {}
    hopCountC = {}
    nodeMap = {}
    tx = {}
    Blanc = False
    for filename in filesToProcess:
        edges = {}
        print("Trying :" + filename)
        with open(filename, 'r') as csvh:
            dialect = csv.Sniffer().has_header(csvh.read(1024))
            csvh.seek(0)
            reader = csv.DictReader(csvh, dialect=dialect)
            for row in reader:
                time = float(row['time'])
                if row['event'] == 'Find':
                    Blanc = True
                if row['event'] == 'PathUpdate':
                    node1 = row['node1']
                    node2 = row['node2']
                    if node1 not in edges:
                        edges[node1] = {}
                    if node2 not in edges[node1]:
                        edges[node1][node2] = float(row['amount'])
                    else :
                        edges[node1][node2] -= float(row['amount'])
                    

                elif (row['event'] == 'Start' and not Blanc)  or row['event'] == 'Find':
                    txid = row['amount']
                    #if( int(txid) % 10000 >= 1000 ):
                    #    continue
                    amount = float(row['value'])                    
                    if txid not in hopCountC:
                        hopCountC[txid] = 0
          
                    if Speedy:
                        if txid not in nodeMap:
                            nodeMap[txid] = [0,0,0]
                        if row['node1'] != '':
                            nodeMap[txid][1]=row['node1']
                        else :
                            nodeMap[txid][2]=row['node2']
                        nodeMap[txid][0] += 1
                        if nodeMap[txid][0] == 2:
                            hopCountC[txid] += minPath(edges,  nodeMap[txid][1], nodeMap[txid][2], amount )

                    elif row['node1'] != '':
                        nodes = row['node1'].split("|")
                        #print(nodes)
                        hopCountC[txid] += minPath(edges,  nodes[0], nodes[1], amount )
                        hopCountC[txid] += minPath(edges,  nodes[1], nodes[2], amount )

                    else:
                        nodes = row['node2'].split("|")
                        hopCountC[txid] += minPath(edges,  nodes[1], nodes[0], amount )


                elif (row['event'] == 'Pay'):         
                    txid = row['amount']
                    if txid not in hopCountR:
                        hopCountR[txid] = 0
                    hopCountR[txid]+=1    
    f = open(traceID+"-Hops.txt","w")


    hopCountR_copy = copy.copy(hopCountR)
    hopCountR.clear()
    for txid in hopCountR_copy:
        if len(txid.split('_')) > 1:
            n_txid = txid.split('_')[0]
            if n_txid not in hopCountR:
                hopCountR[n_txid] = 0
            hopCountR[n_txid] += hopCountR_copy[txid]
        else:
            hopCountR[txid] = hopCountR_copy[txid]
    
    for txid in hopCountR:
        hopCountR[txid] = hopCountR[txid]/10

    for txid in hopCountC:
        if txid in hopCountR:
            f.write(str(hopCountR[txid])+"\t"+str(hopCountC[txid]) + "\t"+ str(hopCountR[txid]/ hopCountC[txid])+ "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))

