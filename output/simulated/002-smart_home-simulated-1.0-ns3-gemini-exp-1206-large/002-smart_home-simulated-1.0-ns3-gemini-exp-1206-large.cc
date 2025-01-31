
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
#include "ns3/boolean.h"
#include "ns3/config.h"

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

    // Scenario duration
    double simulationTimeSeconds = 5400; // 1.5 hours

    // WiFi settings
    int nWiFiAPNodes = 2;
    int nWiFiStaNodes = 7;

    /************************
     *  Create WiFi Nodes  *
     ************************/
    NS_LOG_DEBUG("Create WiFi Nodes...");

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWiFiStaNodes);

    NodeContainer wifiApNodes;
    wifiApNodes.Create(nWiFiAPNodes);

    YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(wifichannel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    // AP configurations
    WifiMacHelper macAP1, macAP2;
    Ssid ssidAP1 = Ssid("HomeNet_Extended");
    Ssid ssidAP2 = Ssid("Guest_Network");

    macAP1.SetType("ns3::ApWifiMac",
                   "Ssid", SsidValue(ssidAP1));
    macAP2.SetType("ns3::ApWifiMac",
                   "Ssid", SsidValue(ssidAP2));

    NetDeviceContainer apDevices;
    apDevices.Add(wifi.Install(phy, macAP1, wifiApNodes.Get(0)));
    apDevices.Add(wifi.Install(phy, macAP2, wifiApNodes.Get(1)));

    // Station configurations
    WifiMacHelper macSta;
    macSta.SetType("ns3::StaWifiMac",
                   "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices;
    for (int i = 0; i < nWiFiStaNodes; ++i) {
        if (i < 6) {
            macSta.SetAttribute("Ssid", SsidValue(ssidAP1));
        } else {
            macSta.SetAttribute("Ssid", SsidValue(ssidAP2));
        }
        staDevices.Add(wifi.Install(phy, macSta, wifiStaNodes.Get(i)));
    }
    // Mobility
    MobilityHelper wifimobility;
    wifimobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> apPositionAlloc = CreateObject<ListPositionAllocator>();
    apPositionAlloc->Add(Vector(0.0, 0.0, 0.0)); // Example position for AP-001
    apPositionAlloc->Add(Vector(15.0, 0.0, 0.0)); // Example position for AP-002
    wifimobility.SetPositionAllocator(apPositionAlloc);
    wifimobility.Install(wifiApNodes);

    Ptr<ListPositionAllocator> staPositionAlloc = CreateObject<ListPositionAllocator>();
    staPositionAlloc->Add(Vector(3.0, 0.0, 0.0));   // ST-001
    staPositionAlloc->Add(Vector(8.0, 0.0, 0.0));   // ST-002
    staPositionAlloc->Add(Vector(5.0, 0.0, 0.0));   // ST-003
    staPositionAlloc->Add(Vector(7.0, 0.0, 0.0));   // SLB-001
    staPositionAlloc->Add(Vector(12.0, 0.0, 0.0));  // SDL-001
    staPositionAlloc->Add(Vector(10.0, 0.0, 0.0));  // SSC-001
    staPositionAlloc->Add(Vector(17.0, 0.0, 0.0));  // STG-001
    wifimobility.SetPositionAllocator(staPositionAlloc);
    wifimobility.Install(wifiStaNodes);

    // Internet stack
    InternetStackHelper stack;
    stack.Install(wifiApNodes);
    stack.Install(wifiStaNodes);

    // IP address assignment
    Ipv4AddressHelper address;
    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer ap1Interfaces = address.Assign(apDevices.Get(0));
    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ap2Interfaces = address.Assign(apDevices.Get(1));

    Ipv4InterfaceContainer staInterfaces;
    for (int i = 0; i < nWiFiStaNodes; ++i) {
        if (i < 6) {
            address.SetBase("192.168.2.0", "255.255.255.0");
            staInterfaces.Add(address.Assign(staDevices.Get(i)).Get(0));
        } else {
            address.SetBase("192.168.3.0", "255.255.255.0");
            staInterfaces.Add(address.Assign(staDevices.Get(i)).Get(0));
        }
    }

    // Security settings
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue("DsssRate1Mbps"));
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("999999"));
    
    for (uint32_t i = 0; i < apDevices.GetN(); ++i) {
        Ptr<WifiNetDevice> apDevice = DynamicCast<WifiNetDevice>(apDevices.Get(i));
        Ptr<WifiMac> apMac = apDevice->GetMac();
        if (apMac->GetSsid() == ssidAP1) {
            apMac->SetAttribute("AuthType", EnumValue(ns3::WIFI_AUTH_WPA2_PSK));
            apMac->SetAttribute("WpaKeyMgmt", EnumValue(ns3::WIFI_KEY_MGMT_PSK));
            apMac->SetAttribute("WpaPsk", StringValue("secure_password"));
        } else if (apMac->GetSsid() == ssidAP2) {
            apMac->SetAttribute("AuthType", EnumValue(ns3::WIFI_AUTH_WPA2_PSK));
            apMac->SetAttribute("WpaKeyMgmt", EnumValue(ns3::WIFI_KEY_MGMT_PSK));
            apMac->SetAttribute("WpaPsk", StringValue("guest_pass"));
        }
    }

    for (uint32_t i = 0; i < staDevices.GetN(); ++i) {
        Ptr<WifiNetDevice> staDevice = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        Ptr<WifiMac> staMac = staDevice->GetMac();
        if (staMac->GetSsid() == ssidAP1) {
            staMac->SetAttribute("AuthType", EnumValue(ns3::WIFI_AUTH_WPA2_PSK));
            staMac->SetAttribute("WpaKeyMgmt", EnumValue(ns3::WIFI_KEY_MGMT_PSK));
            staMac->SetAttribute("WpaPsk", StringValue("secure_password"));
        } else if (staMac->GetSsid() == ssidAP2) {
            staMac->SetAttribute("AuthType", EnumValue(ns3::WIFI_AUTH_WPA2_PSK));
            staMac->SetAttribute("WpaKeyMgmt", EnumValue(ns3::WIFI_KEY_MGMT_PSK));
            staMac->SetAttribute("WpaPsk", StringValue("guest_pass"));
        }
    }

    // Applications
    uint16_t sinkPort = 8080;

    // ST-001 (TCP, maximum throughput)
    OnOffHelper onOffST001("ns3::TcpSocketFactory", InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort));
    onOffST001.SetAttribute("DataRate", StringValue("100Mbps")); // High data rate for maximum throughput
    onOffST001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffST001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=2700]"));
    onOffST001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppST001 = onOffST001.Install(wifiStaNodes.Get(0));
    clientAppST001.Start(Seconds(0));
    clientAppST001.Stop(Seconds(2700));

    // ST-002 (UDP, 5 Mbps)
    OnOffHelper onOffST002("ns3::UdpSocketFactory", InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort));
    onOffST002.SetConstantRate(DataRate("5Mbps"));
    onOffST002.SetAttribute("PacketSize", UintegerValue(1024));
    onOffST002.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=2700]"));
    onOffST002.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppST002 = onOffST002.Install(wifiStaNodes.Get(1));
    clientAppST002.Start(Seconds(10));
    clientAppST002.Stop(Seconds(2710));

    // ST-003 (TCP, 1 Mbps)
    OnOffHelper onOffST003("ns3::TcpSocketFactory", InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort));
    onOffST003.SetConstantRate(DataRate("1Mbps"));
    onOffST003.SetAttribute("PacketSize", UintegerValue(1024));
    onOffST003.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1800]"));
    onOffST003.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppST003 = onOffST003.Install(wifiStaNodes.Get(2));
    clientAppST003.Start(Seconds(60));
    clientAppST003.Stop(Seconds(1860));

    // SLB-001 (UDP, 1.5 Mbps)
    OnOffHelper onOffSLB001("ns3::UdpSocketFactory", InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort));
    onOffSLB001.SetConstantRate(DataRate("1.5Mbps"));
    onOffSLB001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSLB001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1500]"));
    onOffSLB001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppSLB001 = onOffSLB001.Install(wifiStaNodes.Get(3));
    clientAppSLB001.Start(Seconds(120));
    clientAppSLB001.Stop(Seconds(1620));

    // SDL-001 (TCP, maximum throughput)
    OnOffHelper onOffSDL001("ns3::TcpSocketFactory", InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort));
    onOffSDL001.SetAttribute("DataRate", StringValue("100Mbps")); // High data rate for maximum throughput
    onOffSDL001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSDL001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=300]"));
    onOffSDL001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppSDL001 = onOffSDL001.Install(wifiStaNodes.Get(4));
    clientAppSDL001.Start(Seconds(300));
    clientAppSDL001.Stop(Seconds(600));

    // SSC-001 (UDP, 2 Mbps)
    OnOffHelper onOffSSC001("ns3::UdpSocketFactory", InetSocketAddress(ap1Interfaces.GetAddress(0), sinkPort));
    onOffSSC001.SetConstantRate(DataRate("2Mbps"));
    onOffSSC001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSSC001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1800]"));
    onOffSSC001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppSSC001 = onOffSSC001.Install(wifiStaNodes.Get(5));
    clientAppSSC001.Start(Seconds(50));
    clientAppSSC001.Stop(Seconds(1850));

    // STG-001 (TCP, 1 Mbps)
    OnOffHelper onOffSTG001("ns3::TcpSocketFactory", InetSocketAddress(ap2Interfaces.GetAddress(0), sinkPort));
    onOffSTG001.SetConstantRate(DataRate("1Mbps"));
    onOffSTG001.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSTG001.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1200]"));
    onOffSTG001.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientAppSTG001 = onOffSTG001.Install(wifiStaNodes.Get(6));
    clientAppSTG001.Start(Seconds(180));
    clientAppSTG001.Stop(Seconds(1380));

    // Install PacketSinkHelper on AP nodes to receive data
    PacketSinkHelper sinkHelperTCP("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    PacketSinkHelper sinkHelperUDP("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = sinkHelperTCP.Install(wifiApNodes);
    sinkApps.Add(sinkHelperUDP.Install(wifiApNodes));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(simulationTimeSeconds));

    // Tracing
    phy.EnablePcap("wifi-ap", apDevices);
    phy.EnablePcap("wifi-sta", staDevices);

    /**********
     *  Run  *
     **********/

    Simulator::Stop(Seconds(simulationTimeSeconds));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
