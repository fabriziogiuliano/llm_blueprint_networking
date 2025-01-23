
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

     Ptr<ListPositionAllocator> allocatorGW = CreateObject<ListPositionAllocator>();
    allocatorGW->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
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

    // Spreading factors for each end device
    std::vector<int> spreadingFactors = {7, 7, 9, 8}; 
    for (uint32_t i = 0; i < endDevices.GetN (); ++i)
    {
      Ptr<Node> node = endDevices.Get (i);
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<ClassAEndDeviceLorawanMac> mac =
          DynamicCast<ClassAEndDeviceLorawanMac> (loraNetDevice->GetMac ());
      mac->SetSpreadingFactor (spreadingFactors[i]);
    }

    NS_LOG_DEBUG("Completed configuration");

    /*********************************************
     *  Install applications on the end devices  *
     *********************************************/

    Time appStopTime = Seconds(simulationTimeSeconds);
    PeriodicSenderHelper appHelper = PeriodicSenderHelper();
    appHelper.SetPacketSize(23);

    // Install applications based on data_transmission type
    for (uint32_t i = 0; i < endDevices.GetN(); ++i) {
        if (i == 0 || i == 1) { // Constant transmission each 1 minute for IOT-001 and IOT-002
           appHelper.SetPeriod(Seconds(60));
           ApplicationContainer appContainer = appHelper.Install(endDevices.Get(i));
           appContainer.Start(Seconds(0));
           appContainer.Stop(appStopTime);
        } else if (i == 2) { // periodic transmission every 2 minutes for IOT-003
            appHelper.SetPeriod(Seconds(120));
           ApplicationContainer appContainer = appHelper.Install(endDevices.Get(i));
           appContainer.Start(Seconds(0));
           appContainer.Stop(appStopTime);
        } else if (i == 3){ //event-based transmission for IOT-004
            //No periodic application for event-based
        }
    }

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
    YansWifiPhyHelper phy;
    phy.SetChannel (wifichannel.Create ());

    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;

   NetDeviceContainer staDevices;
   NetDeviceContainer apDevices;

   //Configure AP nodes
    for (uint32_t i = 0; i < wifiApNodes.GetN(); ++i) {
        Ssid ssid;
        if (i == 0) {
              ssid = Ssid ("SmartCityAP");
        }else {
              ssid = Ssid ("SmartParkAP");
         }

           mac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid));

          NetDeviceContainer apDevice = wifi.Install (phy, mac, wifiApNodes.Get(i));
          apDevices.Add(apDevice);
    }

    //Configure STA nodes
     for (uint32_t i = 0; i < wifiStaNodes.GetN(); ++i) {
          Ssid ssid;
          if (i == 0 || i == 1) {
               ssid = Ssid ("SmartCityAP");
          } else {
               ssid = Ssid ("SmartParkAP");
          }
             mac.SetType ("ns3::StaWifiMac",
                        "Ssid", SsidValue (ssid),
                        "ActiveProbing", BooleanValue (false));

            NetDeviceContainer staDevice = wifi.Install (phy, mac, wifiStaNodes.Get(i));
            staDevices.Add(staDevice);
    }


    MobilityHelper wifimobility;
    wifimobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

   Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
    allocatorAPWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
    allocatorAPWiFi->Add(Vector(38.1022222222222, 13.3433333333333, 1.5));
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNodes);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    allocatorStaWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5));
    allocatorStaWiFi->Add(Vector(38.1044444444444, 13.3422222222222, 1.5));
    allocatorStaWiFi->Add(Vector(38.1011111111111, 13.3411111111111, 1.5));
    wifimobility.SetPositionAllocator(allocatorStaWiFi);
    wifimobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install (wifiApNodes);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;
     Ipv4InterfaceContainer apNodeInterfaces;
      Ipv4InterfaceContainer staNodeInterfaces;
    
    //Assign IP addresses for AP nodes
       for (uint32_t i = 0; i < wifiApNodes.GetN(); ++i) {
        if (i == 0) {
           address.SetBase ("192.168.1.0", "255.255.255.0");
        } else {
           address.SetBase ("192.168.2.0", "255.255.255.0");
         }

          NetDeviceContainer apDevice = apDevices.Get(i);
         Ipv4InterfaceContainer apNodeInterface = address.Assign (apDevice);
          apNodeInterfaces.Add(apNodeInterface);
    }

    //Assign IP addresses for STA nodes
      for (uint32_t i = 0; i < wifiStaNodes.GetN(); ++i) {
            if (i == 0 || i == 1) {
                address.SetBase ("192.168.1.0", "255.255.255.0");
            } else {
                  address.SetBase ("192.168.2.0", "255.255.255.0");
           }

         NetDeviceContainer staDevice = staDevices.Get(i);
          Ipv4InterfaceContainer staNodeInterface = address.Assign (staDevice);
           staNodeInterfaces.Add(staNodeInterface);
     }


    // Print the IP addresses of the WiFi STA nodes
    for (uint32_t i = 0; i < staNodeInterfaces.GetN(); ++i)
    {
        Ipv4Address addr = staNodeInterfaces.GetAddress (i);
        std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }

    NS_LOG_DEBUG("WiFi - Setup Udp and Tcp");
    
    // UDP sink and on-off client for STA-001
    uint16_t sinkPortUDP = 8080;
    Address sinkAddressUDP (InetSocketAddress (Ipv4Address::GetAny (), sinkPortUDP));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkAddressUDP);
    ApplicationContainer sinkAppsUDP = packetSinkHelper.Install (wifiStaNodes.Get (0));
    sinkAppsUDP.Start (Seconds (0.0));
    sinkAppsUDP.Stop (Seconds (simulationTimeSeconds));

    OnOffHelper onoff ("ns3::UdpSocketFactory",
                        InetSocketAddress (staNodeInterfaces.GetAddress(0), sinkPortUDP));
    onoff.SetConstantRate (DataRate ("1Mbps"));
    onoff.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientAppsUDP = onoff.Install (wifiStaNodes.Get (0));
    clientAppsUDP.Start (Seconds (1.0));
     clientAppsUDP.Stop (Seconds (simulationTimeSeconds));

    // TCP sink and on-off client for STA-002
    uint16_t sinkPortTCP = 9090;
     PacketSinkHelper packetSinkHelperTCP ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(), sinkPortTCP));
    ApplicationContainer sinkAppsTCP = packetSinkHelperTCP.Install (wifiStaNodes.Get(1));
     sinkAppsTCP.Start (Seconds (0.0));
    sinkAppsTCP.Stop (Seconds (simulationTimeSeconds));

    OnOffHelper onoffTCP ("ns3::TcpSocketFactory",
                         InetSocketAddress (staNodeInterfaces.GetAddress(1), sinkPortTCP));
    onoffTCP.SetConstantRate (DataRate ("1Mbps"));
    onoffTCP.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientAppsTCP = onoffTCP.Install (wifiStaNodes.Get(1));
    clientAppsTCP.Start (Seconds (1.0));
     clientAppsTCP.Stop (Seconds (simulationTimeSeconds));

     //HTTP Client for STA-003
    uint16_t portHTTP = 80;
   
    OnOffHelper onoffHTTP ("ns3::TcpSocketFactory",
                       InetSocketAddress (staNodeInterfaces.GetAddress(2), portHTTP));
    onoffHTTP.SetConstantRate (DataRate ("1Mbps"));
    onoffHTTP.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientAppsHTTP = onoffHTTP.Install (wifiStaNodes.Get(2));
    clientAppsHTTP.Start (Seconds (1.0));
     clientAppsHTTP.Stop (Seconds (simulationTimeSeconds));

    //Tracing
    phy.EnablePcap ("wifi-ap", apDevices);
    phy.EnablePcap ("wifi-sta", staDevices);

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
