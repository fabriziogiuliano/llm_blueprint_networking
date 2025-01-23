
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ExtendedSmartHomeNetwork");

int main(int argc, char* argv[]) {
    // Configuration
    double simulationTime = 3600; // 1 hour

    // WiFi Configuration
    NodeContainer wifiApNodes;
    wifiApNodes.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(7);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("SmartHomeAPMyExperiment");

    // AP Configuration
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, wifiApNodes);
    Ptr<WifiNetDevice> apDevice = StaticCast<WifiNetDevice>(apDevices.Get(0));
    apDevice->SetAddress(Mac48Address::Allocate());

    // STA Configuration
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    // Mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));       // AP-001
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));      // ST-001
    positionAlloc->Add(Vector(5.0, 0.0, 0.0));       // SLB-001
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));       // SDL-001
    positionAlloc->Add(Vector(15.0, 0.0, 0.0));      // SSC-001
    positionAlloc->Add(Vector(7.0, 0.0, 0.0));      // SRT-001
    positionAlloc->Add(Vector(12.0, 0.0, 0.0));      // SMS-001
    positionAlloc->Add(Vector(3.0, 0.0, 0.0));       // SVC-001

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNodes);
    mobility.Install(wifiStaNodes);

    // Internet Stack
    InternetStackHelper stack;
    stack.Install(wifiApNodes);
    stack.Install(wifiStaNodes);

    // IP Addressing
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevices);
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign(staDevices);

    // Set STA IP Addresses
    Ipv4AddressHelper::Assign(staDevices.Get(0), Ipv4Address("192.168.1.103"), Ipv4Mask("255.255.255.0"));
    Ipv4AddressHelper::Assign(staDevices.Get(1), Ipv4Address("192.168.1.102"), Ipv4Mask("255.255.255.0"));
    Ipv4AddressHelper::Assign(staDevices.Get(2), Ipv4Address("192.168.1.101"), Ipv4Mask("255.255.255.0"));
    Ipv4AddressHelper::Assign(staDevices.Get(3), Ipv4Address("192.168.1.100"), Ipv4Mask("255.255.255.0"));
    Ipv4AddressHelper::Assign(staDevices.Get(4), Ipv4Address("192.168.1.104"), Ipv4Mask("255.255.255.0"));
    Ipv4AddressHelper::Assign(staDevices.Get(5), Ipv4Address("192.168.1.105"), Ipv4Mask("255.255.255.0"));
    Ipv4AddressHelper::Assign(staDevices.Get(6), Ipv4Address("192.168.1.106"), Ipv4Mask("255.255.255.0"));
    

    // Set STA MAC Addresses
    StaticCast<WifiNetDevice>(staDevices.Get(0))->SetAddress(Mac48Address("61:5F:64:5E:90:EB"));
    StaticCast<WifiNetDevice>(staDevices.Get(1))->SetAddress(Mac48Address("60:36:1E:9A:0A:0C"));
    StaticCast<WifiNetDevice>(staDevices.Get(2))->SetAddress(Mac48Address("39:9F:51:CD:F7:08"));
    StaticCast<WifiNetDevice>(staDevices.Get(3))->SetAddress(Mac48Address("0B:E3:41:0A:33:B7"));
    StaticCast<WifiNetDevice>(staDevices.Get(4))->SetAddress(Mac48Address("A1:B2:C3:D4:E5:F6"));
    StaticCast<WifiNetDevice>(staDevices.Get(5))->SetAddress(Mac48Address("F1:E2:D3:C4:B5:A6"));
    StaticCast<WifiNetDevice>(staDevices.Get(6))->SetAddress(Mac48Address("1A:2B:3C:4D:5E:6F"));
    

    // WPA2-PSK Security
    std::string passphrase = "12345678";
    Config::SetDefault("ns3::WifiNetDevice::HtSupported", BooleanValue(false));

    // Applications
    // ST-001 (UDP, maximum throughput)
    uint16_t portSt001 = 9;
    OnOffHelper onOffSt001("ns3::UdpSocketFactory",
                           InetSocketAddress(apInterface.GetAddress(0), portSt001));
    onOffSt001.SetAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    onOffSt001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSt001.SetAttribute("MaxBytes", UintegerValue(0));
    onOffSt001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1800]"));
    onOffSt001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppsSt001 = onOffSt001.Install(wifiStaNodes.Get(0));
    clientAppsSt001.Start(Seconds(0));
    clientAppsSt001.Stop(Seconds(1800));

    PacketSinkHelper sinkSt001("ns3::UdpSocketFactory",
                               InetSocketAddress(Ipv4Address::GetAny(), portSt001));
    ApplicationContainer sinkAppsSt001 = sinkSt001.Install(wifiApNodes.Get(0));
    sinkAppsSt001.Start(Seconds(0));
    sinkAppsSt001.Stop(Seconds(1800));

    // SLB-001 (TCP, maximum throughput)
    uint16_t portSlb001 = 10;
    OnOffHelper onOffSlb001("ns3::TcpSocketFactory",
                            InetSocketAddress(apInterface.GetAddress(0), portSlb001));
    onOffSlb001.SetAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    onOffSlb001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSlb001.SetAttribute("MaxBytes", UintegerValue(0));
    onOffSlb001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1800]"));
    onOffSlb001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppsSlb001 = onOffSlb001.Install(wifiStaNodes.Get(1));
    clientAppsSlb001.Start(Seconds(0));
    clientAppsSlb001.Stop(Seconds(1800));

    PacketSinkHelper sinkSlb001("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), portSlb001));
    ApplicationContainer sinkAppsSlb001 = sinkSlb001.Install(wifiApNodes.Get(0));
    sinkAppsSlb001.Start(Seconds(0));
    sinkAppsSlb001.Stop(Seconds(1800));

    // SDL-001 (UDP, 1 Mbps, 600 seconds)
    uint16_t portSdl001 = 11;
    OnOffHelper onOffSdl001("ns3::UdpSocketFactory",
                            InetSocketAddress(apInterface.GetAddress(0), portSdl001));
    onOffSdl001.SetAttribute("DataRate", DataRateValue(DataRate("1Mbps")));
    onOffSdl001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSdl001.SetAttribute("MaxBytes", UintegerValue(75000000));
    onOffSdl001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=600]"));
    onOffSdl001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppsSdl001 = onOffSdl001.Install(wifiStaNodes.Get(2));
    clientAppsSdl001.Start(Seconds(10));
    clientAppsSdl001.Stop(Seconds(610));

    PacketSinkHelper sinkSdl001("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), portSdl001));
    ApplicationContainer sinkAppsSdl001 = sinkSdl001.Install(wifiApNodes.Get(0));
    sinkAppsSdl001.Start(Seconds(10));
    sinkAppsSdl001.Stop(Seconds(610));

    // SSC-001 (UDP, 2 Mbps, 600 seconds)
    uint16_t portSsc001 = 12;
    OnOffHelper onOffSsc001("ns3::UdpSocketFactory",
                            InetSocketAddress(apInterface.GetAddress(0), portSsc001));
    onOffSsc001.SetAttribute("DataRate", DataRateValue(DataRate("2Mbps")));
    onOffSsc001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSsc001.SetAttribute("MaxBytes", UintegerValue(150000000));
    onOffSsc001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=600]"));
    onOffSsc001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppsSsc001 = onOffSsc001.Install(wifiStaNodes.Get(3));
    clientAppsSsc001.Start(Seconds(5));
    clientAppsSsc001.Stop(Seconds(605));

    PacketSinkHelper sinkSsc001("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), portSsc001));
    ApplicationContainer sinkAppsSsc001 = sinkSsc001.Install(wifiApNodes.Get(0));
    sinkAppsSsc001.Start(Seconds(5));
    sinkAppsSsc001.Stop(Seconds(605));

    // SRT-001 (TCP, 500 kbps, 900 seconds)
    uint16_t portSrt001 = 13;
    OnOffHelper onOffSrt001("ns3::TcpSocketFactory",
                            InetSocketAddress(apInterface.GetAddress(0), portSrt001));
    onOffSrt001.SetAttribute("DataRate", DataRateValue(DataRate("500kbps")));
    onOffSrt001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSrt001.SetAttribute("MaxBytes", UintegerValue(56250000));
    onOffSrt001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=900]"));
    onOffSrt001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppsSrt001 = onOffSrt001.Install(wifiStaNodes.Get(4));
    clientAppsSrt001.Start(Seconds(30));
    clientAppsSrt001.Stop(Seconds(930));

    PacketSinkHelper sinkSrt001("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), portSrt001));
    ApplicationContainer sinkAppsSrt001 = sinkSrt001.Install(wifiApNodes.Get(0));
    sinkAppsSrt001.Start(Seconds(30));
    sinkAppsSrt001.Stop(Seconds(930));

    // SMS-001 (UDP, 1.5 Mbps, 300 seconds)
    uint16_t portSms001 = 14;
    OnOffHelper onOffSms001("ns3::UdpSocketFactory",
                            InetSocketAddress(apInterface.GetAddress(0), portSms001));
    onOffSms001.SetAttribute("DataRate", DataRateValue(DataRate("1.5Mbps")));
    onOffSms001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSms001.SetAttribute("MaxBytes", UintegerValue(56250000));
    onOffSms001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=300]"));
    onOffSms001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppsSms001 = onOffSms001.Install(wifiStaNodes.Get(5));
    clientAppsSms001.Start(Seconds(45));
    clientAppsSms001.Stop(Seconds(345));

    PacketSinkHelper sinkSms001("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), portSms001));
    ApplicationContainer sinkAppsSms001 = sinkSms001.Install(wifiApNodes.Get(0));
    sinkAppsSms001.Start(Seconds(45));
    sinkAppsSms001.Stop(Seconds(345));

    // SVC-001 (TCP, maximum throughput, 1200 seconds)
    uint16_t portSvc001 = 15;
    OnOffHelper onOffSvc001("ns3::TcpSocketFactory",
                            InetSocketAddress(apInterface.GetAddress(0), portSvc001));
    onOffSvc001.SetAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    onOffSvc001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSvc001.SetAttribute("MaxBytes", UintegerValue(0));
    onOffSvc001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1200]"));
    onOffSvc001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppsSvc001 = onOffSvc001.Install(wifiStaNodes.Get(6));
    clientAppsSvc001.Start(Seconds(15));
    clientAppsSvc001.Stop(Seconds(1215));

    PacketSinkHelper sinkSvc001("ns3::TcpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), portSvc001));
    ApplicationContainer sinkAppsSvc001 = sinkSvc001.Install(wifiApNodes.Get(0));
    sinkAppsSvc001.Start(Seconds(15));
    sinkAppsSvc001.Stop(Seconds(1215));
    
    // Simulation
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
