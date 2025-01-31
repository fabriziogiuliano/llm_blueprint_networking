
#include "ns3/building-allocator.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/buildings-helper.h"
#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/pointer.h"
#include "ns3/position-allocator.h"
#include "ns3/simulator.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/inet-socket-address.h"

#include <algorithm>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdvancedSmartHomeNetwork");

int nWiFiAPNodes = 2;
int nWiFiStaNodes = 7;
double simulationTimeSeconds = 5400; // 1.5 hours

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("simulationTime", "The time (s) for which to simulate", simulationTimeSeconds);
    cmd.Parse(argc, argv);

    LogComponentEnable("AdvancedSmartHomeNetwork", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");

    /************************
     *  Create WiFi Nodes  *
     ************************/
    NS_LOG_DEBUG("Create WiFi Nodes...");

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWiFiStaNodes);

    NodeContainer wifiApNodes;
    wifiApNodes.Create(nWiFiAPNodes);

    YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
    phy.SetChannel(wifichannel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;

    // AP-001
    Ssid ssid1 = Ssid("HomeNet_Extended");
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid1),
                "BeaconInterval", TimeValue(Seconds(2.5)),
                "BeaconJitter", TimeValue(MicroSeconds(1024)));
    NetDeviceContainer apDevices1 = wifi.Install(phy, mac, wifiApNodes.Get(0));

    // AP-002
    Ssid ssid2 = Ssid("Guest_Network");
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid2),
                "BeaconInterval", TimeValue(Seconds(2.5)),
                "BeaconJitter", TimeValue(MicroSeconds(1024)));
    NetDeviceContainer apDevices2 = wifi.Install(phy, mac, wifiApNodes.Get(1));

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // AP-001
    positionAlloc->Add(Vector(10.0, 0.0, 0.0)); // AP-002
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiApNodes);

    Ptr<ListPositionAllocator> staPositionAlloc = CreateObject<ListPositionAllocator>();
    staPositionAlloc->Add(Vector(3.0, 0.0, 0.0)); // ST-001
    staPositionAlloc->Add(Vector(8.0, 0.0, 0.0)); // ST-002
    staPositionAlloc->Add(Vector(5.0, 0.0, 0.0)); // ST-003
    staPositionAlloc->Add(Vector(7.0, 0.0, 0.0)); // SLB-001
    staPositionAlloc->Add(Vector(12.0, 0.0, 0.0)); // SDL-001
    staPositionAlloc->Add(Vector(10.0, 0.0, 0.0)); // SSC-001
    staPositionAlloc->Add(Vector(2.0, 0.0, 0.0)); // STG-001
    mobility.SetPositionAllocator(staPositionAlloc);
    mobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install(wifiApNodes);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;

    // AP-001
    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface1 = address.Assign(apDevices1);
    address.SetBase("192.168.2.0", "255.255.255.0", "0.0.0.101");
    Ipv4InterfaceContainer staInterface1 = address.Assign(wifiStaNodes.Get(0));

    // AP-002
    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface2 = address.Assign(apDevices2);
    address.SetBase("192.168.3.0", "255.255.255.0", "0.0.0.101");
    Ipv4InterfaceContainer staInterface2 = address.Assign(wifiStaNodes.Get(6));

    /************************
     *  Setup Applications  *
     ************************/
    NS_LOG_DEBUG("Setup Applications...");

    // ST-001
    uint16_t port1 = 8080;
    Address sinkAddress1(InetSocketAddress(Ipv4Address("192.168.2.1"), port1));
    PacketSinkHelper packetSinkHelper1("ns3::TcpSocketFactory", sinkAddress1);
    ApplicationContainer sinkApps1 = packetSinkHelper1.Install(wifiStaNodes.Get(0));
    sinkApps1.Start(Seconds(0.0));
    sinkApps1.Stop(Seconds(2700.0));

    OnOffHelper onoff1("ns3::TcpSocketFactory", sinkAddress1);
    onoff1.SetConstantRate(DataRate("100Mbps"));
    onoff1.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps1 = onoff1.Install(wifiStaNodes.Get(0));
    clientApps1.Start(Seconds(0.0));
    clientApps1.Stop(Seconds(2700.0));

    // ST-002
    uint16_t port2 = 8081;
    Address sinkAddress2(InetSocketAddress(Ipv4Address("192.168.2.1"), port2));
    PacketSinkHelper packetSinkHelper2("ns3::UdpSocketFactory", sinkAddress2);
    ApplicationContainer sinkApps2 = packetSinkHelper2.Install(wifiStaNodes.Get(1));
    sinkApps2.Start(Seconds(10.0));
    sinkApps2.Stop(Seconds(2710.0));

    OnOffHelper onoff2("ns3::UdpSocketFactory", sinkAddress2);
    onoff2.SetConstantRate(DataRate("5Mbps"));
    onoff2.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps2 = onoff2.Install(wifiStaNodes.Get(1));
    clientApps2.Start(Seconds(10.0));
    clientApps2.Stop(Seconds(2710.0));

    // ST-003
    uint16_t port3 = 8082;
    Address sinkAddress3(InetSocketAddress(Ipv4Address("192.168.2.1"), port3));
    PacketSinkHelper packetSinkHelper3("ns3::TcpSocketFactory", sinkAddress3);
    ApplicationContainer sinkApps3 = packetSinkHelper3.Install(wifiStaNodes.Get(2));
    sinkApps3.Start(Seconds(60.0));
    sinkApps3.Stop(Seconds(2460.0));

    OnOffHelper onoff3("ns3::TcpSocketFactory", sinkAddress3);
    onoff3.SetConstantRate(DataRate("1Mbps"));
    onoff3.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps3 = onoff3.Install(wifiStaNodes.Get(2));
    clientApps3.Start(Seconds(60.0));
    clientApps3.Stop(Seconds(2460.0));

    // SLB-001
    uint16_t port4 = 8083;
    Address sinkAddress4(InetSocketAddress(Ipv4Address("192.168.2.1"), port4));
    PacketSinkHelper packetSinkHelper4("ns3::UdpSocketFactory", sinkAddress4);
    ApplicationContainer sinkApps4 = packetSinkHelper4.Install(wifiStaNodes.Get(3));
    sinkApps4.Start(Seconds(120.0));
    sinkApps4.Stop(Seconds(1620.0));

    OnOffHelper onoff4("ns3::UdpSocketFactory", sinkAddress4);
    onoff4.SetConstantRate(DataRate("1.5Mbps"));
    onoff4.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps4 = onoff4.Install(wifiStaNodes.Get(3));
    clientApps4.Start(Seconds(120.0));
    clientApps4.Stop(Seconds(1620.0));

    // SDL-001
    uint16_t port5 = 8084;
    Address sinkAddress5(InetSocketAddress(Ipv4Address("192.168.2.1"), port5));
    PacketSinkHelper packetSinkHelper5("ns3::TcpSocketFactory", sinkAddress5);
    ApplicationContainer sinkApps5 = packetSinkHelper5.Install(wifiStaNodes.Get(4));
    sinkApps5.Start(Seconds(300.0));
    sinkApps5.Stop(Seconds(600.0));

    OnOffHelper onoff5("ns3::TcpSocketFactory", sinkAddress5);
    onoff5.SetConstantRate(DataRate("100Mbps"));
    onoff5.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps5 = onoff5.Install(wifiStaNodes.Get(4));
    clientApps5.Start(Seconds(300.0));
    clientApps5.Stop(Seconds(600.0));

    // SSC-001
    uint16_t port6 = 8085;
    Address sinkAddress6(InetSocketAddress(Ipv4Address("192.168.2.1"), port6));
    PacketSinkHelper packetSinkHelper6("ns3::UdpSocketFactory", sinkAddress6);
    ApplicationContainer sinkApps6 = packetSinkHelper6.Install(wifiStaNodes.Get(5));
    sinkApps6.Start(Seconds(50.0));
    sinkApps6.Stop(Seconds(2050.0));

    OnOffHelper onoff6("ns3::UdpSocketFactory", sinkAddress6);
    onoff6.SetConstantRate(DataRate("2Mbps"));
    onoff6.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps6 = onoff6.Install(wifiStaNodes.Get(5));
    clientApps6.Start(Seconds(50.0));
    clientApps6.Stop(Seconds(2050.0));

    // STG-001
    uint16_t port7 = 8086;
    Address sinkAddress7(InetSocketAddress(Ipv4Address("192.168.3.1"), port7));
    PacketSinkHelper packetSinkHelper7("ns3::TcpSocketFactory", sinkAddress7);
    ApplicationContainer sinkApps7 = packetSinkHelper7.Install(wifiStaNodes.Get(6));
    sinkApps7.Start(Seconds(180.0));
    sinkApps7.Stop(Seconds(1380.0));

    OnOffHelper onoff7("ns3::TcpSocketFactory", sinkAddress7);
    onoff7.SetConstantRate(DataRate("1Mbps"));
    onoff7.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps7 = onoff7.Install(wifiStaNodes.Get(6));
    clientApps7.Start(Seconds(180.0));
    clientApps7.Stop(Seconds(1380.0));

    /**********
     *  Run    *
     **********/

    Simulator::Stop(Seconds(simulationTimeSeconds));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
