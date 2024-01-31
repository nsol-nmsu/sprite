#!/bin/bash



max=1
for (( i=0; i < $max; ++i ))
	do
		NS_GLOBAL_VALUE="RngRun=$i" ./waf --run="Sprite-scenario-test --Run=$i --Method2=0 --Lighting=0" &
done 
wait
echo "All Simulations Complete Method1"

