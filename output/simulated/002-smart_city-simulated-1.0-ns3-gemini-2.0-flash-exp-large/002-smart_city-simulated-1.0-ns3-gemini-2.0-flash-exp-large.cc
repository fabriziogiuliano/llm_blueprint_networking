
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
#include <sstream>
#include <iomanip>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("SmartCityNetwork");

// Network settings
int nDevices = 4;                 //!< Number of end device nodes to create
int nGateways = 1;                  //!< Number of gateway nodes to create
int nWiFiAPNodes=2;
int nWiFiStaNodes=3;
double radiusMeters = 1000;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 7200; //!< Scenario duration (s) in simulated time

// Channel model
bool realisticChannelModel = false; //!< Whether to use a more realistic channel model with
                                    //!< Buildings and correlated shadowing

// Output control
bool printBuildingInfo = true; //!< Whether to print building information
std::string output_file_name="smart_city_results.txt";

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
    LogComponentEnable("SmartCityNetwork", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");
    // Create the time value from the period
    Time simulationTime = Seconds(simulationTimeSeconds);

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
    NodeContainer endDevices;
    endDevices.Create(nDevices);

    // Assign a mobility model to each node
    
    Ptr<ListPositionAllocator> allocatorED = CreateObject<ListPositionAllocator>();
    allocatorED->Add(Vector(38.10863528672466, 13.34050633101243, 1.2));
    allocatorED->Add(Vector(38.0998337384608, 13.337136092765382, 1.2));
    allocatorED->Add(Vector(38.1055555555555, 13.3444444444444, 1.2));
    allocatorED->Add(Vector(38.1012345678901, 13.3387654321098, 1.2));
    loramobility.SetPositionAllocator(allocatorED);
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

    Ptr<ListPositionAllocator> allocatorGW = CreateObject<ListPositionAllocator>();
    // Make it so that nodes are at a certain height > 0
    allocatorGW->Add(Vector(38.10351066811096, 13.3459399220741, 15.0));
    loramobility.SetPositionAllocator(allocatorGW);
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
    std::vector<int> spreadingFactors = {7,7,9,8};
    for (uint32_t i = 0; i < endDevices.GetN (); ++i)
    {
        Ptr<Node> node = endDevices.Get(i);
        Ptr<LoraNetDevice> loraNetDevice = node->GetDevice(0)->GetObject<LoraNetDevice>();
        Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac()->GetObject<ClassAEndDeviceLorawanMac>();
         mac->SetSpreadingFactor(spreadingFactors[i]);
    }
   LorawanMacHelper::SetSpreadingFactorsUp(endDevices, gateways, channel);
    NS_LOG_DEBUG("Completed configuration");

    /*********************************************
     *  Install applications on the end devices  *
     *********************************************/

    Time appStopTime = simulationTime;
    PeriodicSenderHelper appHelper = PeriodicSenderHelper();
    appHelper.SetPacketSize(23);
    ApplicationContainer appContainer;
     
     //Traffic Management Sensor
    appHelper.SetPeriod(Seconds(60));
    appContainer = appHelper.Install(endDevices.Get(0));
    appContainer.Start(Seconds(0));
    appContainer.Stop(appStopTime);
    
    //Waste Management
    appHelper.SetPeriod(Seconds(60));
    appContainer = appHelper.Install(endDevices.Get(1));
    appContainer.Start(Seconds(0));
    appContainer.Stop(appStopTime);
    
    //Air Quality Monitoring
    appHelper.SetPeriod(Seconds(120));
     appContainer = appHelper.Install(endDevices.Get(2));
     appContainer.Start(Seconds(0));
     appContainer.Stop(appStopTime);
     
     //Parking Management - Event Based not implemented

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
     NetDeviceContainer staDevices;
    NetDeviceContainer apDevices;
    
    //Configure AP 1
    Ssid ssid1 = Ssid ("SmartCityAP");
    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid1));
    apDevices.Add(wifi.Install (phy, mac, wifiApNode.Get(0)));
    
    //Configure AP 2
     Ssid ssid2 = Ssid ("SmartParkAP");
    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid2));
     apDevices.Add(wifi.Install (phy, mac, wifiApNode.Get(1)));


    //Configure STA 1
    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid1),
                "ActiveProbing", BooleanValue (false));
    staDevices.Add(wifi.Install (phy, mac, wifiStaNodes.Get(0)));
    
    //Configure STA 2
      mac.SetType ("ns3::StaWifiMac",
                  "Ssid", SsidValue (ssid1),
                  "ActiveProbing", BooleanValue (false));
    staDevices.Add(wifi.Install (phy, mac, wifiStaNodes.Get(1)));
    
      //Configure STA 3
      mac.SetType ("ns3::StaWifiMac",
                  "Ssid", SsidValue (ssid2),
                  "ActiveProbing", BooleanValue (false));
    staDevices.Add(wifi.Install (phy, mac, wifiStaNodes.Get(2)));

    MobilityHelper wifimobility;
    wifimobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
     allocatorAPWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
     allocatorAPWiFi->Add(Vector(38.1022222222222, 13.3433333333333, 1.5));
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNode);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
     allocatorStaWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
      allocatorStaWiFi->Add(Vector(38.1044444444444, 13.3422222222222, 1.5));
      allocatorStaWiFi->Add(Vector(38.1011111111111, 13.3411111111111, 1.5));
    wifimobility.SetPositionAllocator(allocatorStaWiFi);
    wifimobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase ("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apNodeInterface;
     apNodeInterface = address.Assign (apDevices.Get(0));
     
     address.SetBase ("192.168.2.0", "255.255.255.0");
     apNodeInterface.Add(address.Assign(apDevices.Get(1)));
     
     address.SetBase ("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces;
    staNodeInterfaces = address.Assign (staDevices.Get(0));
    staNodeInterfaces.Add(address.Assign(staDevices.Get(1)));
    
    address.SetBase ("192.168.2.0", "255.255.255.0");
    staNodeInterfaces.Add(address.Assign(staDevices.Get(2)));

    // Print the IP addresses of the WiFi STA nodes
    for (uint32_t i = 0; i < staNodeInterfaces.GetN(); ++i)
    {
        Ipv4Address addr = staNodeInterfaces.GetAddress (i);
        std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }
    NS_LOG_DEBUG("WiFi - Setup Udp");
    
    //UDP sink for Camera
    uint16_t sinkPort = 8080;
    Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkAddress);
    ApplicationContainer sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (0));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (simulationTime);

    OnOffHelper onoff ("ns3::UdpSocketFactory",
                        InetSocketAddress (staNodeInterfaces.GetAddress(0), sinkPort));
    onoff.SetConstantRate (DataRate ("1Mbps"));
    onoff.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps = onoff.Install (wifiStaNodes.Get (0));
    clientApps.Start (Seconds (1.0));
    clientApps.Stop (simulationTime);
    
    //TCP sink for Street Light
       uint16_t sinkPort2 = 8081;
       PacketSinkHelper packetSinkHelper2 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort2));
       ApplicationContainer sinkApps2 = packetSinkHelper2.Install (wifiStaNodes.Get (1));
       sinkApps2.Start (Seconds (0.0));
       sinkApps2.Stop (simulationTime);

       OnOffHelper onoff2 ("ns3::TcpSocketFactory",InetSocketAddress (staNodeInterfaces.GetAddress(1), sinkPort2));
       onoff2.SetConstantRate (DataRate ("500kbps"));
      onoff2.SetAttribute ("PacketSize", UintegerValue (512));
       ApplicationContainer clientApps2 = onoff2.Install (wifiStaNodes.Get (1));
       clientApps2.Start (Seconds (1.0));
       clientApps2.Stop (simulationTime);
       
      //HTTP sink for Park Kiosk
        uint16_t sinkPort3 = 80;
       PacketSinkHelper packetSinkHelper3 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort3));
      ApplicationContainer sinkApps3 = packetSinkHelper3.Install (wifiStaNodes.Get (2));
      sinkApps3.Start (Seconds (0.0));
       sinkApps3.Stop (simulationTime);

       OnOffHelper onoff3 ("ns3::TcpSocketFactory",InetSocketAddress (staNodeInterfaces.GetAddress(2), sinkPort3));
      onoff3.SetConstantRate (DataRate ("500kbps"));
      onoff3.SetAttribute ("PacketSize", UintegerValue (512));
       ApplicationContainer clientApps3 = onoff3.Install (wifiStaNodes.Get (2));
       clientApps3.Start (Seconds (1.0));
       clientApps3.Stop (simulationTime);

    //Tracing
    phy.EnablePcap ("wifi-ap", apDevices);
    phy.EnablePcap ("wifi-sta", staDevices);

    /**********
     *  Run    *
     **********/

    Simulator::Stop(simulationTime + Hours(1));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    ///////////////////////////
    // Print results to file //
    ///////////////////////////
    NS_LOG_INFO("Computing performance metrics...");

    LoraPacketTracker& tracker = helper.GetPacketTracker();
    std::ofstream myfile;
    myfile.open (output_file_name.c_str());
    myfile << "Total MAC packets: " << tracker.CountMacPacketsGlobally (Seconds (0), simulationTime + Hours (1)) << std::endl;
    myfile << "LoRa Packet Delivery Ratio: " <<  (double)tracker.CountMacPacketsGlobally(Seconds(0), simulationTime + Hours(1))/(double)(nDevices * (simulationTimeSeconds/60)) << std::endl;
    myfile.close ();
    
    
     
    std::cout << "Total MAC packets: " << tracker.CountMacPacketsGlobally(Seconds(0), simulationTime + Hours(1)) << std::endl;
   std::cout << "LoRa Packet Delivery Ratio: " <<  (double)tracker.CountMacPacketsGlobally(Seconds(0), simulationTime + Hours(1))/(double)(nDevices * (simulationTimeSeconds/60)) << std::endl;

    return 0;
}
