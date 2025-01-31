
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartHome");

int main(int argc, char *argv[])
{
  // Set up logging
  LogComponentEnable("SmartHome", LOG_LEVEL_INFO);

  // Constants
  double simulationTime = 3600; // 1 hour

  // Create WiFi Access Point
  NodeContainer apNodes;
  apNodes.Create(1);

  // Create WiFi Stations
  NodeContainer staNodes;
  staNodes.Create(4);

  // WiFi Channel
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phy;
  phy.SetChannel(channel.Create());

  // WiFi Helper
  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211a);
  wifi.SetRemoteStationManager("ns3::AarfWifiManager");

  // MAC Layer - Access Point
  WifiMacHelper macAp;
  Ssid ssid = Ssid("SmartHomeAPMyExperiment");
  macAp.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid),
                "BeaconInterval", TimeValue(MicroSeconds(102400)));
  
  NetDeviceContainer apDevice;
  apDevice = wifi.Install(phy, macAp, apNodes.Get(0));

  // MAC Layer - Stations
  WifiMacHelper macSta;
  macSta.SetType("ns3::StaWifiMac",
                 "Ssid", SsidValue(ssid),
                 "ActiveProbing", BooleanValue(false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install(phy, macSta, staNodes);

  // Mobility
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(apNodes);
  mobility.Install(staNodes);

  // Set positions
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // AP-001
  positionAlloc->Add(Vector(10.0, 0.0, 0.0)); // ST-001
  positionAlloc->Add(Vector(5.0, 0.0, 0.0));  // SLB-001
  positionAlloc->Add(Vector(1.0, 0.0, 0.0));  // SDL-001
  positionAlloc->Add(Vector(15.0, 0.0, 0.0)); // SSC-001
  mobility.SetPositionAllocator(positionAlloc);

  // Internet Stack
  InternetStackHelper stack;
  stack.Install(apNodes);
  stack.Install(staNodes);

  // IP Addresses
  Ipv4AddressHelper address;
  address.SetBase("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign(apDevice);
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign(staDevices);

  // Set MAC Addresses
  Ptr<Node> st001Node = staNodes.Get(0);
  Ptr<NetDevice> st001Device = staDevices.Get(0);
  st001Device->SetAddress(Mac48Address("61:5F:64:5E:90:EB"));

  Ptr<Node> slb001Node = staNodes.Get(1);
  Ptr<NetDevice> slb001Device = staDevices.Get(1);
  slb001Device->SetAddress(Mac48Address("60:36:1E:9A:0A:0C"));

  Ptr<Node> sdl001Node = staNodes.Get(2);
  Ptr<NetDevice> sdl001Device = staDevices.Get(2);
  sdl001Device->SetAddress(Mac48Address("39:9F:51:CD:F7:08"));
  
  Ptr<Node> ssc001Node = staNodes.Get(3);
  Ptr<NetDevice> ssc001Device = staDevices.Get(3);
  ssc001Device->SetAddress(Mac48Address("0B:E3:41:0A:33:B7"));
  // UDP Application - ST-001
  uint16_t st001Port = 9; 
  OnOffHelper st001OnOff("ns3::UdpSocketFactory",
                        InetSocketAddress(apInterface.GetAddress(0), st001Port));
  st001OnOff.SetConstantRate(DataRate("100Mb/s"));
  ApplicationContainer st001App = st001OnOff.Install(st001Node);
  st001App.Start(Seconds(0.0));
  st001App.Stop(Seconds(1800.0));

  // TCP Application - SLB-001
  uint16_t slb001Port = 10;
  BulkSendHelper slb001Source("ns3::TcpSocketFactory",
                             InetSocketAddress(apInterface.GetAddress(0), slb001Port));
  slb001Source.SetAttribute("MaxBytes", UintegerValue(0));
  ApplicationContainer slb001App = slb001Source.Install(slb001Node);
  slb001App.Start(Seconds(0.0));
  slb001App.Stop(Seconds(1800.0));

  PacketSinkHelper slb001Sink("ns3::TcpSocketFactory",
                              InetSocketAddress(Ipv4Address::GetAny(), slb001Port));
  ApplicationContainer slb001SinkApp = slb001Sink.Install(apNodes.Get(0));
  slb001SinkApp.Start(Seconds(0.0));
  slb001SinkApp.Stop(Seconds(1800.0));

  // UDP Application - SDL-001
  uint16_t sdl001Port = 11;
  OnOffHelper sdl001OnOff("ns3::UdpSocketFactory",
                        InetSocketAddress(apInterface.GetAddress(0), sdl001Port));
  sdl001OnOff.SetConstantRate(DataRate("1Mb/s"));
  ApplicationContainer sdl001App = sdl001OnOff.Install(sdl001Node);
  sdl001App.Start(Seconds(10.0));
  sdl001App.Stop(Seconds(610.0));

  // UDP Application - SSC-001
  uint16_t ssc001Port = 12;
  OnOffHelper ssc001OnOff("ns3::UdpSocketFactory",
                        InetSocketAddress(apInterface.GetAddress(0), ssc001Port));
  ssc001OnOff.SetConstantRate(DataRate("2Mb/s"));
  ApplicationContainer ssc001App = ssc001OnOff.Install(ssc001Node);
  ssc001App.Start(Seconds(5.0));
  ssc001App.Stop(Seconds(605.0));

  // Packet Sinks for UDP Applications
  PacketSinkHelper sinkUdp("ns3::UdpSocketFactory",
                           InetSocketAddress(Ipv4Address::GetAny(), st001Port));
  ApplicationContainer sinkUdpApp = sinkUdp.Install(apNodes.Get(0));
  sinkUdpApp.Start(Seconds(0.0));
  sinkUdpApp.Stop(Seconds(1800));

  sinkUdp.SetAttribute("Local", AddressValue(InetSocketAddress(Ipv4Address::GetAny(), sdl001Port)));
  ApplicationContainer sinkUdpAppSdl = sinkUdp.Install(apNodes.Get(0));
  sinkUdpAppSdl.Start(Seconds(0.0));
  sinkUdpAppSdl.Stop(Seconds(610));

  sinkUdp.SetAttribute("Local", AddressValue(InetSocketAddress(Ipv4Address::GetAny(), ssc001Port)));
  ApplicationContainer sinkUdpAppSsc = sinkUdp.Install(apNodes.Get(0));
  sinkUdpAppSsc.Start(Seconds(0.0));
  sinkUdpAppSsc.Stop(Seconds(605));

  // Enable Tracing
  phy.EnablePcap("smart-home-ap", apDevice.Get(0));
  phy.EnablePcap("smart-home-st-001", staDevices.Get(0));
  phy.EnablePcap("smart-home-slb-001", staDevices.Get(1));
  phy.EnablePcap("smart-home-sdl-001", staDevices.Get(2));
  phy.EnablePcap("smart-home-ssc-001", staDevices.Get(3));

  // Run Simulation
  Simulator::Stop(Seconds(simulationTime));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}
