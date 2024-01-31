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

    amountMap = {}
    partialCount = 0

    with open("TXMap.txt", 'r') as infile:
        reader = infile.readlines()
        for rows in reader:
            values = rows.split(" ")
            txID = values[0]
            amount = float(values[1].split("|")[2])
            amountMap[txID] = amount

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

                if row['event'] == 'Start':
                    txid = row['amount']
                    amount = float(row['value'])
                    #print(amount, amountMap[txid])
                    if amount < amountMap[txid]:
                        partialCount+=1


    print(partialCount)
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))

