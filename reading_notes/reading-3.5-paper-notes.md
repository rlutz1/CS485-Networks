# Reading 3.5, Saltzer, Reed & Clark (1984), "End-to-End Arguments in System Design" Notes

## 3.5: 

## Paper

written: 1984

MIT paper

### purpose

"This paper presents a design principle that helps guide placement of functions among the modules of a distributed computer system" (p1)
+ distributed computer system == network

**end-to-end argument** -- the principle; functions placed at low levels of a system may be redundant or have little value compared with the cost of providing them at low level

**possible point:** -- we don't need to be doing error checking, costly mechanisms for security, etc if they have a high *computational?* cost and therefore impact performance
+ potentially: high cost should be offloaded to hosts and not on low level devices like packet switches?
+ being a system designer == deciding on "function placement" -- *separation of concerns, who does what job and when?*
  + "The argument appeals to application requirements and provides a rationale for **moving a function upward in a layered system closer to the application that uses the function.**" (p1)
    + whatever actually needs the thing done needs to be responsible for it to avoid things doing unnecessary work that doesn't impact them and affects overall performance
    + keep lower level network "dumb"
   
### general notes

+ considering where the errors of a file transfer application (as an example app) should be handled by the application (applications must be correct) or potentially give a recheck, like a checksum, as a responsibility of the application
+ consider instead: a guarantee given by the transmission protocol that the communication in between hosts won't be fucked up/lost.
  + but it succeeds in only reducing the checksum checking by the application, not in eliminating those other issues.
  + application must still ensure failure detection OUTSIDE of reliability

interesting piece:

```
An interesting example of the pitfalls that one can encounter turned up recently at the Massachusetts Institute of Technology. One network system involving several local networks connected by gateways used a packet checksum on each hop from one gateway to the next, on the assumption that the primary threat to correct communication was corruption of bits during transmission. Application programmers, aware of this checksum, assumed that the network was providing reliable transmission, without realizing that the transmitted data were unpro-tected while stored in each gateway. One gateway computer developed a transient error: while copying data from an input to an output buffer a byte pair was interchanged, with a frequency of about one such interchange in every million bytes passed. Over a period of time many of the source files of an operating system were repeatedly transferred through the defective gateway. Some of these source files were corrupted by byte exchanges, and their owners were forced to the ultimate end-to-end error check: manual comparison with and correction from old listings.
```
+ so they had files corrupted and they legit had to go into old versions and check against them :*(

interesting sentence:

```
The probability that all packets of a file arrive correctly decreases exponentially with the file length, and thus the expected time to transmit the file grows exponentially with file length.
```
(p280)
+ point being: some work at the lower levels to make sure that things get where they need to be is worth some computation
+ the end-to end check needs to still happen regardless of reliability of the comm system.
  + higher reliability will affect speed at the lower level, affect bandwidth
  + lower reliability offloads more comp to the application level to check more shit
    + **its a tradeoff as always**

+ acknowledgements should be end-to-end because they are not always necessary -- if it was implemented at the lower level, then there would be no opt out and unnecessary comp wasted there
  + encryption is the same argument
+ sometimes duplicates happen in the network -- many networks allow this to happen and dont care, up to app to detect and handle
  + it's already handled by most applications, so there's no point in adding that to a lower level system
+ haha SWALLOW
+ RISC