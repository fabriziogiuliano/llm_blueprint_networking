
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ExtendedSmartHomeNetwork");

int
main(int argc, char* argv[])
{
    // Configuration
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnable("ExtendedSmartHomeNetwork", LOG_LEVEL_INFO);

    // Simulation Time
    Time simulationTime = Hours(1);

    // Wi-Fi Access Point Node
    NodeContainer wifiApNodes;
    wifiApNodes.Create(1);

    // Wi-Fi Station Nodes
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(7);

    // Mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNodes);
    mobility.Install(wifiStaNodes);

    // Set positions based on distance_meters
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));         // AP-001
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));        // ST-001
    positionAlloc->Add(Vector(5.0, 0.0, 0.0));         // SLB-001
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));         // SDL-001
    positionAlloc->Add(Vector(15.0, 0.0, 0.0));        // SSC-001
    positionAlloc->Add(Vector(7.0, 0.0, 0.0));         // SRT-001
    positionAlloc->Add(Vector(12.0, 0.0, 0.0));        // SMS-001
    positionAlloc->Add(Vector(3.0, 0.0, 0.0));         // SVC-001
    

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiApNodes);
    mobility.Install(wifiStaNodes);

    // Wi-Fi Channel
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Wi-Fi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    // MAC
    WifiMacHelper mac;
    Ssid ssid = Ssid("SmartHomeAPMyExperiment");

    // Install AP
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, wifiApNodes.Get(0));

    // Install Stations
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    // WPA Configuration
    for (uint32_t i = 0; i < staDevices.GetN(); ++i)
    {
        Ptr<WifiNetDevice> wifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        Ptr<RegularWifiMac> regularWifiMac = DynamicCast<RegularWifiMac>(wifiNetDevice->GetMac());
        regularWifiMac->SetSsid(ssid);

        if (regularWifiMac != 0) {
          regularWifiMac->ConfigureStandard(WIFI_STANDARD_80211a);
        }
    }

    // Internet Stack
    InternetStackHelper stack;
    stack.Install(wifiApNodes);
    stack.Install(wifiStaNodes);

    // IP Addresses
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevices);
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign(staDevices);
   
    // Update IP addresses based on blueprint
    apInterface.SetAddress(0, Ipv4Address("192.168.1.1"));
    staInterfaces.SetAddress(0, Ipv4Address("192.168.1.103"));
    staInterfaces.SetAddress(1, Ipv4Address("192.168.1.102"));
    staInterfaces.SetAddress(2, Ipv4Address("192.168.1.101"));
    staInterfaces.SetAddress(3, Ipv4Address("192.168.1.100"));
    staInterfaces.SetAddress(4, Ipv4Address("192.168.1.104"));
    staInterfaces.SetAddress(5, Ipv4Address("192.168.1.105"));
    staInterfaces.SetAddress(6, Ipv4Address("192.168.1.106"));

    // UDP/TCP Applications
    // ST-001 (UDP, Maximum Throughput)
    uint16_t st001Port = 9;
    OnOffHelper st001Udp("ns3::UdpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), st001Port));
    st001Udp.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    st001Udp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    st001Udp.SetAttribute("DataRate", StringValue("2000Mbps")); 
    st001Udp.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer st001UdpApp = st001Udp.Install(wifiStaNodes.Get(0));
    st001UdpApp.Start(Seconds(0));
    st001UdpApp.Stop(Seconds(1800));

    // SLB-001 (TCP, Maximum Throughput)
    uint16_t slb001Port = 10;
    OnOffHelper slb001Tcp("ns3::TcpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), slb001Port));
    slb001Tcp.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    slb001Tcp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    slb001Tcp.SetAttribute("DataRate", StringValue("2000Mbps"));
    slb001Tcp.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer slb001TcpApp = slb001Tcp.Install(wifiStaNodes.Get(1));
    slb001TcpApp.Start(Seconds(0));
    slb001TcpApp.Stop(Seconds(1800));

    // SDL-001 (UDP, 1 Mbps)
    uint16_t sdl001Port = 11;
    OnOffHelper sdl001Udp("ns3::UdpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), sdl001Port));
    sdl001Udp.SetConstantRate(DataRate("1Mbps"));
    sdl001Udp.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer sdl001UdpApp = sdl001Udp.Install(wifiStaNodes.Get(2));
    sdl001UdpApp.Start(Seconds(10));
    sdl001UdpApp.Stop(Seconds(610));

    // SSC-001 (UDP, 2 Mbps)
    uint16_t ssc001Port = 12;
    OnOffHelper ssc001Udp("ns3::UdpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), ssc001Port));
    ssc001Udp.SetConstantRate(DataRate("2Mbps"));
    ssc001Udp.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer ssc001UdpApp = ssc001Udp.Install(wifiStaNodes.Get(3));
    ssc001UdpApp.Start(Seconds(5));
    ssc001UdpApp.Stop(Seconds(605));

    // SRT-001 (TCP, 500 kbps)
    uint16_t srt001Port = 13;
    OnOffHelper srt001Tcp("ns3::TcpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), srt001Port));
    srt001Tcp.SetConstantRate(DataRate("500kbps"));
    srt001Tcp.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer srt001TcpApp = srt001Tcp.Install(wifiStaNodes.Get(4));
    srt001TcpApp.Start(Seconds(30));
    srt001TcpApp.Stop(Seconds(930));

    // SMS-001 (UDP, 1.5 Mbps)
    uint16_t sms001Port = 14;
    OnOffHelper sms001Udp("ns3::UdpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), sms001Port));
    sms001Udp.SetConstantRate(DataRate("1.5Mbps"));
    sms001Udp.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer sms001UdpApp = sms001Udp.Install(wifiStaNodes.Get(5));
    sms001UdpApp.Start(Seconds(45));
    sms001UdpApp.Stop(Seconds(345));

    // SVC-001 (TCP, Maximum Throughput)
    uint16_t svc001Port = 15;
    OnOffHelper svc001Tcp("ns3::TcpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), svc001Port));
    svc001Tcp.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    svc001Tcp.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    svc001Tcp.SetAttribute("DataRate", StringValue("2000Mbps"));
    svc001Tcp.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer svc001TcpApp = svc001Tcp.Install(wifiStaNodes.Get(6));
    svc001TcpApp.Start(Seconds(15));
    svc001TcpApp.Stop(Seconds(1215));

    // Packet Sink on AP
    PacketSinkHelper sinkUdp("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), st001Port));
    ApplicationContainer sinkUdpApps = sinkUdp.Install(wifiApNodes.Get(0));
    sinkUdpApps.Start(Seconds(0));
    sinkUdpApps.Stop(simulationTime);

    PacketSinkHelper sinkTcp("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), slb001Port));
    ApplicationContainer sinkTcpApps = sinkTcp.Install(wifiApNodes.Get(0));
    sinkTcpApps.Start(Seconds(0));
    sinkTcpApps.Stop(simulationTime);

    PacketSinkHelper sinkUdpSdl("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sdl001Port));
    ApplicationContainer sinkUdpAppsSdl = sinkUdpSdl.Install(wifiApNodes.Get(0));
    sinkUdpAppsSdl.Start(Seconds(0));
    sinkUdpAppsSdl.Stop(simulationTime);

    PacketSinkHelper sinkUdpSsc("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ssc001Port));
    ApplicationContainer sinkUdpAppsSsc = sinkUdpSsc.Install(wifiApNodes.Get(0));
    sinkUdpAppsSsc.Start(Seconds(0));
    sinkUdpAppsSsc.Stop(simulationTime);

    PacketSinkHelper sinkTcpSrt("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), srt001Port));
    ApplicationContainer sinkTcpAppsSrt = sinkTcpSrt.Install(wifiApNodes.Get(0));
    sinkTcpAppsSrt.Start(Seconds(0));
    sinkTcpAppsSrt.Stop(simulationTime);

    PacketSinkHelper sinkUdpSms("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sms001Port));
    ApplicationContainer sinkUdpAppsSms = sinkUdpSms.Install(wifiApNodes.Get(0));
    sinkUdpAppsSms.Start(Seconds(0));
    sinkUdpAppsSms.Stop(simulationTime);

    PacketSinkHelper sinkTcpSvc("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), svc001Port));
    ApplicationContainer sinkTcpAppsSvc = sinkTcpSvc.Install(wifiApNodes.Get(0));
    sinkTcpAppsSvc.Start(Seconds(0));
    sinkTcpAppsSvc.Stop(simulationTime);
    // Enable PCAP
    phy.EnablePcap("extended-smart-home-ap", apDevices.Get(0));
    phy.EnablePcap("extended-smart-home-st", staDevices);

    // Run Simulation
    Simulator::Stop(simulationTime);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
