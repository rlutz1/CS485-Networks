# Vids for HW 9

+ https://www.youtube.com/watch?v=bdh2kfgxVuw
+ https://www.youtube.com/watch?v=jJU2AVX6gpU

## vid 1: Routing algorithms

both algos are computing least distance/shortest cost, but in different ways.
+ routing algorithm is trying to find a good route from A -> B!

**path**: *sequence of routers* packets traverse from given initial source host to final dest host

**GOOD route**: least cose, fastest, least congested
+ this is often left up to the network operator

typically we will use graphs to abstract: nodes-> routers, edges -> links
+ edge weight -> cost defined specific to the problem
  + defined by network algorithm
    + cost could be 1 -> shortest path
    + could be related to congestion level...

tradeoffs: 
+ routers have complete topology: **global**
  + *link state algorithms*
  + routers know everything everywhere about paths
  + dijkstras is an example of this
+ routers can be **decentralized** instead of global
  + iterative computation, exchange of info with neighbors; routers initially only know link costs to attached neighbors
  + *distance vector algos*
  + bellman-ford algo is example of this
+ static vs dynamic -- routes change slowly over time vs routes changing more quickly
  + another thing to take into consideration when setting up algos and cost considerations
  
## link state algo

**DIJKSTRAS**

+ centralized 
  + whole topology and link costs are known to the router-node
+ iterative
  + after k iterations, know least cost path to k destinations
+ computes least cost paths from src to ALL other nodes

you can get the forwarding table of a node from this by just saying -- if a node shortest path is in the direct connection, just send, otherwise send on next shortest path step, etc...

## distance vector algo

based on bellman ford eq