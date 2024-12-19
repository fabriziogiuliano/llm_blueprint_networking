
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
#include "ns3/packet-sink-helper.h"
#include "ns3/ssid.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/packet.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartHome");

int main(int argc, char* argv[])
{
  CommandLine cmd(__FILE__);
  cmd.Parse(argc, argv);

  LogComponentEnable("SmartHome", LOG_LEVEL_ALL);

  Time simulationTime = Hours(1);
  int nWifiApNodes = 1;
  int nWifiStaNodes = 4;

  NodeContainer wifiApNodes;
  wifiApNodes.Create(nWifiApNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create(nWifiStaNodes);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy;
  phy.SetChannel(channel.Create());

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211a);
  wifi.SetRemoteStationManager("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid("SmartHomeAPMyExperiment");

  NetDeviceContainer apDevices;
  NetDeviceContainer staDevices;

  mac.SetType("ns3::ApWifiMac",
              "Ssid", SsidValue(ssid),
              "QosSupported", BooleanValue(true));
  apDevices = wifi.Install(phy, mac, wifiApNodes);

  mac.SetType("ns3::StaWifiMac",
              "Ssid", SsidValue(ssid),
              "QosSupported", BooleanValue(true),
              "ActiveProbing", BooleanValue(false));
  staDevices = wifi.Install(phy, mac, wifiStaNodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0)); 
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiApNodes);

  Ptr<ListPositionAllocator> positionAllocSta = CreateObject<ListPositionAllocator>();
  positionAllocSta->Add(Vector(10.0, 0.0, 0.0)); 
  positionAllocSta->Add(Vector(5.0, 0.0, 0.0));  
  positionAllocSta->Add(Vector(1.0, 0.0, 0.0));  
  positionAllocSta->Add(Vector(15.0, 0.0, 0.0)); 
  mobility.SetPositionAllocator(positionAllocSta);
  mobility.Install(wifiStaNodes);

  InternetStackHelper stack;
  stack.Install(wifiApNodes);
  stack.Install(wifiStaNodes);

  Ipv4AddressHelper address;
  address.SetBase("192.168.1.0", "255.255.255.0");

  Ipv4InterfaceContainer apInterfaces;
  apInterfaces = address.Assign(apDevices);
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign(staDevices);
  

  for (uint32_t i = 0; i < staDevices.GetN(); i++)
  {
    Ptr<NetDevice> dev = staDevices.Get(i);
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
    Ptr<WifiMac> wifi_mac = wifi_dev->GetMac();
    if (i==0)
    {
      wifi_mac->SetAddress(Mac48Address("61:5F:64:5E:90:EB"));
    }
    else if (i==1)
    {
      wifi_mac->SetAddress(Mac48Address("60:36:1E:9A:0A:0C"));
    }
    else if (i==2)
    {
      wifi_mac->SetAddress(Mac48Address("39:9F:51:CD:F7:08"));
    }
    else if (i==3)
    {
      wifi_mac->SetAddress(Mac48Address("0B:E3:41:0A:33:B7"));
    }
  }

  Packet::EnablePrinting();

  uint16_t port1 = 9;
  OnOffHelper onOffUDP1("ns3::UdpSocketFactory", InetSocketAddress(apInterfaces.GetAddress(0), port1));
  onOffUDP1.SetAttribute("DataRate", StringValue("3Mbps"));
  onOffUDP1.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer udpClientApp1 = onOffUDP1.Install(wifiStaNodes.Get(0));
  udpClientApp1.Start(Seconds(0.0));
  udpClientApp1.Stop(Minutes(30.0));
  
  PacketSinkHelper sinkHelperUDP1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port1));
  ApplicationContainer udpServerApp1 = sinkHelperUDP1.Install(wifiApNodes.Get(0));
  udpServerApp1.Start(Seconds(0.0));
  udpServerApp1.Stop(Minutes(30.0));

  uint16_t port2 = 10;
  OnOffHelper onOffTCP1("ns3::TcpSocketFactory", InetSocketAddress(apInterfaces.GetAddress(0), port2));
  onOffTCP1.SetAttribute("DataRate", StringValue("3Mbps"));
  onOffTCP1.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer tcpClientApp1 = onOffTCP1.Install(wifiStaNodes.Get(1));
  tcpClientApp1.Start(Seconds(0.0));
  tcpClientApp1.Stop(Minutes(30.0));
  
  PacketSinkHelper sinkHelperTCP1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port2));
  ApplicationContainer tcpServerApp1 = sinkHelperTCP1.Install(wifiApNodes.Get(0));
  tcpServerApp1.Start(Seconds(0.0));
  tcpServerApp1.Stop(Minutes(30.0));

  uint16_t port3 = 11;
  OnOffHelper onOffUDP2("ns3::UdpSocketFactory", InetSocketAddress(apInterfaces.GetAddress(0), port3));
  onOffUDP2.SetAttribute("DataRate", StringValue("1Mbps"));
  onOffUDP2.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer udpClientApp2 = onOffUDP2.Install(wifiStaNodes.Get(2));
  udpClientApp2.Start(Seconds(10.0));
  udpClientApp2.Stop(Minutes(10.0)+Seconds(10.0));
  
  PacketSinkHelper sinkHelperUDP2("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port3));
  ApplicationContainer udpServerApp2 = sinkHelperUDP2.Install(wifiApNodes.Get(0));
  udpServerApp2.Start(Seconds(0.0));
  udpServerApp2.Stop(Minutes(10.0)+Seconds(10.0));

  uint16_t port4 = 12;
  OnOffHelper onOffUDP3("ns3::UdpSocketFactory", InetSocketAddress(apInterfaces.GetAddress(0), port4));
  onOffUDP3.SetAttribute("DataRate", StringValue("2Mbps"));
  onOffUDP3.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer udpClientApp3 = onOffUDP3.Install(wifiStaNodes.Get(3));
  udpClientApp3.Start(Seconds(5.0));
  udpClientApp3.Stop(Minutes(10.0)+Seconds(5.0));
  
  PacketSinkHelper sinkHelperUDP3("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port4));
  ApplicationContainer udpServerApp3 = sinkHelperUDP3.Install(wifiApNodes.Get(0));
  udpServerApp3.Start(Seconds(0.0));
  udpServerApp3.Stop(Minutes(10.0)+Seconds(5.0));

  phy.EnablePcap("wifi-ap", apDevices, false);
  phy.EnablePcap("wifi-sta", staDevices, false);

  Simulator::Stop(simulationTime);

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
