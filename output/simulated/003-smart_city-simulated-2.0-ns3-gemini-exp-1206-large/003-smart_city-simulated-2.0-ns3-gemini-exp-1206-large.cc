
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
#include "ns3/config.h"
#include "ns3/wifi-mac-header.h"

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
double radiusMeters = 1000;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 7200; //!< Scenario duration (s) in simulated time

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
    cmd.AddValue("nDevices", "Number of end devices to include in the simulation", nDevices);
    cmd.AddValue("radius", "The radius (m) of the area to simulate", radiusMeters);
    cmd.AddValue("simulationTime", "The time (s) for which to simulate", simulationTimeSeconds);
    cmd.AddValue("appPeriod",
                 "The period in seconds to be used by periodically transmitting applications",
                 appPeriodSeconds);
    cmd.AddValue("print", "Whether or not to print building information", printBuildingInfo);
    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("ExpandedSmartCityNetwork", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");
    // Create the time value from the period
    Time appPeriod = Seconds(appPeriodSeconds);

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

    Ptr<ListPositionAllocator> allocatorEndDevices = CreateObject<ListPositionAllocator>();
    allocatorEndDevices->Add(Vector(38.10863528672466, 13.34050633101243, 1.2));
    allocatorEndDevices->Add(Vector(38.0998337384608, 13.337136092765382, 1.2));
    allocatorEndDevices->Add(Vector(38.1022222222222, 13.3422222222222, 1.2));
    allocatorEndDevices->Add(Vector(38.1044444444444, 13.3444444444444, 1.2));
    loramobility.SetPositionAllocator(allocatorEndDevices);
    loramobility.Install(endDevices);

    // IOT-001
    PeriodicSenderHelper appHelper = PeriodicSenderHelper();
    appHelper.SetPeriod(Seconds(60)); // constant transmission each 1 minute
    appHelper.SetPacketSize(23);
    ApplicationContainer appContainer = appHelper.Install(endDevices.Get(0)); 

    // IOT-002
    PeriodicSenderHelper appHelper2 = PeriodicSenderHelper();
    appHelper2.SetPeriod(Seconds(60)); // constant transmission each 1 minute
    appHelper2.SetPacketSize(23);
    appContainer.Add(appHelper2.Install(endDevices.Get(1)));

    // IOT-003
    PeriodicSenderHelper appHelper3 = PeriodicSenderHelper();
    appHelper3.SetPeriod(Seconds(300)); // periodic transmission every 5 minutes
    appHelper3.SetPacketSize(23);
    appContainer.Add(appHelper3.Install(endDevices.Get(2)));

    // IOT-004
    PeriodicSenderHelper appHelper4 = PeriodicSenderHelper();
    appHelper4.SetPeriod(Seconds(1));
    appHelper4.SetPacketSize(23);
    appContainer.Add(appHelper4.Install(endDevices.Get(3)));

    Ptr<ClassAEndDeviceLorawanMac> mac0 = DynamicCast<ClassAEndDeviceLorawanMac>(
    endDevices.Get(0)->GetDevice(0)->GetObject<LorawanNetDevice>()->GetMac());
    mac0->SetSpreadingFactor(7);
    Ptr<ClassAEndDeviceLorawanMac> mac1 = DynamicCast<ClassAEndDeviceLorawanMac>(
    endDevices.Get(1)->GetDevice(0)->GetObject<LorawanNetDevice>()->GetMac());
    mac1->SetSpreadingFactor(7);
    Ptr<ClassAEndDeviceLorawanMac> mac2 = DynamicCast<ClassAEndDeviceLorawanMac>(
    endDevices.Get(2)->GetDevice(0)->GetObject<LorawanNetDevice>()->GetMac());
    mac2->SetSpreadingFactor(8);
    Ptr<ClassAEndDeviceLorawanMac> mac3 = DynamicCast<ClassAEndDeviceLorawanMac>(
    endDevices.Get(3)->GetDevice(0)->GetObject<LorawanNetDevice>()->GetMac());
    mac3->SetSpreadingFactor(9);

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

    NodeContainer wifiApNodes;
    wifiApNodes.Create (nWiFiAPNodes);

    YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper wifiphy;
    wifiphy.SetChannel (wifichannel.Create ());

    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;

    // AP-001
    Ssid ssid = Ssid ("SmartCityAP");

    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install (wifiphy, mac, wifiStaNodes);

    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install (wifiphy, mac, wifiApNodes.Get(0));

    // AP-002
    Ssid ssid2 = Ssid ("SmartLibraryAP");

    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid2),
                "ActiveProbing", BooleanValue (false));
    staDevices.Add(wifi.Install (wifiphy, mac, wifiStaNodes.Get(2)));

    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid2));
    apDevices.Add(wifi.Install (wifiphy, mac, wifiApNodes.Get(1)));

    MobilityHelper wifimobility;
    wifimobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
    allocatorAPWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
    allocatorAPWiFi->Add(Vector(38.1012345678901, 13.3412345678901, 1.5));
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNodes);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    allocatorStaWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
    allocatorStaWiFi->Add(Vector(38.1011111111111, 13.3433333333333, 1.5)); 
    allocatorStaWiFi->Add(Vector(38.1055555555555, 13.3455555555555, 1.5));    
    wifimobility.SetPositionAllocator(allocatorStaWiFi);
    wifimobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install (wifiApNodes);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;

    address.SetBase ("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apNodeInterface = address.Assign (apDevices.Get(0));
    address.Assign (staDevices.Get(0));
    address.Assign (staDevices.Get(1));

    address.SetBase ("192.168.2.0", "255.255.255.0");
    address.Assign (apDevices.Get(1));
    address.Assign (staDevices.Get(2));

    // Print the IP addresses of the WiFi STA nodes
    for (uint32_t i = 0; i < staDevices.GetN(); ++i)
    {
        Ipv4Address addr = staDevices.Get(i)->GetAddress (1, 0).GetLocal ();
        std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }

    NS_LOG_DEBUG("WiFi - Setup UDP for STA-001");
    uint16_t sinkPort = 8080;
    Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkAddress);
    ApplicationContainer sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (0));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (simulationTimeSeconds));

    OnOffHelper onoff ("ns3::UdpSocketFactory",
                        InetSocketAddress (apNodeInterface.GetAddress(0), sinkPort));
    onoff.SetConstantRate (DataRate ("1Mbps"));
    onoff.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps = onoff.Install (wifiStaNodes.Get (0));
    clientApps.Start (Seconds (1.0));
    clientApps.Stop (Seconds (simulationTimeSeconds));

    NS_LOG_DEBUG("WiFi - Setup TCP for STA-002");
    uint16_t sinkPort2 = 8081;
    Address sinkAddress2 (InetSocketAddress (Ipv4Address::GetAny (), sinkPort2));
    PacketSinkHelper packetSinkHelper2 ("ns3::TcpSocketFactory", sinkAddress2);
    ApplicationContainer sinkApps2 = packetSinkHelper2.Install (wifiApNodes.Get (0));
    sinkApps2.Start (Seconds (0.0));
    sinkApps2.Stop (Seconds (simulationTimeSeconds));

    OnOffHelper onoff2 ("ns3::TcpSocketFactory",
                        InetSocketAddress (apNodeInterface.GetAddress(0), sinkPort2));
    onoff2.SetConstantRate (DataRate ("500kbps"));
    onoff2.SetAttribute ("PacketSize", UintegerValue (512));
    ApplicationContainer clientApps2 = onoff2.Install (wifiStaNodes.Get (1));
    clientApps2.Start (Seconds (1.0));
    clientApps2.Stop (Seconds (simulationTimeSeconds));

    NS_LOG_DEBUG("WiFi - Setup HTTP for STA-003");
    uint16_t sinkPort3 = 8082;
    Address sinkAddress3 (InetSocketAddress (Ipv4Address::GetAny (), sinkPort3));
    PacketSinkHelper packetSinkHelper3 ("ns3::TcpSocketFactory", sinkAddress3);
    ApplicationContainer sinkApps3 = packetSinkHelper3.Install (wifiApNodes.Get (1));
    sinkApps3.Start (Seconds (0.0));
    sinkApps3.Stop (Seconds (simulationTimeSeconds));

    OnOffHelper onoff3 ("ns3::TcpSocketFactory",
                        InetSocketAddress (apDevices.Get(1)->GetAddress(1,0).GetLocal(), sinkPort3));
    onoff3.SetConstantRate (DataRate ("250kbps"));
    onoff3.SetAttribute ("PacketSize", UintegerValue (256));
    ApplicationContainer clientApps3 = onoff3.Install (wifiStaNodes.Get (2));
    clientApps3.Start (Seconds (1.0));
    clientApps3.Stop (Seconds (simulationTimeSeconds));

    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Ssid", SsidValue (ssid));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Wpa", BooleanValue (true));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/PairwiseMasterKeys", StringValue ("12345678"));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/GroupMasterKey", StringValue ("12345678"));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/GroupRekeyTime", TimeValue (Seconds (86400)));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/GroupRekeyTries", UintegerValue (2));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/PairwiseRekeyTime", TimeValue (Seconds (86400)));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/PairwiseRekeyTries", UintegerValue (2));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/WpaKeyMgmt", StringValue ("WPA-PSK"));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/RsnCapabilities", StringValue ("ShortPreambleSupported=1,ShortSlotTimeSupported=1,CfpSupported=0,CfpPeriod=0,NumSupportedQoSDataRates=0,CastCipher=0,MfpCapable=0,MfpRequired=0,NoPairwise=0,PreAuthEnabled=0"));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop", StringValue ("ns3::EdcaTxopN"));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs", StringValue ("ns3::Aifs"));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs/Aifsn", UintegerValue (2));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs/Acs", StringValue ("ns3::Ac0"));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs/ShortSlotTimeSupported", BooleanValue (true));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Cwmin", UintegerValue (15));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Cwmax", UintegerValue (1023));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/TxopLimit", TimeValue (Seconds (0)));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/BssCtx", BooleanValue (false));
    Config::Set ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/AckPolicy", StringValue ("ns3::NormalAck"));

    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Ssid", SsidValue (ssid));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Wpa", BooleanValue (true));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/PairwiseMasterKeys", StringValue ("12345678"));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/GroupMasterKey", StringValue ("12345678"));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/GroupRekeyTime", TimeValue (Seconds (86400)));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/GroupRekeyTries", UintegerValue (2));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/PairwiseRekeyTime", TimeValue (Seconds (86400)));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/PairwiseRekeyTries", UintegerValue (2));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/WpaKeyMgmt", StringValue ("WPA-PSK"));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/RsnCapabilities", StringValue ("ShortPreambleSupported=1,ShortSlotTimeSupported=1,CfpSupported=0,CfpPeriod=0,NumSupportedQoSDataRates=0,CastCipher=0,MfpCapable=0,MfpRequired=0,NoPairwise=0,PreAuthEnabled=0"));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop", StringValue ("ns3::EdcaTxopN"));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs", StringValue ("ns3::Aifs"));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs/Aifsn", UintegerValue (2));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs/Acs", StringValue ("ns3::Ac0"));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs/ShortSlotTimeSupported", BooleanValue (true));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Cwmin", UintegerValue (15));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Cwmax", UintegerValue (1023));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/TxopLimit", TimeValue (Seconds (0)));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/BssCtx", BooleanValue (false));
    Config::Set ("/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/AckPolicy", StringValue ("ns3::NormalAck"));

    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Ssid", SsidValue (ssid));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Wpa", BooleanValue (true));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/PairwiseMasterKeys", StringValue ("12345678"));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/GroupMasterKey", StringValue ("12345678"));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/GroupRekeyTime", TimeValue (Seconds (86400)));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/GroupRekeyTries", UintegerValue (2));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/PairwiseRekeyTime", TimeValue (Seconds (86400)));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/PairwiseRekeyTries", UintegerValue (2));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/WpaKeyMgmt", StringValue ("WPA-PSK"));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/RsnCapabilities", StringValue ("ShortPreambleSupported=1,ShortSlotTimeSupported=1,CfpSupported=0,CfpPeriod=0,NumSupportedQoSDataRates=0,CastCipher=0,MfpCapable=0,MfpRequired=0,NoPairwise=0,PreAuthEnabled=0"));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop", StringValue ("ns3::EdcaTxopN"));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs", StringValue ("ns3::Aifs"));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs/Aifsn", UintegerValue (2));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs/Acs", StringValue ("ns3::Ac0"));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Aifs/ShortSlotTimeSupported", BooleanValue (true));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Cwmin", UintegerValue (15));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Txop/Cwmax", UintegerValue (1023));
    Config::Set ("/NodeList/4/DeviceList/*/$ns3::WifiNetDevice/Mac/Tx