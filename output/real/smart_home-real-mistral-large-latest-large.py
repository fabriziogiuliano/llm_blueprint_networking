
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
from dotenv import load_dotenv
from fabric import Connection

load_dotenv()
server = os.getenv('CHIRPSTACK_URL')  # Adjust this to your ChirpStack instance
api_token = os.getenv('CHIRPSTACK_API_TOKEN')  # Obtain from ChirpStack
tenant_id = os.getenv('CHIRPSTACK_TENANT_ID') #this is a tenant ID for fabrizio.giuliano@unipa.it

channel = grpc.insecure_channel(server)
auth_token = [("authorization", "Bearer %s" % api_token)]

def ListGateways(tenant_id):
  client = api.GatewayServiceStub(channel)
  req = api.ListGatewaysRequest()
  req.tenant_id = tenant_id
  req.limit = 1000
  resp = client.List(req, metadata=auth_token)
  return resp

def ListApplications(tenant_id):
  client = api.ApplicationServiceStub(channel)
  req = api.ListApplicationsRequest()
  req.tenant_id = tenant_id
  req.limit = 1000
  resp = client.List(req, metadata=auth_token)
  return resp

def ListDevices(application_id):
  client = api.DeviceServiceStub(channel)
  req = api.ListDevicesRequest()
  req.application_id = application_id
  req.limit = 1000
  resp = client.List(req, metadata=auth_token)
  return resp

def CreateApplication(name, tenant_id):
  client = api.ApplicationServiceStub(channel)
  req = api.CreateApplicationRequest()
  req.application.name = name
  req.application.tenant_id = tenant_id
  resp = client.Create(req, metadata=auth_token)
  return resp

def CreateDeviceProfile(name, tenant_id, region="EU868", mac_version="LORAWAN_1_0_3", revision ="RP002_1_0_3", supports_otaa=True):
  client = api.DeviceProfileServiceStub(channel)
  req = api.CreateDeviceProfileRequest()
  req.device_profile.name=name
  req.device_profile.tenant_id = tenant_id
  req.device_profile.region = region
  req.device_profile.reg_params_revision = revision
  req.device_profile.mac_version = mac_version
  req.device_profile.supports_otaa = supports_otaa
  resp = client.Create(req, metadata=auth_token)
  return resp

def CreateDevice(
  dev_eui,
  name,
  application_id,
  device_profile_id,
  application_key,
  skip_fcnt_check=True,
  is_disabled=False
  ):
  client = api.DeviceServiceStub(channel)
  req = api.CreateDeviceRequest()
  req.device.dev_eui = dev_eui
  req.device.name = name
  req.device.description = ""
  req.device.application_id = application_id
  req.device.device_profile_id = device_profile_id
  req.device.skip_fcnt_check = skip_fcnt_check
  req.device.is_disabled = is_disabled
  resp = client.Create(req, metadata=auth_token)
  req = api.CreateDeviceKeysRequest()
  req.device_keys.dev_eui = dev_eui
  req.device_keys.nwk_key = application_key
  req.device_keys.app_key = application_key
  resp = client.CreateKeys(req, metadata=auth_token)
  return resp

def CreateGateway(gateway_id, name, tenant_id, description=None, location=None):
  client = api.GatewayServiceStub(channel)
  req = api.CreateGatewayRequest()
  req.gateway.gateway_id=gateway_id
  req.gateway.name = name
  req.gateway.description =description
  req.gateway.tenant_id=tenant_id
  req.gateway.stats_interval=60
  resp = client.Create(req, metadata=auth_token)
  return resp

def DeleteGateway(gateway_id):
  client = api.GatewayServiceStub(channel)
  req = api.DeleteGatewayRequest()
  req.gateway_id=gateway_id
  resp = client.Delete(req, metadata=auth_token)
  return resp

def get_dict(str_application_id):
    k, v = str_application_id.split(': ')
    v = v.replace('"','').strip("\n")
    return {k: v}

def GetDeviceProfile(id):
  client = api.DeviceProfileServiceStub(channel)
  req = api.GetDeviceProfileRequest()
  req.id = id
  resp = client.Get(req, metadata=auth_token)
  return resp

def DeleteApplication(id):
  try:
    client = api.ApplicationServiceStub(channel)
    req = api.DeleteApplicationRequest()
    req.id=id
    resp = client.Delete(req, metadata=auth_token)
    return resp
  except Exception:
    print(traceback.format_exc())

def ListDeviceProfiles(tenant_id):
  client = api.DeviceProfileServiceStub(channel)
  req = api.ListDeviceProfilesRequest()
  req.tenant_id = tenant_id
  req.limit = 1000
  resp = client.List(req, metadata=auth_token)
  return resp

def configure_wifi_ap(ap_config):
    hostapd_conf = f"""
    interface=wlan0
    driver=nl80211
    ssid={ap_config['ssid']}
    hw_mode=g
    channel=7
    wmm_enabled=0
    macaddr_acl=0
    auth_algs=1
    ignore_broadcast_ssid=0
    wpa=2
    wpa_passphrase={ap_config['wpa_passphrase']}
    wpa_key_mgmt={ap_config['wpa_key_mgmt']}
    rsn_pairwise=CCMP
    """

    dnsmasq_conf = f"""
    interface=wlan0
    dhcp-range=192.168.1.10,192.168.1.100,255.255.255.0,24h
    """

    for station in ap_config['wifi_stations']:
        dnsmasq_conf += f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n"

    with Connection('root@10.8.8.16') as c:
        c.put(io.StringIO(hostapd_conf), '/etc/hostapd/hostapd.conf')
        c.put(io.StringIO(dnsmasq_conf), '/etc/dnsmasq.conf')
        c.run('systemctl restart hostapd')
        c.run('systemctl restart dnsmasq')

def configure_wifi_stations(stations):
    for station in stations:
        with Connection(f"root@{station['eth_IP']}") as c:
            c.run(f"iwconfig wlan0 essid {station['ssid']} key {station['wpa_passphrase']}")
            c.run(f"ifconfig wlan0 {station['wlan_IP']} netmask 255.255.255.0")

# TEST BLUEPRINT
blueprint = {
  "name": "Smart Home",
  "description": "A network blueprint for a smart home with various automated devices.",
  "experiment_duration":"1 hour",
  "wifi_ap":
  [
    {
      "id":"AP-001",
      "ssid":"SmartHomeAPMyExperiment",
      "wpa_passphrase":12345678,
      "wpa_key_mgmt":"WPA-PSK",
      "wlan_IP":"192.168.1.1",
      "eth_IP":"10.8.8.16"
    }
  ],
  "wifi_stations":[
    {
      "id":"ST-001",
      "ssid":"SmartHomeAPMyExperiment",
      "activity": "UDP trasmission at maximum throughput of 30 minutes, start at 0",
      "wpa_passphrase":12345678,
      "wpa_key_mgmt":"WPA-PSK",
      "wlan_IP":"192.168.1.103",
      "wlan_MAC_ADDR":"61:5F:64:5E:90:EB",
      "distance":"10 meter far from AP",
      "eth_IP":"10.8.8.17",
      "user":"root",
      "password":123456
    },
    {
      "id":"SLB-001",
      "ssid":"SmartHomeAPMyExperiment",
      "activity": "TCP trasmission at maximum throughput of 30 minutes, start at 0",
      "wpa_passphrase":12345678,
      "wpa_key_mgmt":"WPA-PSK",
      "wlan_IP":"192.168.1.102",
      "distance":"5 meter far from AP",
      "wlan_MAC_ADDR":"60:36:1E:9A:0A:0C",
      "eth_IP":"10.8.8.18",
      "user":"root",
      "password":123456
    },
    {
      "id":"SDL-001",
      "ssid":"SmartHomeAPMyExperiment",
      "activity": "UDP trasmission at 1Mbps of 10 minutes, start at 10sec",
      "wpa_passphrase":12345678,
      "wpa_key_mgmt":"WPA-PSK",
      "wlan_IP":"192.168.1.101",
      "distance":"1 meter far from AP",
      "wlan_MAC_ADDR":"39:9F:51:CD:F7:08",
      "eth_IP":"10.8.8.19",
      "user":"root",
      "password":123456
    },
    {
      "id":"SSC-001",
      "ssid":"SmartHomeAPMyExperiment",
      "activity": "UDP trasmission at 2Mbps of 10 minutes, start at 5sec",
      "wpa_passphrase":12345678,
      "wpa_key_mgmt":"WPA-PSK",
      "wlan_IP":"192.168.1.100",
      "distance":"15 meter far from AP",
      "wlan_MAC_ADDR":"0B:E3:41:0A:33:B7",
      "eth_IP":"10.8.8.20",
      "user":"root",
      "password":123456
    }
  ]
}

# Configure WiFi AP
configure_wifi_ap(blueprint['wifi_ap'][0])

# Configure WiFi Stations
configure_wifi_stations(blueprint['wifi_stations'])
```