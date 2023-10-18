#include "ns3/core-module.h"       // 包含ns3核心模块的头文件
#include "ns3/network-module.h"    // 包含ns3网络模块的头文件
#include "ns3/internet-module.h"   // 包含ns3互联网模块的头文件
#include "ns3/point-to-point-module.h" // 包含ns3点对点模块的头文件
#include "ns3/applications-module.h"   // 包含ns3应用程序模块的头文件
#include <string>

using namespace ns3;   // 使用ns3命名空间
using namespace std;   // 使用C++标准库的std命名空间

// 定义日志组件的名字
NS_LOG_COMPONENT_DEFINE ("AtcnCaseStudy");

// 定义用于跟踪拥塞窗口的回调函数
void tracer_CWnd(uint32_t x_old, uint32_t x_new) {
    cout << "TCP_Cwnd" << "," << Simulator::Now().GetNanoSeconds() << "," << x_new << endl;
    // 输出拥塞窗口变化的信息，包括时间和新的窗口大小
}

// 指定总传输字节数
static const uint32_t totalTxBytes = 2000000;
// 指定当前传输字节数
static uint32_t currentTxBytes = 0;

// 指定每次写入的数据大小
static const uint32_t writeSize = 1040;
uint8_t data[writeSize];

// 写数据直到缓冲区满
void WriteUntilBufferFull (Ptr<Socket> localSocket, uint32_t txSpace);

// 启动流
void StartFlow (Ptr<Socket> localSocket, Ipv4Address servAddress, uint16_t servPort) {
    localSocket->Connect (InetSocketAddress (servAddress, servPort));
    localSocket->SetSendCallback (MakeCallback (&WriteUntilBufferFull));
    WriteUntilBufferFull (localSocket, localSocket->GetTxAvailable ());
    // 启动数据传输流，并设置回调以处理数据传输
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
    // 持续写数据直到缓冲区满或传输完成
}

int main (int argc, char *argv[]) {
    // 接受命令行参数
    CommandLine cmd (__FILE__);
    cmd.Parse (argc, argv);

    NodeContainer nodes;    // 创建节点容器
    nodes.Create (4);       // 创建4个路由节点

    PointToPointHelper p2pHelper;   // 创建点对点连接的帮助类
    p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("10Mbps")); // 设置数据速率
    p2pHelper.SetChannelAttribute ("Delay", StringValue ("5ms"));       // 设置信道延迟

    // 创建节点之间的点对点连接
    NetDeviceContainer R1_R2 = p2pHelper.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer R2_R3 = p2pHelper.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer R3_R4 = p2pHelper.Install(nodes.Get(2), nodes.Get(3));  //路由R3-R4
    
    InternetStackHelper stack;  // 创建互联网协议栈的帮助类
    stack.Install (nodes);     // 在节点上安装互联网协议栈

    Ipv4AddressHelper address;   // 创建IPv4地址帮助类
    address.SetBase ("10.1.1.0", "255.255.255.0");  // 设置IP地址和子网掩码
    Ipv4InterfaceContainer R1_2_intf = address.Assign (R1_R2); // 分配IP地址给设备
    address.SetBase ("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer R2_3_intf = address.Assign (R2_R3);
    address.SetBase ("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer R3_4_intf = address.Assign (R3_R4);
    // 创建主机节点容器(来自green)
    NodeContainer hosts;
    hosts.Create (1);

    // 创建服务器节点容器（来自host）
    NodeContainer servers;
    servers.Create (1);

    // 安装主机到路由器连接和服务器到路由器连接
    NetDeviceContainer hostToR1 = p2pHelper.Install (hosts.Get (0), nodes.Get (0));
    NetDeviceContainer serverToR4 = p2pHelper.Install (servers.Get (0), nodes.Get (3));
    address.SetBase ("10.1.4.0", "255.255.255.0"); 
    Ipv4InterfaceContainer host_R1_intf = address.Assign (hostToR1);
    address.SetBase ("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer R4_server_intf = address.Assign (serverToR4);

    // 安装互联网协议栈到主机和服务器
    stack.Install (hosts);
    stack.Install (servers);

      // 创建紫色和灰色网络
    NodeContainer purpleNetwork;
    purpleNetwork.Create (1);

   
    NodeContainer grayNetwork;
    grayNetwork.Create (1);
    // 连接
    NetDeviceContainer purpleToR2 = p2pHelper.Install (purpleNetwork.Get (0), nodes.Get (1));
    NetDeviceContainer grayToR3 = p2pHelper.Install (grayNetwork.Get (0), nodes.Get (2));
    address.SetBase ("10.1.6.0", "255.255.255.0"); 
    Ipv4InterfaceContainer purple_R2_intf = address.Assign (purpleToR2);
    address.SetBase ("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer gray_R3_intf = address.Assign (grayToR3);

    // 设置全局路由表
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    uint16_t port = 8080;

    // 创建本地套接字并绑定
    Ptr<Socket> localSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
    localSocket->Bind ();

    // 在仿真器中安排开始流
    Simulator::ScheduleNow (&StartFlow, localSocket, R3_4_intf.GetAddress (1), port);

    // 创建数据包接收端
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
    ApplicationContainer sinkApps = sink.Install (nodes.Get (5));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (10.0));

    // 连接到拥塞窗口的回调
    Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&tracer_CWnd));

    // 启用数据包捕获
    p2pHelper.EnablePcapAll("atcn-cs", true);

    Simulator::Stop (Seconds (10.0)); // 设置仿真停止时间
    Simulator::Run ();   // 运行仿真
    Simulator::Destroy ();  // 销毁仿真器

    return 0;
}
