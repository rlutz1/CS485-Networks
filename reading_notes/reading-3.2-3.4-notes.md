# Reading 3.2 - 3.4 Notes

## multiplexing, demultiplexing

"extending host to host network layer to process to process transport layer"

delivering data does not go directly to process, but to the socket intermediary
+ processes can have multiple sockets

**receiving, demultiplexing** -- transport layer examines headers to id the receiving socket and directs the segment to that socket
+ looking at what letters are addressed to who and handing them out 

**sending, multiplexing** -- bundling up data and header information to pass on to the network layer
+ collecting letters and handing them to the mail person

### port numbers

unique id for a socket

port numbers are a **16 bit** number (0 - 65535)
+ 0 - 1023 are **well known**
  + typically for well known protocols like http, restricted
+ well knowns outlined in RFC 1700, updated at IANA

UDP basically examines the destination port number in segment and directs segment to corresponding socket
-> TCP is more subtle

![ports in multiplex](images/image-43.png)


### connectionless (UDP) multi/demulti

you can get an automatic port assigned by OS or bind a specific one (1024 - 65535)
  + python `.bind("", 12345)`

```
process -> 12345 -> UDP multi ----net----> UDP demulti -> 23456 -> process
```

### connectionful (TCP) multi/demulti

UDP is defined by (destination IP, dest port)
+ don't technically need any source info

TCP is defined by (dest IP, dest port, source IP, source port)
+ all 4 values are used to demulti to the correct socket

**weird subtelty**
+ UDP, when it demultis, if two segments sent have same dest IP, port, it'll demulti *to the same socket*; if the source IP, port is different, it makes no difference.
+ TCP, on the other hand, uses all 4 values to direct to the correct socket, so if same dests and different sources, these two segments will go to *2 different sockets*.
  + this is the real evidence for two different clients having a "tunnel" to the server.

![tcp perconnection http processes](images/image-44.png)

**NOTE:** not typically a 1-to-1 correspondence between client connection and process (the http processes seen above)
+ typically today: a new server thread and connection socket is spawned for each new client that is attached to a single http process

**nmap** -- a port scanner that can see what applications are attached to which ports
+ can be handy if you want to see if a security vulnerable application is running so you can attack ;)

![non persistent tcp reading](images/image-45.png)


## UDP in depth

at the very least, there must be a transport service multi/demulti offered to even know where tf data needs to go on a machine

*defined in RFC 768*

UDP does 
+ multi/demulti
+ light error checking (checksum)
+ adds nothing to IP (app is almost directly talking with IP when using UDP)

DNS uses UDP (haha)
+ can always re-try a request if something seems to fail (no reply received).

**reasons UDP is used**
+ **finer app control over what data sent and when**
  + TCP has congestion control which could throttle traffic
  + TCP resends segments until receipt has been ack by dest -- sometimes real-time apps have a maximum sending rate
+ **no connection establishment**
  + no connection *delay*
  + dns likely uses this for this reason -- to avoid the introduced delay
+ **no connection state**
  + no tracking of parameters for each connection, so more active clients can typically be supported
+ **small packet header overhead**
  + TCP header -> 20 bytes, UDP header -> 8 bytes

sometimes in modern eraHTTP actually runs over UDP and there is application level error checking.
+ UDP preferred for network management since typically those applications are used when network is in stressed state

**NOTE**: sometimes udp blocked for security

![udp/tcp usage by apps](images/image-46.png)

**udp can be great, but the potential congestion issues could cause issues for udp and tcp users alike**

### udp segment

header has only 4 fields -- 2 bytes each, 8 bytes total
1. source port
2. dest port
3. length -- num bytes in udp segment, header + data
4. checksum - used by recipient to check if errors introduced to segment (others use it too, but not mentioned here)

and then the data field is the message passed by application layer

![udp segment](images/image-47.png)

#### checksum

**error detection**

"UDP at the sender side performs the 1s complement of the sum of all the 16-bit words in the segment, with any overflow encountered during the sum being wrapped around"

**addition (3 2 byters):**

![adding](images/image-48.png)
+ last addition had overflow, wrapped around

**ones compliment:**

+ obtained by converting all 0s -> 1s, 1s -> 0s
+ ^^^ this is the **checksum**!

![check sum](images/image-49.png)

**if all 4 2byters are added and no errors, ALL ONES! 11111111....if errors, at least 1+ zero will be in the sum**

note that there is no recovery from the error, just a knowledge of it
+ some udp implementations discard the msg, some pass on with a warning

## priniciples of reliable data transfer (intro to TCP)


