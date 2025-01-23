
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/config-store.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartHome");

int
main(int argc, char* argv[])
{
    // Set up logging
    LogComponentEnable("SmartHome", LOG_LEVEL_INFO);
    // Enable logging for UdpClient and UdpServer
    LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

    // Configuration parameters
    double simulationTime = 3600; // 1 hour
    std::string phyMode = "DsssRate1Mbps";
    double rss = -80;  // -dBm
    uint32_t packetSize = 1000; // bytes

    // Command line arguments
    CommandLine cmd(__FILE__);
    cmd.AddValue("phyMode", "Wifi physical mode", phyMode);
    cmd.AddValue("rss", "Received Signal Strength", rss);
    cmd.AddValue("packetSize", "Size of packets generated", packetSize);
    cmd.AddValue("simulationTime", "Total duration of the simulation", simulationTime);
    cmd.Parse(argc, argv);

    // Convert to time object
    Time simTime = Seconds(simulationTime);

    // Create WiFi AP node
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    // Create WiFi STA nodes
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(4);

    // Set up WiFi channel and physical layer
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Set up WiFi MAC and PHY
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    // Configure AP
    WifiMacHelper macAp;
    Ssid ssid = Ssid("SmartHomeAPMyExperiment");
    macAp.SetType("ns3::ApWifiMac",
                  "Ssid", SsidValue(ssid),
                  "BeaconGeneration", BooleanValue(true),
                  "BeaconInterval", TimeValue(Seconds(2.5)));

    NetDeviceContainer apDevice;
    apDevice = wifi.Install(phy, macAp, wifiApNode);

    // Configure STAs
    WifiMacHelper macSta;
    macSta.SetType("ns3::StaWifiMac",
                   "Ssid", SsidValue(ssid),
                   "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, macSta, wifiStaNodes);

    // Mobility models
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    // Position the AP
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));

    // Positions for the STAs
    positionAlloc->Add(Vector(10.0, 0.0, 0.0)); // ST-001
    positionAlloc->Add(Vector(5.0, 0.0, 0.0));  // SLB-001
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));  // SDL-001
    positionAlloc->Add(Vector(15.0, 0.0, 0.0)); // SSC-001

    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    // Internet stack
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    // IP address assignment
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevice);
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign(staDevices);

    // UDP and TCP Applications
    // ST-001: UDP transmission at maximum throughput for 30 minutes
    uint16_t udpPort = 9;
    OnOffHelper udpOnOff("ns3::UdpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), udpPort));
    udpOnOff.SetConstantRate(DataRate("500Mbps"));
    udpOnOff.SetAttribute("PacketSize", UintegerValue(packetSize));
    ApplicationContainer udpApp = udpOnOff.Install(wifiStaNodes.Get(0));
    udpApp.Start(Seconds(0.0));
    udpApp.Stop(Seconds(1800));

    // SLB-001: TCP transmission at maximum throughput for 30 minutes
    uint16_t tcpPort = 50000;
    OnOffHelper tcpOnOff("ns3::TcpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), tcpPort));
    tcpOnOff.SetConstantRate(DataRate("500Mbps"));
    tcpOnOff.SetAttribute("PacketSize", UintegerValue(packetSize));
    ApplicationContainer tcpApp = tcpOnOff.Install(wifiStaNodes.Get(1));
    tcpApp.Start(Seconds(0.0));
    tcpApp.Stop(Seconds(1800));

    // SDL-001: UDP transmission at 1Mbps for 10 minutes
    OnOffHelper udpOnOffSDL("ns3::UdpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), udpPort));
    udpOnOffSDL.SetConstantRate(DataRate("1Mbps"));
    udpOnOffSDL.SetAttribute("PacketSize", UintegerValue(packetSize));
    ApplicationContainer udpAppSDL = udpOnOffSDL.Install(wifiStaNodes.Get(2));
    udpAppSDL.Start(Seconds(10.0));
    udpAppSDL.Stop(Seconds(610.0));

    // SSC-001: UDP transmission at 2Mbps for 10 minutes
    OnOffHelper udpOnOffSSC("ns3::UdpSocketFactory", InetSocketAddress(apInterface.GetAddress(0), udpPort));
    udpOnOffSSC.SetConstantRate(DataRate("2Mbps"));
    udpOnOffSSC.SetAttribute("PacketSize", UintegerValue(packetSize));
    ApplicationContainer udpAppSSC = udpOnOffSSC.Install(wifiStaNodes.Get(3));
    udpAppSSC.Start(Seconds(5.0));
    udpAppSSC.Stop(Seconds(605.0));

    // Install UdpServer on the AP for UDP traffic
    UdpServerHelper udpServer(udpPort);
    ApplicationContainer serverApps = udpServer.Install(wifiApNode.Get(0));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(simulationTime));

    // Install TcpServer on the AP for TCP traffic
    PacketSinkHelper tcpSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), tcpPort));
    ApplicationContainer tcpServerApp = tcpSink.Install(wifiApNode.Get(0));
    tcpServerApp.Start(Seconds(0.0));
    tcpServerApp.Stop(Seconds(simulationTime));

    // Enable PCAP tracing
    phy.EnablePcap("smart-home-ap", apDevice.Get(0));
    phy.EnablePcap("smart-home-st001", staDevices.Get(0), true);
    phy.EnablePcap("smart-home-slb001", staDevices.Get(1), true);
    phy.EnablePcap("smart-home-sdl001", staDevices.Get(2), true);
    phy.EnablePcap("smart-home-ssc001", staDevices.Get(3), true);
    

    // Run simulation
    Simulator::Stop(simTime);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
