
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

NS_LOG_COMPONENT_DEFINE("SmartHomeExample");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("SmartHomeExample", LOG_LEVEL_ALL);

    /************************
     *  Create WiFi Nodes  *
     ************************/
    NS_LOG_DEBUG("Create WiFi Nodes...");

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(4);

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(wifichannel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("SmartHomeAPMyExperiment");

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install(phy, mac, wifiApNode);

    MobilityHelper wifimobility;
    wifimobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
    allocatorAPWiFi->Add(Vector(0.0, 0.0, 1.5));
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNode);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    allocatorStaWiFi->Add(Vector(10.0, 0.0, 1.5));
    allocatorStaWiFi->Add(Vector(5.0, 0.0, 1.5));
    allocatorStaWiFi->Add(Vector(1.0, 0.0, 1.5));
    allocatorStaWiFi->Add(Vector(15.0, 0.0, 1.5));
    wifimobility.SetPositionAllocator(allocatorStaWiFi);
    wifimobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");

    Ipv4InterfaceContainer staNodeInterfaces;
    staNodeInterfaces = address.Assign(staDevices);
    Ipv4InterfaceContainer apNodeInterface;
    apNodeInterface = address.Assign(apDevices);

    // Print the IP addresses of the WiFi STA nodes
    for (uint32_t i = 0; i < staNodeInterfaces.GetN(); ++i)
    {
        Ipv4Address addr = staNodeInterfaces.GetAddress(i);
        std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }

    NS_LOG_DEBUG("WiFi - Setup Udp");

    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkAddress);
    ApplicationContainer sinkApps = packetSinkHelper.Install(wifiStaNodes.Get(0));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(1800.0));

    OnOffHelper onoff("ns3::UdpSocketFactory",
                       InetSocketAddress(Ipv4Address("192.168.1.1"), sinkPort));
    onoff.SetConstantRate(DataRate("1Mbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = onoff.Install(wifiStaNodes.Get(1));
    clientApps.Start(Seconds(10.0));
    clientApps.Stop(Seconds(610.0));

    OnOffHelper onoff2("ns3::UdpSocketFactory",
                        InetSocketAddress(Ipv4Address("192.168.1.1"), sinkPort));
    onoff2.SetConstantRate(DataRate("2Mbps"));
    onoff2.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps2 = onoff2.Install(wifiStaNodes.Get(2));
    clientApps2.Start(Seconds(5.0));
    clientApps2.Stop(Seconds(605.0));

    OnOffHelper onoff3("ns3::UdpSocketFactory",
                        InetSocketAddress(Ipv4Address("192.168.1.1"), sinkPort));
    onoff3.SetConstantRate(DataRate("1Mbps"));
    onoff3.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps3 = onoff3.Install(wifiStaNodes.Get(3));
    clientApps3.Start(Seconds(10.0));
    clientApps3.Stop(Seconds(610.0));

    //Tracing
    phy.EnablePcap("wifi-ap", apDevices);
    phy.EnablePcap("wifi-sta", staDevices);

    /**********
     *  Run    *
     **********/

    Simulator::Stop(Seconds(3600));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
