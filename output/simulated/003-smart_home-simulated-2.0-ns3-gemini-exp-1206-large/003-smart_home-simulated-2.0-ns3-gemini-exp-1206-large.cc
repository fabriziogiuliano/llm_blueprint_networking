
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
#include "ns3/string.h"
#include "ns3/config.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

#include <algorithm>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ExtendedSmartHomeNetwork");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("ExtendedSmartHomeNetwork", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");

    // Parameters from the blueprint
    double simulationTimeSeconds = 3600; // 1 hour
    
    int nWiFiAPNodes = 1;
    int nWiFiStaNodes = 7;

    /************************
     *  Create WiFi Nodes  *
     ************************/
    NS_LOG_DEBUG("Create WiFi Nodes...");

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nWiFiStaNodes);

    NodeContainer wifiApNode;
    wifiApNode.Create(nWiFiAPNodes);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a); // Or another standard as needed
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("SmartHomeAPMyExperiment");

    // Configure AP
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid),
                "QosSupported", BooleanValue(true));
    NetDeviceContainer apDevice;
    apDevice = wifi.Install(phy, mac, wifiApNode.Get(0));
    
    // Configure Stations
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "QosSupported", BooleanValue(true),
                "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiStaNodes);
    
    // Mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    // Define positions for AP and stations based on the blueprint
    positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // AP-001
    positionAlloc->Add(Vector(10.0, 0.0, 0.0)); // ST-001
    positionAlloc->Add(Vector(5.0, 0.0, 0.0));  // SLB-001
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));  // SDL-001
    positionAlloc->Add(Vector(15.0, 0.0, 0.0)); // SSC-001
    positionAlloc->Add(Vector(7.0, 0.0, 0.0));  // SRT-001
    positionAlloc->Add(Vector(12.0, 0.0, 0.0)); // SMS-001
    positionAlloc->Add(Vector(3.0, 0.0, 0.0));  // SVC-001
    
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    // Internet stack
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apNodeInterface;
    apNodeInterface = address.Assign(apDevice);
    Ipv4InterfaceContainer staNodeInterfaces;
    staNodeInterfaces = address.Assign(staDevices);

    // Set AP IP
    Names::Add ("AP-001", wifiApNode.Get (0));
    Ptr<Ipv4> apIpv4 = wifiApNode.Get(0)->GetObject<Ipv4>();
    apIpv4->SetAddress(1, Ipv4InterfaceAddress(Ipv4Address("192.168.1.1"), Ipv4Mask("255.255.255.0")));
    
    
    // Set station IPs and MAC addresses
    Names::Add ("ST-001", wifiStaNodes.Get (0));
    Ptr<Ipv4> st1Ipv4 = wifiStaNodes.Get(0)->GetObject<Ipv4>();
    st1Ipv4->SetAddress(1, Ipv4InterfaceAddress(Ipv4Address("192.168.1.103"), Ipv4Mask("255.255.255.0")));
    Config::Set ("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Mac/Address", Mac48AddressValue (Mac48Address ("61:5F:64:5E:90:EB")));

    Names::Add ("SLB-001", wifiStaNodes.Get (1));
    Ptr<Ipv4> st2Ipv4 = wifiStaNodes.Get(1)->GetObject<Ipv4>();
    st2Ipv4->SetAddress(1, Ipv4InterfaceAddress(Ipv4Address("192.168.1.102"), Ipv4Mask("255.255.255.0")));
    Config::Set ("/NodeList/2/DeviceList/0/$ns3::WifiNetDevice/Mac/Address", Mac48AddressValue (Mac48Address ("60:36:1E:9A:0A:0C")));
    
    Names::Add ("SDL-001", wifiStaNodes.Get (2));
    Ptr<Ipv4> st3Ipv4 = wifiStaNodes.Get(2)->GetObject<Ipv4>();
    st3Ipv4->SetAddress(1, Ipv4InterfaceAddress(Ipv4Address("192.168.1.101"), Ipv4Mask("255.255.255.0")));
    Config::Set ("/NodeList/3/DeviceList/0/$ns3::WifiNetDevice/Mac/Address", Mac48AddressValue (Mac48Address ("39:9F:51:CD:F7:08")));

    Names::Add ("SSC-001", wifiStaNodes.Get (3));
    Ptr<Ipv4> st4Ipv4 = wifiStaNodes.Get(3)->GetObject<Ipv4>();
    st4Ipv4->SetAddress(1, Ipv4InterfaceAddress(Ipv4Address("192.168.1.100"), Ipv4Mask("255.255.255.0")));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Mac/Address", Mac48AddressValue (Mac48Address ("0B:E3:41:0A:33:B7")));

    Names::Add ("SRT-001", wifiStaNodes.Get (4));
    Ptr<Ipv4> st5Ipv4 = wifiStaNodes.Get(4)->GetObject<Ipv4>();
    st5Ipv4->SetAddress(1, Ipv4InterfaceAddress(Ipv4Address("192.168.1.104"), Ipv4Mask("255.255.255.0")));
    Config::Set ("/NodeList/5/DeviceList/0/$ns3::WifiNetDevice/Mac/Address", Mac48AddressValue (Mac48Address ("A1:B2:C3:D4:E5:F6")));

    Names::Add ("SMS-001", wifiStaNodes.Get (5));
    Ptr<Ipv4> st6Ipv4 = wifiStaNodes.Get(5)->GetObject<Ipv4>();
    st6Ipv4->SetAddress(1, Ipv4InterfaceAddress(Ipv4Address("192.168.1.105"), Ipv4Mask("255.255.255.0")));
    Config::Set ("/NodeList/6/DeviceList/0/$ns3::WifiNetDevice/Mac/Address", Mac48AddressValue (Mac48Address ("F1:E2:D3:C4:B5:A6")));

    Names::Add ("SVC-001", wifiStaNodes.Get (6));
    Ptr<Ipv4> st7Ipv4 = wifiStaNodes.Get(6)->GetObject<Ipv4>();
    st7Ipv4->SetAddress(1, Ipv4InterfaceAddress(Ipv4Address("192.168.1.106"), Ipv4Mask("255.255.255.0")));
    Config::Set ("/NodeList/7/DeviceList/0/$ns3::WifiNetDevice/Mac/Address", Mac48AddressValue (Mac48Address ("1A:2B:3C:4D:5E:6F")));
    
    //WPA2 PSK configuration
    StringValue pskValue = StringValue("12345678");
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMps/Ssid", SsidValue (ssid));
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMps/Authentication/Psk", pskValue);
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMps/Authentication/Wpa", StringValue ("WPA2"));
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMps/Authentication/Wpa2Psk", BooleanValue (true));
    
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMps/Ssid", SsidValue (ssid));
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMps/Authentication/Psk", pskValue);
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMps/Authentication/Wpa", StringValue ("WPA2"));
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMps/Authentication/Wpa2Psk", BooleanValue (true));

    // Install applications for each station based on the blueprint
    // ST-001
    uint16_t st1Port = 9;
    OnOffHelper onOffSt1("ns3::UdpSocketFactory", InetSocketAddress(apNodeInterface.GetAddress(0), st1Port));
    onOffSt1.SetAttribute("DataRate", StringValue("500Mbps"));
    onOffSt1.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSt1.SetAttribute("StartTime", TimeValue(Seconds(0)));
    onOffSt1.SetAttribute("StopTime", TimeValue(Seconds(1800)));
    ApplicationContainer st1ClientApp = onOffSt1.Install(wifiStaNodes.Get(0));

    PacketSinkHelper sinkSt1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), st1Port));
    ApplicationContainer st1SinkApp = sinkSt1.Install(wifiApNode.Get(0));
    st1SinkApp.Start(Seconds(0.0));
    st1SinkApp.Stop(Seconds(1800));

    // SLB-001
    uint16_t slb1Port = 10;
    OnOffHelper onOffSlb1("ns3::TcpSocketFactory", InetSocketAddress(apNodeInterface.GetAddress(0), slb1Port));
    onOffSlb1.SetAttribute("DataRate", StringValue("500Mbps"));
    onOffSlb1.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSlb1.SetAttribute("StartTime", TimeValue(Seconds(0)));
    onOffSlb1.SetAttribute("StopTime", TimeValue(Seconds(1800)));
    ApplicationContainer slb1ClientApp = onOffSlb1.Install(wifiStaNodes.Get(1));

    PacketSinkHelper sinkSlb1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), slb1Port));
    ApplicationContainer slb1SinkApp = sinkSlb1.Install(wifiApNode.Get(0));
    slb1SinkApp.Start(Seconds(0.0));
    slb1SinkApp.Stop(Seconds(1800));

    // SDL-001
    uint16_t sdl1Port = 11;
    OnOffHelper onOffSdl1("ns3::UdpSocketFactory", InetSocketAddress(apNodeInterface.GetAddress(0), sdl1Port));
    onOffSdl1.SetAttribute("DataRate", StringValue("1Mbps"));
    onOffSdl1.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSdl1.SetAttribute("StartTime", TimeValue(Seconds(10)));
    onOffSdl1.SetAttribute("StopTime", TimeValue(Seconds(610)));
    ApplicationContainer sdl1ClientApp = onOffSdl1.Install(wifiStaNodes.Get(2));

    PacketSinkHelper sinkSdl1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sdl1Port));
    ApplicationContainer sdl1SinkApp = sinkSdl1.Install(wifiApNode.Get(0));
    sdl1SinkApp.Start(Seconds(0.0));
    sdl1SinkApp.Stop(Seconds(610));

    // SSC-001
    uint16_t ssc1Port = 12;
    OnOffHelper onOffSsc1("ns3::UdpSocketFactory", InetSocketAddress(apNodeInterface.GetAddress(0), ssc1Port));
    onOffSsc1.SetAttribute("DataRate", StringValue("2Mbps"));
    onOffSsc1.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSsc1.SetAttribute("StartTime", TimeValue(Seconds(5)));
    onOffSsc1.SetAttribute("StopTime", TimeValue(Seconds(605)));
    ApplicationContainer ssc1ClientApp = onOffSsc1.Install(wifiStaNodes.Get(3));

    PacketSinkHelper sinkSsc1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ssc1Port));
    ApplicationContainer ssc1SinkApp = sinkSsc1.Install(wifiApNode.Get(0));
    ssc1SinkApp.Start(Seconds(0.0));
    ssc1SinkApp.Stop(Seconds(605));

    // SRT-001
    uint16_t srt1Port = 13;
    OnOffHelper onOffSrt1("ns3::TcpSocketFactory", InetSocketAddress(apNodeInterface.GetAddress(0), srt1Port));
    onOffSrt1.SetAttribute("DataRate", StringValue("500Kbps"));
    onOffSrt1.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSrt1.SetAttribute("StartTime", TimeValue(Seconds(30)));
    onOffSrt1.SetAttribute("StopTime", TimeValue(Seconds(930)));
    ApplicationContainer srt1ClientApp = onOffSrt1.Install(wifiStaNodes.Get(4));

    PacketSinkHelper sinkSrt1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), srt1Port));
    ApplicationContainer srt1SinkApp = sinkSrt1.Install(wifiApNode.Get(0));
    srt1SinkApp.Start(Seconds(0.0));
    srt1SinkApp.Stop(Seconds(930));

    // SMS-001
    uint16_t sms1Port = 14;
    OnOffHelper onOffSms1("ns3::UdpSocketFactory", InetSocketAddress(apNodeInterface.GetAddress(0), sms1Port));
    onOffSms1.SetAttribute("DataRate", StringValue("1.5Mbps"));
    onOffSms1.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSms1.SetAttribute("StartTime", TimeValue(Seconds(45)));
    onOffSms1.SetAttribute("StopTime", TimeValue(Seconds(345)));
    ApplicationContainer sms1ClientApp = onOffSms1.Install(wifiStaNodes.Get(5));

    PacketSinkHelper sinkSms1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sms1Port));
    ApplicationContainer sms1SinkApp = sinkSms1.Install(wifiApNode.Get(0));
    sms1SinkApp.Start(Seconds(0.0));
    sms1SinkApp.Stop(Seconds(345));

    // SVC-001
    uint16_t svc1Port = 15;
    OnOffHelper onOffSvc1("ns3::TcpSocketFactory", InetSocketAddress(apNodeInterface.GetAddress(0), svc1Port));
    onOffSvc1.SetAttribute("DataRate", StringValue("500Mbps"));
    onOffSvc1.SetAttribute("PacketSize", UintegerValue(1024));
    onOffSvc1.SetAttribute("StartTime", TimeValue(Seconds(15)));
    onOffSvc1.SetAttribute("StopTime", TimeValue(Seconds(1215)));
    ApplicationContainer svc1ClientApp = onOffSvc1.Install(wifiStaNodes.Get(6));

    PacketSinkHelper sinkSvc1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), svc1Port));
    ApplicationContainer svc1SinkApp = sinkSvc1.Install(wifiApNode.Get(0));
    svc1SinkApp.Start(Seconds(0.0));
    svc1SinkApp.Stop(Seconds(1215));

    // Tracing
    phy.EnablePcap("wifi-ap", apDevice.Get(0));
    phy.EnablePcap("wifi-st1", staDevices.Get(0));
    phy.EnablePcap("wifi-slb1", staDevices.Get(1));
    phy.EnablePcap("wifi-sdl1", staDevices.Get(2));
    phy.EnablePcap("wifi-ssc1", staDevices.Get(3));
    phy.EnablePcap("wifi-srt1", staDevices.Get(4));
    phy.EnablePcap("wifi-sms1", staDevices.Get(5));
    phy.EnablePcap("wifi-svc1", staDevices.Get(6));

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
