
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
#include <sstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"



using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("SmartCityExample");

// Network settings
int nDevices = 2;                 //!< Number of end device nodes to create
int nGateways = 1;                  //!< Number of gateway nodes to create
int nWiFiAPNodes = 1;
int nWiFiStaNodes = 1;
double radiusMeters = 1000;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 600; //!< Scenario duration (s) in simulated time

// Channel model
bool realisticChannelModel = false; //!< Whether to use a more realistic channel model with
                                    //!< Buildings and correlated shadowing

int appPeriodSeconds = 60; //!< Duration (s) of the inter-transmission time of end devices

// Output control
bool printBuildingInfo = true; //!< Whether to print building information

struct LoraDeviceConfig {
    std::string dev_eui;
    std::string name;
    std::string application_key;
    std::string type;
    std::string application;
    std::string location;
    std::string sensor_type;
    int sf;
    double latitude;
    double longitude;
    std::string data_flow;
};
struct WifiAPConfig
{
  std::string type;
  std::string id;
  std::string ssid;
  std::string wpa_passphrase;
  std::string wpa_key_mgmt;
  std::string wlan_IP;
  std::string eth_IP;
  std::string application;
  std::string location;
  std::string protocol;
    double latitude;
    double longitude;
};
struct WifiSTAConfig
{
    std::string id;
    std::string ssid;
    std::string wpa_passphrase;
    std::string wpa_key_mgmt;
    std::string wlan_IP;
    std::string wlan_MAC_ADDR;
    std::string eth_IP;
    std::string user;
    std::string password;
    std::string type;
    std::string application;
    std::string location;
    std::string sensor_type;
    std::string protocol;
    std::string data_flow;
    std::string ip_address;
    double latitude;
    double longitude;

};
struct LoraGatewayConfig {
        std::string name;
        std::string description;
        double latitude;
        double longitude;
        std::string gateway_id;
        std::string protocol;
        std::string application;
        std::string location;

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
    LogComponentEnable("SmartCityExample", LOG_LEVEL_ALL);
    
    // Read JSON configuration from file
     std::ifstream file("blueprint.json");
    if (!file.is_open()) {
        std::cerr << "Failed to open blueprint.json" << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonString = buffer.str();
    file.close();

    // Parse JSON using a simple method (you might want to use a proper JSON library)
    std::vector<LoraDeviceConfig> loraDeviceConfigs;
    std::vector<WifiAPConfig> wifiAPConfigs;
    std::vector<WifiSTAConfig> wifiSTAConfigs;
    std::vector<LoraGatewayConfig> loraGatewayConfigs;

    size_t pos = jsonString.find("\"lora_devices\":");
    if (pos != std::string::npos) {
        size_t start = jsonString.find("[", pos) + 1;
        size_t end = jsonString.find("]", start);
        if (start != std::string::npos && end != std::string::npos)
        {
            std::string devicesJson = jsonString.substr(start, end - start);
            std::stringstream ss(devicesJson);
            std::string deviceJson;
            while(std::getline(ss, deviceJson, '{'))
            {
                if (deviceJson.empty()) continue;
                std::stringstream ss_dev(deviceJson);
                std::string token;
                LoraDeviceConfig config;
                while(std::getline(ss_dev, token, ','))
                {
                    size_t colon_pos = token.find(":");
                    if(colon_pos != std::string::npos)
                    {
                        std::string key = token.substr(0, colon_pos);
                        std::string value = token.substr(colon_pos+1);
                        
                         key.erase(std::remove(key.begin(), key.end(), '"'), key.end());
                        value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
                        
                        if(key == "dev_eui") config.dev_eui=value;
                        if(key == "name") config.name=value;
                        if(key == "application_key") config.application_key = value;
                        if(key == "type") config.type =value;
                        if(key == "application") config.application = value;
                        if(key == "location") config.location =value;
                        if(key == "sensor_type") config.sensor_type =value;
                        if(key == "SF") config.sf = std::stoi(value);
                        if(key == "latitude") config.latitude = std::stod(value);
                        if(key == "longitude") config.longitude = std::stod(value);
                         if(key == "data_flow") config.data_flow =value;


                    }
                }

                loraDeviceConfigs.push_back(config);
            }
        }

    }
     pos = jsonString.find("\"wifi_ap\":");
    if (pos != std::string::npos) {
        size_t start = jsonString.find("[", pos) + 1;
        size_t end = jsonString.find("]", start);
            if (start != std::string::npos && end != std::string::npos)
        {
            std::string devicesJson = jsonString.substr(start, end - start);
            std::stringstream ss(devicesJson);
            std::string deviceJson;
            while(std::getline(ss, deviceJson, '{'))
            {
                if (deviceJson.empty()) continue;
                std::stringstream ss_dev(deviceJson);
                std::string token;
                WifiAPConfig config;
                while(std::getline(ss_dev, token, ','))
                {
                    size_t colon_pos = token.find(":");
                     if(colon_pos != std::string::npos)
                    {
                        std::string key = token.substr(0, colon_pos);
                        std::string value = token.substr(colon_pos+1);
                         key.erase(std::remove(key.begin(), key.end(), '"'), key.end());
                        value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
                        if(key == "type") config.type=value;
                        if(key == "id") config.id=value;
                        if(key == "ssid") config.ssid = value;
                        if(key == "wpa_passphrase") config.wpa_passphrase =value;
                        if(key == "wpa_key_mgmt") config.wpa_key_mgmt = value;
                        if(key == "wlan_IP") config.wlan_IP =value;
                        if(key == "eth_IP") config.eth_IP =value;
                        if(key == "application") config.application=value;
                         if(key == "location") config.location=value;
                         if(key == "protocol") config.protocol = value;
                          if(key == "latitude") config.latitude = std::stod(value);
                         if(key == "longitude") config.longitude = std::stod(value);


                    }
                }

                wifiAPConfigs.push_back(config);
            }
        }

    }
    
      pos = jsonString.find("\"wifi_stations\":");
    if (pos != std::string::npos) {
        size_t start = jsonString.find("[", pos) + 1;
        size_t end = jsonString.find("]", start);
           if (start != std::string::npos && end != std::string::npos)
        {
            std::string devicesJson = jsonString.substr(start, end - start);
            std::stringstream ss(devicesJson);
            std::string deviceJson;
            while(std::getline(ss, deviceJson, '{'))
            {
                if (deviceJson.empty()) continue;
                std::stringstream ss_dev(deviceJson);
                std::string token;
                WifiSTAConfig config;
                while(std::getline(ss_dev, token, ','))
                {
                    size_t colon_pos = token.find(":");
                     if(colon_pos != std::string::npos)
                    {
                         std::string key = token.substr(0, colon_pos);
                        std::string value = token.substr(colon_pos+1);
                         key.erase(std::remove(key.begin(), key.end(), '"'), key.end());
                        value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
                        if(key == "id") config.id=value;
                        if(key == "ssid") config.ssid=value;
                        if(key == "wpa_passphrase") config.wpa_passphrase=value;
                         if(key == "wpa_key_mgmt") config.wpa_key_mgmt=value;
                        if(key == "wlan_IP") config.wlan_IP = value;
                        if(key == "wlan_MAC_ADDR") config.wlan_MAC_ADDR = value;
                         if(key == "eth_IP") config.eth_IP =value;
                        if(key == "user") config.user=value;
                        if(key == "password") config.password=value;
                        if(key == "type") config.type =value;
                        if(key == "application") config.application=value;
                        if(key == "location") config.location =value;
                        if(key == "sensor_type") config.sensor_type =value;
                         if(key == "protocol") config.protocol = value;
                         if(key == "data_flow") config.data_flow=value;
                         if(key == "ip_address") config.ip_address = value;
                          if(key == "latitude") config.latitude = std::stod(value);
                         if(key == "longitude") config.longitude = std::stod(value);
                    }

                }

                wifiSTAConfigs.push_back(config);
            }
        }
    }
       pos = jsonString.find("\"lora_gateways\":");
    if (pos != std::string::npos) {
        size_t start = jsonString.find("[", pos) + 1;
        size_t end = jsonString.find("]", start);
          if (start != std::string::npos && end != std::string::npos)
        {
             std::string devicesJson = jsonString.substr(start, end - start);
             std::stringstream ss(devicesJson);
            std::string deviceJson;
            while(std::getline(ss, deviceJson, '{'))
            {
                 if (deviceJson.empty()) continue;
                std::stringstream ss_dev(deviceJson);
                std::string token;
                 LoraGatewayConfig config;
                 while(std::getline(ss_dev, token, ','))
                {
                    size_t colon_pos = token.find(":");
                     if(colon_pos != std::string::npos)
                    {
                        std::string key = token.substr(0, colon_pos);
                        std::string value = token.substr(colon_pos+1);
                         key.erase(std::remove(key.begin(), key.end(), '"'), key.end());
                        value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
                        if(key == "name") config.name=value;
                        if(key == "description") config.description=value;
                        if(key == "latitude") config.latitude = std::stod(value);
                        if(key == "longitude") config.longitude = std::stod(value);
                        if(key == "gateway_id") config.gateway_id=value;
                        if(key == "protocol") config.protocol=value;
                        if(key == "application") config.application=value;
                        if(key == "location") config.location=value;
                    }
                 }
                   loraGatewayConfigs.push_back(config);
            }
        }
    }
    nDevices = loraDeviceConfigs.size();
    nGateways= loraGatewayConfigs.size();
    nWiFiAPNodes = wifiAPConfigs.size();
    nWiFiStaNodes = wifiSTAConfigs.size();
    
    
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
   int i = 0;
    for (auto j = endDevices.Begin(); j != endDevices.End(); ++j,++i)
    {
       
        Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel>();
        Vector position = mobility->GetPosition();
        position.x= loraDeviceConfigs[i].latitude;
        position.y= loraDeviceConfigs[i].longitude;
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
    for (uint32_t i =0; i< loraGatewayConfigs.size();i++)
    {
        allocator->Add(Vector(loraGatewayConfigs[i].latitude,loraGatewayConfigs[i].longitude, 15.0));
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
    Ssid ssid = Ssid ("SmartCityAP");
    

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
    for(uint32_t i = 0; i < wifiAPConfigs.size(); i++)
    {
         allocatorAPWiFi->Add(Vector(wifiAPConfigs[i].latitude, wifiAPConfigs[i].longitude, 1.5));
    }
    wifimobility.SetPositionAllocator(allocatorAPWiFi);
    wifimobility.Install(wifiApNode);

    Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
    for (uint32_t i = 0; i < wifiSTAConfigs.size();i++)
    {
       allocatorStaWiFi->Add(Vector(wifiSTAConfigs[i].latitude, wifiSTAConfigs[i].longitude, 1.5));
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
    ApplicationContainer sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (0));
    sinkApps.Start (Seconds (0.0));
    sinkApps.Stop (Seconds (10.0));

    OnOffHelper onoff ("ns3::UdpSocketFactory",
                        InetSocketAddress (Ipv4Address ("192.168.1.1"), sinkPort));
    onoff.SetConstantRate (DataRate ("1Mbps"));
    onoff.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps = onoff.Install (wifiStaNodes.Get (0));
    clientApps.Start (Seconds (1.0));
    clientApps.Stop (Seconds (10.0));

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

