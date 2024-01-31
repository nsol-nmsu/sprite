#!/bin/bash



max=1
for (( i=0; i < $max; ++i ))
	do
		NS_GLOBAL_VALUE="RngRun=$i" ./waf --run="blanc-scenario-test --Run=$i --Lighting=0"  &
done 
wait
echo "All Simulations Complete Blanc"
