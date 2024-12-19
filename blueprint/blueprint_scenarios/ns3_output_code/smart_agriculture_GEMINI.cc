
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
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "ns3/json.h"


using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("SmartAgriculture");

// Network settings
int nDevices = 5;                 //!< Number of end device nodes to create
int nGateways = 1;                  //!< Number of gateway nodes to create
int nWiFiAPNodes = 0;
int nWiFiStaNodes = 0;
double radiusMeters = 1000;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 600; //!< Scenario duration (s) in simulated time

// Channel model
bool realisticChannelModel = true; //!< Whether to use a more realistic channel model with
                                    //!< Buildings and correlated shadowing

int appPeriodSeconds = 60; //!< Duration (s) of the inter-transmission time of end devices

// Output control
bool printBuildingInfo = true; //!< Whether to print building information

// Custom Structs for Blueprint

struct LoraGateway {
    std::string name;
    std::string description;
    double latitude;
    double longitude;
    double altitude;
    std::string gateway_id;
};

struct LoraDevice {
    std::string type;
    std::string name;
    std::string location;
    std::vector<std::string> responsibilities;
    std::string dev_eui;
    double latitude;
    double longitude;
    std::string application_key;
    std::string sensor_type;
    std::string data_rate;
    std::vector<std::string> parameters;
    std::string control_type;
    std::string water_flow_rate;
    std::string purpose;
    std::string flight_time;
};


struct WiFiAP {
    std::string id;
    std::string application;
    std::string location;
    std::string protocol;
    std::string ip_address;
    double latitude;
    double longitude;
};


struct WiFiStation {
  std::string id;
  std::string location;
  std::string protocol;
  std::string ip_address;
  double latitude;
  double longitude;
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
    LogComponentEnable("SmartAgriculture", LOG_LEVEL_ALL);
    

    /******************
     * Read JSON File *
     ******************/
    NS_LOG_DEBUG("Reading JSON File...");
    std::ifstream file("blueprint.json");
    if (!file.is_open()) {
        std::cerr << "Error: Could not open blueprint.json" << std::endl;
        return 1;
    }

    Json::Value root;
    file >> root;
    file.close();

    // Extract and store data from JSON
    std::vector<LoraGateway> loraGateways;
    for (const auto& gw : root["components"]["lora_gateways"]) {
        LoraGateway gateway;
        gateway.name = gw["name"].asString();
        gateway.description = gw["description"].asString();
        gateway.latitude = gw["latitude"].asDouble();
        gateway.longitude = gw["longitude"].asDouble();
        gateway.altitude = gw["altitude"].asDouble();
        gateway.gateway_id = gw["gateway_id"].asString();
        loraGateways.push_back(gateway);
    }

    std::vector<LoraDevice> loraDevices;
    for (const auto& dev : root["components"]["lora_devices"]) {
        LoraDevice device;
        device.type = dev["type"].asString();
        device.name = dev["name"].asString();
        device.location = dev["location"].asString();

         // Handle the responsibilities array (if present)
        if (dev.isMember("responsibilities")) {
           for (const auto& resp : dev["responsibilities"]) {
                device.responsibilities.push_back(resp.asString());
            }
         }
        device.dev_eui = dev["dev_eui"].asString();
        device.latitude = dev["latitude"].asDouble();
        device.longitude = dev["longitude"].asDouble();
       
        if (dev.isMember("application_key")) {
             device.application_key = dev["application_key"].asString();
        }
        if (dev.isMember("sensor_type")) {
            device.sensor_type = dev["sensor_type"].asString();
         }
        if (dev.isMember("data_rate")) {
           device.data_rate = dev["data_rate"].asString();
        }
         if (dev.isMember("parameters")) {
             for (const auto& param : dev["parameters"]) {
                 device.parameters.push_back(param.asString());
             }
         }
        if (dev.isMember("control_type")) {
            device.control_type = dev["control_type"].asString();
        }
        if (dev.isMember("water_flow_rate")) {
           device.water_flow_rate = dev["water_flow_rate"].asString();
        }
         if (dev.isMember("purpose")) {
           device.purpose = dev["purpose"].asString();
        }
         if (dev.isMember("flight_time")) {
           device.flight_time = dev["flight_time"].asString();
        }

        loraDevices.push_back(device);
    }

  std::vector<WiFiAP> wifiAPs;
    for (const auto& ap : root["components"]["wifi_ap"]) {
        WiFiAP wifiap;

      if(ap.isMember("id")){
        wifiap.id = ap["id"].asString();
      }
       if(ap.isMember("application")){
         wifiap.application = ap["application"].asString();
      }
       if(ap.isMember("location")){
        wifiap.location = ap["location"].asString();
      }
      if(ap.isMember("protocol")){
         wifiap.protocol = ap["protocol"].asString();
      }
       if(ap.isMember("ip_address")){
        wifiap.ip_address = ap["ip_address"].asString();
      }
       if(ap.isMember("latitude")){
        wifiap.latitude = ap["latitude"].asDouble();
      }
       if(ap.isMember("longitude")){
         wifiap.longitude = ap["longitude"].asDouble();
      }

        wifiAPs.push_back(wifiap);
    }

    std::vector<WiFiStation> wifiStations;
    for(const auto& station : root["components"]["wifi_stations"]){
      WiFiStation wifistation;

      if(station.isMember("id")){
         wifistation.id = station["id"].asString();
       }
      if(station.isMember("location")){
        wifistation.location = station["location"].asString();
      }
       if(station.isMember("protocol")){
         wifistation.protocol = station["protocol"].asString();
      }
      if(station.isMember("ip_address")){
       wifistation.ip_address = station["ip_address"].asString();
      }
      if(station.isMember("latitude")){
       wifistation.latitude = station["latitude"].asDouble();
      }
      if(station.isMember("longitude")){
        wifistation.longitude = station["longitude"].asDouble();
      }

      wifiStations.push_back(wifistation);
    }

    nDevices = loraDevices.size();
    nGateways = loraGateways.size();
    nWiFiAPNodes = wifiAPs.size();
    nWiFiStaNodes = wifiStations.size();
    
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

    // Set the positions of the end devices based on the JSON data
    Ptr<ListPositionAllocator> endDeviceAllocator = CreateObject<ListPositionAllocator>();
    for (const auto& device : loraDevices) {
      endDeviceAllocator->Add(Vector(device.latitude, device.longitude, 1.2));
    }
    loramobility.SetPositionAllocator(endDeviceAllocator);
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
    // Set Gateway Positions
    for(const auto& gateway : loraGateways){
        allocator->Add(Vector(gateway.latitude, gateway.longitude, gateway.altitude));
    }

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

    NodeContainer wifiApNode;
    wifiApNode.Create (nWiFiAPNodes);

    YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (wifichannel.Create ());

    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("ns-3-ssid");

    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install (phy, mac, wifiStaNodes);

    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install (phy, mac, wifiApNode);

    MobilityHelper wifimobility;
    wifimobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
    for(const auto& ap : wifiAPs){
       allocatorAPWiFi->Add(Vector(ap.latitude, ap.longitude, 1.5));
    }

    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNode);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    for(const auto& station : wifiStations){
       allocatorStaWiFi->Add(Vector(station.latitude, station.longitude, 1.5));
    }   
    wifimobility.SetPositionAllocator(allocatorStaWiFi);
    wifimobility.Install(wifiStaNodes);

    InternetStackHelper stack;
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase ("192.168.1.0", "255.255.255.0");

    Ipv4InterfaceContainer staNodeInterfaces;
    staNodeInterfaces = address.Assign (staDevices);
    Ipv4InterfaceContainer apNodeInterface;
    apNodeInterface = address.Assign (apDevices);

    // Print the IP addresses of the WiFi STA nodes
    for (uint32_t i = 0; i < staNodeInterfaces.GetN(); ++i)
    {
        Ipv4Address addr = staNodeInterfaces.GetAddress (i);
        std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }
    NS_LOG_DEBUG("WiFi - Setup Udp");

    uint16_t sinkPort = 8080;
    Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
    PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkAddress);
    if(nWiFiStaNodes >0){
        ApplicationContainer sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (0));
        sinkApps.Start (Seconds (0.0));
        sinkApps.Stop (Seconds (10.0));

        OnOffHelper onoff ("ns3::UdpSocketFactory",
                            InetSocketAddress (Ipv4Address ("192.168.1.1"), sinkPort));
        onoff.SetConstantRate (DataRate ("1Mbps"));
        onoff.SetAttribute ("PacketSize", UintegerValue (1024));
        if(nWiFiStaNodes > 1){
           ApplicationContainer clientApps = onoff.Install (wifiStaNodes.Get (1));
           clientApps.Start (Seconds (1.0));
           clientApps.Stop (Seconds (10.0));
        }
   }


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


**Key Improvements:**

1.  **JSON Parsing:** The code now includes JSON parsing using the `ns3::json.h` library to read data from the provided `blueprint.json` file.
2.  **Dynamic Node Creation:** It dynamically creates LoRa gateways and devices based on the information extracted from the JSON.
3.  **Positioning from JSON:** The code now sets the positions of both LoRa and WiFi devices directly from the latitude, longitude, and altitude (for gateways) specified in the JSON.
4.  **Flexibility:** It can handle different numbers of LoRa devices and gateways, as well as different types of sensors, depending on the data in the JSON file.
5.  **Struct for Data Organization:** Custom structs (`LoraGateway`, `LoraDevice`, `WiFiAP`, `WiFiStation`) are introduced for organizing the data parsed from the JSON which makes it easier to work with the data.
6. **WiFi Support:** Basic WiFi AP and Stations creation and initialization.
7. **Number of Devices:** The number of devices will be inferred from the json file (LoRa, WiFi).

**To Use This Code:**

1.  **Save:** Save this code as a `.cc` file (e.g., `smart-agriculture.cc`).
2.  **Create `blueprint.json`:** Make sure you have the `blueprint.json` file in the same directory or specify the path in the code.
3.  **Compile:** Compile the code with NS-3:

    bash
    ./waf configure
    ./waf build
    ./waf --run "smart-agriculture"
    

    (You might need to adjust the run command based on your specific NS-3 setup and whether you want to enable debugging etc.)
4.  **Run:** The simulation will run, and the results will be printed to the console and saved to `lora-packet-tracker.txt`.
5.  **Buildings File:** If buildings are enabled, a `buildings.txt` will be created.

**Important Notes:**

*   This code provides a basic framework.  You might need to extend it with:
    *   Specific application logic for each type of device (e.g., sending soil moisture data, controlling irrigation, drone applications)
    *   More sophisticated WiFi settings and traffic patterns
    *   Advanced analysis and visualization of simulation results
*   Ensure you have NS-3 installed with the necessary modules for LoRaWAN, WiFi, and JSON parsing.
*   The `realisticChannelModel` is set to true. If you want to disable buildings, set to false.
*   This code is a starting point. Further development will be needed to fit your specific Smart Agriculture use case, including the need for actual application logic.

This updated code is more flexible and can directly use the information from your `blueprint.json` file to create and configure your smart agriculture network simulation in NS-3. Let me know if you have any further questions or modifications!
