from __future__ import division 
import csv 
import sys 
from collections import defaultdict 
import numpy as np 
import matplotlib.pyplot as plt 
import glob

def main(filepath=None, traceID=None, maxSuccess=None, blanc=False):

    if filepath is None and maxSuccess is None:
        sys.stderr.write("usage: python3 %s <file-path> <expected-success-number>\n")
        return 1 
    xmax = 250
    print(blanc)
    cumulatedFullLatencies = []
    filesToProcess = glob.glob(filepath+"*" )
    print (filesToProcess)
    fullLatencies = []
    overhead = 0
    runningOverhead = []
    for filename in filesToProcess:
        runningOverhead.append([])
    fullLatencies_map = {}    
    file = 0
    fails = []
    retry_map = set()
    retryCount = 0
    for filename in filesToProcess:
        pending = {}

        latencies = defaultdict(lambda: [])
        print("Trying :" + filename)
        with open(filename, 'r') as csvh:
            current_overhead = 0
            #dialect = csv.Sniffer().sniff(csvh.read(10*1024))
            dialect = csv.Sniffer().has_header(csvh.read(1024))
            csvh.seek(0)
            reader = csv.DictReader(csvh, dialect=dialect)

            for row in reader:
                if row['event'] == 'Start':
                    if blanc == False:
                        time = float(row['time'])
                        if (row['txID']) not  in pending:   
                            pending[(row['txID'])] = time     

                elif row['event'] == 'Find':
                    if blanc == False:
                       continue
                    time = float(row['time'])
                    if (row['txID']) not  in pending:   
                       pending[(row['txID'])] = time   

                elif (row['event'] == 'Complete') and row['txID'] in pending:
                    time = float(row['time'])
                    startTime = pending[( row['txID'])]
                    fullLatencies.append(1000. * (time - startTime))
                    fullLatencies_map[ row['txID'] ] = 1000. * (time - startTime)
                    del pending[( row['txID'])]

                elif (row['event'] == 'Retry'):
                    retryCount += 1
                    retry_map.add(row['txID'])


                elif (row['event'] == 'Ad'):
                    timestep = int(float(row['time'])/10)
                    while len(runningOverhead[file]) <= timestep:
                        runningOverhead[file].append(current_overhead)
                    overhead += 1
                    current_overhead += 1
                    runningOverhead[file][timestep] = current_overhead
                elif (row['event'] == 'Fail'):
                    fails.append(row['txID'])

        file += 1
        for each in pending:
            fullLatencies_map[each]=100000000

    c = 0
    f = open(traceID + "UnsortedLatencies.csv","w")
    f.write("TxID,Lat\n")
    for txid in fullLatencies_map:
        if txid not in retry_map:
            lat = fullLatencies_map[txid]
            f.write(str(txid)+","+str(lat) + "\n")
 
    print(retryCount)
    #Dump Latencies

    fullLatencies = []
    for  txid in fullLatencies_map:
        #if txid not in retry_map and fullLatencies_map[txid] != 100000000:
        fullLatencies.append(fullLatencies_map[txid])

    fullLatencies.sort()
    f = open(traceID+"SortedLatencies.txt","w")
    for each in fullLatencies:
        f.write(str(each) + "\n")

    #Dump Latencies
    f = open(traceID + "UnsortedLatenciesRetries.csv","w")
    f.write("TxID,Lat\n")
    for txid in fullLatencies_map: 
        if txid in retry_map:
            lat = fullLatencies_map[txid]
            f.write(str(txid)+","+str(lat) + "\n")
    #Dump Latencies

    fullLatencies = []
    for  txid in fullLatencies_map:
        if txid  in retry_map and fullLatencies_map[txid] != 100000000:
            fullLatencies.append(fullLatencies_map[txid])

    fullLatencies.sort()
    f = open(traceID+"SortedLatenciesRetries.txt","w")
    for each in fullLatencies:
        f.write(str(each) + "\n")

    f = open(traceID+"Failures.csv","w")
    f.write("TxID\n")
    for each in fails:
        f.write(each + ",\n")

    #Dump Latencies
    fullLatencies.sort()
    f = open(traceID+"Overhead.csv","w")
    t = 0

    avgOverhead = []

    maxAmount = 0
    for i in range(len(runningOverhead)) :
        if len(runningOverhead[i]) > maxAmount:
            maxAmount = len(runningOverhead[i])
    for i in range(len(runningOverhead)) :
        while len(runningOverhead[i]) < maxAmount:
            runningOverhead[i].append(runningOverhead[i][-1])
    for i in range(len(runningOverhead)) :
        while len(avgOverhead) < len (runningOverhead[i]):
            avgOverhead.append(0)
        for k in range(len(runningOverhead[i])):
            avgOverhead[k] += runningOverhead[i][k]

    for each in avgOverhead:
        f.write(str(t)+","+str(each/len(runningOverhead)) + "\n")   
        t+=10     
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))

