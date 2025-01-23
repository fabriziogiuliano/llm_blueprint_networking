
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
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"


#include <algorithm>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("SmartHomeExample");

// Network settings
int nDevices = 0;                 //!< Number of end device nodes to create
int nGateways = 0;                  //!< Number of gateway nodes to create
int nWiFiAPNodes=2;
int nWiFiStaNodes=7;
double radiusMeters = 1000;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 5400; //!< Scenario duration (s) in simulated time

// Channel model
bool realisticChannelModel = false; //!< Whether to use a more realistic channel model with
                                    //!< Buildings and correlated shadowing

int appPeriodSeconds = 60; //!< Duration (s) of the inter-transmission time of end devices

// Output control
bool printBuildingInfo = true; //!< Whether to print building information


struct WifiStationData {
    std::string station_id;
    std::string ssid;
    std::string device_type;
    std::string protocol;
    std::string throughput;
    double duration;
    double start_time;
    std::string wpa_passphrase;
    std::string wpa_key_mgmt;
    std::string wlan_IP;
    std::string wlan_MAC_ADDR;
    double distance_meters;
    std::string location;
    std::string eth_IP;
    std::string user;
    std::string password;
};


struct WifiAPData {
    std::string ap_id;
    std::string ssid;
    std::string wpa_passphrase;
    std::string wpa_key_mgmt;
    std::string wlan_IP;
    std::string eth_IP;
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

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWiFiStaNodes);

    NodeContainer wifiApNodes;
    wifiApNodes.Create (nWiFiAPNodes);

     YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    wifiPhy.SetChannel (wifichannel.Create ());
    
    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;


    NetDeviceContainer staDevices;
    NetDeviceContainer apDevices;
    std::vector<WifiStationData> wifiStations;
    std::vector<WifiAPData> wifiAPs;


    // Parse AP data
    WifiAPData ap1;
        ap1.ap_id="AP-001";
        ap1.ssid="HomeNet_Extended";
        ap1.wpa_passphrase="secure_password";
        ap1.wpa_key_mgmt="WPA2-PSK";
        ap1.wlan_IP="192.168.2.1";
        ap1.eth_IP="10.8.8.1";
    wifiAPs.push_back(ap1);

    WifiAPData ap2;
        ap2.ap_id="AP-002";
        ap2.ssid="Guest_Network";
        ap2.wpa_passphrase="guest_pass";
        ap2.wpa_key_mgmt="WPA2-PSK";
        ap2.wlan_IP="192.168.3.1";
        ap2.eth_IP="10.8.9.1";
    wifiAPs.push_back(ap2);


    //Parse Station data
    WifiStationData sta1;
        sta1.station_id="ST-001";
        sta1.ssid="HomeNet_Extended";
        sta1.device_type="Laptop";
        sta1.protocol="TCP";
        sta1.throughput="maximum";
        sta1.duration=2700;
        sta1.start_time=0;
        sta1.wpa_passphrase="secure_password";
        sta1.wpa_key_mgmt="WPA2-PSK";
        sta1.wlan_IP="192.168.2.101";
        sta1.wlan_MAC_ADDR="A1:B2:C3:D4:E5:01";
        sta1.distance_meters=3;
        sta1.location="Living Room";
        sta1.eth_IP="10.8.8.10";
        sta1.user="user1";
        sta1.password="user1pass";
    wifiStations.push_back(sta1);

    WifiStationData sta2;
        sta2.station_id="ST-002";
        sta2.ssid="HomeNet_Extended";
        sta2.device_type="Smart TV";
        sta2.protocol="UDP";
        sta2.throughput="5000000";
        sta2.duration=2700;
        sta2.start_time=10;
        sta2.wpa_passphrase="secure_password";
        sta2.wpa_key_mgmt="WPA2-PSK";
        sta2.wlan_IP="192.168.2.102";
        sta2.wlan_MAC_ADDR="A1:B2:C3:D4:E5:02";
        sta2.distance_meters=8;
        sta2.location="Family Room";
        sta2.eth_IP="10.8.8.11";
        sta2.user="user2";
        sta2.password="user2pass";
    wifiStations.push_back(sta2);

    WifiStationData sta3;
        sta3.station_id="ST-003";
        sta3.ssid="HomeNet_Extended";
        sta3.device_type="Tablet";
        sta3.protocol="TCP";
        sta3.throughput="1000000";
        sta3.duration=1800;
        sta3.start_time=60;
        sta3.wpa_passphrase="secure_password";
        sta3.wpa_key_mgmt="WPA2-PSK";
        sta3.wlan_IP="192.168.2.103";
        sta3.wlan_MAC_ADDR="A1:B2:C3:D4:E5:03";
        sta3.distance_meters=5;
        sta3.location="Bedroom 1";
        sta3.eth_IP="10.8.8.12";
        sta3.user="user3";
        sta3.password="user3pass";
    wifiStations.push_back(sta3);

    WifiStationData sta4;
        sta4.station_id="SLB-001";
        sta4.device_type="Smart Speaker";
        sta4.ssid="HomeNet_Extended";
        sta4.protocol="UDP";
        sta4.throughput="1500000";
        sta4.duration=1500;
        sta4.start_time=120;
        sta4.wpa_passphrase="secure_password";
        sta4.wpa_key_mgmt="WPA2-PSK";
        sta4.wlan_IP="192.168.2.104";
        sta4.wlan_MAC_ADDR="A1:B2:C3:D4:E5:04";
        sta4.distance_meters=7;
        sta4.location="Kitchen";
        sta4.eth_IP="10.8.8.13";
        sta4.user="user4";
        sta4.password="user4pass";
    wifiStations.push_back(sta4);

    WifiStationData sta5;
        sta5.station_id="SDL-001";
        sta5.device_type="Smartphone";
        sta5.ssid="HomeNet_Extended";
        sta5.protocol="TCP";
        sta5.throughput="maximum";
        sta5.duration=300;
        sta5.start_time=300;
        sta5.wpa_passphrase="secure_password";
        sta5.wpa_key_mgmt="WPA2-PSK";
        sta5.wlan_IP="192.168.2.105";
        sta5.wlan_MAC_ADDR="A1:B2:C3:D4:E5:05";
        sta5.distance_meters=12;
        sta5.location="Office";
        sta5.eth_IP="10.8.8.14";
        sta5.user="user5";
        sta5.password="user5pass";
    wifiStations.push_back(sta5);

    WifiStationData sta6;
        sta6.station_id="SSC-001";
        sta6.device_type="Security Camera";
        sta6.ssid="HomeNet_Extended";
        sta6.protocol="UDP";
        sta6.throughput="2000000";
        sta6.duration=1800;
        sta6.start_time=50;
        sta6.wpa_passphrase="secure_password";
        sta6.wpa_key_mgmt="WPA2-PSK";
        sta6.wlan_IP="192.168.2.106";
        sta6.wlan_MAC_ADDR="A1:B2:C3:D4:E5:06";
        sta6.distance_meters=10;
        sta6.location="Backyard";
        sta6.eth_IP="10.8.8.15";
        sta6.user="user6";
        sta6.password="user6pass";
    wifiStations.push_back(sta6);
  
   WifiStationData sta7;
        sta7.station_id="STG-001";
        sta7.device_type="Guest Device";
        sta7.ssid="Guest_Network";
        sta7.protocol="TCP";
        sta7.throughput="1000000";
        sta7.duration=1200;
        sta7.start_time=180;
        sta7.wpa_passphrase="guest_pass";
        sta7.wpa_key_mgmt="WPA2-PSK";
        sta7.wlan_IP="192.168.3.101";
        sta7.wlan_MAC_ADDR="A1:B2:C3:D4:E5:07";
        sta7.distance_meters=2;
        sta7.location="Guest Room";
        sta7.eth_IP="10.8.9.10";
        sta7.user="guest";
        sta7.password="guestpass";
    wifiStations.push_back(sta7);


    for (int i = 0; i < nWiFiAPNodes; ++i)
    {
        Ssid ssid = Ssid (wifiAPs[i].ssid);
         mac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
        NetDeviceContainer apDev = wifi.Install (wifiPhy, mac, wifiApNodes.Get(i));
        apDevices.Add(apDev);
    }

    for (int i = 0; i < nWiFiStaNodes; ++i)
    {
        Ssid ssid = Ssid (wifiStations[i].ssid);
         mac.SetType ("ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid),
                    "ActiveProbing", BooleanValue (false));
        NetDeviceContainer staDev = wifi.Install (wifiPhy, mac, wifiStaNodes.Get(i));
        staDevices.Add(staDev);
    }

    MobilityHelper wifimobility;
    wifimobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

   Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
    allocatorAPWiFi->Add(Vector(38.10351066811091, 13.3459399220741, 1.5));
    allocatorAPWiFi->Add(Vector(38.10900480778928, 13.34586773182469, 1.5));
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNodes);


    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    allocatorStaWiFi->Add(Vector(38.10863528672426, 13.34050633101243, 1.5));
    allocatorStaWiFi->Add(Vector(38.10873528672426, 13.34060633101243, 1.5));
    allocatorStaWiFi->Add(Vector(38.10883528672426, 13.34070633101243, 1.5));
    allocatorStaWiFi->Add(Vector(38.10893528672426, 13.34080633101243, 1.5));
    allocatorStaWiFi->Add(Vector(38.10903528672426, 13.34090633101243, 1.5));
    allocatorStaWiFi->Add(Vector(38.10913528672426, 13.34100633101243, 1.5));
    allocatorStaWiFi->Add(Vector(38.10923528672426, 13.34110633101243, 1.5));
    wifimobility.SetPositionAllocator(allocatorStaWiFi);
    wifimobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install (wifiApNodes);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase ("192.168.0.0", "255.255.0.0");

    Ipv4InterfaceContainer staNodeInterfaces;
    staNodeInterfaces = address.Assign (staDevices);
    Ipv4InterfaceContainer apNodeInterfaces;
    apNodeInterfaces = address.Assign(apDevices);

    // Print the IP addresses of the WiFi STA nodes
    for (uint32_t i = 0; i < staNodeInterfaces.GetN(); ++i)
    {
        Ipv4Address addr = staNodeInterfaces.GetAddress (i);
         std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }
    NS_LOG_DEBUG("WiFi - Setup Traffic");
   
    for (uint32_t i = 0; i < nWiFiStaNodes; ++i)
    {

        uint16_t port = 8000 + i;
        if (wifiStations[i].protocol == "UDP")
        {
            PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (staNodeInterfaces.GetAddress(i), port));
            ApplicationContainer sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (i));
            sinkApps.Start (Seconds (wifiStations[i].start_time));
            sinkApps.Stop (Seconds (wifiStations[i].start_time + wifiStations[i].duration));

            OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress (staNodeInterfaces.GetAddress(i), port));
            if (wifiStations[i].throughput != "maximum")
            {
              onoff.SetConstantRate (DataRate (wifiStations[i].throughput));
            }
            onoff.SetAttribute ("PacketSize", UintegerValue (1024));

            ApplicationContainer clientApps;
            if (i < nWiFiStaNodes -1)
              clientApps = onoff.Install (wifiStaNodes.Get(i+1));
            else
              clientApps = onoff.Install (wifiStaNodes.Get(0));


            clientApps.Start (Seconds (wifiStations[i].start_time));
            clientApps.Stop (Seconds (wifiStations[i].start_time + wifiStations[i].duration));
           }
        else if (wifiStations[i].protocol == "TCP")
        {
            PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (staNodeInterfaces.GetAddress(i), port));
            ApplicationContainer sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (i));
            sinkApps.Start (Seconds (wifiStations[i].start_time));
            sinkApps.Stop (Seconds (wifiStations[i].start_time + wifiStations[i].duration));

            OnOffHelper onoff ("ns3::TcpSocketFactory", InetSocketAddress (staNodeInterfaces.GetAddress(i), port));
           
           if (wifiStations[i].throughput != "maximum")
            {
              onoff.SetConstantRate (DataRate (wifiStations[i].throughput));
            }
           onoff.SetAttribute ("PacketSize", UintegerValue (1024));

             ApplicationContainer clientApps;
            if (i < nWiFiStaNodes -1)
              clientApps = onoff.Install (wifiStaNodes.Get(i+1));
            else
              clientApps = onoff.Install (wifiStaNodes.Get(0));

            clientApps.Start (Seconds (wifiStations[i].start_time));
            clientApps.Stop (Seconds (wifiStations[i].start_time + wifiStations[i].duration));
        }
    }

    //Tracing
    wifiPhy.EnablePcap ("wifi-ap", apDevices);
    wifiPhy.EnablePcap ("wifi-sta", staDevices);


    /**********
     *  Run    *
     **********/

    Simulator::Stop(appStopTime + Hours(1));

    NS_LOG_INFO("*** Running simulation...");

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    std::ofstream myfile;
    myfile.open ("flow-monitor.txt");


    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
         myfile << "Flow ID: " << i->first  << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress << " Src Port " << t.sourcePort << " Dst Port " << t.destinationPort << "\n";
         myfile << "  Tx Packets: " << i->second.txPackets << "\n";
         myfile << "  Rx Packets: " << i->second.rxPackets << "\n";
         myfile << "  Lost Packets: " << i->second.lostPackets << "\n";
         myfile << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps\n";
         myfile << "  Delay: " << i->second.delaySum.GetSeconds()  / i->second.rxPackets << " s\n";
         myfile << "  Jitter: " << i->second.jitterSum.GetSeconds()  / i->second.rxPackets << " s\n";
        myfile << "----------------------------------------------------------------\n";

    }


    myfile.close ();
    Simulator::Destroy();

    ///////////////////////////
    // Print results to file //
    ///////////////////////////
    NS_LOG_INFO("Computing performance metrics...");

    LoraPacketTracker& tracker = helper.GetPacketTracker();
    std::cout << tracker.CountMacPacketsGlobally(Seconds(0), appStopTime + Hours(1)) << std::endl;
    std::ofstream myfile2;
    myfile2.open ("lora-packet-tracker.txt");
    myfile2 << "Total MAC packets: " << tracker.CountMacPacketsGlobally (Seconds (0), appStopTime + Hours (1)) << std::endl;
    myfile2.close ();

    return 0;
}
