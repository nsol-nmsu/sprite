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
    return distanceTable[endNode]




def main(filepath=None, traceID=None, Speedy=False):

    if filepath is None:
        sys.stderr.write("usage: python3 %s <file-path> <expected-success-number>\n")
        return 1
    filesToProcess = glob.glob(filepath+"*" )
    print (filesToProcess)
    hopCountR = {}
    nodeMap = {}
    tx = {}
    landMarkDistance = {}
    for filename in filesToProcess:
        edges = {}
        print("Trying :" + filename)
        with open(filename, 'r') as csvh:
            dialect = csv.Sniffer().has_header(csvh.read(1024))
            csvh.seek(0)
            reader = csv.DictReader(csvh, dialect=dialect)
            for row in reader:
                time = float(row['time'])
                if row['event'] == 'PathUpdate':
                    node1 = row['node1']
                    node2 = row['node2']
                    if node1 not in edges:
                        edges[node1] = {}
                    if node2 not in edges[node1]:
                        edges[node1][node2] = float(row['amount'])
                    else :
                        edges[node1][node2] -= float(row['amount'])
                    
                elif (row['event'] == 'Start'):          
                    txid = row['amount']
                    if row['node1'] != '':
                        if row['node1'] not in landMarkDistance:
                            landMarkDistance[ row['node1'] ] = {}
                        if txid.split('_')[1] not in landMarkDistance[ row['node1'] ]:
                            landMarkDistance[ row['node1'] ][ txid.split('_')[1] ] = minPath(edges,  row['node1'], txid.split('_')[1], 1.0 )
                    else :
                        if row['node2'] not in landMarkDistance:
                            landMarkDistance[ row['node2'] ] = {}
                        if txid.split('_')[1] not in landMarkDistance[ row['node2'] ]:
                            landMarkDistance[ row['node2'] ][ txid.split('_')[1] ] = minPath(edges,  row['node2'], txid.split('_')[1], 1.0 )


                elif (row['event'] == 'Pay'):         
                    txid = row['amount']
                    if txid not in hopCountR:
                        hopCountR[txid] = 0
                    hopCountR[txid]+=1    
    f = open(traceID+"-Hops.txt","w")


    maxHops = {}
    for txid in hopCountR:
        if len(txid.split('_')) > 1:
            n_txid = txid.split('_')[0]
            if n_txid not in maxHops:
                maxHops[n_txid] = 0
                            
            hopCountR[n_txid] += hopCountR_copy[txid]

    
    for txid in hopCountR:
        hopCountR[txid] = hopCountR[txid]/10

    if txid in hopCountR:
        f.write()
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))

