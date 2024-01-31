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
            dis = distanceTable[node]
            if minNode == "" or dis < shortestPath:
                shortestPath = dis
                minNode = node
        node_list.remove(minNode)
        if shortestPath == 10000000000:
            continue
        M_N = edge_list[minNode]
        for nodeTo in M_N:
            value = M_N[nodeTo]
            if value < minValue:
                continue
            value = shortestPath+1
            if value < distanceTable[nodeTo]:
                distanceTable[nodeTo] = value
            if distanceTable[endNode] != 10000000000:
                return distanceTable[endNode]
    return distanceTable[endNode]




def main(filepath=None, traceID=None, Speedy=False):

    if filepath is None:
        sys.stderr.write("usage: python3 %s <file-path> <expected-success-number>\n")
        return 1
    filesToProcess = glob.glob(filepath+"*" )
    print (filesToProcess)
    txIDs = set()   
    nodeMap = {}
    hopCountC = {}

    with open(traceID+"Failures.csv", 'r') as csvh:
        dialect = csv.Sniffer().has_header(csvh.read(1024))
        csvh.seek(0)
        reader = csv.DictReader(csvh, dialect=dialect)
        for row in reader:
           txIDs.add(row['TxID'])
    fails = 0
    for filename in filesToProcess:
        edges = {}
        min_link = 100000000000000000
        print("Trying :" + filename)
        with open(filename, 'r') as csvh:
            dialect = csv.Sniffer().has_header(csvh.read(1024))
            csvh.seek(0)
            reader = csv.DictReader(csvh, dialect=dialect)
            for row in reader:
                time = float(row['time'])
                if row['event'] == 'PathUpdate':
                    node1 = row['node1'].split("|")[0]
                    node2 = row['node2'].split("|")[0]
                    if node1 not in edges:
                        edges[node1] = {}
                    if node2 not in edges[node1]:
                        edges[node1][node2] = float(row['amount'])
                    else :
                        edges[node1][node2] -= float(row['amount'])

                    if edges[node1][node2] < min_link:
                        min_link = edges[node1][node2]
                    
                elif (row['event'] == 'Start' and row['amount'] in txIDs):
                    txid = row['amount']
                    amount = float(row['value'])
                    if txid not in hopCountC:
                        hopCountC[txid] = 0
                    if txid not in nodeMap:
                        nodeMap[txid] = [0,0,0]
                    if Speedy:
                        if row['node1'] != '':
                            nodeMap[txid][1]=row['node1'].split("|")[0]
                        else :
                            nodeMap[txid][2]=row['node2'].split("|")[0]
                        nodeMap[txid][0] += 1
                        if nodeMap[txid][0] == 2:
                            mp = 1
                            #if min_link < amount:
                            mp = minPath(edges,  nodeMap[txid][1], nodeMap[txid][2], amount )
                            hopCountC[txid] += mp

                    elif row['node1'] != '':
                        nodes = row['node1'].split("|")
                        mp = 1
                        #if min_link < amount:
                        mp = minPath(edges, nodes[0].split("|")[0], nodes[1].split("|")[0], amount ) + minPath(edges,  nodes[1].split("|")[0], nodes[2].split("|")[0], amount )
                        hopCountC[txid] += mp
                        #hopCountC[txid] += mp
                    else:
                        nodes = row['node2'].split("|")
                        mp = 1
                        #if min_link < amount:
                        mp = minPath(edges,  nodes[1], nodes[0], amount )
                        hopCountC[txid] += mp
                    #print(txid, hopCountC[txid])

    f = open(traceID+"-FailureRate.txt","w")

    for txid in hopCountC:
        if hopCountC[txid] < 10000000000:
            fails += 1

    f.write(str(fails)+","+str(fails/100000) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))

