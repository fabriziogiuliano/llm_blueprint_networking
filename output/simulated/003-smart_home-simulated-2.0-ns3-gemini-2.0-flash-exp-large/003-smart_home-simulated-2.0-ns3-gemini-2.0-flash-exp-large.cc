
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
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/ipv4-global-routing-helper.h"

#include <algorithm>
#include <ctime>
#include <sstream>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("SmartHomeExample");

// Network settings
int nDevices = 1;                 //!< Number of end device nodes to create
int nGateways = 1;                  //!< Number of gateway nodes to create
int nWiFiAPNodes=1;
//int nWiFiStaNodes=2;
double radiusMeters = 1000;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 3600; //!< Scenario duration (s) in simulated time

// Channel model
bool realisticChannelModel = false; //!< Whether to use a more realistic channel model with
                                    //!< Buildings and correlated shadowing

int appPeriodSeconds = 60; //!< Duration (s) of the inter-transmission time of end devices

// Output control
bool printBuildingInfo = true; //!< Whether to print building information

struct WifiStationInfo {
    std::string station_id;
    std::string ssid;
    std::string protocol;
    std::string throughput;
    int duration;
    int start_time;
    uint32_t wpa_passphrase;
    std::string wpa_key_mgmt;
    std::string wlan_IP;
    std::string wlan_MAC_ADDR;
    double distance_meters;
    std::string eth_IP;
    std::string user;
    uint32_t password;
};

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
    LogComponentEnable("SmartHomeExample", LOG_LEVEL_ALL);

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
    allocator->Add(Vector(38.10863528672477, 13.34050633101244, 15.0));
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
    PeriodicSenderHelper appHelper = PeriodicSenderHelper();
    appHelper.SetPeriod(Seconds(appPeriodSeconds));
    appHelper.SetPacketSize(23);
    ApplicationContainer appContainer = appHelper.Install(endDevices);

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

    NodeContainer wifiApNode;
    wifiApNode.Create (nWiFiAPNodes);

    YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (wifichannel.Create ());

    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("SmartHomeAPMyExperiment");

    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install (phy, mac, wifiApNode);

   MobilityHelper wifimobility;
    wifimobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
    allocatorAPWiFi->Add(Vector(38.10351066811091, 13.3459399220741, 1.5));
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNode);

    InternetStackHelper stack;
    stack.Install (wifiApNode);

    Ipv4AddressHelper address;
    address.SetBase ("192.168.1.0", "255.255.255.0");

    Ipv4InterfaceContainer apNodeInterface;
    apNodeInterface = address.Assign (apDevices);

     //  std::vector<WifiStationInfo> wifiStations = {
    std::vector<WifiStationInfo> wifiStations;
    
    WifiStationInfo station1;
    station1.station_id="ST-001";
    station1.ssid="SmartHomeAPMyExperiment";
    station1.protocol="UDP";
    station1.throughput="maximum";
    station1.duration=1800;
    station1.start_time=0;
    station1.wpa_passphrase=12345678;
    station1.wpa_key_mgmt="WPA-PSK";
    station1.wlan_IP="192.168.1.103";
    station1.wlan_MAC_ADDR="61:5F:64:5E:90:EB";
    station1.distance_meters=10;
    station1.eth_IP="10.8.8.17";
    station1.user="root";
    station1.password=123456;
    wifiStations.push_back(station1);

    WifiStationInfo station2;
    station2.station_id="SLB-001";
    station2.ssid="SmartHomeAPMyExperiment";
    station2.protocol="TCP";
    station2.throughput="maximum";
    station2.duration=1800;
    station2.start_time=0;
    station2.wpa_passphrase=12345678;
    station2.wpa_key_mgmt="WPA-PSK";
    station2.wlan_IP="192.168.1.102";
    station2.distance_meters=5;
    station2.wlan_MAC_ADDR="60:36:1E:9A:0A:0C";
    station2.eth_IP="10.8.8.18";
    station2.user="root";
    station2.password=123456;
    wifiStations.push_back(station2);
    
    WifiStationInfo station3;
    station3.station_id="SDL-001";
    station3.ssid="SmartHomeAPMyExperiment";
    station3.protocol="UDP";
    station3.throughput="1000000";
    station3.duration=600;
    station3.start_time=10;
    station3.wpa_passphrase=12345678;
    station3.wpa_key_mgmt="WPA-PSK";
    station3.wlan_IP="192.168.1.101";
    station3.distance_meters=1;
    station3.wlan_MAC_ADDR="39:9F:51:CD:F7:08";
    station3.eth_IP="10.8.8.19";
    station3.user="root";
    station3.password=123456;
    wifiStations.push_back(station3);

    WifiStationInfo station4;
    station4.station_id="SSC-001";
    station4.ssid="SmartHomeAPMyExperiment";
    station4.protocol="UDP";
    station4.throughput="2000000";
    station4.duration=600;
    station4.start_time=5;
    station4.wpa_passphrase=12345678;
    station4.wpa_key_mgmt="WPA-PSK";
    station4.wlan_IP="192.168.1.100";
    station4.distance_meters=15;
    station4.wlan_MAC_ADDR="0B:E3:41:0A:33:B7";
    station4.eth_IP="10.8.8.20";
    station4.user="root";
    station4.password=123456;
     wifiStations.push_back(station4);
    
    WifiStationInfo station5;
    station5.station_id="SRT-001";
    station5.ssid="SmartHomeAPMyExperiment";
    station5.protocol="TCP";
    station5.throughput="500000";
    station5.duration=900;
    station5.start_time=30;
    station5.wpa_passphrase=12345678;
    station5.wpa_key_mgmt="WPA-PSK";
    station5.wlan_IP="192.168.1.104";
    station5.distance_meters=7;
    station5.wlan_MAC_ADDR="A1:B2:C3:D4:E5:F6";
    station5.eth_IP="10.8.8.21";
    station5.user="user1";
    station5.password=987654;
    wifiStations.push_back(station5);

    WifiStationInfo station6;
    station6.station_id="SMS-001";
    station6.ssid="SmartHomeAPMyExperiment";
    station6.protocol="UDP";
    station6.throughput="1500000";
    station6.duration=300;
    station6.start_time=45;
    station6.wpa_passphrase=12345678;
    station6.wpa_key_mgmt="WPA-PSK";
    station6.wlan_IP="192.168.1.105";
    station6.distance_meters=12;
    station6.wlan_MAC_ADDR="F1:E2:D3:C4:B5:A6";
    station6.eth_IP="10.8.8.22";
    station6.user="user2";
    station6.password=456789;
    wifiStations.push_back(station6);
    
     WifiStationInfo station7;
    station7.station_id="SVC-001";
    station7.ssid="SmartHomeAPMyExperiment";
    station7.protocol="TCP";
    station7.throughput="maximum";
    station7.duration=1200;
    station7.start_time=15;
    station7.wpa_passphrase=12345678;
    station7.wpa_key_mgmt="WPA-PSK";
    station7.wlan_IP="192.168.1.106";
    station7.distance_meters=3;
    station7.wlan_MAC_ADDR="1A:2B:3C:4D:5E:6F";
    station7.eth_IP="10.8.8.23";
    station7.user="admin";
    station7.password=123321;
    wifiStations.push_back(station7);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (wifiStations.size());
    
    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install (phy, mac, wifiStaNodes);

    wifimobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();

    for( const auto &station : wifiStations){
      allocatorStaWiFi->Add(Vector(38.10863528672436, 13.34050633101243, 1.5));
    }
    wifimobility.SetPositionAllocator(allocatorStaWiFi);
    wifimobility.Install(wifiStaNodes);

    stack.Install (wifiStaNodes);

    Ipv4InterfaceContainer staNodeInterfaces;
    staNodeInterfaces = address.Assign (staDevices);

     for (uint32_t i = 0; i < wifiStations.size(); ++i) {
         std::string macAddr = wifiStations[i].wlan_MAC_ADDR;
         Mac48Address mac48 = Mac48Address::ConvertFrom(macAddr.c_str());
        Ptr<WifiNetDevice> wifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        Ptr<WifiMac> wifiMac = wifiNetDevice->GetMac();
        wifiMac->SetAddress(mac48);

        Ipv4Address ipv4Addr = Ipv4Address(wifiStations[i].wlan_IP.c_str());
        staNodeInterfaces.GetAddress(i);
        staNodeInterfaces.SetAddress(i, ipv4Addr);

          std::cout << "IP address of wifiStaNode " << wifiStations[i].station_id << ": " << staNodeInterfaces.GetAddress(i) << " MAC: "<<  wifiMac->GetAddress() <<std::endl;
    }

    // Configure traffic for each station
    for (uint32_t i = 0; i < wifiStations.size(); ++i)
    {
         uint16_t sinkPort = 8080 + i;
          Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
          PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkAddress);
        ApplicationContainer sinkApps;

        if (wifiStations[i].protocol == "UDP"){
            sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (i));
        }
        else if (wifiStations[i].protocol == "TCP")
        {
           sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (i));
        }

        sinkApps.Start (Seconds (wifiStations[i].start_time));
        sinkApps.Stop (Seconds (wifiStations[i].start_time + wifiStations[i].duration));


        OnOffHelper onoff;
           if (wifiStations[i].protocol == "UDP"){
            onoff = OnOffHelper ("ns3::UdpSocketFactory",
                               InetSocketAddress (staNodeInterfaces.GetAddress(i), sinkPort));
          } else if (wifiStations[i].protocol == "TCP"){
            onoff = OnOffHelper ("ns3::TcpSocketFactory",
                               InetSocketAddress (staNodeInterfaces.GetAddress(i), sinkPort));
          }

        if (wifiStations[i].throughput == "maximum")
        {
            onoff.SetConstantRate (DataRate ("100Mbps"));
        } else {
             onoff.SetConstantRate (DataRate (wifiStations[i].throughput));
        }

        onoff.SetAttribute ("PacketSize", UintegerValue (1024));
         
        ApplicationContainer clientApps;
         
        if(i == 0){
          clientApps = onoff.Install (wifiApNode.Get (0));
          }
         else{
           clientApps = onoff.Install (wifiStaNodes.Get (i));
        }
          clientApps.Start (Seconds (wifiStations[i].start_time));
          clientApps.Stop (Seconds (wifiStations[i].start_time + wifiStations[i].duration));
    }

     //Enable FlowMonitor
     FlowMonitorHelper flowmonHelper;
     Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

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

    //Flow Monitor Analysis
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    
    std::ofstream flowFile;
    flowFile.open ("flow-monitor.txt");

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
        flowFile << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
        flowFile << "  Tx Packets: " << i->second.txPackets << "\n";
        flowFile << "  Rx Packets: " << i->second.rxPackets << "\n";
        flowFile << "  Lost Packets: " << i->second.lostPackets << "\n";
        flowFile << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1000000  << " Mbps\n";
    }

    flowFile.close();


    return 0;
}
