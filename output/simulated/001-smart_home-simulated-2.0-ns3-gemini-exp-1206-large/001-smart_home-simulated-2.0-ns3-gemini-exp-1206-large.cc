
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
#include "ns3/config.h"
#include "ns3/ssid.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/error-rate-model.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/wifi-radio-energy-model-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartHome");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);

    cmd.Parse(argc, argv);

    // Set up logging
    LogComponentEnable("SmartHome", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/
    NS_LOG_DEBUG("Setup...");

    // Network settings
    double simulationTime = 3600; //!< Scenario duration (s) in simulated time

    /************************
     *  Create WiFi Nodes  *
     ************************/
    NS_LOG_DEBUG("Create WiFi Nodes...");

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (4);

    NodeContainer wifiApNode;
    wifiApNode.Create (1);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy;
    phy.SetChannel (channel.Create ());

    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211n_5GHZ);
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid ("SmartHomeAPMyExperiment");

    mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
                "ActiveProbing", BooleanValue (false));

    NetDeviceContainer staDevices;
    staDevices = wifi.Install (phy, mac, wifiStaNodes);

    mac.SetType ("ns3::ApWifiMac",
                "Ssid", SsidValue (ssid));

    NetDeviceContainer apDevices;
    apDevices = wifi.Install (phy, mac, wifiApNode);

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 0.0));
    mobility.SetPositionAllocator (positionAlloc);
    mobility.Install (wifiApNode);

    Ptr<ListPositionAllocator> positionAllocSta = CreateObject<ListPositionAllocator> ();
    positionAllocSta->Add (Vector (10.0, 0.0, 0.0));
    positionAllocSta->Add (Vector (5.0, 0.0, 0.0));
    positionAllocSta->Add (Vector (1.0, 0.0, 0.0));
    positionAllocSta->Add (Vector (15.0, 0.0, 0.0));

    mobility.SetPositionAllocator (positionAllocSta);
    mobility.Install (wifiStaNodes);

    InternetStackHelper stack;
    stack.Install (wifiApNode);
    stack.Install (wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase ("192.168.1.0", "255.255.255.0");

    Ipv4InterfaceContainer staNodeInterfaces;
    staNodeInterfaces = address.Assign (staDevices);
    Ipv4InterfaceContainer apNodeInterface;
    apNodeInterface = address.Assign (apDevices);

    // Set static MAC addresses
    Ptr<NetDevice> dev = wifiStaNodes.Get(0)->GetDevice(0);
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
    Ptr<WifiMac> wifi_mac = wifi_dev->GetMac();
    Mac48Address staMacAddress0 = Mac48Address("61:5F:64:5E:90:EB");
    Config::Set("/NodeList/0/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/MacAddress", Mac48AddressValue(staMacAddress0));

    dev = wifiStaNodes.Get(1)->GetDevice(0);
    wifi_dev = DynamicCast<WifiNetDevice>(dev);
    wifi_mac = wifi_dev->GetMac();
    Mac48Address staMacAddress1 = Mac48Address("60:36:1E:9A:0A:0C");
    Config::Set("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/MacAddress", Mac48AddressValue(staMacAddress1));

    dev = wifiStaNodes.Get(2)->GetDevice(0);
    wifi_dev = DynamicCast<WifiNetDevice>(dev);
    wifi_mac = wifi_dev->GetMac();
    Mac48Address staMacAddress2 = Mac48Address("39:9F:51:CD:F7:08");
    Config::Set("/NodeList/2/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/MacAddress", Mac48AddressValue(staMacAddress2));

    dev = wifiStaNodes.Get(3)->GetDevice(0);
    wifi_dev = DynamicCast<WifiNetDevice>(dev);
    wifi_mac = wifi_dev->GetMac();
    Mac48Address staMacAddress3 = Mac48Address("0B:E3:41:0A:33:B7");
    Config::Set("/NodeList/3/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::StaWifiMac/MacAddress", Mac48AddressValue(staMacAddress3));

    // Set WPA2 security
    StringValue pskValue = StringValue("12345678");
    StringValue wpaValue = StringValue("WPA-PSK");
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMac/$ns3::RegularWifiMac/BE_QosSupported", BooleanValue (true));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMac/$ns3::RegularWifiMac/SSID", SsidValue (ssid));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMac/$ns3::RegularWifiMac/BeaconGeneration", BooleanValue (true));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMac/$ns3::RegularWifiMac/EnableBeaconJitter", BooleanValue (false));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMac/$ns3::RegularWifiMac/Wpa", wpaValue);
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMac/$ns3::RegularWifiMac/WpaKeyMgmt", wpaValue);
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMac/$ns3::RegularWifiMac/Psk", pskValue);
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMac/$ns3::RegularWifiMac/GroupCipher", StringValue ("TKIP"));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/ApMac/$ns3::RegularWifiMac/PairwiseCipher", StringValue ("TKIP"));

    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMac/$ns3::RegularWifiMac/BE_QosSupported", BooleanValue (true));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMac/$ns3::RegularWifiMac/SSID", SsidValue (ssid));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMac/$ns3::RegularWifiMac/Wpa", wpaValue);
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMac/$ns3::RegularWifiMac/WpaKeyMgmt", wpaValue);
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMac/$ns3::RegularWifiMac/Psk", pskValue);
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMac/$ns3::RegularWifiMac/GroupCipher", StringValue ("TKIP"));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/StaMac/$ns3::RegularWifiMac/PairwiseCipher", StringValue ("TKIP"));

    // Set fixed IP for AP
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Address", Mac48AddressValue (Mac48Address::Allocate ()));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/LinkLocalAddressing", BooleanValue (false));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/ChannelNumber", UintegerValue (1));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/EnableAsciiInternalTracing", BooleanValue (false));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/EnablePcapInternalTracing", BooleanValue (false));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/Frequency", UintegerValue (5180));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/MaxSupportedTxSpatialStreams", UintegerValue (1));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/MaxSupportedRxSpatialStreams", UintegerValue (1));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/TxPowerStart", DoubleValue (16.0206));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/TxPowerEnd", DoubleValue (16.0206));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/TxPowerLevels", UintegerValue (1));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/TxGain", DoubleValue (0));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/RxGain", DoubleValue (0));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/EnergyDetectionThreshold", DoubleValue (-61.8));
    Config::Set ("/NodeList/4/DeviceList/0/$ns3::WifiNetDevice/Phy/CcaMode1Threshold", DoubleValue (-64.8));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/Address", Ipv4InterfaceAddressValue (Ipv4InterfaceAddress (Ipv4Address ("192.168.1.1"), Ipv4Mask ("255.255.255.0"))));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/EnableDhcp", BooleanValue (false));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/Forwarding", BooleanValue (true));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/Mtu", UintegerValue (1500));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/Name", StringValue ("wlan0"));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/NetDeviceName", StringValue ("wlan0"));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/UseMetric", BooleanValue (false));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/Metric", UintegerValue (0));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/Mtu", UintegerValue (1500));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/Name", StringValue ("eth0"));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/NetDeviceName", StringValue ("eth0"));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/UseMetric", BooleanValue (false));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/Metric", UintegerValue (0));
    Config::Set ("/NodeList/4/$ns3::Ipv4L3Protocol/InterfaceList/0/Address", Ipv4InterfaceAddressValue (Ipv4InterfaceAddress (Ipv4Address ("10.8.8.16"), Ipv4Mask ("255.255.255.0"))));

    Config::Set ("/NodeList/0/$ns3::Ipv4L3Protocol/InterfaceList/0/Address", Ipv4InterfaceAddressValue (Ipv4InterfaceAddress (Ipv4Address ("192.168.1.103"), Ipv4Mask ("255.255.255.0"))));
    Config::Set ("/NodeList/0/$ns3::Ipv4L3Protocol/InterfaceList/0/EnableDhcp", BooleanValue (false));
    Config::Set ("/NodeList/0/$ns3::Ipv4L3Protocol/InterfaceList/0/Forwarding", BooleanValue (false));
    Config::Set ("/NodeList/0/$ns3::Ipv4L3Protocol/InterfaceList/0/Mtu", UintegerValue (1500));
    Config::Set ("/NodeList/0/$ns3::Ipv4L3Protocol/InterfaceList/0/Name", StringValue ("wlan0"));
    Config::Set ("/NodeList/0/$ns3::Ipv4L3Protocol/InterfaceList/0/NetDeviceName", StringValue ("wlan0"));
    Config::Set ("/NodeList/0/$ns3::Ipv4L3Protocol/InterfaceList/0/UseMetric", BooleanValue (false));
    Config::Set ("/NodeList/0/$ns3::Ipv4L3Protocol/InterfaceList/0/Metric", UintegerValue (0));

    Config::Set ("/NodeList/1/$ns3::Ipv4L3Protocol/InterfaceList/0/Address", Ipv4InterfaceAddressValue (Ipv4InterfaceAddress (Ipv4Address ("192.168.1.102"), Ipv4Mask ("255.255.255.0"))));
    Config::Set ("/NodeList/1/$ns3::Ipv4L3Protocol/InterfaceList/0/EnableDhcp", BooleanValue (false));
    Config::Set ("/NodeList/1/$ns3::Ipv4L3Protocol/InterfaceList/0/Forwarding", BooleanValue (false));
    Config::Set ("/NodeList/1/$ns3::Ipv4L3Protocol/InterfaceList/0/Mtu", UintegerValue (1500));
    Config::Set ("/NodeList/1/$ns3::Ipv4L3Protocol/InterfaceList/0/Name", StringValue ("wlan0"));
    Config::Set ("/NodeList/1/$ns3::Ipv4L3Protocol/InterfaceList/0/NetDeviceName", StringValue ("wlan0"));
    Config::Set ("/NodeList/1/$ns3::Ipv4L3Protocol/InterfaceList/0/UseMetric", BooleanValue (false));
    Config::Set ("/NodeList/1/$ns3::Ipv4L3Protocol/InterfaceList/0/Metric", UintegerValue (0));

    Config::Set ("/NodeList/2/$ns3::Ipv4L3Protocol/InterfaceList/0/Address", Ipv4InterfaceAddressValue (Ipv4InterfaceAddress (Ipv4Address ("192.168.1.101"), Ipv4Mask ("255.255.255.0"))));
    Config::Set ("/NodeList/2/$ns3::Ipv4L3Protocol/InterfaceList/0/EnableDhcp", BooleanValue (false));
    Config::Set ("/NodeList/2/$ns3::Ipv4L3Protocol/InterfaceList/0/Forwarding", BooleanValue (false));
    Config::Set ("/NodeList/2/$ns3::Ipv4L3Protocol/InterfaceList/0/Mtu", UintegerValue (1500));
    Config::Set ("/NodeList/2/$ns3::Ipv4L3Protocol/InterfaceList/0/Name", StringValue ("wlan0"));
    Config::Set ("/NodeList/2/$ns3::Ipv4L3Protocol/InterfaceList/0/NetDeviceName", StringValue ("wlan0"));
    Config::Set ("/NodeList/2/$ns3::Ipv4L3Protocol/InterfaceList/0/UseMetric", BooleanValue (false));
    Config::Set ("/NodeList/2/$ns3::Ipv4L3Protocol/InterfaceList/0/Metric", UintegerValue (0));

    Config::Set ("/NodeList/3/$ns3::Ipv4L3Protocol/InterfaceList/0/Address", Ipv4InterfaceAddressValue (Ipv4InterfaceAddress (Ipv4Address ("192.168.1.100"), Ipv4Mask ("255.255.255.0"))));
    Config::Set ("/NodeList/3/$ns3::Ipv4L3Protocol/InterfaceList/0/EnableDhcp", BooleanValue (false));
    Config::Set ("/NodeList/3/$ns3::Ipv4L3Protocol/InterfaceList/0/Forwarding", BooleanValue (false));
    Config::Set ("/NodeList/3/$ns3::Ipv4L3Protocol/InterfaceList/0/Mtu", UintegerValue (1500));
    Config::Set ("/NodeList/3/$ns3::Ipv4L3Protocol/InterfaceList/0/Name", StringValue ("wlan0"));
    Config::Set ("/NodeList/3/$ns3::Ipv4L3Protocol/InterfaceList/0/NetDeviceName", StringValue ("wlan0"));
    Config::Set ("/NodeList/3/$ns3::Ipv4L3Protocol/InterfaceList/0/UseMetric", BooleanValue (false));
    Config::Set ("/NodeList/3/$ns3::Ipv4L3Protocol/InterfaceList/0/Metric", UintegerValue (0));

    // Print the IP addresses of the WiFi STA nodes
    for (uint32_t i = 0; i < staNodeInterfaces.GetN(); ++i)
    {
        Ipv4Address addr = staNodeInterfaces.GetAddress (i);
        std::cout << "IP address of wifiStaNode " << i << ": " << addr << std::endl;
    }

    NS_LOG_DEBUG("WiFi - Setup Applications");

    //ST-001
    uint16_t sinkPort0 = 8080;
    Address sinkAddress0 (InetSocketAddress (staNodeInterfaces.GetAddress (0), sinkPort0));
    PacketSinkHelper packetSinkHelper0 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort0));
    ApplicationContainer sinkApps0 = packetSinkHelper0.Install (wifiStaNodes.Get (0));
    sinkApps0.Start (Seconds (0.0));
    sinkApps0.Stop (Seconds (simulationTime));

    OnOffHelper onoff0 ("ns3::UdpSocketFactory", sinkAddress0);
    onoff0.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1800]"));
    onoff0.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff0.SetAttribute ("DataRate", StringValue ("100Mbps"));
    onoff0.SetAttribute ("PacketSize", UintegerValue (1500));
    ApplicationContainer clientApps0 = onoff0.Install (wifiApNode.Get (0));
    clientApps0.Start (Seconds (0));
    clientApps0.Stop (Seconds (1800));

    //SLB-001
    Address sinkAddress1 (InetSocketAddress (staNodeInterfaces.GetAddress (1), 9));
    PacketSinkHelper packetSinkHelper1 ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9));
    ApplicationContainer sinkApps1 = packetSinkHelper1.Install (wifiStaNodes.Get (1));
    sinkApps1.Start (Seconds (0.0));
    sinkApps1.Stop (Seconds (simulationTime));

    OnOffHelper onoff1 ("ns3::TcpSocketFactory", sinkAddress1);
    onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1800]"));
    onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff1.SetAttribute ("DataRate", StringValue ("100Mbps"));
    onoff1.SetAttribute ("PacketSize", UintegerValue (1500));
    ApplicationContainer clientApps1 = onoff1.Install (wifiApNode.Get (0));
    clientApps1.Start (Seconds (0));
    clientApps1.Stop (Seconds (1800));

    //SDL-001
    uint16_t sinkPort2 = 8082;
    Address sinkAddress2 (InetSocketAddress (staNodeInterfaces.GetAddress (2), sinkPort2));
    PacketSinkHelper packetSinkHelper2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort2));
    ApplicationContainer sinkApps2 = packetSinkHelper2.Install (wifiStaNodes.Get (2));
    sinkApps2.Start (Seconds (0.0));
    sinkApps2.Stop (Seconds (simulationTime));

    OnOffHelper onoff2 ("ns3::UdpSocketFactory", sinkAddress2);
    onoff2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=600]"));
    onoff2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff2.SetConstantRate (DataRate ("1Mbps"));
    onoff2.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps2 = onoff2.Install (wifiApNode.Get (0));
    clientApps2.Start (Seconds (10));
    clientApps2.Stop (Seconds (610));

    //SSC-001
    uint16_t sinkPort3 = 8083;
    Address sinkAddress3 (InetSocketAddress (staNodeInterfaces.GetAddress (3), sinkPort3));
    PacketSinkHelper packetSinkHelper3 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort3));
    ApplicationContainer sinkApps3 = packetSinkHelper3.Install (wifiStaNodes.Get (3));
    sinkApps3.Start (Seconds (0.0));
    sinkApps3.Stop (Seconds (simulationTime));

    OnOffHelper onoff3 ("ns3::UdpSocketFactory", sinkAddress3);
    onoff3.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=600]"));
    onoff3.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onoff3.SetConstantRate (DataRate ("2Mbps"));
    onoff3.SetAttribute ("PacketSize", UintegerValue (1024));
    ApplicationContainer clientApps3 = onoff3.Install (wifiApNode.Get (0));
    clientApps3.Start (Seconds (5));
    clientApps3.Stop (Seconds (605));

    //Tracing
    phy.EnablePcap ("wifi-ap", apDevices);
    phy.EnablePcap ("wifi-sta", staDevices);

    /**********
     *  Run    *
     **********/

    Simulator::Stop(Seconds(simulationTime));

    NS_LOG_INFO("*** Running simulation...");
    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
