# HW 7

## Q1

Paste a quote from the reading that you found interesting (1-3 sentences). Include the section or page number.

### A1

"The principle of 'design for choice' would imply that users should be able to choose with whom they interact, and users should be able to choose the level of transparency they offer to other users. The principle of 'tussle isolation' suggests that these mechanisms should not be overloaded on to any other mechanism, but should be separated. Further, one should consider if within the broad topic of trust, there are separable issues."

-- Section 3.2, Page 6

## Q2

Explain why you picked this quote in at least 5 sentences. Make sure to engage with the material. You will not get points if you just repeat or rephrase what the paper is saying. The idea here is to get your personal reflection even (or especially) if it is not a polished thought.  

### A2

I chose this partially because I can definitely identify with the frustration of being booted by a firewall for whatever I'm trying to accomplish and being irritated by the lack of choice. The entire discussion about choice of trust was interesting, especially the further comments on whether or not transparency about the firewall rules/setup decided upon by a system administrator should be clear or not to the user being affected. There is a level of secrecy I can understand--the entire point of a firewall is to protect the user and host, so opening up the internals or being vague as to the reason I cannot do something is not unreasonable. But it points back to something in my head that came up in our last discussion (within our group): the idea of a network layer dedicated to security--somewhere between application and transport, or after transport. It might not be a good thought, but it was spurred due to the huge security concerns in the modern internet. And to a degree, all of that protection is stacked on the application layer as a responsibility of developers (or in things like TLS as a bandaid on HTTP, TCP, etc...). A firewall almost feels like a security layer; would it be possible to extract that out as its own layer, not to the core, but as its own set of standardized protocols to protect, encrypt, and block? And then, perhaps, it could give users more choice in how they want to share their information, or allow certain application layer items accept different protocols due to different characteristics. This is a half baked thought, but something that I've been continuing to ponder since that discussion, and a tie back in to the reading on user choice design and security concerns.

## Q3

Propose one question or topic you'd like to discuss in small groups on Tuesday. It must connect to networking somehow, ideally something related to this week's reading. It can build on the quote you chose or come from a different part of the paper. In 1-2 sentences, explain why you think it would make a good discussion.

### A3

I think it would be interesting to talk about security protocols as a potential flexibility to the network stack. As in, should we take things like HTTPS and extract out the TLS to its own dedicated layer, allowing for the diversification of secure network interactions that can be chosen by a developer to use for their application or by a user using the application? Is this a valid option, or would it just cause too much of a mess for users? I honestly just think its an interesting notion to consider protocols as a replacement for harsh firewalls, and what are the benefits and drawbacks of each approach. Security and personal information leaking is becoming more and more of a concern these days especially, so I think this ties in well with the sign of the times, so to speak.

## Q4

Did you use Generative AI for this assignment? If so, how? 

### A4

I did not use generative AI for this assignment.


## aux notes

from the intro: 

**This paper explores one important reality that surrounds the Internet today: different stakeholders that are part of the Internet milieu have interests that may be adverse to each other, and these parties each vie to favor their particular interests. We call this process "the tussle".**

how do we plan/deal with internet design in such a way to balance the wants of the huge variety of stakeholders using the system?


```
However, as the Internet be-
comes mainstream it inevitably moves from being an engi-
neering curiosity to being a mirror of the societies in which
it operates. The Internet may have been designed by engi-
neers, but its behavior (and its evolution) is by no means
predictable today.
```


```
his modularity allows tussles about QoS
to be played out without distortions, such as demands that
encryption be avoided simply to leave well-known port in-
formation visible or the encapsulation of applications inside
other applications simply to receive better service
```

satruday night stay

As an example of similar behavior in the Internet, some
acceptable use policies for residential broadband access pro-
hibit the operation of a server in the home. To run a server,
the customer is required to pay a higher “business” rate.
Customers who wish to sidestep this restriction can respond
by shifting to another provider, if there is one, or by tunnel-
ing to disguise the port numbers being used. The probable
outcome of this tussle depends strongly on whether one per-
ceives competition as currently healthy in the Internet, or
eroding to dangerous lev



The principle of “design for choice” would imply that users
should be able to choose with whom they interact, and users
should be able to choose the level of transparency they offer
to other users. The principle of “tussle isola..."...