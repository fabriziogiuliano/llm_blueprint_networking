To create the NS-3 code for the "Smart Agriculture" blueprint, we need to adapt the provided sample NS-3 code to fit the components and network type specified in the "Smart Agriculture" blueprint. Below is the NS-3 code tailored for the "Smart Agriculture" scenario:


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

NS_LOG_COMPONENT_DEFINE("SmartAgricultureExample");

// Network settings
int nDevices = 4;                 //!< Number of end device nodes to create
int nGateways = 1;                  //!< Number of gateway nodes to create
double radiusMeters = 1000;         //!< Radius (m) of the deployment
double simulationTimeSeconds = 600; //!< Scenario duration (s) in simulated time

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
    LogComponentEnable("SmartAgricultureExample", LOG_LEVEL_ALL);

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
    allocator->Add(Vector(0.0, 0.0, 15.0));
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


### Explanation:
1. **Network Settings**:
   - The number of end devices (`nDevices`) is set to 4 to represent the Soil Moisture Sensor, Weather Station, Irrigation Controller, and Drone.
   - The number of gateways (`nGateways`) is set to 1.
   - The radius of the deployment area is set to 1000 meters.
   - The simulation time is set to 600 seconds.

2. **Channel Model**:
   - A realistic channel model with buildings and correlated shadowing can be enabled or disabled.

3. **Mobility**:
   - The mobility model is set to `ConstantPositionMobilityModel` with a uniform disc position allocator.

4. **LoRa Channel Creation**:
   - The LoRa channel is created with a log-distance propagation loss model.
   - If the realistic channel model is enabled, correlated shadowing and building penetration loss are added.

5. **Helpers**:
   - Helpers for LoRa PHY, MAC, and network server are created.

6. **End Devices**:
   - End devices are created and installed with LoRa PHY and MAC.
   - The devices are positioned at a certain height.

7. **Gateways**:
   - Gateways are created and installed with LoRa PHY and MAC.
   - The gateways are positioned at a certain height.

8. **Buildings**:
   - Buildings are handled using a grid building allocator.
   - Buildings are installed for end devices and gateways.

9. **Spreading Factor**:
   - The spreading factor for end devices is set up.

10. **Applications**:
    - Periodic sender applications are installed on the end devices.

11. **Network Server**:
    - A network server is created and connected to the gateways using Point-to-Point links.
    - A forwarder is installed for each gateway.

12. **Simulation Run**:
    - The simulation is run for the specified duration.
    - Performance metrics are computed and printed to a file.

This code sets up a LoRaWAN network for smart agriculture with the specified components and network type. Adjustments can be made based on specific requirements and additional components can be added as needed.