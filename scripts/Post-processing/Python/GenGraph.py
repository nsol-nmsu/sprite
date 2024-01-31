import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import csv 
import glob 
import pandas
from matplotlib.ticker import AutoMinorLocator


means = [] 
times = [] 
stddev = [] 

 
files = sorted(glob.glob("./results/B*Overhead.csv")) 
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
 

duration = []
amount = [] 
duration_er = []
amount_er = []
files = sorted(glob.glob("./results/B*BootStrap.csv")) 
for files in files: 
   duration.append([])
   amount.append([])
   duration_er.append([])
   amount_er.append([])   
   with open(files, mode='r') as infile:   
        reader = infile.readlines()
        
        for rows in reader:
            duration[-1].append(float(rows.split(",")[0].strip("\n")))
            amount[-1].append(float(rows.split(",")[2].strip("\n")))
            duration_er[-1].append(float(rows.split(",")[1].strip("\n")))
            amount_er[-1].append(float(rows.split(",")[3].strip("\n")))            

# set width of bar 
barWidth = 0.25
 
 
# Set position of bar on X axis
r1 = np.arange(len(means))
#r2 = [x + barWidth for x in r1]
f, (ax, ax2) = plt.subplots(2, 1, sharex=True)
# Make the plot
ax.plot(times[0], means[0], color='#7f6d5f', label='BlAnC', linestyle='solid', linewidth=2.5)
ax.plot(times[3], means[3], color='grey', label='SM', linestyle='dashed', linewidth=2.5)
ax2.plot(times[0], means[0], color='#7f6d5f', label='BlAnC', linestyle='solid', linewidth=2.5)
ax2.plot(times[3], means[3], color='grey', label='SM', linestyle='dashed', linewidth=2.5)
ax.plot([0], [0], color='red', label='R2RB', linestyle='dashdot', linewidth=2.5)
ax.plot([0], [0], color='black', label='R2NB', linestyle='dotted', linewidth=2.5)
ax2.plot(times[1], means[1], color='red', label='R2RB', linestyle='dashdot', linewidth=2.5)
ax2.plot(times[2], means[2], color='black', label='R2NB', linestyle='dotted', linewidth=2.5)

ax2.set_ylim(0, 780000)  # most of the d  ata
ax.set_ylim(1250000, 600000000)  # outliers only

ax.spines['bottom'].set_visible(False)
ax2.spines['top'].set_visible(False)
ax.xaxis.tick_top()
ax.tick_params(labeltop=False, labelsize = 12)  # don't put tick labels at the top
ax2.tick_params(labelsize = 12)  # don't put tick labels at the top
ax2.xaxis.tick_bottom()
ax.ticklabel_format(axis='y', style='sci',scilimits=(6,6))
ax2.ticklabel_format(axis='y', style='sci',scilimits=(6,6))

d = .008  # how big to make the diagonal lines in axes coordinates
# arguments to pass to plot, just so we don't keep repeating them
kwargs = dict(transform=ax.transAxes, color='k', clip_on=False)
ax.plot((-d, +d), (-d, +d), **kwargs)        # top-left diagonal
ax.plot((1 - d, 1 + d), (-d, +d), **kwargs)  # top-right diagonal

kwargs.update(transform=ax2.transAxes)  # switch to the bottom axes
ax2.plot((-d, +d), (1 - d, 1 + d), **kwargs)  # bottom-left diagonal
ax2.plot((1 - d, 1 + d), (1 - d, 1 + d), **kwargs)  # bottom-right diagonal

# Add xticks on the middle of the group bars
plt.xlabel('Time (s)', fontweight='bold', fontsize = 12)

f.text(0.04, 0.5, '# of messages', ha='center', va='center', rotation='vertical', fontweight='bold', fontsize = 12)
#f.text(.85, .95, '100,000 Data Points', ha='center', va='center')

# Create legend & Show graphic
ax.legend(["BlAnC", "SM", "R2RB", "R2NB"], fontsize = 12, loc = 'upper left')
plt.show()
 
plt.savefig('results/MessageComplexity-PPT.png')
plt.clf()

# Set position of bar on X axis
# Make the plot
plt.plot(times[0], means[0], color='#7f6d5f', label='BlAnC', linestyle='solid')
plt.plot(times[3], means[3], color='grey', label='SM', linestyle='dashed')

plt.plot(times[1], means[1], color='red', label='R2RB', linestyle='dashdot')
plt.plot(times[2], means[2], color='black', label='R2NB', linestyle='dotted')

plt.yscale("log")

# Add xticks on the middle of the group bars
plt.xlabel('Time (s)', fontweight='bold')
plt.ylabel('# of messages', fontweight='bold')
f.text(.85, .95, '100,000 Data Points', ha='center', va='center')

# Create legend & Show graphic
plt.legend()
plt.show()
 
plt.savefig('results/MessageComplexity-Log-PPT.png')
plt.clf()



# Set position of bar on X axis
# Make the plot
plt.bar('SM', amount[2], yerr = amount_er[2], color='grey', capsize=10, hatch='xx')
plt.bar('R2RB', amount[0], yerr = amount_er[0], color='red', capsize=10, hatch='/')
plt.bar('R2NB', amount[1], yerr = amount_er[1], color='green', capsize=10, hatch='///')
 
# Add xticks on the middle of the group bars
plt.xlabel('Scheme', fontweight='bold', fontsize = 12)
plt.xticks(fontsize = 12)
print( amount[2],  amount[0],  amount[1])

plt.ylabel('# of messages', fontweight='bold', fontsize = 12)
plt.yticks(fontsize = 12)
plt.ticklabel_format(axis='y', style='sci',scilimits=(3,3))
f.text(.85, .95, '10 Runs', ha='center', va='center', fontsize = 12)

# Create legend & Show graphic
plt.show()
 
plt.savefig('results/BootstrapComplexity-PPT.png')
plt.clf()





# Set position of bar on X axis
# Make the plot
plt.bar('SM', duration[2], yerr = duration_er[2], color='grey', capsize=10, hatch='xx')
plt.bar('R2RB', duration[0], yerr = duration_er[0], color='red', capsize=10, hatch='/')
plt.bar('R2NB', duration[1], yerr = duration_er[1], color='green', capsize=10, hatch='///')
 
# Add xticks on the middle of the group bars
plt.xlabel('Scheme', fontweight='bold', fontsize = 12)
plt.xticks(fontsize = 12)
plt.yticks(fontsize = 12)

plt.ylabel('Time (s)', fontweight='bold', fontsize = 12)
f.text(.85, .95, '10 Runs', ha='center', va='center', fontsize = 12)

# Create legend & Show graphic
plt.show()
 
plt.savefig('results/DurationBar-PPT.png')
plt.clf()



means = []
hops = []
counts = []
m_hop = []
m_error = []
hop_group = []
files = sorted(glob.glob("./results/B*LatOverHops.txt")) 
for files in files:  
   means.append([])
   hops.append([])
   counts.append([])
   m_hop.append({})
   m_error.append({})
   hop_group.append([])
   with open(files, mode='r') as infile: 
        reader = csv.reader(infile) 
        mydict = {rows[0]:rows[1:] for rows in reader}        

        tempdict = {}
        for k,v in mydict.items(): 
            for i in range(int(k)):
               hop_group[-1].append(int(v[0]))
            #if float(k) >= 50:
            tempdict[k.split(",")[0].strip()] = v  
            value = v[1]
            means[-1].append(float("%0.3f" % float(value)))
            hops[-1].append(float("%0.3f" % float(v[0])))
            counts[-1].append(float("%0.3f" % float(k)))
            m_hop[-1][int(v[0])] = float("%0.3f" % float(value))
            m_error[-1][int(v[0])] = float("%0.3f" % float(v[2]))

        mydict = tempdict

lats = []      
lats.append([]) 
with open("./results/B_BSortedLatencies.txt", mode='r') as infile: 
  reader = infile.readlines()    
  for rows in reader:
    lats[-1].append(float(rows.split(",")[0].strip("\n")))
lats.append([]) 
with open("./results/B_M1SortedLatencies.txt", mode='r') as infile: 
  reader = infile.readlines()    
  for rows in reader:
    lats[-1].append(float(rows.split(",")[0].strip("\n")))
lats.append([]) 
with open("./results-1/B_M2SortedLatencies.txt", mode='r') as infile: 
  reader = infile.readlines()    
  for rows in reader:
    lats[-1].append(float(rows.split(",")[0].strip("\n")))
lats.append([]) 
with open("./results/B_SMSortedLatencies.txt", mode='r') as infile: 
  reader = infile.readlines()    
  for rows in reader:
    lats[-1].append(float(rows.split(",")[0].strip("\n")))         
lats.append([]) 
with open("./results/B_M1SortedLatenciesRetries.txt", mode='r') as infile: 
  reader = infile.readlines()    
  for rows in reader:
    lats[-1].append(float(rows.split(",")[0].strip("\n")))
lats.append([]) 
with open("./results-1/B_M2SortedLatenciesRetries.txt", mode='r') as infile: 
  reader = infile.readlines()    
  for rows in reader:
    lats[-1].append(float(rows.split(",")[0].strip("\n")))       

# Make the plot
plt.plot(hops[0], means[0], color='#7f6d5f', label='BlAnC', linestyle='solid')
plt.plot(hops[3], means[3], color='grey', label='SM', linestyle='dashed')

plt.plot(hops[1], means[1], color='red', label='R2RB', linestyle='dashdot')
plt.plot(hops[2], means[2], color='black', label='R2NB', linestyle='dotted')
 
# Add xticks on the middle of the group bars
plt.xlabel('Hopcount', fontweight='bold')
 
plt.ylabel('Time (ms)', fontweight='bold')
# Create legend & Show graphic
plt.legend()
f.text(.85, .95, '100,000 Data Points', ha='center', va='center')

plt.show()
 
plt.savefig('results/AvgLatOverHops-lineplot-PPT.png')

plt.clf()


x = np.arange(4)

val1 = 14
val2 = 18
val3 = 22
val4 = 24

if val1 not in m_hop[0]:
  m_hop[0][val1] = 0
  m_error[0][val1] = 0
if val1 not in m_hop[1]:
  m_hop[1][val1] = 0
  m_error[1][val1] = 0
if val1 not in m_hop[2]:
  m_hop[2][val1] = 0
  m_error[2][val1] = 0
if val1 not in m_hop[3]:
  m_hop[3][val1] = 0  
  m_error[3][val1] = 0

if val2 not in m_hop[0]:
  m_hop[0][val2] = 0
  m_error[0][val2] = 0
if val2 not in m_hop[1]:
  m_hop[1][val2] = 0
  m_error[1][val2] = 0
if val2 not in m_hop[2]:
  m_hop[2][val2] = 0
  m_error[2][val2] = 0
if val2 not in m_hop[3]:
  m_hop[3][val2] = 0  
  m_error[3][val2] = 0

if val3 not in m_hop[0]:
  m_hop[0][val3] = 0
  m_error[0][val3] = 0
if val3 not in m_hop[1]:
  m_hop[1][val3] = 0
  m_error[1][val3] = 0
if val3 not in m_hop[2]:
  m_hop[2][val3] = 0
  m_error[2][val3] = 0
if val3 not in m_hop[3]:
  m_hop[3][val3] = 0  
  m_error[3][val3] = 0

if val4 not in m_hop[0]:
  m_hop[0][val4] = 0
  m_error[0][val4] = 0
if val4 not in m_hop[1]:
  m_hop[1][val4] = 0
  m_error[1][val4] = 0
if val4 not in m_hop[2]:
  m_hop[2][val4] = 0
  m_error[2][val4] = 0
if val4 not in m_hop[3]:
  m_hop[3][val4] = 0  
  m_error[3][val4] = 0



BlAnC = [m_hop[0][val1], m_hop[0][val2], m_hop[0][val3], m_hop[0][val4]]
BlAnC_error = [m_error[0][val1], m_error[0][val2], m_error[0][val3], m_error[0][val4]]
SM = [m_hop[3][val1], m_hop[3][val2], m_hop[3][val3], m_hop[3][val4]]
SM_error = [m_error[3][val1], m_error[3][val2], m_error[3][val3], m_error[3][val4]]
R2RB = [m_hop[1][val1], m_hop[1][val2], m_hop[1][val3], m_hop[1][val4]]
R2RB_error = [m_error[1][val1], m_error[1][val2], m_error[1][val3], m_error[1][val4]]
R2NB = [m_hop[2][val1], m_hop[2][val2], m_hop[2][val3], m_hop[2][val4]]
R2NB_error = [m_error[2][val1], m_error[2][val2], m_error[2][val3], m_error[2][val4]]
width = 0.15

# Make the plot
plt.bar(x-0.2, BlAnC, width, yerr=BlAnC_error, color='#7f6d5f', capsize=3, hatch='..')
plt.bar(x-0.05, SM, width, yerr=SM_error, color='grey', capsize=3, hatch='xx')
plt.bar(x+0.05, R2RB, width, yerr=R2RB_error, color='red', capsize=3, hatch='/')
plt.bar(x+0.2, R2NB, width, yerr=R2NB_error, color='green', capsize=3, hatch='///')
plt.xticks(x, [str(val1), str(val2), str(val3), str(val4)], fontsize = 12)
 
# Add xticks on the middle of the group bars
plt.xlabel('Hops', fontweight='bold', fontsize = 12)
#plt.xticks([r + barWidth for r in range(len(means))], ['8', '16', '32', '64'])
plt.ylabel('Time (ms)', fontweight='bold', fontsize = 12)
plt.yticks(fontsize = 12)
# Create legend & Show graphic
plt.legend(["BlAnC", "SM", "R2RB", "R2NB"], fontsize = 12, loc = 'upper left')

plt.grid(color='grey', linewidth=1, axis='x', alpha=0.5)

plt.show()
f.text(.85, .95, '100,000 Transactions', ha='center', va='center', fontsize = 12)

plt.savefig('results/AvgLatOverHops-PPT.png')
plt.clf()


# Make the plot
orderedLats = [lats[0],lats[3], lats[1], lats[2], lats[4], lats[5]]
N = len(lats[0])
x = lats[0]
x1 = []
y = np.arange(N) / float(N)
y1 = []
for i in range(len(x)):
   if x[i] != 100000000:
      x1.append(x[i])
      y1.append(y[i])
plt.plot(x1, y1, label = "BlAnC", linewidth=2.5)

N = len(lats[3])
x = lats[3]
x1 = []
y = np.arange(N) / float(N)
y1 = []
for i in range(len(x)):
   if x[i] != 100000000:
      x1.append(x[i])
      y1.append(y[i])

plt.plot(x1, y1, label = "SM", linewidth=2.5)



N = len(lats[1])
x = lats[1]
x1 = []
y = np.arange(N) / float(N)
y1 = []
for i in range(len(x)):
   if x[i] != 100000000:
      x1.append(x[i])
      y1.append(y[i])
plt.plot(x1, y1, label = "R2RB",  linewidth=2.5)


N = len(lats[2])
x = lats[2]
x1 = []
y = np.arange(N) / float(N)
y1 = []
for i in range(len(x)):
   if x[i] != 100000000:
      x1.append(x[i])
      y1.append(y[i])
plt.plot(x1, y1, label = "R2NB",  linewidth=2.5)

plt.legend( fontsize = 12, loc = 'lower right')

#plt.bar("BlAnC", np.mean(lats[0]), yerr=np.std(lats[0]), color='#7f6d5f', label='BlAnC', linestyle='solid', capsize=10, hatch='..')
#plt.bar("SM", np.mean(lats[2]), yerr=np.std(lats[2]), color='grey', label='SM', linestyle='dashed', capsize=10, hatch='xx')
#plt.bar("R2RB", np.mean(lats[1]), yerr=np.std(lats[1]), color='red', label='R2RB', linestyle='dashdot', capsize=10, hatch='/')
 
# Add xticks on the middle of the group bars
#plt.xlabel('Scheme', fontweight='bold')
#plt.xticks([r + barWidth for r in range(len(means))], ['8', '16', '32', '64'])


lats_i = []
lats_i.append([])
lats_i.append([])
lats_i.append([])
lats_i.append([])

for each in lats[0]:
   if each != 100000000.0:
      lats_i[0].append(each)

for each in lats[1]:
   if each != 100000000.0:
      lats_i[1].append(each)

for each in lats[2]:
   if each != 100000000.0:
      lats_i[2].append(each)    


for each in lats[3]:
   if each != 100000000.0:
      lats_i[3].append(each)                  
      
print("Means: ", np.mean(lats_i[0]), np.mean(lats_i[3]), np.mean(lats_i[1]), np.mean(lats_i[2]))

f.text(.85, .95, '100,000 Transactions', ha='center', va='center', fontsize = 12)

plt.xlabel('Latency (ms)', fontweight='bold', fontsize = 12)
# Create legend & Show graphic
#plt.legend()
plt.yticks(np.arange(0, 1.1, 0.1), fontsize = 12)
plt.xticks(fontsize = 12)
plt.yticks(np.arange(0, 1.05, 0.05), minor=True, fontsize = 12)
plt.grid(which='minor', alpha=0.3)
plt.grid(which='major', alpha=0.6)
plt.show()
#f.text(.85, .95, '100,000 Data Points', ha='center', va='center')
plt.ylabel('Fraction of Transactions', fontweight='bold')

plt.savefig('results/LatCDFLat-PPT_YAXIS2.png')
plt.clf()

# Make the plot
orderedLats = [lats[0], lats[3], lats[1], lats[2]]
bp = plt.boxplot(orderedLats, patch_artist = True, vert = 1, showfliers=False)

colors = ['#7f6d5f', 'grey', 'red', 'green']


for patch, color in zip(bp['boxes'], colors):
    patch.set_facecolor(color)

#plt.xticks(['BlAnC', 'SM', 'R2RB', 'R2NB'])
plt.xticks([r  for r in range(1, 5)], ['BlAnC', 'SM', 'R2RB', 'R2NB'])


plt.xlabel('Scheme', fontweight='bold')
 
plt.ylabel('Time (ms)', fontweight='bold')
# Create legend & Show graphic
plt.show()

plt.savefig('results/AvgLat-PPT-B.png')
plt.clf()

# Make the plot

for each in lats[4]:
   lats[1].append(each)

for each in lats[5]:
   lats[2].append(each)   

orderedLats = [lats[0],lats[3], lats[1], lats[2]]
bp = plt.boxplot(orderedLats, patch_artist = True, vert = 1, showfliers=True)

colors = ['#7f6d5f', 'grey', 'red', 'green']


for patch, color in zip(bp['boxes'], colors):
    patch.set_facecolor(color)

#plt.xticks(['BlAnC', 'SM', 'R2RB', 'R2NB'])
plt.xticks([r  for r in range(1, 5)], ['BlAnC', 'SM', 'R2RB', 'R2NB'])

plt.xlabel('Scheme', fontweight='bold')
 
plt.ylabel('Time (ms)', fontweight='bold')
# Create legend & Show graphic
plt.show()


print(np.mean(lats[0]))
print(np.mean(lats[3]))
print(np.mean(lats[1]))
print(np.mean(lats[2]))
      
plt.savefig('results/AvgLat-PPT-D.png')
plt.clf()



# Make the plot
plt.bar("BlAnC", np.mean(hop_group[0]), yerr= np.std(hop_group[0]), color='#7f6d5f', label='BlAnC', linestyle='solid', hatch='..', capsize=10)
plt.bar("SM", np.mean(hop_group[3]), yerr= np.std(hop_group[3]), color='grey', label='SM', linestyle='dashed', hatch='xx', capsize=10)
plt.bar("R2RB", np.mean(hop_group[1]), yerr= np.std(hop_group[1]), color='red', label='R2RB', linestyle='dashdot', hatch='/', capsize=10)
plt.bar("R2NB", np.mean(hop_group[2]), yerr= np.std(hop_group[2]), color='green', label='R2NB', linestyle='dotted', hatch='///', capsize=10)
 
# Add xticks on the middle of the group bars
plt.xlabel('Scheme', fontweight='bold')
#plt.xticks([r + barWidth for r in range(len(means))], ['8', '16', '32', '64'])
 
plt.ylabel('Avg Hop Count', fontweight='bold')
# Create legend & Show graphic
#plt.legend()
#f.text(.85, .95, '100,000 Data Points', ha='center', va='center')

plt.show()
 
plt.savefig('results/HopCounts-PPT.png')
plt.clf()
