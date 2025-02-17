
#include "ns3/building-allocator.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/buildings-helper.h"
#include "ns3/class-a-end-device-lorawan-mac.h"
#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/double.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/forwarder-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/log.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/network-server-helper.h"
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
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("SmallSmartCity");

// Network settings
int nLoraGateways = 1;
int nLoraDevices = 2;
int nWiFiAPNodes = 1;
int nWiFiStaNodes = 1;
double radiusMeters = 1000;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 3600; //!< Scenario duration (s) in simulated time

// Channel model
bool realisticChannelModel = false; //!< Whether to use a more realistic channel model with
                                    //!< Buildings and correlated shadowing

int appPeriodSeconds = 60; //!< Duration (s) of the inter-transmission time of end devices

// Output control
bool printBuildingInfo = true; //!< Whether to print building information

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("simulationTime", "The time (s) for which to simulate", simulationTimeSeconds);
    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("SmallSmartCity", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");
    // Create the time value from the period
    Time appPeriod = Seconds(appPeriodSeconds);

    // Mobility
    MobilityHelper loramobility;
    loramobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    /************************
     *  LoRA -Create the channel  *
     ************************/
    NS_LOG_DEBUG("LoRa - Create the channel...");
    // Create the lora channel object
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(3.76);
    loss->SetReference(1, 7.7);

    if (realisticChannelModel)
    {
        // Create the correlated shadowing component
        Ptr<CorrelatedShadowingPropagationLossModel> shadowing =
            CreateObject<CorrelatedShadowingPropagationLossModel>();

        // Aggregate shadowing to the logdistance loss
        loss->SetNext(shadowing);

        // Add the effect to the channel propagation loss
        Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss>();

        shadowing->SetNext(buildingLoss);
    }

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();

    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

    /************************
     *  Create the helpers  *
     ************************/
    NS_LOG_DEBUG("LoRa - Create the helpers...");

    // Create the LoraPhyHelper
    LoraPhyHelper phyHelper = LoraPhyHelper();
    phyHelper.SetChannel(channel);

    // Create the LorawanMacHelper
    LorawanMacHelper macHelper = LorawanMacHelper();

    // Create the LoraHelper
    LoraHelper helper = LoraHelper();
    helper.EnablePacketTracking(); // Output filename
    // helper.EnableSimulationTimePrinting ();

    // Create the NetworkServerHelper
    NetworkServerHelper nsHelper = NetworkServerHelper();

    // Create the ForwarderHelper
    ForwarderHelper forHelper = ForwarderHelper();

    /************************
     *  Create End Devices  *
     ************************/
    NS_LOG_DEBUG("LoRa - Create End Devices...");

    // Create a set of nodes
    NodeContainer loraEndDevices;
    loraEndDevices.Create(nLoraDevices);

    // Assign a mobility model to each node
    Ptr<ListPositionAllocator> allocatorLoraDev = CreateObject<ListPositionAllocator>();
    allocatorLoraDev->Add(Vector(38.10863528672466, 13.34050633101243, 1.2)); // IOT-001
    allocatorLoraDev->Add(Vector(38.0998337384608, 13.337136092765382, 1.2)); // IOT-002
    loramobility.SetPositionAllocator(allocatorLoraDev);
    loramobility.Install(loraEndDevices);

    // Create the LoraNetDevices of the end devices
    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator>(nwkId, nwkAddr);

    // Create the LoraNetDevices of the end devices
    macHelper.SetAddressGenerator(addrGen);
    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    helper.Install(phyHelper, macHelper, loraEndDevices);

    // Now end devices are connected to the channel

    // Connect trace sources
    for (auto j = loraEndDevices.Begin(); j != loraEndDevices.End(); ++j)
    {
        Ptr<Node> node = *j;
        Ptr<LoraNetDevice> loraNetDevice = node->GetDevice(0)->GetObject<LoraNetDevice>();
        Ptr<LoraPhy> phy = loraNetDevice->GetPhy();
    }

    /*********************
     *  Create Gateways  *
     *********************/
    NS_LOG_DEBUG("LoRa - Create Gateways...");

    // Create the gateway nodes (allocate them uniformly on the disc)
    NodeContainer loraGateways;
    loraGateways.Create(nLoraGateways);

    Ptr<ListPositionAllocator> allocatorLoraGw = CreateObject<ListPositionAllocator>();
    allocatorLoraGw->Add(Vector(38.10351066811096, 13.3459399220741, 15.0));
    loramobility.SetPositionAllocator(allocatorLoraGw);
    loramobility.Install(loraGateways);

    // Create a netdevice for each gateway
    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    helper.Install(phyHelper, macHelper, loraGateways);

    /**********************
     *  Handle buildings  *
     **********************/
    NS_LOG_DEBUG("LoRa - Handle buildings...");

    double xLength = 130;
    double deltaX = 32;
    double yLength = 64;
    double deltaY = 17;
    int gridWidth = 2 * radiusMeters / (xLength + deltaX);
    int gridHeight = 2 * radiusMeters / (yLength + deltaY);
    if (!realisticChannelModel)
    {
        gridWidth = 0;
        gridHeight = 0;
    }
    Ptr<GridBuildingAllocator> gridBuildingAllocator;
    gridBuildingAllocator = CreateObject<GridBuildingAllocator>();
    gridBuildingAllocator->SetAttribute("GridWidth", UintegerValue(gridWidth));
    gridBuildingAllocator->SetAttribute("LengthX", DoubleValue(xLength));
    gridBuildingAllocator->SetAttribute("LengthY", DoubleValue(yLength));
    gridBuildingAllocator->SetAttribute("DeltaX", DoubleValue(deltaX));
    gridBuildingAllocator->SetAttribute("DeltaY", DoubleValue(deltaY));
    gridBuildingAllocator->SetAttribute("Height", DoubleValue(6));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsX", UintegerValue(2));
    gridBuildingAllocator->SetBuildingAttribute("NRoomsY", UintegerValue(4));
    gridBuildingAllocator->SetBuildingAttribute("NFloors", UintegerValue(2));
    gridBuildingAllocator->SetAttribute(
        "MinX",
        DoubleValue(-gridWidth * (xLength + deltaX) / 2 + deltaX / 2));
    gridBuildingAllocator->SetAttribute(
        "MinY",
        DoubleValue(-gridHeight * (yLength + deltaY) / 2 + deltaY / 2));
    BuildingContainer bContainer = gridBuildingAllocator->Create(gridWidth * gridHeight);

    BuildingsHelper::Install(loraEndDevices);
    BuildingsHelper::Install(loraGateways);

    // Print the buildings
    if (printBuildingInfo)
    {
        std::ofstream myfile;
        myfile.open("buildings.txt");
        std::vector<Ptr<Building>>::const_iterator it;
        int j = 1;
        for (it = bContainer.Begin(); it != bContainer.End(); ++it, ++j)
        {
            Box boundaries = (*it)->GetBoundaries();
            myfile << "set object " << j << " rect from " << boundaries.xMin << ","
                   << boundaries.yMin << " to " << boundaries.xMax << "," << boundaries.yMax
                   << std::endl;
        }
        myfile.close();
    }

    /**********************************************
     *  Set up the end device's spreading factor  *
     **********************************************/
    NS_LOG_DEBUG("LoRa - Set up the end device's spreading factor...");

    LorawanMacHelper::SetSpreadingFactorsUp(loraEndDevices, loraGateways, channel);

    NS_LOG_DEBUG("Completed configuration");

    /*********************************************
     *  Install applications on the end devices  *
     *********************************************/

    Time appStopTime = Seconds(simulationTimeSeconds);
    PeriodicSenderHelper appHelper = PeriodicSenderHelper();
    appHelper.SetPeriod(Seconds(appPeriodSeconds));
    appHelper.SetPacketSize(23);
    ApplicationContainer appContainer = appHelper.Install(loraEndDevices);

    appContainer.Start(Seconds(0));
    appContainer.Stop(appStopTime);

    /**************************
     *  Create network server  *
     ***************************/
    NS_LOG_DEBUG("LoRa - Create network server...");

    // Create the network server node
    Ptr<Node> networkServer = CreateObject<Node>();

    // PointToPoint links between gateways and server
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    // Store network server app registration details for later
    P2PGwRegistration_t gwRegistration;
    for (auto gw = loraGateways.Begin(); gw != loraGateways.End(); ++gw)
    {
        auto container = p2p.Install(networkServer, *gw);
        auto serverP2PNetDev = DynamicCast<PointToPointNetDevice>(container.Get(0));
        gwRegistration.emplace_back(serverP2PNetDev, *gw);
    }

    // Create a network server for the network
    nsHelper.SetGatewaysP2P(gwRegistration);
    nsHelper.SetEndDevices(loraEndDevices);
    nsHelper.Install(networkServer);

    // Create a forwarder for each gateway
    forHelper.Install(loraGateways);

    /************************
     *  Create WiFi Nodes  *
     ************************/
    NS_LOG_DEBUG("Create WiFi Nodes...");

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
    Ssid ssid = Ssid("SmartCityAP");

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
    allocatorAPWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNode);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    allocatorStaWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
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
    sinkApps.Stop(Seconds(simulationTimeSeconds));

    OnOffHelper onoff("ns3::UdpSocketFactory",
                        InetSocketAddress(Ipv4Address("192.168.1.1"), sinkPort));
    onoff.SetConstantRate(DataRate("1Mbps"));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = onoff.Install(wifiApNode.Get(0));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(simulationTimeSeconds));

    // Tracing
    phy.EnablePcap("wifi-ap", apDevices);
    phy.EnablePcap("wifi-sta", staDevices);

    /**********
     *  Run    *
     **********/

    Simulator::Stop(appStopTime + Hours(1));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    ///////////////////////////
    // Print results to file //
    ///////////////////////////
    NS_LOG_INFO("Computing performance metrics...");

    LoraPacketTracker& tracker = helper.GetPacketTracker();
    std::cout << tracker.CountMacPacketsGlobally(Seconds(0), appStopTime + Hours(1)) << std::endl;
    std::ofstream myfile;
    myfile.open("lora-packet-tracker.txt");
    myfile << "Total MAC packets: " << tracker.CountMacPacketsGlobally(Seconds(0), appStopTime + Hours(1)) << std::endl;
    myfile.close();

    return 0;
}
