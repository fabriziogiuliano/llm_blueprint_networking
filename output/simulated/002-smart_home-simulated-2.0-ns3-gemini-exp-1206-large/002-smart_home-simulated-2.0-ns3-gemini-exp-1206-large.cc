
#include "ns3/building-allocator.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/buildings-helper.h"
#include "ns3/command-line.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
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
#include "ns3/wifi-mac-helper.h"
#include "ns3/config.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"

#include <algorithm>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AdvancedSmartHomeNetwork");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("AdvancedSmartHomeNetwork", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");

    // Simulation time
    double simulationTimeSeconds = 5400; // 1.5 hours

    // Create WiFi Access Points
    NodeContainer apNodes;
    apNodes.Create(2);

    // Create WiFi Stations
    NodeContainer staNodes;
    staNodes.Create(7);

    /************************
     *  Create WiFi Channel *
     ************************/
    NS_LOG_DEBUG("Create WiFi Channel...");

    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel(wifiChannel.Create());

    /************************
     *  Install WiFi Devices *
     ************************/
    NS_LOG_DEBUG("Install WiFi Devices...");

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    // Configure APs
    NetDeviceContainer apDevices;
    
    WifiMacHelper apMac;
    Ssid ssidAP1 = Ssid("HomeNet_Extended");
    apMac.SetType("ns3::ApWifiMac",
                  "Ssid", SsidValue(ssidAP1),
                  "BeaconGeneration", BooleanValue(true));
    apDevices.Add(wifi.Install(wifiPhy, apMac, apNodes.Get(0)));

    Ssid ssidAP2 = Ssid("Guest_Network");
    apMac.SetType("ns3::ApWifiMac",
                  "Ssid", SsidValue(ssidAP2),
                  "BeaconGeneration", BooleanValue(true));
    apDevices.Add(wifi.Install(wifiPhy, apMac, apNodes.Get(1)));

    // Configure Stations
    NetDeviceContainer staDevices;
    
    WifiMacHelper staMac;
    staMac.SetType("ns3::StaWifiMac",
                   "Ssid", SsidValue(ssidAP1),
                   "ActiveProbing", BooleanValue(false));
    staDevices.Add(wifi.Install(wifiPhy, staMac, staNodes.Get(0)));
    staDevices.Add(wifi.Install(wifiPhy, staMac, staNodes.Get(1)));
    staDevices.Add(wifi.Install(wifiPhy, staMac, staNodes.Get(2)));
    staDevices.Add(wifi.Install(wifiPhy, staMac, staNodes.Get(3)));
    staDevices.Add(wifi.Install(wifiPhy, staMac, staNodes.Get(4)));
    staDevices.Add(wifi.Install(wifiPhy, staMac, staNodes.Get(5)));
    
    staMac.SetType("ns3::StaWifiMac",
                   "Ssid", SsidValue(ssidAP2),
                   "ActiveProbing", BooleanValue(false));
    staDevices.Add(wifi.Install(wifiPhy, staMac, staNodes.Get(6)));

    /*********************
     *  Mobility Models *
     *********************/
    NS_LOG_DEBUG("Set Mobility Models...");

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNodes);
    mobility.Install(staNodes);

    // Set positions for APs (you can adjust these as needed)
    Ptr<ConstantPositionMobilityModel> ap1Mobility = apNodes.Get(0)->GetObject<ConstantPositionMobilityModel>();
    ap1Mobility->SetPosition(Vector(0.0, 0.0, 0.0));
    Ptr<ConstantPositionMobilityModel> ap2Mobility = apNodes.Get(1)->GetObject<ConstantPositionMobilityModel>();
    ap2Mobility->SetPosition(Vector(15.0, 0.0, 0.0)); 

    // Set positions for STAs based on distance_meters
    Ptr<ConstantPositionMobilityModel> staMobility;
    staMobility = staNodes.Get(0)->GetObject<ConstantPositionMobilityModel>();
    staMobility->SetPosition(Vector(3.0, 0.0, 0.0));
    staMobility = staNodes.Get(1)->GetObject<ConstantPositionMobilityModel>();
    staMobility->SetPosition(Vector(8.0, 0.0, 0.0));
    staMobility = staNodes.Get(2)->GetObject<ConstantPositionMobilityModel>();
    staMobility->SetPosition(Vector(5.0, 0.0, 0.0));
    staMobility = staNodes.Get(3)->GetObject<ConstantPositionMobilityModel>();
    staMobility->SetPosition(Vector(7.0, 0.0, 0.0));
    staMobility = staNodes.Get(4)->GetObject<ConstantPositionMobilityModel>();
    staMobility->SetPosition(Vector(12.0, 0.0, 0.0));
    staMobility = staNodes.Get(5)->GetObject<ConstantPositionMobilityModel>();
    staMobility->SetPosition(Vector(10.0, 0.0, 0.0));
    staMobility = staNodes.Get(6)->GetObject<ConstantPositionMobilityModel>();
    staMobility->SetPosition(Vector(17.0, 0.0, 0.0));

    /*************************
     *  Internet Stack & IPs *
     *************************/
    NS_LOG_DEBUG("Install Internet Stack & Assign IPs...");

    InternetStackHelper internet;
    internet.Install(apNodes);
    internet.Install(staNodes);

    Ipv4AddressHelper ipv4;

    ipv4.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ap1Interfaces = ipv4.Assign(apDevices.Get(0));
    Ipv4InterfaceContainer staInterfaces1 = ipv4.Assign(staDevices.Get(0));
    staInterfaces1.Add(ipv4.Assign(staDevices.Get(1)).Get(0));
    staInterfaces1.Add(ipv4.Assign(staDevices.Get(2)).Get(0));
    staInterfaces1.Add(ipv4.Assign(staDevices.Get(3)).Get(0));
    staInterfaces1.Add(ipv4.Assign(staDevices.Get(4)).Get(0));
    staInterfaces1.Add(ipv4.Assign(staDevices.Get(5)).Get(0));

    ipv4.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ap2Interfaces = ipv4.Assign(apDevices.Get(1));
    Ipv4InterfaceContainer staInterfaces2 = ipv4.Assign(staDevices.Get(6));

    /**********************
    * Set MAC Addresses  *
    **********************/
    NS_LOG_DEBUG("Set MAC Addresses...");

    Ptr<WifiNetDevice> wifiNetDevice;

    wifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(0));
    wifiNetDevice->SetAddress(Mac48Address("A1:B2:C3:D4:E5:01"));

    wifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(1));
    wifiNetDevice->SetAddress(Mac48Address("A1:B2:C3:D4:E5:02"));
    
    wifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(2));
    wifiNetDevice->SetAddress(Mac48Address("A1:B2:C3:D4:E5:03"));

    wifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(3));
    wifiNetDevice->SetAddress(Mac48Address("A1:B2:C3:D4:E5:04"));

    wifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(4));
    wifiNetDevice->SetAddress(Mac48Address("A1:B2:C3:D4:E5:05"));

    wifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(5));
    wifiNetDevice->SetAddress(Mac48Address("A1:B2:C3:D4:E5:06"));
    
    wifiNetDevice = DynamicCast<WifiNetDevice>(staDevices.Get(6));
    wifiNetDevice->SetAddress(Mac48Address("A1:B2:C3:D4:E5:07"));

    /****************************
     *  Configure Applications *
     ****************************/
    NS_LOG_DEBUG("Configure Applications...");

    // Station ST-001 (Laptop) - TCP
    uint16_t sinkPort1 = 5000;
    Address sinkAddress1(InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort1));
    PacketSinkHelper packetSinkHelper1("ns3::TcpSocketFactory", sinkAddress1);
    ApplicationContainer sinkApps1 = packetSinkHelper1.Install(apNodes.Get(0));
    sinkApps1.Start(Seconds(0.0));
    sinkApps1.Stop(Seconds(simulationTimeSeconds));

    OnOffHelper client1("ns3::TcpSocketFactory", sinkAddress1);
    client1.SetAttribute("MaxBytes", UintegerValue(0));
    client1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=2700]"));
    client1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientApps1 = client1.Install(staNodes.Get(0));
    clientApps1.Start(Seconds(0.0));
    clientApps1.Stop(Seconds(simulationTimeSeconds));

    // Station ST-002 (Smart TV) - UDP
    uint16_t sinkPort2 = 5001;
    Address sinkAddress2(InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort2));
    PacketSinkHelper packetSinkHelper2("ns3::UdpSocketFactory", sinkAddress2);
    ApplicationContainer sinkApps2 = packetSinkHelper2.Install(apNodes.Get(0));
    sinkApps2.Start(Seconds(0.0));
    sinkApps2.Stop(Seconds(simulationTimeSeconds));

    OnOffHelper client2("ns3::UdpSocketFactory", sinkAddress2);
    client2.SetAttribute("MaxBytes", UintegerValue(0));
    client2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=2700]"));
    client2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client2.SetConstantRate(DataRate("5Mbps"));
    ApplicationContainer clientApps2 = client2.Install(staNodes.Get(1));
    clientApps2.Start(Seconds(10.0));
    clientApps2.Stop(Seconds(simulationTimeSeconds));

    // Station ST-003 (Tablet) - TCP
    uint16_t sinkPort3 = 5002;
    Address sinkAddress3(InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort3));
    PacketSinkHelper packetSinkHelper3("ns3::TcpSocketFactory", sinkAddress3);
    ApplicationContainer sinkApps3 = packetSinkHelper3.Install(apNodes.Get(0));
    sinkApps3.Start(Seconds(0.0));
    sinkApps3.Stop(Seconds(simulationTimeSeconds));

    OnOffHelper client3("ns3::TcpSocketFactory", sinkAddress3);
    client3.SetAttribute("MaxBytes", UintegerValue(0));
    client3.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1800]"));
    client3.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client3.SetConstantRate(DataRate("1Mbps"));
    ApplicationContainer clientApps3 = client3.Install(staNodes.Get(2));
    clientApps3.Start(Seconds(60.0));
    clientApps3.Stop(Seconds(simulationTimeSeconds));

    // Station SLB-001 (Smart Speaker) - UDP
    uint16_t sinkPort4 = 5003;
    Address sinkAddress4(InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort4));
    PacketSinkHelper packetSinkHelper4("ns3::UdpSocketFactory", sinkAddress4);
    ApplicationContainer sinkApps4 = packetSinkHelper4.Install(apNodes.Get(0));
    sinkApps4.Start(Seconds(0.0));
    sinkApps4.Stop(Seconds(simulationTimeSeconds));

    OnOffHelper client4("ns3::UdpSocketFactory", sinkAddress4);
    client4.SetAttribute("MaxBytes", UintegerValue(0));
    client4.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1500]"));
    client4.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client4.SetConstantRate(DataRate("1.5Mbps"));
    ApplicationContainer clientApps4 = client4.Install(staNodes.Get(3));
    clientApps4.Start(Seconds(120.0));
    clientApps4.Stop(Seconds(simulationTimeSeconds));

    // Station SDL-001 (Smartphone) - TCP
    uint16_t sinkPort5 = 5004;
    Address sinkAddress5(InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort5));
    PacketSinkHelper packetSinkHelper5("ns3::TcpSocketFactory", sinkAddress5);
    ApplicationContainer sinkApps5 = packetSinkHelper5.Install(apNodes.Get(0));
    sinkApps5.Start(Seconds(0.0));
    sinkApps5.Stop(Seconds(simulationTimeSeconds));

    OnOffHelper client5("ns3::TcpSocketFactory", sinkAddress5);
    client5.SetAttribute("MaxBytes", UintegerValue(0));
    client5.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=300]"));
    client5.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientApps5 = client5.Install(staNodes.Get(4));
    clientApps5.Start(Seconds(300.0));
    clientApps5.Stop(Seconds(simulationTimeSeconds));

    // Station SSC-001 (Security Camera) - UDP
    uint16_t sinkPort6 = 5005;
    Address sinkAddress6(InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort6));
    PacketSinkHelper packetSinkHelper6("ns3::UdpSocketFactory", sinkAddress6);
    ApplicationContainer sinkApps6 = packetSinkHelper6.Install(apNodes.Get(0));
    sinkApps6.Start(Seconds(0.0));
    sinkApps6.Stop(Seconds(simulationTimeSeconds));

    OnOffHelper client6("ns3::UdpSocketFactory", sinkAddress6);
    client6.SetAttribute("MaxBytes", UintegerValue(0));
    client6.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1800]"));
    client6.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client6.SetConstantRate(DataRate("2Mbps"));
    ApplicationContainer clientApps6 = client6.Install(staNodes.Get(5));
    clientApps6.Start(Seconds(50.0));
    clientApps6.Stop(Seconds(simulationTimeSeconds));

    // Station STG-001 (Guest Device) - TCP
    uint16_t sinkPort7 = 5006;
    Address sinkAddress7(InetSocketAddress(ap2Interfaces.GetAddress(0), sinkPort7));
    PacketSinkHelper packetSinkHelper7("ns3::TcpSocketFactory", sinkAddress7);
    ApplicationContainer sinkApps7 = packetSinkHelper7.Install(apNodes.Get(1));
    sinkApps7.Start(Seconds(0.0));
    sinkApps7.Stop(Seconds(simulationTimeSeconds));

    OnOffHelper client7("ns3::TcpSocketFactory", sinkAddress7);
    client7.SetAttribute("MaxBytes", UintegerValue(0));
    client7.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1200]"));
    client7.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client7.SetConstantRate(DataRate("1Mbps"));
    ApplicationContainer clientApps7 = client7.Install(staNodes.Get(6));
    clientApps7.Start(Seconds(180.0));
    clientApps7.Stop(Seconds(simulationTimeSeconds));

    // Enable pcap tracing for APs and STAs
    wifiPhy.EnablePcap("advanced_smart_home_ap1", apDevices.Get(0));
    wifiPhy.EnablePcap("advanced_smart_home_ap2", apDevices.Get(1));
    for (uint32_t i = 0; i < staDevices.GetN(); ++i) {
      wifiPhy.EnablePcap("advanced_smart_home_sta" + std::to_string(i), staDevices.Get(i));
    }

    /**********
     *  Run    *
     **********/

    Simulator::Stop(Seconds(simulationTimeSeconds));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    NS_LOG_INFO("Done.");

    return 0;
}
