
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartHome");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    LogComponentEnable("SmartHome", LOG_LEVEL_ALL);

    Time simulationTime = Hours(1);

    NodeContainer wifiApNodes;
    wifiApNodes.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(4);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("SmartHomeAPMyExperiment");

    NetDeviceContainer apDevices;
    NetDeviceContainer staDevices;

    
    Ptr<WifiNetDevice> apWifiNetDevice = DynamicCast<WifiNetDevice>(apDevices.Get(0));
    
    phy.Set("ChannelWidth", UintegerValue(40));
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue("HtMcs7"),
                                "ControlMode", StringValue("HtMcs0"));

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid),
                "BeaconGeneration", BooleanValue(true),
                "BeaconInterval", TimeValue(MicroSeconds(102400)));

    apDevices = wifi.Install(phy, mac, wifiApNodes);

    
    phy.Set("ChannelWidth", UintegerValue (20));
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate6Mbps"),
                                "ControlMode", StringValue ("OfdmRate6Mbps"));

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));
    
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    InternetStackHelper stack;
    stack.Install(wifiApNodes);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apNodeInterfaces = address.Assign(apDevices);
    Ipv4InterfaceContainer staNodeInterfaces = address.Assign(staDevices);

    // Set authentication parameters for AP and STAs
    
    for (uint32_t i = 0; i < wifiApNodes.GetN(); ++i)
    {
        Ptr<WifiNetDevice> apWifiNetDevice = DynamicCast<WifiNetDevice>(apDevices.Get(i));
        Ptr<ns3::ApWifiMac> apWifiMac = DynamicCast<ns3::ApWifiMac>(apWifiNetDevice->GetMac());
        apWifiMac->SetAttribute("AuthType", EnumValue(ns3::AUTH_TYPE_PSK));
        apWifiMac->SetAttribute("WpaKeyMgmt", StringValue("WPA-PSK"));
        apWifiMac->SetAttribute("Psk", StringValue("12345678"));
    }
    for (uint32_t i = 0; i < wifiStaNodes.GetN(); ++i)
    {
        Ptr<WifiNetDevice> staWifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        Ptr<ns3::StaWifiMac> staWifiMac = DynamicCast<ns3::StaWifiMac>(staWifiNetDevice->GetMac());
        staWifiMac->SetAuthType(ns3::AUTH_TYPE_PSK);
        staWifiMac->SetWpaKeyMgmt("WPA-PSK");
        staWifiMac->SetPsk("12345678");
    }
    

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));   // AP-001
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));  // ST-001
    positionAlloc->Add(Vector(5.0, 0.0, 0.0));   // SLB-001
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));   // SDL-001
    positionAlloc->Add(Vector(15.0, 0.0, 0.0));  // SSC-001
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNodes);
    mobility.Install(wifiStaNodes);

    // Applications for ST-001: UDP transmission at maximum throughput for 30 minutes
    uint16_t port1 = 9;
    UdpClientHelper client1(apNodeInterfaces.GetAddress(0), port1);
    client1.SetAttribute("MaxPackets", UintegerValue(4294967295));
    client1.SetAttribute("Interval", TimeValue(Seconds(0.00001)));
    client1.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp1 = client1.Install(wifiStaNodes.Get(0));
    clientApp1.Start(Seconds(0.0));
    clientApp1.Stop(Minutes(30.0));

    PacketSinkHelper sink1("ns3::UdpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port1));
    ApplicationContainer sinkApp1 = sink1.Install(wifiApNodes.Get(0));
    sinkApp1.Start(Seconds(0.0));
    sinkApp1.Stop(Minutes(30.0));

    // Applications for SLB-001: TCP transmission at maximum throughput for 30 minutes
    uint16_t port2 = 10;
    OnOffHelper client2("ns3::TcpSocketFactory",
                        InetSocketAddress(apNodeInterfaces.GetAddress(0), port2));
    client2.SetAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    client2.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp2 = client2.Install(wifiStaNodes.Get(1));
    clientApp2.Start(Seconds(0.0));
    clientApp2.Stop(Minutes(30.0));

    PacketSinkHelper sink2("ns3::TcpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port2));
    ApplicationContainer sinkApp2 = sink2.Install(wifiApNodes.Get(0));
    sinkApp2.Start(Seconds(0.0));
    sinkApp2.Stop(Minutes(30.0));

    // Applications for SDL-001: UDP transmission at 1Mbps for 10 minutes
    uint16_t port3 = 11;
    UdpClientHelper client3(apNodeInterfaces.GetAddress(0), port3);
    client3.SetAttribute("MaxPackets", UintegerValue(4294967295));
    client3.SetAttribute("Interval", TimeValue(Seconds(0.008))); 
    client3.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp3 = client3.Install(wifiStaNodes.Get(2));
    clientApp3.Start(Seconds(10.0));
    clientApp3.Stop(Minutes(10.0) + Seconds(10.0));

    PacketSinkHelper sink3("ns3::UdpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port3));
    ApplicationContainer sinkApp3 = sink3.Install(wifiApNodes.Get(0));
    sinkApp3.Start(Seconds(0.0));
    sinkApp3.Stop(Minutes(10.0) + Seconds(10.0));

    // Applications for SSC-001: UDP transmission at 2Mbps for 10 minutes
    uint16_t port4 = 12;
    UdpClientHelper client4(apNodeInterfaces.GetAddress(0), port4);
    client4.SetAttribute("MaxPackets", UintegerValue(4294967295));
    client4.SetAttribute("Interval", TimeValue(Seconds(0.004))); 
    client4.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp4 = client4.Install(wifiStaNodes.Get(3));
    clientApp4.Start(Seconds(5.0));
    clientApp4.Stop(Minutes(10.0) + Seconds(5.0));

    PacketSinkHelper sink4("ns3::UdpSocketFactory",
                          InetSocketAddress(Ipv4Address::GetAny(), port4));
    ApplicationContainer sinkApp4 = sink4.Install(wifiApNodes.Get(0));
    sinkApp4.Start(Seconds(0.0));
    sinkApp4.Stop(Minutes(10.0) + Seconds(5.0));

    
    for (uint32_t i = 0; i < staDevices.GetN(); ++i)
    {
        Ptr<WifiNetDevice> staWifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        Ptr<RegularWifiMac> staWifiMac = DynamicCast<RegularWifiMac>(staWifiNetDevice->GetMac());
        std::stringstream ss;
        ss << "61:5F:64:5E:90:EB"; 
        if (i == 1) ss.str("60:36:1E:9A:0A:0C");
        if (i == 2) ss.str("39:9F:51:CD:F7:08");
        if (i == 3) ss.str("0B:E3:41:0A:33:B7");

        Mac48Address macAddress;
        ss >> macAddress;
        staWifiMac->SetAddress(macAddress);
    }
    

    Simulator::Stop(simulationTime);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
