from __future__ import division 
import csv 
import sys 
from collections import defaultdict 
import numpy as np 
import matplotlib.pyplot as plt 
import glob

def main(filepath=None, traceID=None, maxSuccess=None):

    if filepath is None and maxSuccess is None:
        sys.stderr.write("usage: python3 %s <file-path> <expected-success-number>\n")
        return 1 
    filesToProcess = glob.glob(filepath+"*" )
    print (filesToProcess)
    overhead = []
    duration = []
    for filename in filesToProcess:
        pending = {}

        latencies = defaultdict(lambda: [])
        print("Trying :" + filename)
        with open(filename, 'r') as csvh:
            current_overhead = 0
            dialect = csv.Sniffer().has_header(csvh.read(1024))
            csvh.seek(0)
            reader = csv.DictReader(csvh, dialect=dialect)
            duration.append(0)
            overhead.append(0)
            for row in reader:
                if row['event'] == 'Start':
                    break  
                elif (row['event'] == 'Ad'):
                    overhead[-1] += 1
                    duration[-1] = float(row['time'])

    #Dump Latencies
    f = open(traceID+"BootStrap.csv","w")
    f.write(str(np.mean(duration))+","+str(np.std(duration))+","+str(np.mean(overhead)) + ","+str(np.std(overhead)) +"\n")   

    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))

