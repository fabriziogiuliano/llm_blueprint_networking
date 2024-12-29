```python
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
from fabric import Connection
from dotenv import load_dotenv

load_dotenv()
server = os.getenv('CHIRPSTACK_URL')
api_token = os.getenv('CHIRPSTACK_API_TOKEN')
tenant_id = os.getenv('CHIRPSTACK_TENANT_ID')

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

def GetApplicationID(name, tenant_id):
    client = api.ApplicationServiceStub(channel)
    req = api.GetApplicationRequest()
    req.name = name
    req.tenant_id = tenant_id
    resp = client.Get(req, metadata=auth_token)
    return resp

def CreateDeviceProfile(name, tenant_id, region="EU868", mac_version="LORAWAN_1_0_3", revision="RP002_1_0_3", supports_otaa=True):
    client = api.DeviceProfileServiceStub(channel)
    req = api.CreateDeviceProfileRequest()
    req.device_profile.name = name
    req.device_profile.tenant_id = tenant_id
    req.device_profile.region = region
    req.device_profile.reg_params_revision = revision
    req.device_profile.mac_version = mac_version
    req.device_profile.supports_otaa = supports_otaa
    resp = client.Create(req, metadata=auth_token)
    return resp

def DeleteDviceProfile(id):
    client = api.DeviceProfileServiceStub(channel)
    req = api.DeleteDeviceProfileRequest()
    req.id = id
    resp = client.Delete(req, metadata=auth_token)
    return resp

def CreateDevice(dev_eui, name, application_id, device_profile_id, application_key, skip_fcnt_check=True, is_disabled=False):
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

def DeleteDevice(device_id):
    client = api.DeviceServiceStub(channel)
    req = api.DeleteDeviceRequest()
    req.dev_eui = device_id
    resp = client.Delete(req, metadata=auth_token)
    return resp

def CreateGateway(gateway_id, name, tenant_id, description=None, location=None):
    client = api.GatewayServiceStub(channel)
    req = api.CreateGatewayRequest()
    req.gateway.gateway_id = gateway_id
    req.gateway.name = name
    req.gateway.description = description
    req.gateway.tenant_id = tenant_id
    req.gateway.stats_interval = 60
    resp = client.Create(req, metadata=auth_token)
    return resp

def DeleteGateway(gateway_id):
    client = api.GatewayServiceStub(channel)
    req = api.DeleteGatewayRequest()
    req.gateway_id = gateway_id
    resp = client.Delete(req, metadata=auth_token)
    return resp

def get_dict(str_application_id):
    k, v = str_application_id.split(': ')
    v = v.replace('"', '').strip("\n")
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
        req.id = id
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

blueprint = {
  "name": "Smart Home",
  "description": "A network blueprint for a smart home with various automated devices.",
  "experiment_duration":"1 hour",
  "wifi_ap":
  [
    {
      "id":"AP-001",
      "ssid":"SmartHomeAPMyExperiment",
      "wpa_passphrase":"12345678",
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
      "wpa_passphrase":"12345678",
      "wpa_key_mgmt":"WPA-PSK",
      "wlan_IP":"192.168.1.103",
      "wlan_MAC_ADDR":"61:5F:64:5E:90:EB",
      "distance":"10 meter far from AP",
      "eth_IP":"10.8.8.17",
      "user":"root",
      "password":"123456"
    },
    {
      "id":"SLB-001",
      "ssid":"SmartHomeAPMyExperiment",
      "activity": "TCP trasmission at maximum throughput of 30 minutes, start at 0",
      "wpa_passphrase":"12345678",
      "wpa_key_mgmt":"WPA-PSK",
      "wlan_IP":"192.168.1.102",
      "distance":"5 meter far from AP",
      "wlan_MAC_ADDR":"60:36:1E:9A:0A:0C",
      "eth_IP":"10.8.8.18",
      "user":"root",
      "password":"123456"
    },
    {
      "id":"SDL-001",
      "ssid":"SmartHomeAPMyExperiment",
      "activity": "UDP trasmission at 1Mbps of 10 minutes, start at 10sec",
      "wpa_passphrase":"12345678",
      "wpa_key_mgmt":"WPA-PSK",
      "wlan_IP":"192.168.1.101",
      "distance":"1 meter far from AP",
      "wlan_MAC_ADDR":"39:9F:51:CD:F7:08",
      "eth_IP":"10.8.8.19",
      "user":"root",
      "password":"123456"
    },
    {
      "id":"SSC-001",
      "ssid":"SmartHomeAPMyExperiment",
      "activity": "UDP trasmission at 2Mbps of 10 minutes, start at 5sec",
      "wpa_passphrase":"12345678",
      "wpa_key_mgmt":"WPA-PSK",
      "wlan_IP":"192.168.1.100",
      "distance":"15 meter far from AP",
      "wlan_MAC_ADDR":"0B:E3:41:0A:33:B7",
      "eth_IP":"10.8.8.20",
      "user":"root",
      "password":"123456"
    }
  ]
}

# Configure WiFi Access Point
ap_config = blueprint["wifi_ap"][0]
ap_eth_ip = ap_config["eth_IP"]

with Connection(host=ap_eth_ip, user="root", connect_kwargs={"password": "123456"}) as c:
    # Create hostapd.conf
    hostapd_conf = f"""
interface=wlan0
driver=nl80211
ssid={ap_config["ssid"]}
hw_mode=g
channel=7
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase={ap_config["wpa_passphrase"]}
wpa_key_mgmt={ap_config["wpa_key_mgmt"]}
wpa_pairwise=TKIP
rsn_pairwise=CCMP
    """
    c.put(hostapd_conf, "/etc/hostapd/hostapd.conf")

    # Create dnsmasq.conf
    dnsmasq_conf = f"""
interface=wlan0
dhcp-range={ap_config["wlan_IP"].rsplit('.', 1)[0]}.10,{ap_config["wlan_IP"].rsplit('.', 1)[0]}.250,255.255.255.0,12h
    """
    for station in blueprint["wifi_stations"]:
        dnsmasq_conf += f'dhcp-host={station["wlan_MAC_ADDR"]},{station["wlan_IP"]}\n'

    c.put(dnsmasq_conf, "/etc/dnsmasq.conf")

    # Restart services
    c.sudo("systemctl restart hostapd")
    c.sudo("systemctl restart dnsmasq")

# Configure WiFi Stations
for station in blueprint["wifi_stations"]:
    station_eth_ip = station["eth_IP"]
    with Connection(host=station_eth_ip, user=station["user"], connect_kwargs={"password": station["password"]}) as c:
        # Configure wpa_supplicant.conf
        wpa_supplicant_conf = f"""
network={{
    ssid="{station["ssid"]}"
    psk="{station["wpa_passphrase"]}"
    key_mgmt={station["wpa_key_mgmt"]}
}}
        """
        c.put(wpa_supplicant_conf, "/etc/wpa_supplicant/wpa_supplicant-wlan0.conf")

        # Restart wpa_supplicant
        c.sudo("wpa_cli -i wlan0 reconfigure")

```