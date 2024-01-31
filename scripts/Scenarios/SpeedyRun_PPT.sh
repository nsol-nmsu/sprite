#!/bin/bash



max=1
for (( i=0; i < $max; ++i ))
	do
		NS_GLOBAL_VALUE="RngRun=$i" ./waf --run="SpeedyM-scenario --Run=$i --Lighting=0" &
done 
wait
echo "All Simulations Complete SpeedyM"
