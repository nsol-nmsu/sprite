import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import csv 
import glob 
import pandas


means = [] 
times = [] 
stddev = [] 

 
files = sorted(glob.glob("./results/LN*Overhead.csv")) 
for files in files:  
   means.append([])
   times.append([])
   with open(files, mode='r') as infile:         
        reader = csv.reader(infile) 
        mydict = {rows[0]:rows[1:] for rows in reader}        

        tempdict = {}
        for k,v in mydict.items(): 
            tempdict[k.split("\t")[0].strip()] = v  
            value = v[0]
            means[-1].append(float("%0.3f" % float(value)))
            times[-1].append(float("%0.3f" % float(k)))
        mydict = tempdict
      

      #rfame_setup_means.append(float("%0.3f" % float(mydict['Setup'][0])))
      #rfame_setup_stddev.append(float("%0.3f" % float(mydict['Setup'][1])))  
 


# set width of bar
barWidth = 0.25
 
 
# Set position of bar on X axis
r1 = np.arange(len(means))
#r2 = [x + barWidth for x in r1]
f, (ax, ax2) = plt.subplots(2, 1, sharex=True)
# Make the plot
ax.plot(times[0], means[0], color='#7f6d5f', label='BLANC', linestyle='solid')
ax.plot(times[3], means[3], color='grey', label='Speedy', linestyle='dashed')
ax2.plot(times[0], means[0], color='#7f6d5f', label='BLANC', linestyle='solid')
ax2.plot(times[3], means[3], color='grey', label='Speedy', linestyle='dashed')

ax2.plot(times[1], means[1], color='red', label='R2RB', linestyle='dashdot')
#plt.bar(r2, means, color='#557f2d', width=barWidth, edgecolor='white', label='Fame')

ax2.set_ylim(0, 650000)  # most of the d  ata
ax.set_ylim(1500000, 100000000)  # outliers only

ax.spines['bottom'].set_visible(False)
ax2.spines['top'].set_visible(False)
ax.xaxis.tick_top()
ax.tick_params(labeltop=False)  # don't put tick labels at the top
ax2.xaxis.tick_bottom()
plt.ticklabel_format(axis='y', style='sci',scilimits=(6,6))


d = .008  # how big to make the diagonal lines in axes coordinates
# arguments to pass to plot, just so we don't keep repeating them
kwargs = dict(transform=ax.transAxes, color='k', clip_on=False)
ax.plot((-d, +d), (-d, +d), **kwargs)        # top-left diagonal
ax.plot((1 - d, 1 + d), (-d, +d), **kwargs)  # top-right diagonal

kwargs.update(transform=ax2.transAxes)  # switch to the bottom axes
ax2.plot((-d, +d), (1 - d, 1 + d), **kwargs)  # bottom-left diagonal
ax2.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)  # bottom-right diagonal

# Add xticks on the middle of the group bars
plt.xlabel('Time (s)', fontweight='bold')
#plt.xticks([r + barWidth for r in range(len(means))], ['8', '16', '32', '64'])

f.text(0.06, 0.5, 'Message Complexity', ha='center', va='center', rotation='vertical', fontweight='bold')

# Create legend & Show graphic
plt.legend()
plt.show()
 
plt.savefig('results/MessageComplexityLN.png')
plt.clf()


means = []
hops = []
counts = []
files = sorted(glob.glob("./results/LN*LatOverHops.txt")) 
for files in files:  
   means.append([])
   hops.append([])
   counts.append([])

   with open(files, mode='r') as infile: 
        reader = csv.reader(infile) 
        mydict = {rows[0]:rows[1:] for rows in reader}        

        tempdict = {}
        for k,v in mydict.items(): 
            if float(k) >= 50:
              tempdict[k.split(",")[0].strip()] = v  
              value = v[1]
              means[-1].append(float("%0.3f" % float(value)))
              hops[-1].append(float("%0.3f" % float(v[0])))
              counts[-1].append(float("%0.3f" % float(k)))
        mydict = tempdict
 
# Set position of bar on X axis
r1 = np.arange(len(means))
#r2 = [x + barWidth for x in r1]
 
# Make the plot
plt.plot(hops[0], means[0], color='#7f6d5f', label='BLANC', linestyle='solid')
plt.plot(hops[3], means[3], color='grey', label='Speedy', linestyle='dashed')
plt.plot(hops[1], means[1], color='red', label='R2RB', linestyle='dashdot')
#plt.bar(r2, means, color='#557f2d', width=barWidth, edgecolor='white', label='Fame')
 
# Add xticks on the middle of the group bars
plt.xlabel('Hopcount', fontweight='bold')
#plt.xticks([r + barWidth for r in range(len(means))], ['8', '16', '32', '64'])
 
plt.ylabel('Time (ms)', fontweight='bold')
# Create legend & Show graphic
plt.legend()
plt.show()
 
plt.savefig('results/AvgLatOverHopsLN.png')

plt.clf()
# Make the plot
plt.bar("BLANC", np.mean(means[0]), color='#7f6d5f', label='BLANC', linestyle='solid')
plt.bar("Speedy", np.mean(means[3]), color='grey', label='Speedy', linestyle='dashed')
plt.bar("R2RB", np.mean(means[1]), color='red', label='R2RB', linestyle='dashdot')
#plt.bar(r2, means, color='#557f2d', width=barWidth, edgecolor='white', label='Fame')
 
# Add xticks on the middle of the group bars
plt.xlabel('Scheme', fontweight='bold')
#plt.xticks([r + barWidth for r in range(len(means))], ['8', '16', '32', '64'])
 
plt.ylabel('Time (ms)', fontweight='bold')
# Create legend & Show graphic
#plt.legend()
plt.show()
 
plt.savefig('results/AvgLatLN.png')
plt.clf()

# Make the plot
plt.bar("BLANC", np.mean(hops[0]), color='#7f6d5f', label='BLANC', linestyle='solid')
plt.bar("Speedy", np.mean(hops[3]), color='grey', label='Speedy', linestyle='dashed')
plt.bar("R2RB", np.mean(hops[1]), color='red', label='R2RB', linestyle='dashdot')
#plt.bar(r2, means, color='#557f2d', width=barWidth, edgecolor='white', label='Fame')
 
# Add xticks on the middle of the group bars
plt.xlabel('Scheme', fontweight='bold')
#plt.xticks([r + barWidth for r in range(len(means))], ['8', '16', '32', '64'])
 
plt.ylabel('Avg Hop Count', fontweight='bold')
# Create legend & Show graphic
#plt.legend()
plt.show()
 
plt.savefig('results/HopCountsLN.png')
plt.clf()
