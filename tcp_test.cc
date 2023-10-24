#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h" 
#include "ns3/tcp-socket-factory.h"
#include <string>

using namespace ns3;
using namespace std;

//here is topo:
//                                           purple
//                                             |
//                                             |10.1.6.0
//                                             |                             10.1.4.0           10.1.5.0
//host(green)---------------R1-----------------R2------------------R3------------------R4----------------server(blue)
//               10.1.1.0          10.1.2.0          10.1.3.0      |
//                                                                 |10.1.7.0
//                                                                 |
//                                                                gray


NS_LOG_COMPONENT_DEFINE ("FirstNS3Codes");  // 日志组件
static const uint32_t totalTxBytes = 2000000;  // 指定总传输字节数
static uint32_t currentTxBytes = 0;  // 指定当前传输字节数
static const uint32_t writeSize = 1024;  // 指定每次写入的数据大小
uint8_t data[writeSize];  //数据区
// 写数据直到缓冲区满
void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace);
// 启动流
void StartFlow (Ptr<Socket> localSocket, Ipv4Address servAddress, uint16_t servPort);
// 跟踪拥塞窗口的回调函数
void tracer_CWnd(uint32_t x_old, uint32_t x_new);

int 
main (int argc, char *argv[]) {
    CommandLine cmd (__FILE__);
    cmd.Parse (argc, argv);
    Time::SetResolution(Time::NS);
    NodeContainer nodes;  //结点
    nodes.Create (6);

    PointToPointHelper p2pHelper;   // 创建点对点连接的帮助类
    p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("10Mbps")); 
    p2pHelper.SetChannelAttribute ("Delay", StringValue ("5ms"));

    // 创建路由器之间的点对点连接
    NetDeviceContainer R1_R2 = p2pHelper.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer R2_R3 = p2pHelper.Install(nodes.Get(2), nodes.Get(3));
    NetDeviceContainer R3_R4 = p2pHelper.Install(nodes.Get(3), nodes.Get(4));  //路由R3-R4
    // 安装主机到路由器连接和服务器到路由器连接
    NetDeviceContainer host_R1 = p2pHelper.Install (nodes.Get (0), nodes.Get (1));
    NetDeviceContainer server_R4 = p2pHelper.Install (nodes.Get (4), nodes.Get (5));
    InternetStackHelper stack;  // 创建互联网协议栈的帮助类
    stack.Install (nodes);

    // 配置ip
    Ipv4AddressHelper address;   // 创建IPv4地址帮助类
    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer R1_2_intf = address.Assign (R1_R2);
    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer R2_3_intf = address.Assign (R2_R3);
    address.SetBase ("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer R3_4_intf = address.Assign (R3_R4);
    address.SetBase ("10.1.1.0", "255.255.255.0"); 
    Ipv4InterfaceContainer host_R1_intf = address.Assign (host_R1);
    address.SetBase ("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer R4_server_intf = address.Assign (server_R4);

    // 创建紫色和灰色网络
    NodeContainer purpleNetwork;
    purpleNetwork.Create (1);
    NodeContainer grayNetwork;
    grayNetwork.Create (1);
    // 连接并配置网址ip
    NetDeviceContainer purple_R2 = p2pHelper.Install (purpleNetwork.Get (0), nodes.Get (2));
    NetDeviceContainer gray_R3 = p2pHelper.Install (grayNetwork.Get (0), nodes.Get (3));
    // 安装互联网协议栈到主机和服务器
    stack.Install (purpleNetwork);
    stack.Install (grayNetwork);
    address.SetBase ("10.1.6.0", "255.255.255.0"); 
    Ipv4InterfaceContainer purple_R2_intf = address.Assign (purple_R2);
    address.SetBase ("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer gray_R3_intf = address.Assign (gray_R3);
   
    // 设置全局路由表
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    uint16_t port = 8080;

    // 创建本地套接字并绑定
    Ptr<Socket> localSocket = Socket::CreateSocket (nodes.Get(0), TcpSocketFactory::GetTypeId ());
    // 创建 TCP NewReno 套接字
    // Ptr<Socket> localSocket = Socket::CreateSocket (nodes.Get(0), TcpNewReno::GetTypeId ());
    // 创建 TCP Reno 套接字
    //Ptr<Socket> localSocket = Socket::CreateSocket (nodes.Get(0), TcpLinuxReno::GetTypeId ());
    // 创建 TCP Vegas 套接字
    // Ptr<Socket> localSocket = Socket::CreateSocket (nodes.Get(0), TcpVegas::GetTypeId ());

    // 设置拥塞控制算法相关参数
    localSocket->Bind ();
    // localSocket->SetAttribute ("SegmentSize", UintegerValue (writeSize));
    // localSocket->SetAttribute ("SlowStartThreshold", UintegerValue (currentTxBytes));
    // localSocket->SetAttribute ("HighWaterMark", UintegerValue (totalTxBytes));
    Simulator::ScheduleNow (&StartFlow, localSocket, R4_server_intf.GetAddress (1), port);






    // 发送端
    BulkSendHelper bulksendHelper("ns3::TcpSocketFactory", InetSocketAddress(host_R1_intf.GetAddress(1), port));
    ApplicationContainer sendApp = bulksendHelper.Install(nodes.Get(0));
    sendApp.Start(Seconds(0.));
    sendApp.Stop(Seconds(20.));
    // 接收端
    PacketSinkHelper sink ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (nodes.Get (5));  //接受端
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (20.0));

    // 连接到拥塞窗口的回调
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&tracer_CWnd));
    p2pHelper.EnablePcapAll("tcp", true);  // 启用数据包捕获,捕获tcp相关的数据包
    Simulator::Stop (Seconds (25.0)); 
    Simulator::Run (); 
    Simulator::Destroy ();
    return 0;
}


// 写数据直到缓冲区满
void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace) {
    while (currentTxBytes < totalTxBytes && localSocket->GetTxAvailable () > 0) {
        uint32_t left = totalTxBytes - currentTxBytes;
        uint32_t dataOffset = currentTxBytes % writeSize;
        uint32_t toWrite = writeSize - dataOffset;
        toWrite = std::min (toWrite, left);
        toWrite = std::min (toWrite, localSocket->GetTxAvailable ());
        int amountSent = localSocket->Send (&::data[dataOffset], toWrite, 0);
        if(amountSent < 0) {
            return;
        }
        currentTxBytes += amountSent;
    }

    if (currentTxBytes >= totalTxBytes) {
        localSocket->Close ();
    }
}

void StartFlow (Ptr<Socket> localSocket, Ipv4Address servAddress, uint16_t servPort) {
    localSocket->Connect (InetSocketAddress (servAddress, servPort));
    localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
    WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
    // 启动数据传输流，并设置回调以处理数据传输
}

// 跟踪拥塞窗口的回调函数
void tracer_CWnd(uint32_t x_old, uint32_t x_new) {
    cout << "TCP_Cwnd" << "," << Simulator::Now().GetNanoSeconds() << "    old cwnd: "<<x_old<<"--->new cwnd:" << x_new << endl;
}