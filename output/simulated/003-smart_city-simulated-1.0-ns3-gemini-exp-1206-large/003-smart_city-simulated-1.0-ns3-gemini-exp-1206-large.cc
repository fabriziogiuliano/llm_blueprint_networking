
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
#include "ns3/wpa-configuration.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ap.h"
#include "ns3/string.h"

#include <algorithm>
#include <ctime>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("ExpandedSmartCityNetwork");

// Network settings
int nDevices = 4;                 //!< Number of end device nodes to create
int nGateways = 2;                  //!< Number of gateway nodes to create
int nWiFiAPNodes=2;
int nWiFiStaNodes=3;
double radiusMeters = 500;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 7200; //!< Scenario duration (s) in simulated time

// Channel model
bool realisticChannelModel = false; //!< Whether to use a more realistic channel model with
                                    //!< Buildings and correlated shadowing

// Output control
bool printBuildingInfo = false; //!< Whether to print building information

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("nDevices", "Number of end devices to include in the simulation", nDevices);
    cmd.AddValue("radius", "The radius (m) of the area to simulate", radiusMeters);
    cmd.AddValue("simulationTime", "The time (s) for which to simulate", simulationTimeSeconds);
    cmd.AddValue("print", "Whether or not to print building information", printBuildingInfo);
    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("ExpandedSmartCityNetwork", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");

    // Mobility
    MobilityHelper loramobility;
    loramobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator",
                                  "rho",
                                  DoubleValue(radiusMeters),
                                  "X",
                                  DoubleValue(0.0),
                                  "Y",
                                  DoubleValue(0.0));
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
    NodeContainer endDevices;
    endDevices.Create(nDevices);

    // Assign a mobility model to each node
    loramobility.Install(endDevices);

    // Make it so that nodes are at a certain height > 0
    for (auto j = endDevices.Begin(); j != endDevices.End(); ++j)
    {
        Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel>();
        Vector position = mobility->GetPosition();
        position.z = 1.2;
        mobility->SetPosition(position);
    }

    // Create the LoraNetDevices of the end devices
    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator>(nwkId, nwkAddr);

    // Create the LoraNetDevices of the end devices
    macHelper.SetAddressGenerator(addrGen);
    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    helper.Install(phyHelper, macHelper, endDevices);

    // Now end devices are connected to the channel

    // Connect trace sources
    for (auto j = endDevices.Begin(); j != endDevices.End(); ++j)
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
    NodeContainer gateways;
    gateways.Create(nGateways);

    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator>();
    // Make it so that nodes are at a certain height > 0
    allocator->Add(Vector(38.10351066811096, 13.3459399220741, 15.0));
    allocator->Add(Vector(38.1012345678901, 13.3412345678901, 15.0));
    loramobility.SetPositionAllocator(allocator);
    loramobility.Install(gateways);

    // Create a netdevice for each gateway
    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    helper.Install(phyHelper, macHelper, gateways);

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

    BuildingsHelper::Install(endDevices);
    BuildingsHelper::Install(gateways);

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

    LorawanMacHelper::SetSpreadingFactorsUp(endDevices, gateways, channel);

    NS_LOG_DEBUG("Completed configuration");

    /*********************************************
     *  Install applications on the end devices  *
     *********************************************/

    Time appStopTime = Seconds(simulationTimeSeconds);

        // IOT-001 (Traffic Management)
    Ptr<Node> iot001 = endDevices.Get(0);
    Ptr<MobilityModel> mobility_iot001 = iot001->GetObject<MobilityModel>();
    mobility_iot001->SetPosition(Vector(38.10863528672466, 13.34050633101243, 1.2));
    PeriodicSenderHelper appHelper_iot001 = PeriodicSenderHelper();
    appHelper_iot001.SetPeriod(Seconds(60)); // constant transmission each 1 minute
    appHelper_iot001.SetPacketSize(23);
    ApplicationContainer appContainer_iot001 = appHelper_iot001.Install(iot001);
    appContainer_iot001.Start(Seconds(0));
    appContainer_iot001.Stop(appStopTime);

    // IOT-002 (Waste Management)
    Ptr<Node> iot002 = endDevices.Get(1);
    Ptr<MobilityModel> mobility_iot002 = iot002->GetObject<MobilityModel>();
    mobility_iot002->SetPosition(Vector(38.0998337384608, 13.337136092765382, 1.2));
    PeriodicSenderHelper appHelper_iot002 = PeriodicSenderHelper();
    appHelper_iot002.SetPeriod(Seconds(60)); // constant transmission each 1 minute
    appHelper_iot002.SetPacketSize(23);
    ApplicationContainer appContainer_iot002 = appHelper_iot002.Install(iot002);
    appContainer_iot002.Start(Seconds(0));
    appContainer_iot002.Stop(appStopTime);

        // IOT-003 (Air Quality Monitoring)
    Ptr<Node> iot003 = endDevices.Get(2);
    Ptr<MobilityModel> mobility_iot003 = iot003->GetObject<MobilityModel>();
    mobility_iot003->SetPosition(Vector(38.1022222222222, 13.3422222222222, 1.2));
    PeriodicSenderHelper appHelper_iot003 = PeriodicSenderHelper();
    appHelper_iot003.SetPeriod(Seconds(300)); // periodic transmission every 5 minutes
    appHelper_iot003.SetPacketSize(23);
    ApplicationContainer appContainer_iot003 = appHelper_iot003.Install(iot003);
    appContainer_iot003.Start(Seconds(0));
    appContainer_iot003.Stop(appStopTime);

        // IOT-004 (Parking Management)
    Ptr<Node> iot004 = endDevices.Get(3);
    Ptr<MobilityModel> mobility_iot004 = iot004->GetObject<MobilityModel>();
    mobility_iot004->SetPosition(Vector(38.1044444444444, 13.3444444444444, 1.2));
    OnOffHelper onoff_iot004 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 80));
    onoff_iot004.SetConstantRate (DataRate ("100kb/s"));
    onoff_iot004.SetAttribute ("PacketSize", UintegerValue (50));
    ApplicationContainer appContainer_iot004 = onoff_iot004.Install(iot004);
    appContainer_iot004.Start(Seconds(0));
    appContainer_iot004.Stop(appStopTime);

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
    for (auto gw = gateways.Begin(); gw != gateways.End(); ++gw)
    {
        auto container = p2p.Install(networkServer, *gw);
        auto serverP2PNetDev = DynamicCast<PointToPointNetDevice>(container.Get(0));
        gwRegistration.emplace_back(serverP2PNetDev, *gw);
    }

    // Create a network server for the network
    nsHelper.SetGatewaysP2P(gwRegistration);
    nsHelper.SetEndDevices(endDevices);
    nsHelper.Install(networkServer);

    // Create a forwarder for each gateway
    forHelper.Install(gateways);

    /************************
     *  Create WiFi Nodes  *
     ************************/
    NS_LOG_DEBUG("Create WiFi Nodes...");

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWiFiStaNodes);

    NodeContainer wifiApNode;
    wifiApNode.Create (nWiFiAPNodes);

    YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (wifichannel.Create ());

    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
        // AP-001
    Ssid ssid_ap001 = Ssid ("SmartCityAP");
    mac.SetType ("ns3::ApWifiMac",
                 "Ssid", SsidValue (ssid_ap001),
                 "BeaconGeneration", BooleanValue (true),
                 "BeaconInterval", TimeValue (Seconds (1.0)));

    NetDeviceContainer apDevice_ap001;
    apDevice_ap001 = wifi.Install (phy, mac, wifiApNode.Get(0));

        // AP-002
    Ssid ssid_ap002 = Ssid ("SmartLibraryAP");
    mac.SetType ("ns3::ApWifiMac",
                 "Ssid", SsidValue (ssid_ap002),
                 "BeaconGeneration", BooleanValue (true),
                 "BeaconInterval", TimeValue (Seconds (1.0)));

    NetDeviceContainer apDevice_ap002;
    apDevice_ap002 = wifi.Install (phy, mac, wifiApNode.Get(1));

    // STA-001
    Ssid ssid_sta001 = Ssid ("SmartCityAP");
    mac.SetType ("ns3::StaWifiMac",
                 "Ssid", SsidValue (ssid_sta001),
                 "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevice_sta001;
    staDevice_sta001 = wifi.Install (phy, mac, wifiStaNodes.Get(0));

        // STA-002
    Ssid ssid_sta002 = Ssid ("SmartCityAP");
    mac.SetType ("ns3::StaWifiMac",
                 "Ssid", SsidValue (ssid_sta002),
                 "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevice_sta002;
    staDevice_sta002 = wifi.Install (phy, mac, wifiStaNodes.Get(1));

        // STA-003
    Ssid ssid_sta003 = Ssid ("SmartLibraryAP");
    mac.SetType ("ns3::StaWifiMac",
                 "Ssid", SsidValue (ssid_sta003),
                 "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevice_sta003;
    staDevice_sta003 = wifi.Install (phy, mac, wifiStaNodes.Get(2));

    MobilityHelper wifimobility;
    wifimobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
    allocatorAPWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
    allocatorAPWiFi->Add(Vector(38.1012345678901, 13.3412345678901, 1.5));
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNode);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    allocatorStaWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
    allocatorStaWiFi->Add(Vector(38.1011111111111, 13.3433333333333, 1.5));
    allocatorStaWiFi->Add(Vector(38.1055555555555, 13.3455555555555, 1.5));
    wifimobility.SetPositionAllocator(allocatorStaWiFi);
    wifimobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase ("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apNodeInterface_ap001 = address.Assign (apDevice_ap001);

    address.SetBase ("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer apNodeInterface_ap002 = address.Assign (apDevice_ap002);

    address.SetBase ("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterface_sta001 = address.Assign (staDevice_sta001);

    address.SetBase ("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterface_sta002 = address.Assign (staDevice_sta002);

    address.SetBase ("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterface_sta003 = address.Assign (staDevice_sta003);

        // Install WPA configurations

    // WPA configuration for STA-001 and AP-001
    Ptr<WifiNetDevice> wifiNetDevice_sta001 = DynamicCast<WifiNetDevice>(staDevice_sta001.Get(0));
    Ptr<ns3::RegularWifiMac> wifiMac_sta001 = DynamicCast<ns3::RegularWifiMac>(wifiNetDevice_sta001->GetMac());
    Ptr<WifiNetDevice> wifiNetDevice_ap001 = DynamicCast<WifiNetDevice>(apDevice_ap001.Get(0));
    Ptr<ns3::ApWifiMac> wifiMac_ap001 = DynamicCast<ns3::ApWifiMac>(wifiNetDevice_ap001->GetMac());

    WpaConfiguration wpaConfig_sta001;
    wpaConfig_sta001.SetSsid(ssid_sta001);
    wpaConfig_sta001.SetAuthType(WpaConfiguration::WpaType::WPA);
    wpaConfig_sta001.SetKeyPassphrase("12345678");

    wpaConfig_sta001.Configure(wifiMac_sta001);
    wpaConfig_sta001.Configure(wifiMac_ap001);

        // WPA configuration for STA-002 and AP-001
    Ptr<WifiNetDevice> wifiNetDevice_sta002 = DynamicCast<WifiNetDevice>(staDevice_sta002.Get(0));
    Ptr<ns3::RegularWifiMac> wifiMac_sta002 = DynamicCast<ns3::RegularWifiMac>(wifiNetDevice_sta002->GetMac());

    WpaConfiguration wpaConfig_sta002;
    wpaConfig_sta002.SetSsid(ssid_sta002);
    wpaConfig_sta002.SetAuthType(WpaConfiguration::WpaType::WPA);
    wpaConfig_sta002.SetKeyPassphrase("12345678");

    wpaConfig_sta002.Configure(wifiMac_sta002);
    wpaConfig_sta002.Configure(wifiMac_ap001); // Use the same AP MAC

        // WPA2 configuration for STA-003 and AP-002
    Ptr<WifiNetDevice> wifiNetDevice_sta003 = DynamicCast<WifiNetDevice>(staDevice_sta003.Get(0));
    Ptr<ns3::RegularWifiMac> wifiMac_sta003 = DynamicCast<ns3::RegularWifiMac>(wifiNetDevice_sta003->GetMac());
    Ptr<WifiNetDevice> wifiNetDevice_ap002 = DynamicCast<WifiNetDevice>(apDevice_ap002.Get(0));
    Ptr<ns3::ApWifiMac> wifiMac_ap002 = DynamicCast<ns3::ApWifiMac>(wifiNetDevice_ap002->GetMac());

    WpaConfiguration wpaConfig_sta003;
    wpaConfig_sta003.SetSsid(ssid_sta003);
    wpaConfig_sta003.SetAuthType(WpaConfiguration::WpaType::WPA2);
    wpaConfig_sta003.SetKeyPassphrase("87654321");

    wpaConfig_sta003.Configure(wifiMac_sta003);
    wpaConfig_sta003.Configure(wifiMac_ap002);

    // Print the IP addresses of the WiFi STA nodes
    for (uint32_t i = 0; i < wifiStaNodes.GetN(); ++i)
    {
        Ipv4InterfaceContainer interface;
        if(i==0)
        {
            interface=staNodeInterface_sta001;
        }
        else if(i==1)
        {
            interface=staNodeInterface_sta002;
        }
        else if(i==2)
        {
            interface=staNodeInterface_sta003;
        }
        
        Ipv4Address addr = interface.GetAddress (0);
        std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }

    NS_LOG_DEBUG("WiFi - Setup Udp");
        //STA-001
    uint16_t sinkPort_sta001 = 8080;
    Address sinkAddress_sta001 (InetSocketAddress (Ipv4Address::GetAny (), sinkPort_sta001));
    PacketSinkHelper packetSinkHelper_sta001 ("ns3::UdpSocketFactory", sinkAddress_sta001);
    ApplicationContainer sinkApps_sta001 = packetSinkHelper_sta001.Install (wifiStaNodes.Get (0));
    sinkApps_sta001.Start (Seconds (0.0));
    sinkApps_sta001.Stop (Seconds (simulationTimeSeconds));

    OnOffHelper onoff_sta001 ("ns3::UdpSocketFactory",
                        InetSocketAddress (Ipv4Address ("192.168.1.101"), sinkPort_sta001));
    onoff_sta001.SetConstantRate (DataRate ("1Mbps"));
    onoff_sta001.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps_sta001 = onoff_sta001.Install (wifiStaNodes.Get (0));
    clientApps_sta001.Start (Seconds (1.0));
    clientApps_sta001.Stop (Seconds (simulationTimeSeconds));

        //STA-002
    uint16_t sinkPort_sta002 = 8081;
    Address sinkAddress_sta002 (InetSocketAddress (Ipv4Address::GetAny (), sinkPort_sta002));
    PacketSinkHelper packetSinkHelper_sta002 ("ns3::TcpSocketFactory", sinkAddress_sta002);
    ApplicationContainer sinkApps_sta002 = packetSinkHelper_sta002.Install (wifiStaNodes.Get (1));
    sinkApps_sta002.Start (Seconds (0.0));
    sinkApps_sta002.Stop (Seconds (simulationTimeSeconds));

    OnOffHelper onoff_sta002 ("ns3::TcpSocketFactory",
                        InetSocketAddress (Ipv4Address ("192.168.1.102"), sinkPort_sta002));
    onoff_sta002.SetConstantRate (DataRate ("500kbps"));
    onoff_sta002.SetAttribute ("PacketSize", UintegerValue (512));
    ApplicationContainer clientApps_sta002 = onoff_sta002.Install (wifiStaNodes.Get (1));
    clientApps_sta002.Start (Seconds (1.0));
    clientApps_sta002.Stop (Seconds (simulationTimeSeconds));

        //STA-003
    uint16_t sinkPort_sta003 = 8082;
    Address sinkAddress_sta003 (InetSocketAddress (Ipv4Address::GetAny (), sinkPort_sta003));
    PacketSinkHelper packetSinkHelper_sta003 ("ns3::TcpSocketFactory", sinkAddress_sta003);
    ApplicationContainer sinkApps_sta003 = packetSinkHelper_sta003.Install (wifiStaNodes.Get (2));
    sinkApps_sta003.Start (Seconds (0.0));
    sinkApps_sta003.Stop (Seconds (simulationTimeSeconds));

    OnOffHelper onoff_sta003 ("ns3::TcpSocketFactory",
                        InetSocketAddress (Ipv4Address ("192.168.2.100"), sinkPort_sta003));
    onoff_sta003.SetConstantRate (DataRate ("250kbps"));
    onoff_sta003.SetAttribute ("PacketSize", UintegerValue (256));
    ApplicationContainer clientApps_sta003 = onoff_sta003.Install (wifiStaNodes.Get (2));
    clientApps_sta003.Start (Seconds (1.0));
    clientApps_sta003.Stop (Seconds (simulationTimeSeconds));

    //Tracing
    phy.EnablePcap ("wifi-ap", apDevice_ap001.Get(0));
    phy.EnablePcap ("wifi-ap", apDevice_ap002.Get(0));
    phy.EnablePcap ("wifi-sta", staDevice_sta001.Get(0));
    phy.EnablePcap ("wifi-sta", staDevice_sta002.Get(0));
    phy.EnablePcap ("wifi-sta", staDevice_sta003.Get(0));

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
    myfile.open ("lora-packet-tracker.txt");
    myfile << "Total MAC packets: " << tracker.CountMacPacketsGlobally (Seconds (0), appStopTime + Hours (1)) << std::endl;
    myfile.close ();

    return 0;
}
