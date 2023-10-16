#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  // 创建节点容器，总共有4个节点
  NodeContainer nodes;
  nodes.Create (4);

  // 创建点对点连接的帮助类
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));  // 设置数据速率为10Mbps
  p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));        // 设置信道延迟为5ms

  // 安装点对点设备，连接四个节点，创建线性网络拓扑
  NetDeviceContainer devices;
  devices = p2p.Install (nodes.Get (0), nodes.Get (1));
  devices.Add (p2p.Install (nodes.Get (1), nodes.Get (2)));
  devices.Add (p2p.Install (nodes.Get (2), nodes.Get (3)));

  // 安装互联网协议栈
  InternetStackHelper stack;
  stack.Install (nodes);

  // 设置IPv4地址分配
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // 创建主机节点容器
  NodeContainer hosts;
  hosts.Create (1);

  // 创建服务器节点容器
  NodeContainer servers;
  servers.Create (1);

  // 安装主机到路由器连接和服务器到路由器连接
  NetDeviceContainer hostToR1 = p2p.Install (hosts.Get (0), nodes.Get (0));
  NetDeviceContainer serverToR4 = p2p.Install (servers.Get (0), nodes.Get (3));

  // 安装互联网协议栈到主机和服务器
  stack.Install (hosts);
  stack.Install (servers);

  // 设置UDP回显服务器的端口
  uint16_t port = 8080;

  // 创建UDP回显服务器和客户端应用程序
  ApplicationContainer serverApps, clientApps;

  // 创建UDP回显服务器
  UdpEchoServerHelper echoServer (port);
  serverApps.Add (echoServer.Install (servers.Get (0)));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  // 创建UDP回显客户端
  UdpEchoClientHelper echoClient (interfaces.GetAddress (3), port);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  clientApps.Add (echoClient.Install (hosts.Get (0)));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // 创建紫色网络
  NodeContainer purpleNetwork;
  purpleNetwork.Create (1);

  // 创建紫色网络到路由器R2的连接
  NetDeviceContainer purpleToR2 = p2p.Install (purpleNetwork.Get (0), nodes.Get (1));

  // 创建灰色网络
  NodeContainer grayNetwork;
  grayNetwork.Create (1);

  // 创建灰色网络到路由器R3的连接
  NetDeviceContainer grayToR3 = p2p.Install (grayNetwork.Get (0), nodes.Get (2));

  // 设置TCP拥塞控制算法为NewReno
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpNewReno::GetTypeId ()));

  // 创建并运行仿真，仿真时间为10秒
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
