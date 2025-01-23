
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

NS_LOG_COMPONENT_DEFINE("ExtendedSmartHomeNetwork");

int nWiFiAPNodes = 1;
int nWiFiStaNodes = 7;
double simulationTimeSeconds = 3600;

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("simulationTime", "The time (s) for which to simulate", simulationTimeSeconds);
    cmd.Parse(argc, argv);

    LogComponentEnable("ExtendedSmartHomeNetwork", LOG_LEVEL_ALL);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWiFiStaNodes);

    NodeContainer wifiApNode;
    wifiApNode.Create(nWiFiAPNodes);

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
    allocatorStaWiFi->Add(Vector(7.0, 0.0, 1.5));
    allocatorStaWiFi->Add(Vector(12.0, 0.0, 1.5));
    allocatorStaWiFi->Add(Vector(3.0, 0.0, 1.5));
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

    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkAddress);

    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address("192.168.1.1"), sinkPort));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer sinkApps;
    ApplicationContainer clientApps;

    sinkApps.Add(packetSinkHelper.Install(wifiStaNodes.Get(0)));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(1800.0));

    onoff.SetConstantRate(DataRate("1Mbps"));
    clientApps.Add(onoff.Install(wifiStaNodes.Get(1)));
    clientApps.Start(Seconds(0.0));
    clientApps.Stop(Seconds(1800.0));

    onoff.SetConstantRate(DataRate("1Mbps"));
    clientApps.Add(onoff.Install(wifiStaNodes.Get(2)));
    clientApps.Start(Seconds(10.0));
    clientApps.Stop(Seconds(610.0));

    onoff.SetConstantRate(DataRate("2Mbps"));
    clientApps.Add(onoff.Install(wifiStaNodes.Get(3)));
    clientApps.Start(Seconds(5.0));
    clientApps.Stop(Seconds(605.0));

    onoff.SetConstantRate(DataRate("500Kbps"));
    clientApps.Add(onoff.Install(wifiStaNodes.Get(4)));
    clientApps.Start(Seconds(30.0));
    clientApps.Stop(Seconds(930.0));

    onoff.SetConstantRate(DataRate("1.5Mbps"));
    clientApps.Add(onoff.Install(wifiStaNodes.Get(5)));
    clientApps.Start(Seconds(45.0));
    clientApps.Stop(Seconds(345.0));

    onoff.SetConstantRate(DataRate("1Mbps"));
    clientApps.Add(onoff.Install(wifiStaNodes.Get(6)));
    clientApps.Start(Seconds(15.0));
    clientApps.Stop(Seconds(1215.0));

    Simulator::Stop(Seconds(simulationTimeSeconds));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
