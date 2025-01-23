
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

    Ssid ssid1 = Ssid("HomeNet_Extended");
    Ssid ssid2 = Ssid("Guest_Network");

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid1),
                "BeaconGeneration", BooleanValue(true),
                "BeaconInterval", TimeValue(Seconds(2.5)));

    NetDeviceContainer apDevices1 = wifi.Install(phy, mac, wifiApNodes.Get(0));

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid2),
                "BeaconGeneration", BooleanValue(true),
                "BeaconInterval", TimeValue(Seconds(2.5)));

    NetDeviceContainer apDevices2 = wifi.Install(phy, mac, wifiApNodes.Get(1));

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
    allocatorAPWiFi->Add(Vector(0.0, 0.0, 0.0));
    allocatorAPWiFi->Add(Vector(10.0, 0.0, 0.0));
    mobility.SetPositionAllocator(allocatorAPWiFi);
    mobility.Install(wifiApNodes);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    allocatorStaWiFi->Add(Vector(3.0, 0.0, 0.0));
    allocatorStaWiFi->Add(Vector(8.0, 0.0, 0.0));
    allocatorStaWiFi->Add(Vector(5.0, 0.0, 0.0));
    allocatorStaWiFi->Add(Vector(7.0, 0.0, 0.0));
    allocatorStaWiFi->Add(Vector(12.0, 0.0, 0.0));
    allocatorStaWiFi->Add(Vector(10.0, 0.0, 0.0));
    allocatorStaWiFi->Add(Vector(2.0, 0.0, 0.0));
    mobility.SetPositionAllocator(allocatorStaWiFi);
    mobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install(wifiApNodes);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer apNodeInterface1 = address.Assign(apDevices1);
    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer apNodeInterface2 = address.Assign(apDevices2);

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid1),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices1 = wifi.Install(phy, mac, wifiStaNodes.Get(0));
    NetDeviceContainer staDevices2 = wifi.Install(phy, mac, wifiStaNodes.Get(1));
    NetDeviceContainer staDevices3 = wifi.Install(phy, mac, wifiStaNodes.Get(2));
    NetDeviceContainer staDevices4 = wifi.Install(phy, mac, wifiStaNodes.Get(3));
    NetDeviceContainer staDevices5 = wifi.Install(phy, mac, wifiStaNodes.Get(4));
    NetDeviceContainer staDevices6 = wifi.Install(phy, mac, wifiStaNodes.Get(5));

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid2),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices7 = wifi.Install(phy, mac, wifiStaNodes.Get(6));

    Ipv4InterfaceContainer staNodeInterfaces1 = address.Assign(staDevices1);
    Ipv4InterfaceContainer staNodeInterfaces2 = address.Assign(staDevices2);
    Ipv4InterfaceContainer staNodeInterfaces3 = address.Assign(staDevices3);
    Ipv4InterfaceContainer staNodeInterfaces4 = address.Assign(staDevices4);
    Ipv4InterfaceContainer staNodeInterfaces5 = address.Assign(staDevices5);
    Ipv4InterfaceContainer staNodeInterfaces6 = address.Assign(staDevices6);
    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces7 = address.Assign(staDevices7);

    /************************
     *  Setup Applications  *
     ************************/
    NS_LOG_DEBUG("Setup Applications...");

    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkAddress);

    ApplicationContainer sinkApps1 = packetSinkHelper.Install(wifiStaNodes.Get(0));
    sinkApps1.Start(Seconds(0.0));
    sinkApps1.Stop(Seconds(2700.0));

    OnOffHelper onoff1("ns3::UdpSocketFactory",
                       InetSocketAddress(Ipv4Address("192.168.2.1"), sinkPort));
    onoff1.SetConstantRate(DataRate("5Mbps"));
    onoff1.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps1 = onoff1.Install(wifiStaNodes.Get(1));
    clientApps1.Start(Seconds(10.0));
    clientApps1.Stop(Seconds(2710.0));

    OnOffHelper onoff2("ns3::TcpSocketFactory",
                       InetSocketAddress(Ipv4Address("192.168.2.1"), sinkPort));
    onoff2.SetConstantRate(DataRate("1Mbps"));
    onoff2.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps2 = onoff2.Install(wifiStaNodes.Get(2));
    clientApps2.Start(Seconds(60.0));
    clientApps2.Stop(Seconds(2460.0));

    OnOffHelper onoff3("ns3::UdpSocketFactory",
                       InetSocketAddress(Ipv4Address("192.168.2.1"), sinkPort));
    onoff3.SetConstantRate(DataRate("1.5Mbps"));
    onoff3.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps3 = onoff3.Install(wifiStaNodes.Get(3));
    clientApps3.Start(Seconds(120.0));
    clientApps3.Stop(Seconds(1620.0));

    OnOffHelper onoff4("ns3::TcpSocketFactory",
                       InetSocketAddress(Ipv4Address("192.168.2.1"), sinkPort));
    onoff4.SetConstantRate(DataRate("1Mbps"));
    onoff4.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps4 = onoff4.Install(wifiStaNodes.Get(4));
    clientApps4.Start(Seconds(300.0));
    clientApps4.Stop(Seconds(600.0));

    OnOffHelper onoff5("ns3::UdpSocketFactory",
                       InetSocketAddress(Ipv4Address("192.168.2.1"), sinkPort));
    onoff5.SetConstantRate(DataRate("2Mbps"));
    onoff5.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps5 = onoff5.Install(wifiStaNodes.Get(5));
    clientApps5.Start(Seconds(50.0));
    clientApps5.Stop(Seconds(2350.0));

    OnOffHelper onoff6("ns3::TcpSocketFactory",
                       InetSocketAddress(Ipv4Address("192.168.3.1"), sinkPort));
    onoff6.SetConstantRate(DataRate("1Mbps"));
    onoff6.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps6 = onoff6.Install(wifiStaNodes.Get(6));
    clientApps6.Start(Seconds(180.0));
    clientApps6.Stop(Seconds(1380.0));

    /**********
     *  Run    *
     **********/

    Simulator::Stop(Seconds(simulationTimeSeconds));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
