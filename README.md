SPRITE: Secure and Private Routing in Payment Channel Networks
======

The code here is used to evaluate SPRITE which was published in ACM ASIACCS 2024.

Full version of SPRITE can be found on [Cryptology ePrint Archive](https://eprint.iacr.org/2024/122)

----------------


Overview:
----------------
Payment channel networks are a promising solution to the scalability challenge of blockchains and are designed for significantly increased 
transaction throughput compared to the layer one blockchain. Since payment channel networks are essentially decentralized peer-to-peer networks, 
routing transactions is a fundamental challenge. Payment channel networks have some unique security and privacy requirements that make 
pathfinding challenging, for instance, network topology is not publicly known, and sender/receiver privacy should be preserved, in addition 
to providing atomicity guarantees for payments. In this paper, we present an efficient privacy-preserving routing protocol, SPRITE, for 
payment channel networks that supports concurrent transactions. By finding paths offline and processing transactions online, SPRITE can process 
transactions in just two rounds, which is more efficient compared to prior work. We evaluate SPRITE’s performance using Lightning Network 
data and prove its security using the Universal Composability framework. In contrast to the current cutting-edge methods that achieve rapid 
transactions, our approach significantly reduces the message complexity of the system by 3 orders of magnitude while maintaining similar latencies.

Code Checkout:
----------------
This code is designed as an ns-3 module, so after pulling that code our module can be placed with the src folder of ns-3. 

The version of ns-3 used for our expermients can be checked out with this command. 

`git clone https://github.com/nsol-nmsu/ndnQoS.git`

The following steps can be taken after pulling this version of ns-3 to build our codebase appropriately. 

`cd ndnQoS/ns-3/`

`git clone https://github.com/nsol-nmsu/sprite.git src/sprite`

`./waf configure --enable-examples`

`./waf`


Running the code:
----------------

Bash scripts that run our scenarios can be found in the [scripts/Scenarios](https://github.com/nsol-nmsu/sprite/tree/main/scripts/Scenarios) folder, each script is named after the scenario it runs. 

After running the experiments post-processing can be done with the script found in [scripts/Post-processing](https://github.com/nsol-nmsu/sprite/tree/main/scripts/Post-processing) named processResults.sh
