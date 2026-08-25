# Reading 2.1 - 2.2 Notes

pages 80 - 116

TODO: mention of socket programming exercises at end of chapter 2 

## 2.1: Principles of Network Applications

application software is designated to end-systems, so network core hardware only worries about the transmitting of information 
+ enables rapid development

![app level](images/image-2.png)

### architectural designs for network apps

more than just this, but usually

1. **client-server architecture**: client always requests something from server and server returns.
   + clients never talk directly, only through server
   + **data centers** are just many many servers
2. **peer-to-peer (P2P) architecture**: not talking through a dedicated server, but two smaller machines talk directly
   + example: bittorrent -> file sharing 
   + liked for scaleability
   + dislikes for security concerns, performance, and reliability

![p2p v cs](images/image-3.png)

communications between processes, whether p2p or client/server, communicate through **messages** with specific protocol

process sends messages *into and receives* messages via network through a *softwar interface* called a **socket**
+ process is a house, socket is a door
+ sending process assumes there's infrastructure to get its message on its way out the socket (door)
+ socket is kinda the API between the application and the network

![socket](images/image-4.png)

when sending messages need
1. destination address (IP)
2. know the process to send it to (or socket to send to) -> this comes in form of **port number**
   + some ports are specific to popular applications 
     + 80 -> web server 
     + 25 -> SMTP

### transport layer protocols

need to choose one when sending a message
+ like choosing to travel over car or plane--pick one that suits your purpose

ways to analyze the different protocols
1. **reliable data transfer** (fights packet loss)
2. **throughput** (guarantee at a specific rate; elastic apps provide as much as possible at the time)
3. **timing** (general time capping on delays)
4. **security** (encryption, etc)

![reqs](images/image-5.png)

#### TCP

+ handshack between client and server happens before any data sent
  + after that, back and forth flows freely
+ very reliable in transfer
+ TCP has a congestion-control where it throttles a sending process (client or server) when network congested
+ side note: TLS is an enhancement for TCP that adds encryption/authenticaion to the data passed around (TCP does not natively encrypt)
```
sending data -> TLS socket --> TLS -> TCP socket --...--> TCP socket --> TLS --> TLS socket --> receiving process
```
#### UDP

+ lightweight, no frills, minimal
+ no handshaking/connection made
+ unreliable
+ things may arrive out of order


![protos](images/image-6.png)

### app layer protocol

*stopped at 2.1.5, p94*

## 2.2: Web and HTTP