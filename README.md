## the-Congestion-Control-Process-in-TCP-in-NS-3

Programming Assignment for CN2023: Understanding the Congestion Control  Process in TCP in NS-3.

## design goals

You need to simulate a multi-hop network shown in Fig.1. Concretely, the neighbored hosts and routers are all connected with each other through the point-to-point link with 10Mbps bandwidth and 5ms delay. In running, each host contained in the green cloud connect to a server in the blue cloud through a routing path of (R1, R2, R3, R4) through TCP. In addition, the TCP is adopted with a congestion algorithm like newReno, Reno and Vegas. 

Meanwhile, each host in the purple cloud sends UDP packets to a sink in the gray cloud at different intensities in sending rates via a path (R2, R3). The UDP is used to cause different congestion level of the link between (R2, R3) that can affect the QoS performance of the TCP.

More details:[link](https://github.com/zhouby-zjl/course-acn23a/blob/main/Experiments/Programming-Assignment.pdf)

## topological graph(Fig.1.)

<div align=center><img src="https://github.com/gitover22/the-Congestion-Control-Process-in-TCP-in-NS-3/blob/main/pic/topo.png" height="450"/> </div>

## about tcp_test.cc

This file is the source file from which the topology is built. However, it has not been further optimized

**tips**:you can copy it to your ns-allinone-3.35/ns-3.35/scratch,and input commands below to run it:

`./waf --run scratch/tcp_test.cc`