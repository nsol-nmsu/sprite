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
    lat_map = {}
    for filename in filesToProcess:
        print("Trying :" + filename)
        with open(filename, 'r') as csvh:
            dialect = csv.Sniffer().has_header(csvh.read(1024))
            csvh.seek(0)
            reader = csv.DictReader(csvh, dialect=dialect)
            for row in reader:
                time = float(row['time'])

                if (row['event'] == 'Pay'):         
                    txid = row['amount']
                    if txid not in hopCountR:
                        hopCountR[txid] = 0
                    hopCountR[txid]+=1     

    with open(traceID+"UnsortedLatencies.csv", 'r') as csvh:
        dialect = csv.Sniffer().has_header(csvh.read(1024))
        csvh.seek(0)
        reader = csv.DictReader(csvh, dialect=dialect)
        for row in reader:
            if ( float(row['Lat']) < 10000):
                lat_map[row['TxID']] = float(row['Lat'])
                    
    f = open(traceID+"-HopCount.txt","w")

    for txid in hopCountR:
        f.write(str(hopCountR[txid])+ "\n")
    f = open(traceID+"-LatOverHops.txt","w")

    t_h = {}
    t_h_c = {}

    hopCountR_copy = copy.copy(hopCountR)
    hopCountR.clear()
    for txid in hopCountR_copy:
        if len(txid.split('_')) > 1:
            n_txid = txid.split('_')[0]
            if n_txid not in hopCountR or hopCountR[n_txid] < hopCountR_copy[txid]:
                hopCountR[n_txid] = hopCountR_copy[txid]
        else:
            hopCountR[txid] = hopCountR_copy[txid]
    for txid in hopCountR:
        if txid in lat_map and lat_map[txid] < 10000:
            if hopCountR[txid] not in t_h:
                t_h[hopCountR[txid]] = []
            t_h[hopCountR[txid]].append(lat_map[txid])
    myKeys = list(t_h.keys())
    myKeys.sort()            
    for hc in myKeys:
        f.write(str(len(t_h[hc]))+","+str(hc)+","+str(np.mean(t_h[hc]))+","+str(np.std(t_h[hc]))+ "\n")        
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))

