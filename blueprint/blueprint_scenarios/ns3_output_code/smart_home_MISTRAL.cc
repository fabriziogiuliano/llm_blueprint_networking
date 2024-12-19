To generate the corresponding NS-3 code based on the provided TEST BLUEPRINT, we need to adapt the SAMPLE NS-3 code to match the specifics of the TEST BLUEPRINT. The TEST BLUEPRINT specifies details for a WiFi Access Point (AP) and multiple WiFi stations. We will configure the NS-3 code to reflect these details.

Here is the NS-3 code adapted to the TEST BLUEPRINT:


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartHomeAPMyExperiment");

int main(int argc, char* argv[])
{
    LogComponentEnable("SmartHomeAPMyExperiment", LOG_LEVEL_ALL);

    // Create nodes
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(4);

    // Create WiFi channel and physical layer
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
    wifiPhy.SetChannel(wifiChannel.Create());

    // Create WiFi helper and configure it
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    // Create WiFi MAC helper and configure it
    WifiMacHelper wifiMac;
    Ssid ssid = Ssid("SmartHomeAPMyExperiment");

    // Configure AP
    wifiMac.SetType("ns3::ApWifiMac",
                    "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice;
    apDevice = wifi.Install(wifiPhy, wifiMac, wifiApNode);

    // Configure STA
    wifiMac.SetType("ns3::StaWifiMac",
                    "Ssid", SsidValue(ssid),
                    "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install(wifiPhy, wifiMac, wifiStaNodes);

    // Set up mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // AP position
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiApNode);

    positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(10.0, 10.0, 0.0)); // STA-001 position
    positionAlloc->Add(Vector(20.0, 20.0, 0.0)); // SLB-001 position
    positionAlloc->Add(Vector(30.0, 30.0, 0.0)); // SDL-001 position
    positionAlloc->Add(Vector(40.0, 40.0, 0.0)); // SSC-001 position
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiStaNodes);

    // Install internet stack
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");

    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevice);

    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign(staDevices);

    // Print IP addresses
    for (uint32_t i = 0; i < staInterfaces.GetN(); ++i)
    {
        Ipv4Address addr = staInterfaces.GetAddress(i);
        std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }

    // Set up UDP applications
    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkAddress);
    ApplicationContainer sinkApps = packetSinkHelper.Install(wifiStaNodes.Get(0));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(10.0));

    OnOffHelper onoff("ns3::UdpSocketFactory",
                      InetSocketAddress(Ipv4Address("192.168.1.1"), sinkPort));
    onoff.SetConstantRate(DataRate("1Mbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = onoff.Install(wifiStaNodes.Get(1));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(10.0));

    // Enable pcap tracing
    wifiPhy.EnablePcap("wifi-ap", apDevice);
    wifiPhy.EnablePcap("wifi-sta", staDevices);

    // Run the simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}


### Explanation:
1. **Node Creation**: We create nodes for the WiFi AP and WiFi stations.
2. **WiFi Configuration**: We configure the WiFi channel, physical layer, and MAC layer.
3. **Mobility**: We set up the mobility models for the nodes.
4. **Internet Stack**: We install the internet stack on the nodes.
5. **IP Addressing**: We assign IP addresses to the nodes.
6. **UDP Applications**: We set up UDP applications for data transmission.
7. **Tracing**: We enable pcap tracing for the WiFi devices.
8. **Simulation**: We run the simulation for a specified duration.

This code sets up a WiFi network with one access point and four stations, matching the details provided in the TEST BLUEPRINT.