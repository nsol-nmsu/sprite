from __future__ import division 
import csv 
import sys 
from collections import defaultdict 
import numpy as np 
import matplotlib.pyplot as plt 
import glob
import networkx as nx
import copy


def main(filepath=None, traceID=None, maxSuccess=None):

    if filepath is None and maxSuccess is None:
        sys.stderr.write("usage: python3 %s <file-path> <expected-success-number>\n")
        return 1
    filesToProcess = glob.glob(filepath+"*" )
    print (filesToProcess)
    edges = {}
    hopCountR = {}
    tx = {}
    nodeMap = {}

    lat_map = {}
    for filename in filesToProcess:
        print("Trying :" + filename)
        with open(filename, 'r') as csvh:
            dialect = csv.Sniffer().has_header(csvh.read(1024))
            csvh.seek(0)
            reader = csv.DictReader(csvh, dialect=dialect)
            for row in reader:
                time = float(row['time'])

                if (row['event'] == 'Start'):  
                    txid = row['amount']       
                    if txid not in nodeMap:
                        nodeMap[txid] = [0,0,0]
                    if row['node1'] != '':
                        nodeMap[txid][1]=row['node1'].split("|")[0]
                    else :
                        nodeMap[txid][2]=row['node2'].split("|")[0]
                    nodeMap[txid][0]=row['value']

    print(len(nodeMap))
    f = open("TXMap.txt","w")
    for i in range (0,100000):
        txID = str(i)
        f.write(  txID+ " " + nodeMap[txID][1] + "|" + nodeMap[txID][2] + "|" + nodeMap[txID][0]+"\n" )

                
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))

