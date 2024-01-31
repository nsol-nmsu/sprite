#!/bin/bash



max=10
for (( i=0; i < $max; ++i ))
	do
		NS_GLOBAL_VALUE="RngRun=$i" ./waf --run="Sprite-scenario-test --Run=$i --Method2=0 --Lighting=1"  &
done 
wait
echo "All Simulations Complete Method1"
