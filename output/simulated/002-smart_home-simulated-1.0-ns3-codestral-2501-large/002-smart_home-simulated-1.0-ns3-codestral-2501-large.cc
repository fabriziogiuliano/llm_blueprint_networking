
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
#include "ns3/periodic-sender-helper.h"
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

#include <algorithm>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdvancedSmartHomeNetwork");

int nWiFiAPNodes = 2;
int nWiFiStaNodes = 7;
double simulationTimeSeconds = 5400;

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
    YansWifiPhyHelper phy;
    phy.SetChannel(wifichannel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid1 = Ssid("HomeNet_Extended");
    Ssid ssid2 = Ssid("Guest_Network");

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid1),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid1));

    NetDeviceContainer apDevices1;
    apDevices1 = wifi.Install(phy, mac, wifiApNodes.Get(0));

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid2));

    NetDeviceContainer apDevices2;
    apDevices2 = wifi.Install(phy, mac, wifiApNodes.Get(1));

    MobilityHelper wifimobility;
    wifimobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
    allocatorAPWiFi->Add(Vector(0, 0, 1.5));
    allocatorAPWiFi->Add(Vector(0, 0, 1.5));
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNodes);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    allocatorStaWiFi->Add(Vector(3, 0, 1.5));
    allocatorStaWiFi->Add(Vector(8, 0, 1.5));
    allocatorStaWiFi->Add(Vector(5, 0, 1.5));
    allocatorStaWiFi->Add(Vector(7, 0, 1.5));
    allocatorStaWiFi->Add(Vector(12, 0, 1.5));
    allocatorStaWiFi->Add(Vector(10, 0, 1.5));
    allocatorStaWiFi->Add(Vector(2, 0, 1.5));
    wifimobility.SetPositionAllocator(allocatorStaWiFi);
    wifimobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install(wifiApNodes);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address1;
    address1.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces1;
    staNodeInterfaces1 = address1.Assign(staDevices);
    Ipv4InterfaceContainer apNodeInterface1;
    apNodeInterface1 = address1.Assign(apDevices1);

    Ipv4AddressHelper address2;
    address2.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces2;
    staNodeInterfaces2 = address2.Assign(staDevices.Get(6));
    Ipv4InterfaceContainer apNodeInterface2;
    apNodeInterface2 = address2.Assign(apDevices2);

    NS_LOG_DEBUG("WiFi - Setup Applications");

    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkAddress);

    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address("192.168.2.1"), sinkPort));
    onoff.SetConstantRate(DataRate("1Mbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer sinkApps;
    ApplicationContainer clientApps;

    sinkApps = packetSinkHelper.Install(wifiStaNodes.Get(1));
    sinkApps.Start(Seconds(10));
    sinkApps.Stop(Seconds(2710));

    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientApps = onoff.Install(wifiStaNodes.Get(0));
    clientApps.Start(Seconds(0));
    clientApps.Stop(Seconds(2700));

    sinkApps = packetSinkHelper.Install(wifiStaNodes.Get(3));
    sinkApps.Start(Seconds(120));
    sinkApps.Stop(Seconds(1620));

    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientApps = onoff.Install(wifiStaNodes.Get(2));
    clientApps.Start(Seconds(60));
    clientApps.Stop(Seconds(1860));

    sinkApps = packetSinkHelper.Install(wifiStaNodes.Get(5));
    sinkApps.Start(Seconds(50));
    sinkApps.Stop(Seconds(1850));

    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientApps = onoff.Install(wifiStaNodes.Get(4));
    clientApps.Start(Seconds(300));
    clientApps.Stop(Seconds(600));

    sinkApps = packetSinkHelper.Install(wifiStaNodes.Get(6));
    sinkApps.Start(Seconds(180));
    sinkApps.Stop(Seconds(1380));

    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientApps = onoff.Install(wifiStaNodes.Get(6));
    clientApps.Start(Seconds(180));
    clientApps.Stop(Seconds(1380));

    //Tracing
    phy.EnablePcap("wifi-ap", apDevices1);
    phy.EnablePcap("wifi-sta", staDevices);

    /**********
     *  Run    *
     **********/

    Simulator::Stop(Seconds(simulationTimeSeconds));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
