
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
#include "ns3/wifi-mac.h"
#include "ns3/nstime.h"
#include "ns3/string.h"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE("SmartHomeExample");

// Network settings
int nDevices = 0;                 //!< Number of end device nodes to create
int nGateways = 0;                  //!< Number of gateway nodes to create
int nWiFiAPNodes = 2;
int nWiFiStaNodes = 7;
double radiusMeters = 100;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 5400; //!< Scenario duration (s) in simulated time

// Channel model
bool realisticChannelModel = false; //!< Whether to use a more realistic channel model with
                                    //!< Buildings and correlated shadowing

// Output control
bool printBuildingInfo = true; //!< Whether to print building information

// Struct to hold device data
struct DeviceData {
    std::string stationId;
    std::string deviceType;
    std::string ssid;
    std::string wpaPassphrase;
    std::string wpaKeyMgmt;
    std::string wlanIP;
    std::string wlanMACAddr;
    double distanceMeters;
    std::string location;
     std::string ethIP;
    std::string user;
    std::string password;
    std::string protocol;
    std::string throughput;
    double duration;
    double startTime;
};

struct ApData{
    std::string apId;
    std::string ssid;
    std::string wpaPassphrase;
    std::string wpaKeyMgmt;
    std::string wlanIP;
    std::string ethIP;
};


// Function to parse the JSON string and populate DeviceData
std::vector<DeviceData> parseDevices(std::string jsonString) {
    std::vector<DeviceData> devices;
    size_t pos = jsonString.find("\"wifi_stations\":");
    if (pos == std::string::npos) return devices;
    pos = jsonString.find("[", pos) + 1;
    size_t end = jsonString.find("]", pos);
    if (end == std::string::npos) return devices;

    std::string devicesStr = jsonString.substr(pos, end - pos);
    std::stringstream ss(devicesStr);
    std::string deviceStr;
    while (std::getline(ss, deviceStr, '{')) {
        if (deviceStr.empty()) continue;
        std::stringstream deviceSS("{" + deviceStr);
        DeviceData device;
        std::string pair;
        while (std::getline(deviceSS, pair, ',')) {
             size_t colonPos = pair.find(":");
            if (colonPos == std::string::npos) continue;
            std::string key = pair.substr(0, colonPos);
            std::string value = pair.substr(colonPos + 1);

             key.erase(std::remove(key.begin(), key.end(), '"'), key.end());
              value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
            
            if (key.find("station_id") != std::string::npos) {
                device.stationId = value;
            } else if (key.find("device_type") != std::string::npos) {
                device.deviceType = value;
             }else if (key.find("ssid") != std::string::npos) {
                device.ssid = value;
            } else if (key.find("wpa_passphrase") != std::string::npos) {
                device.wpaPassphrase = value;
            } else if (key.find("wpa_key_mgmt") != std::string::npos) {
                device.wpaKeyMgmt = value;
            } else if (key.find("wlan_IP") != std::string::npos) {
                device.wlanIP = value;
            }else if (key.find("wlan_MAC_ADDR") != std::string::npos) {
                device.wlanMACAddr = value;
            } else if (key.find("distance_meters") != std::string::npos) {
                device.distanceMeters = std::stod(value);
            } else if (key.find("location") != std::string::npos) {
                 device.location = value;
            } else if (key.find("eth_IP") != std::string::npos) {
                device.ethIP = value;
            }
            else if (key.find("user") != std::string::npos) {
                device.user = value;
            }
             else if (key.find("password") != std::string::npos) {
                device.password = value;
            }else if (key.find("protocol") != std::string::npos)
            {
                device.protocol = value;
            }else if (key.find("throughput") != std::string::npos)
            {
                device.throughput = value;
            }else if (key.find("duration") != std::string::npos)
            {
                device.duration = std::stod(value);
            }else if (key.find("start_time") != std::string::npos)
            {
                 device.startTime = std::stod(value);
            }
        }
        devices.push_back(device);
    }
    return devices;
}


// Function to parse the JSON string and populate ApData
std::vector<ApData> parseAps(std::string jsonString) {
     std::vector<ApData> aps;
    size_t pos = jsonString.find("\"wifi_access_points\":");
    if (pos == std::string::npos) return aps;
    pos = jsonString.find("[", pos) + 1;
    size_t end = jsonString.find("]", pos);
    if (end == std::string::npos) return aps;

    std::string apsStr = jsonString.substr(pos, end - pos);
    std::stringstream ss(apsStr);
    std::string apStr;
    while (std::getline(ss, apStr, '{')) {
        if (apStr.empty()) continue;
        std::stringstream apSS("{" + apStr);
        ApData ap;
        std::string pair;
        while (std::getline(apSS, pair, ',')) {
            size_t colonPos = pair.find(":");
            if (colonPos == std::string::npos) continue;
            std::string key = pair.substr(0, colonPos);
            std::string value = pair.substr(colonPos + 1);

            key.erase(std::remove(key.begin(), key.end(), '"'), key.end());
            value.erase(std::remove(value.begin(), value.end(), '"'), value.end());

           if (key.find("ap_id") != std::string::npos) {
                ap.apId = value;
            } else if (key.find("ssid") != std::string::npos) {
                ap.ssid = value;
            } else if (key.find("wpa_passphrase") != std::string::npos) {
                ap.wpaPassphrase = value;
            } else if (key.find("wpa_key_mgmt") != std::string::npos) {
                ap.wpaKeyMgmt = value;
            } else if (key.find("wlan_IP") != std::string::npos) {
                ap.wlanIP = value;
            }else if (key.find("eth_IP") != std::string::npos) {
                 ap.ethIP = value;
            }
        }
        aps.push_back(ap);
    }
    return aps;
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.AddValue("simulationTime", "The time (s) for which to simulate", simulationTimeSeconds);
    cmd.AddValue("print", "Whether or not to print building information", printBuildingInfo);
    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("SmartHomeExample", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");
    // Create the time value from the period
    Time simulationTime = Seconds(simulationTimeSeconds);

    // Read JSON from file
    std::ifstream file("test_blueprint.json");
    if (!file.is_open()) {
        std::cerr << "Failed to open test_blueprint.json" << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonString = buffer.str();
    file.close();

    // Parse devices from the JSON string
     std::vector<DeviceData> devices = parseDevices(jsonString);
     std::vector<ApData> aps = parseAps(jsonString);

    nWiFiStaNodes = devices.size();
    nWiFiAPNodes = aps.size();
    
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

    MobilityHelper loramobility;
    loramobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

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

    Time appStopTime = simulationTime;
    PeriodicSenderHelper appHelper = PeriodicSenderHelper();
    appHelper.SetPeriod(Seconds(300)); // Set a default period
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
    wifiStaNodes.Create(nWiFiStaNodes);

    NodeContainer wifiApNodes;
    wifiApNodes.Create(nWiFiAPNodes);

    YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (wifichannel.Create ());

    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;

    NetDeviceContainer staDevices;
    NetDeviceContainer apDevices;
    std::vector<Ipv4InterfaceContainer> staNodeInterfaces;
     std::vector<Ipv4InterfaceContainer> apNodeInterfaces;

    MobilityHelper wifimobility;
     wifimobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    InternetStackHelper stack;
    stack.Install (wifiApNodes);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;

    uint32_t ap_index=0;

    for (const auto& ap : aps) {
         Ssid ssid = Ssid (ap.ssid);
           mac.SetType ("ns3::ApWifiMac",
            "Ssid", SsidValue (ssid));
        NetDeviceContainer currentApDevice = wifi.Install(phy, mac, wifiApNodes.Get(ap_index));
        apDevices.Add(currentApDevice);
        
        Ptr<ListPositionAllocator> allocatorAPWiFi = CreateObject<ListPositionAllocator>();
          allocatorAPWiFi->Add(Vector(38.10351066811091 + (ap_index * 5) , 13.3459399220741 + (ap_index * 5), 1.5));
        wifimobility.SetPositionAllocator(allocatorAPWiFi);
        wifimobility.Install(wifiApNodes.Get(ap_index));

          address.SetBase (ap.wlanIP, "255.255.255.0");
            apNodeInterfaces.push_back(address.Assign (currentApDevice));
        ap_index++;
    }

    uint32_t sta_index=0;

    for (const auto& device : devices) {
        Ssid ssid = Ssid(device.ssid);
        mac.SetType ("ns3::StaWifiMac",
            "Ssid", SsidValue (ssid),
             "ActiveProbing", BooleanValue (false));
        NetDeviceContainer currentStaDevice = wifi.Install(phy, mac, wifiStaNodes.Get(sta_index));
        staDevices.Add(currentStaDevice);
       
        Ptr<ListPositionAllocator> allocatorStaWiFi = CreateObject<ListPositionAllocator>();
         allocatorStaWiFi->Add(Vector(38.10863528672426+ (sta_index * 5), 13.34050633101243+ (sta_index * 5), 1.5));
        wifimobility.SetPositionAllocator(allocatorStaWiFi);
        wifimobility.Install(wifiStaNodes.Get(sta_index));

         address.SetBase (device.wlanIP, "255.255.255.0");
         staNodeInterfaces.push_back(address.Assign (currentStaDevice));

         sta_index++;
    }

    // Print the IP addresses of the WiFi STA nodes
     for (uint32_t i = 0; i < staNodeInterfaces.size(); ++i)
    {
        Ipv4Address addr = staNodeInterfaces[i].GetAddress (0);
       std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }
    

    NS_LOG_DEBUG("WiFi - Setup Udp");

    uint16_t sinkPort = 8080;
   
    for (uint32_t i = 0; i < nWiFiStaNodes; ++i) {
            
      if (devices[i].protocol == "UDP") {
        Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
        PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkAddress);
         ApplicationContainer sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (i));
        sinkApps.Start (Seconds (devices[i].startTime));
        sinkApps.Stop (Seconds (devices[i].startTime + devices[i].duration));
        
        if (devices[i].throughput != "maximum") {
          OnOffHelper onoff ("ns3::UdpSocketFactory",
                        InetSocketAddress (staNodeInterfaces[i].GetAddress(0), sinkPort));
          onoff.SetConstantRate (DataRate (std::stod(devices[i].throughput)));
          onoff.SetAttribute ("PacketSize", UintegerValue (1024));
           ApplicationContainer clientApps = onoff.Install (wifiStaNodes.Get (i));
          clientApps.Start (Seconds (devices[i].startTime));
           clientApps.Stop (Seconds (devices[i].startTime + devices[i].duration));
       }
      } else if(devices[i].protocol == "TCP") {
          Address sinkAddress (InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
          PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkAddress);
           ApplicationContainer sinkApps = packetSinkHelper.Install (wifiStaNodes.Get (i));
           sinkApps.Start (Seconds (devices[i].startTime));
           sinkApps.Stop (Seconds (devices[i].startTime + devices[i].duration));
        
         if (devices[i].throughput != "maximum") {
            OnOffHelper onoff ("ns3::TcpSocketFactory",
                      InetSocketAddress (staNodeInterfaces[i].GetAddress(0), sinkPort));
            onoff.SetConstantRate (DataRate (std::stod(devices[i].throughput)));
            onoff.SetAttribute ("PacketSize", UintegerValue (1024));
            ApplicationContainer clientApps = onoff.Install (wifiStaNodes.Get (i));
           clientApps.Start (Seconds (devices[i].startTime));
           clientApps.Stop (Seconds (devices[i].startTime + devices[i].duration));
          } else {
             OnOffHelper onoff ("ns3::TcpSocketFactory",
                      InetSocketAddress (staNodeInterfaces[i].GetAddress(0), sinkPort));
              onoff.SetAttribute ("PacketSize", UintegerValue (1024));
             ApplicationContainer clientApps = onoff.Install (wifiStaNodes.Get (i));
           clientApps.Start (Seconds (devices[i].startTime));
           clientApps.Stop (Seconds (devices[i].startTime + devices[i].duration));
            }
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
