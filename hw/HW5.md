# HW 5

## Q1

Suppose a process in Host C has a UDP socket with port number 6789. Suppose both Host A and Host B each send a UDP segment to Host C with destination port number 6789. Will both of these segments be directed to the same socket at Host C? If so, how will the process at Host C know that these two segments originated from two different hosts?

Now suppose instead that Host A and Host B each open a TCP connection to that same port on Host C. Are these two connections handled by the same socket? Explain what's different about how TCP demultiplexes compared to UDP.

### A1

UDP only uses the tuple (destination IP, destination port) to route incoming segments. This has the consequence that despite the source machine address/port, all traffic bound for that (destination IP, destination port) will be routed to the same socket. UDP can differentiate between clients, however, because information grabbed from IP headers. The operating system esssentially snags the source IP and destination IP out of the IP header (that technically UDP never sees) and creates a pseudo-IP header for UDP to utilize in checksumming. However, that does mean that UDP has the information necessary (source IP and port) to distinguish between clients.

TCP, however, uses 4 pieces of information in its tuple to route: (source IP, source port, destination IP, destination port). This is, from a high level, evidence of the "tunnel" of TCP achieves--the destination IP and port are the same information given to UDP, but TCP uses the unique information of the source IP and port to distinguish segments to specific clients' sockets. The segment will be routed to a unique socket-per-client that is differentiated by the source IP and port information, and not all to the same socket like UDP. The IP information likewise comes from the psuedo-IP header as well.

Sources other than textbook: 
[UDP further reading](https://www.geeksforgeeks.org/computer-networks/user-datagram-protocol-udp/)
[TCP header formatting](https://www.geeksforgeeks.org/computer-networks/tcp-ip-packet-format/), [TCP](https://www.geeksforgeeks.org/computer-networks/what-is-transmission-control-protocol-tcp/)
[psuedo-header mentions, generally more information about this stack](https://www.w3.org/People/Frystyk/thesis/TcpIp.html)

## Q2

Why is it that voice and video traffic is often sent over TCP rather than UDP in today’s Internet? (Hint: The answer we are looking for has nothing to do with TCP’s congestion-control mechanism.)

### A2

On the congestion control note: congestion control can actually be a pain when streaming because the throttling due to other network traffic can slow things down for a service that is expected to be as fast and high quality as possible (especially video).

The other thoughts as to why the preference:

+ **Packets are guaranteed to be in order, and not lost**: when we're dealing with data streams where chronology/sequence matters, such as a movie being streamed, we want to ensure we dont miss scenes (packet loss) or scenes are arriving a non-guaranteed chronological order (no guarantee of packet ordering). TCP addresses packet loss and ordering issues, so this is a perk for sequential data streams.
+ **Security concerns**: many firewalls will block UDP traffic due to potential abuse of the port. So, it is more of a guarantee to use TCP instead, avoiding most of the blocking people's machines may have.
  + **IP spoofing -- security example**: It is far more possible to "spoof" an IP address with UDP rather than TCP. With UDP, it is somewhat simple to alter the IP address on the IPv4 header and send off to another machine, resulting in the IP injection address to receive a potentially malicious packet ("reflection/amplification attach"); another risk is denial of service (DoS) attacks, where the malicious actor sends a ton of requests via randomly generated/inaccurate IPs to protect their IP and confuse the receiving machine to not be able to tell what traffic is legitimate and which is malicious. 
  
  With TCP, the malicious actor must be in on the be able to correctly guess at the sequence numbers of the interaction; otherwise the request is "trashed". This is just one example of why UDP is blocked, but an interesting one.
  + [interesting read on ip spoofing with tcp.](https://security.stackexchange.com/questions/139408/ip-spoofing-with-real-ip-when-tcp-3-way-handshake-has-been-made) and [spoofing with udp](https://security.stackexchange.com/questions/155007/is-it-possible-to-successfully-send-a-spoofed-udp-header-with-a-completely-unrel)

#### A2 aux

pirating haha -- TLS encryption instead of stealing unencrypted data; maybe not though, TCP doesn't really encrypt, TLS does, and udp can encrypt if it wants to in a layer above
+ extra thought though -- the [handshake prevents IP spoofing](https://en.wikipedia.org/wiki/IP_address_spoofing).

things don't arrive out of order -- so like if we're sending a movie, we don't want to play out of order

reliability -- things actually get there, don't wnat a mess

security -- sometimes udp gets blocked

[interesting take on the question itself](https://community.spiceworks.com/t/udp-and-firewalls/814648/6)
+ not necessarily a simple 
  
[wtf in the sketchy is this](https://code.kryo.se/iodine/README.html)

## Q3

Visit the Go-Back-N interactive animation at the K&R textbook companion Web site here [Links to an external site..](https://media.pearsoncmg.com/ph/esm/ecs_kurose_compnetwork_8/cw/content/interactiveanimations/go-back-n-protocol/index.html)

+ Have the source send five packets, and then pause the animation before any of the five packets reach the destination. Then kill the first packet and resume the animation. Describe what happens.
+ Repeat the experiment, but now let the first packet reach the destination and kill the first acknowledgment. Describe again what happens.
+ Finally, try sending six packets. What happens?

### A3

1. **Kill first packet before anything reaches dest**: The server "trashes" packets 1 - 4 since packet 0 never was received, and 1 - 4 are therefore considered out of order. Then, the timer for packet 0 times out and the client sends all packets in the window again (0 - 4). Upon receipt of all these packets now, the server accepts them all (all showing up in order) and sends back an ACK for all 5 packets. Once ACK received up to a specific number (cumulative ACK) received, the sliding window shifts up to that number. So, all ACK received -> sliding window shifts up to start at 5.

2. **First packet makes it, kill first acknowledgement**: The sliding window still slides to 5. This is because a "cumulative ACK" is used. Meaning: if the ACK returned says it got all packets up to that number. So, the ACK for packet 2 means that the server received packets 0 - 2. This works because if the server receives packet 2 and it has not gotten packets 0 - 1, it trashes packet 2 since it is then considered "out of order" by the servers internal protocol tracking.

3. **Sending 6 packets**: You cannot send 6 packets until the window slides. For this protocol, you cannot send sequence numbers outside of the current window (there can only be N unACK'd packets in the pipeline)--for example, you need to wait for packet 0+ to get an ACK back before you can send packet 6. This limitation allows for pipelining and making the most of a network capability while also ensure the complexity and tracking isn't overly complex.

## Q4

Did you use Generative AI for this assignment? If so, how?

### A4

I did not use GenAI for this assignment.