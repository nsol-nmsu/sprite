#!/bin/bash

python Python/postProcBlanc.py Blanc-Method1-trace results/B_M1
python Python/postProcBlanc.py Blanc-Method2-trace results/B_M2
python Python/postProcBlanc.py Blanc-Base-trace results/B_B 1 1
python Python/postProcBlanc.py Blanc-Speedy-trace results/B_SM


python Python/hopCountBlancSolo.py Blanc-Method1-path results/B_M1
python Python/hopCountBlancSolo.py Blanc-Method2-path results/B_M2
python Python/hopCountBlancSolo.py Blanc-Base-path results/B_B
python Python/hopCountBlancSolo.py Blanc-Speedy-path results/B_SM

python Python/hopCountBlanc.py Blanc-Method1-path results/B_M1
python Python/hopCountBlanc.py Blanc-Method2-path results/B_M2
python Python/hopCountBlanc.py Blanc-Speedy-path results/B_SM

python Python/failCount.py Blanc-Method1-trace results/B_M1
python Python/failCount.py Blanc-Method2-trace results/B_M2
python Python/failCount.py Blanc-Base-trace results/B_B 1 1
python Python/failCount.py Blanc-Speedy-trace results/B_SM


python Python/findBootstrap.py Blanc-Method1-path results/B_M1
python Python/findBootstrap.py Blanc-Method2-path results/B_M2
python Python/findBootstrap.py Blanc-Speedy-path results/B_SM

python Python/postProcBlanc.py LN-Method1-trace results/LN_M1
python Python/postProcBlanc.py LN-Base-trace results/LN_B 1 1
python Python/postProcBlanc.py LN-Speedy-trace results/LN_SM


python Python/hopCountBlancSolo.py LN-Method1-path results/LN_M1
python Python/hopCountBlancSolo.py LN-Base-path results/LN_B
python Python/hopCountBlancSolo.py LN-Speedy-path results/LN_SM

python Python/hopCountBlanc.py LN-Method1-path results/LN_M1
python Python/hopCountBlanc.py LN-Speedy-path results/LN_SM

python Python/failCount.py LN-Method1-trace results/LN_M1
python Python/failCount.py LN-Base-trace results/LN_B 1 1
python Python/failCount.py LN-Speedy-trace results/LN_SM


python Python/findBootstrap.py LN-Method1-path results/LN_M1
python Python/findBootstrap.py LN-Speedy-path results/LN_SM
