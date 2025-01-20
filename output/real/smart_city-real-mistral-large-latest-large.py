
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
from dotenv import load_dotenv
from fabric import Connection

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

def CreateDeviceProfile(name, tenant_id, region="EU868", mac_version="LORAWAN_1_0_3", revision ="RP002_1_0_3", supports_otaa=True):
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

def CreateDevice(dev_eui, name, application_id, device_profile_id, application_key, skip_fcnt_check=True, is_disabled=False):
  client = api.DeviceServiceStub(channel)
  req = api.CreateDeviceRequest()
  req.device.dev_eui = dev_eui
  req.device.name = name
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
  req.gateway.gateway_id = gateway_id
  req.gateway.name = name
  req.gateway.description = description
  req.gateway.tenant_id = tenant_id
  req.gateway.stats_interval = 60
  resp = client.Create(req, metadata=auth_token)
  return resp

def get_dict(str_application_id):
    k, v = str_application_id.split(': ')
    v = v.replace('"','').strip("\n")
    return {k: v}

def configure_wifi_ap(ap_config):
    hostapd_conf = f"""
    interface=wlan0
    ssid={ap_config['ssid']}
    wpa=2
    wpa_passphrase={ap_config['wpa_passphrase']}
    wpa_key_mgmt={ap_config['wpa_key_mgmt']}
    rsn_pairwise=CCMP
    """
    dnsmasq_conf = f"""
    interface=wlan0
    dhcp-range=192.168.1.10,192.168.1.250,12h
    dhcp-option=3,{ap_config['wlan_IP']}
    dhcp-option=6,{ap_config['wlan_IP']}
    """
    with Connection(host=ap_config['eth_IP'], user='root', connect_kwargs={"password": "123456"}) as c:
        c.put(hostapd_conf, '/etc/hostapd/hostapd.conf')
        c.put(dnsmasq_conf, '/etc/dnsmasq.conf')
        c.run('systemctl restart hostapd')
        c.run('systemctl restart dnsmasq')

def configure_wifi_station(station_config):
    dnsmasq_conf = f"""
    host={station_config['wlan_MAC_ADDR']},{station_config['wlan_IP']}
    """
    with Connection(host=station_config['eth_IP'], user='root', connect_kwargs={"password": "123456"}) as c:
        c.put(dnsmasq_conf, '/etc/dnsmasq.conf')
        c.run('systemctl restart dnsmasq')

# Create Application
print("[STEP1] CREATE APPLICATION")
dict_application_id = get_dict(str(CreateApplication(name="Small Smart City", tenant_id=tenant_id)))
application_id = dict_application_id["id"]

# Create Gateway
print("[STEP2] CREATE GATEWAY")
CreateGateway(
  gateway_id="05b0da50148fd6b1",
  name="GW-001",
  description="Gateway City Hall",
  tenant_id=tenant_id
)

# Create Device Profile
print("[STEP3] CREATE DEVICE PROFILE")
dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="SmallSmartCity_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
device_profile_id = dict_device_profile_id["id"]

# Create LoRa Devices
print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")
CreateDevice(
    dev_eui="81a63bce0927e700",
    name="IOT-001",
    application_id=application_id,
    device_profile_id=device_profile_id,
    application_key="4ba2162e111f9c2c216b7d68e09f43a8",
    skip_fcnt_check=False,
    is_disabled=False
)
CreateDevice(
    dev_eui="89697aa089cc3005",
    name="IOT-002",
    application_id=application_id,
    device_profile_id=device_profile_id,
    application_key="a1ac00d450efb7facb46a243cbf7731f",
    skip_fcnt_check=False,
    is_disabled=False
)

# Configure WiFi AP
print("[STEP5] CONFIGURE WIFI AP")
ap_config = {
    "ssid": "SmartCityAP",
    "wpa_passphrase": "12345678",
    "wpa_key_mgmt": "WPA-PSK",
    "wlan_IP": "192.168.1.1",
    "eth_IP": "10.8.8.16"
}
configure_wifi_ap(ap_config)

# Configure WiFi Stations
print("[STEP6] CONFIGURE WIFI STATIONS")
station_config = {
    "wlan_MAC_ADDR": "08:5B:28:2B:9E:74",
    "wlan_IP": "192.168.1.101",
    "eth_IP": "10.8.8.17"
}
configure_wifi_station(station_config)
