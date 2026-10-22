# gaps to fill for midterm/weird questions

+ what IS the cwnd? what IS the rwnd?
  + 14A
+ is there a delay before the tcp sender starts sending things after the ack in the 3 way handshake is sent? or does it start sending right away, assuming the ack will arrive?
+ gobackn -> does it send packets immediately once the window shifts or does it wait for all to get ack'd that are expecting ack?
+ fast retransmit -- does it retransmit all packets or just the implied loss?
  + 17B; does TCP buffer? can't recall

## questions on review to revisit and why

+ 6
+ 7 (nums and answers)
+ 8b, 8d
+ 9b -- demulti generally
+ 11
+ 13c -> believe its if more packets sent and we get 3 dup acks, but think/look for any other reasons.
+ 14 -> think 1536B, but more specifically B; I think that's when A will send gentle prods until rwnd is not 0, but regardless it stops sending bytes of data i believe
+ 15 -> ss mode is just when the cwnd < ssthresh, right? and do you set cwnd = ssthresh after the halving? or before? implication is after.
+ 18C! and D

### qs left (minimum)
+ 19
+ 1 - 5 (delays)
+ the interactive problems on that website would be stellar


# cheatsheet things

+ rdt versions? i don't want to waste space with that but if i do i do, and would love to not waste time memorization that.
+ principles of rdt
~~+ d_nodal = d_q + d_proc + d_t + d_prop~~
  ~~+ diff between d_t and d_prop?~~
~~+ tcp rules (see TCP Problems on the review sheet)~~ 
+ pic of the network stack with little hint as to what each layer focuses on