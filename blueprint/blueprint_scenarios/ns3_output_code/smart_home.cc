
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/ssid.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartHomeWiFiNetworkExample");

int main(int argc, char *argv[])
{
    // Set up logging
    LogComponentEnable("SmartHomeWiFiNetworkExample", LOG_LEVEL_ALL);

    /***********
     *  Setup  *
     ***********/

    // Create nodes
    NodeContainer wifiNodes;
    wifiNodes.Create(5); // 1 AP + 4 devices

    // Create Wi-Fi channel and physical layer
    YansWifiChannelHelper wifichannel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(wifichannel.Create());

    // Set Wi-Fi standard to 802.11a
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);

    // Set remote station manager
    WifiMacHelper mac;
    Ssid ssid = Ssid("SmartHomeWiFi");
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    // Install Wi-Fi devices
    NetDeviceContainer staDevices;
    staDevices = wifi.Install(phy, mac, wifiNodes.Get(1));
    staDevices.Add(wifi.Install(phy, mac, wifiNodes.Get(2)));
    staDevices.Add(wifi.Install(phy, mac, wifiNodes.Get(3)));
    staDevices.Add(wifi.Install(phy, mac, wifiNodes.Get(4)));

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice;
    apDevice = wifi.Install(phy, mac, wifiNodes.Get(0));

    // Set up mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> allocatorWiFi = CreateObject<ListPositionAllocator>();
    allocatorWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5)); // AP position
    allocatorWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5)); // Smart Hub position
    allocatorWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5)); // Smart Thermostat position
    allocatorWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5)); // Smart Light Bulb position
    allocatorWiFi->Add(Vector(38.10351066811096, 13.3459399220741, 1.5)); // Smart Door Lock position
    mobility.SetPositionAllocator(allocatorWiFi);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiNodes);

    // Install Internet stack
    InternetStackHelper stack;
    stack.Install(wifiNodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces;
    staInterfaces = address.Assign(staDevices);
    Ipv4InterfaceContainer apInterface;
    apInterface = address.Assign(apDevice);

    // Create applications
    uint16_t port = 9;
    OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address("192.168.1.101"), port));
    onoff.SetConstantRate(DataRate("1Mbps"));
    ApplicationContainer apps = onoff.Install(wifiNodes.Get(1));
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(10.0));

    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    apps = sink.Install(wifiNodes.Get(0));
    apps.Start(Seconds(1.0));
    apps.Stop(Seconds(10.0));

    // Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
