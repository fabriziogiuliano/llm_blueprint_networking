
#include "ns3/building-allocator.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/buildings-helper.h"
#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/pointer.h"
#include "ns3/position-allocator.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/ssid.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/tcp-socket-factory.h"

#include <algorithm>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdvancedSmartHomeNetwork");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("AdvancedSmartHomeNetwork", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");

    // Simulation time
    double simulationTimeSeconds = 5400; // 1.5 hours
    Time simulationTime = Seconds(simulationTimeSeconds);

    /************************
     *  Create WiFi Nodes  *
     ************************/
    NS_LOG_DEBUG("Create WiFi Nodes...");

    NodeContainer wifiApNodes;
    wifiApNodes.Create(2);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(7);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a); // You can change the standard if needed
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    
    NetDeviceContainer apDevices[2];
    NetDeviceContainer staDevices[7];
    
    //AP-001
    Ssid ssid_ap1 = Ssid("HomeNet_Extended");
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid_ap1),
                 "BeaconGeneration", BooleanValue (true));

    apDevices[0] = wifi.Install(phy, mac, wifiApNodes.Get(0));

    //AP-002
    Ssid ssid_ap2 = Ssid("Guest_Network");
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid_ap2),
                "BeaconGeneration", BooleanValue (true));

    apDevices[1] = wifi.Install(phy, mac, wifiApNodes.Get(1));

    //ST-001
    Ssid ssid_st0 = Ssid("HomeNet_Extended");
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid_st0),
                "ActiveProbing", BooleanValue(false));
    staDevices[0] = wifi.Install(phy, mac, wifiStaNodes.Get(0));

    //ST-002
    Ssid ssid_st1 = Ssid("HomeNet_Extended");
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid_st1),
                "ActiveProbing", BooleanValue(false));
    staDevices[1] = wifi.Install(phy, mac, wifiStaNodes.Get(1));

    //ST-003
    Ssid ssid_st2 = Ssid("HomeNet_Extended");
    mac.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid_st2),
                 "ActiveProbing", BooleanValue(false));
    staDevices[2] = wifi.Install(phy, mac, wifiStaNodes.Get(2));

    //SLB-001
    Ssid ssid_st3 = Ssid("HomeNet_Extended");
    mac.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid_st3),
                 "ActiveProbing", BooleanValue(false));
    staDevices[3] = wifi.Install(phy, mac, wifiStaNodes.Get(3));

    //SDL-001
    Ssid ssid_st4 = Ssid("HomeNet_Extended");
    mac.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid_st4),
                 "ActiveProbing", BooleanValue(false));
    staDevices[4] = wifi.Install(phy, mac, wifiStaNodes.Get(4));

    //SSC-001
    Ssid ssid_st5 = Ssid("HomeNet_Extended");
    mac.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid_st5),
                 "ActiveProbing", BooleanValue(false));
    staDevices[5] = wifi.Install(phy, mac, wifiStaNodes.Get(5));

    //STG-001
    Ssid ssid_st6 = Ssid("Guest_Network");
    mac.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid_st6),
                 "ActiveProbing", BooleanValue(false));
    staDevices[6] = wifi.Install(phy, mac, wifiStaNodes.Get(6));
    
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));   // AP-001
    positionAlloc->Add(Vector(15.0, 0.0, 0.0));  // AP-002
    positionAlloc->Add(Vector(3.0, 0.0, 0.0));   // ST-001
    positionAlloc->Add(Vector(8.0, 0.0, 0.0));   // ST-002
    positionAlloc->Add(Vector(5.0, 0.0, 0.0));   // ST-003
    positionAlloc->Add(Vector(7.0, 0.0, 0.0));   // SLB-001
    positionAlloc->Add(Vector(12.0, 0.0, 0.0));  // SDL-001
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));  // SSC-001
    positionAlloc->Add(Vector(17.0, 0.0, 0.0));  // STG-001

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiApNodes);
    mobility.Install(wifiStaNodes);
    

    InternetStackHelper stack;
    stack.Install(wifiApNodes);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface_0 = address.Assign(apDevices[0]);
    Ipv4InterfaceContainer stInterface_0 = address.Assign(staDevices[0]);
    Ipv4InterfaceContainer stInterface_1 = address.Assign(staDevices[1]);
    Ipv4InterfaceContainer stInterface_2 = address.Assign(staDevices[2]);
    Ipv4InterfaceContainer stInterface_3 = address.Assign(staDevices[3]);
    Ipv4InterfaceContainer stInterface_4 = address.Assign(staDevices[4]);
    Ipv4InterfaceContainer stInterface_5 = address.Assign(staDevices[5]);

    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface_1 = address.Assign(apDevices[1]);
    Ipv4InterfaceContainer stInterface_6 = address.Assign(staDevices[6]);
    

    // ST-001 Application (Laptop - TCP)
    uint16_t port_st0 = 9; 
    OnOffHelper onOff_st0("ns3::TcpSocketFactory", InetSocketAddress(apInterface_0.GetAddress(0), port_st0));
    onOff_st0.SetAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    onOff_st0.SetAttribute("PacketSize", UintegerValue(1024));
    onOff_st0.SetAttribute("StartTime", TimeValue(Seconds(0)));
    onOff_st0.SetAttribute("StopTime", TimeValue(Seconds(2700)));
    ApplicationContainer clientApps_st0 = onOff_st0.Install(wifiStaNodes.Get(0));

    PacketSinkHelper sink_st0("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port_st0));
    ApplicationContainer sinkApps_st0 = sink_st0.Install(wifiApNodes.Get(0));
    sinkApps_st0.Start(Seconds(0.0));

    // ST-002 Application (Smart TV - UDP)
    uint16_t port_st1 = 8000;
    OnOffHelper onOff_st1("ns3::UdpSocketFactory", InetSocketAddress(apInterface_0.GetAddress(0), port_st1));
    onOff_st1.SetConstantRate(DataRate(5000000));
    onOff_st1.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps_st1 = onOff_st1.Install(wifiStaNodes.Get(1));
    clientApps_st1.Start(Seconds(10));
    clientApps_st1.Stop(Seconds(2700 + 10));

    PacketSinkHelper sink_st1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port_st1));
    ApplicationContainer sinkApps_st1 = sink_st1.Install(wifiApNodes.Get(0));
    sinkApps_st1.Start(Seconds(0));

    // ST-003 Application (Tablet - TCP)
    uint16_t port_st2 = 8001;
    OnOffHelper onOff_st2("ns3::TcpSocketFactory", InetSocketAddress(apInterface_0.GetAddress(0), port_st2));
    onOff_st2.SetConstantRate(DataRate(1000000));
    onOff_st2.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps_st2 = onOff_st2.Install(wifiStaNodes.Get(2));
    clientApps_st2.Start(Seconds(60));
    clientApps_st2.Stop(Seconds(1800 + 60));

    PacketSinkHelper sink_st2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port_st2));
    ApplicationContainer sinkApps_st2 = sink_st2.Install(wifiApNodes.Get(0));
    sinkApps_st2.Start(Seconds(0));

    // SLB-001 Application (Smart Speaker - UDP)
    uint16_t port_st3 = 8002;
    OnOffHelper onOff_st3("ns3::UdpSocketFactory", InetSocketAddress(apInterface_0.GetAddress(0), port_st3));
    onOff_st3.SetConstantRate(DataRate(1500000));
    onOff_st3.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps_st3 = onOff_st3.Install(wifiStaNodes.Get(3));
    clientApps_st3.Start(Seconds(120));
    clientApps_st3.Stop(Seconds(1500 + 120));

    PacketSinkHelper sink_st3("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port_st3));
    ApplicationContainer sinkApps_st3 = sink_st3.Install(wifiApNodes.Get(0));
    sinkApps_st3.Start(Seconds(0));

    // SDL-001 Application (Smartphone - TCP)
    uint16_t port_st4 = 8003;
    OnOffHelper onOff_st4("ns3::TcpSocketFactory", InetSocketAddress(apInterface_0.GetAddress(0), port_st4));
    onOff_st4.SetAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    onOff_st4.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps_st4 = onOff_st4.Install(wifiStaNodes.Get(4));
    clientApps_st4.Start(Seconds(300));
    clientApps_st4.Stop(Seconds(300 + 300));

    PacketSinkHelper sink_st4("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port_st4));
    ApplicationContainer sinkApps_st4 = sink_st4.Install(wifiApNodes.Get(0));
    sinkApps_st4.Start(Seconds(0));

    // SSC-001 Application (Security Camera - UDP)
    uint16_t port_st5 = 8004;
    OnOffHelper onOff_st5("ns3::UdpSocketFactory", InetSocketAddress(apInterface_0.GetAddress(0), port_st5));
    onOff_st5.SetConstantRate(DataRate(2000000));
    onOff_st5.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps_st5 = onOff_st5.Install(wifiStaNodes.Get(5));
    clientApps_st5.Start(Seconds(50));
    clientApps_st5.Stop(Seconds(1800 + 50));

    PacketSinkHelper sink_st5("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port_st5));
    ApplicationContainer sinkApps_st5 = sink_st5.Install(wifiApNodes.Get(0));
    sinkApps_st5.Start(Seconds(0));

    // STG-001 Application (Guest Device - TCP)
    uint16_t port_st6 = 8005;
    OnOffHelper onOff_st6("ns3::TcpSocketFactory", InetSocketAddress(apInterface_1.GetAddress(0), port_st6));
    onOff_st6.SetConstantRate(DataRate(1000000));
    onOff_st6.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps_st6 = onOff_st6.Install(wifiStaNodes.Get(6));
    clientApps_st6.Start(Seconds(180));
    clientApps_st6.Stop(Seconds(1200 + 180));

    PacketSinkHelper sink_st6("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port_st6));
    ApplicationContainer sinkApps_st6 = sink_st6.Install(wifiApNodes.Get(1));
    sinkApps_st6.Start(Seconds(0));

    // Enable PCAP tracing on AP and STA devices
    for (int i=0; i<2; i++) {
        phy.EnablePcap("wifi-ap", apDevices[i]);
    }

    for (int i=0; i<7; i++) {
        phy.EnablePcap("wifi-sta", staDevices[i]);
    }
    

    /**********
     *  Run    *
     **********/

    Simulator::Stop(simulationTime);

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    NS_LOG_INFO("Done.");

    return 0;
}
