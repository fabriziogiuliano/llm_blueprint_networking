
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
  req = api.CreateApplicationRequest()
  req = api.GetApp
  req.i
  req.application.name = name
  req.application.tenant_id = tenant_id
  resp = client.Create(req, metadata=auth_token)
  return resp

def CreateDeviceProfile(name, tenant_id, region="EU868", mac_version="LORAWAN_1_0_3", revision ="RP002_1_0_3", supports_otaa=True):
  client = api.DeviceProfileServiceStub(channel)
  req = api.CreateDeviceProfileRequest()
  req.device_profile
  req.device_profile.name=name
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

def DeleteDevice(device_id):
  client = api.DeviceServiceStub(channel)
  req = api.DeleteDeviceRequest()
  req.dev_eui=device_id
  resp = client.Delete(req,metadata=auth_token)
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
print("[STEP1] CREATE APPLICATION")

dict_application_id = get_dict(str(CreateApplication(name="TEST-BLUEPRINT",tenant_id=tenant_id)))
application_id=dict_application_id["id"]

print("[STEP2] CREATE GATEWAY")
print(DeleteGateway("05b0da50148fd6b1"))
CreateGateway(
  gateway_id="05b0da50148fd6b1",
  name="TEST-GW",
  description="Gateway Building 9",
  tenant_id=tenant_id
)

print("[STEP3] CREATE DEVICE PROFILE")

dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0",mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
device_profile_id=dict_device_profile_id["id"]
print(device_profile_id)

print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")
print(CreateDevice(
    dev_eui="81a63bce0927e700",
    name="IOT-001",
    application_id=application_id,
    device_profile_id=device_profile_id,
    application_key="4ba2162e111f9c2c216b7d68e09f43a8",
    skip_fcnt_check=False,
    is_disabled=False
))

print(CreateDevice(
    dev_eui="89697aa089cc3005",
    name="IOT-002",
    application_id=application_id,
    device_profile_id=device_profile_id,
    application_key="a1ac00d450efb7facb46a243cbf7731f",
    skip_fcnt_check=False,
    is_disabled=False
))

wifi_ap_config = {
    "type": "WiFi AP",
    "id": "AP-001",
    "ssid": "SmartCityAP",
    "wpa_passphrase": "12345678",
    "wpa_key_mgmt": "WPA-PSK",
    "wlan_IP": "192.168.1.1",
    "eth_IP": "10.8.8.16",
    "application": "WiFi Access Point",
    "location": "City Hall",
    "protocol": "WiFi",
    "latitude": 38.10351066811096,
    "longitude": 13.3459399220741
}

wifi_stations_config = [
    {
        "id": "STA-001",
        "ssid": "SmartCityAP",
        "wpa_passphrase": "12345678",
        "wpa_key_mgmt": "WPA-PSK",
        "wlan_IP": "192.168.1.101",
        "wlan_MAC_ADDR": "08:5B:28:2B:9E:74",
        "eth_IP": "10.8.8.17",
        "user": "root",
        "password": "123456",
        "type": "Camera",
        "application": "Camera Monitoring",
        "location": "Park C",
        "sensor_type": "Survelliance camera",
        "protocol": "Wi-Fi",
        "data_flow": "UDP flow at 1Mbps",
        "ip_address": "192.168.1.100",
        "latitude": 38.10351066811096,
        "longitude": 13.3459399220741
    }
]


if wifi_ap_config:
    print("[STEP5] CONFIGURE WIFI ACCESS POINT")
    
    hostapd_conf = f"""
interface=wlan0
driver=nl80211
ssid={wifi_ap_config['ssid']}
hw_mode=g
channel=6
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_key_mgmt={wifi_ap_config['wpa_key_mgmt']}
wpa_passphrase={wifi_ap_config['wpa_passphrase']}
    """
    
    dnsmasq_conf = f"""
interface=wlan0
dhcp-range={wifi_ap_config['wlan_IP']},192.168.1.254,255.255.255.0,12h
    """
    
    with open("hostapd.conf", "w") as f:
        f.write(hostapd_conf)
    with open("dnsmasq.conf", "w") as f:
         f.write(dnsmasq_conf)

    c = Connection(host=wifi_ap_config['eth_IP'], user="root", connect_kwargs={"password": "123456"})
    c.run("sudo apt update")
    c.run("sudo apt install -y hostapd dnsmasq")
    c.run("sudo systemctl unmask hostapd")
    c.run("sudo systemctl enable hostapd")
    c.run("sudo systemctl enable dnsmasq")
    c.put("hostapd.conf", "/etc/hostapd/hostapd.conf")
    c.put("dnsmasq.conf", "/etc/dnsmasq.conf")
    c.run("sudo systemctl restart hostapd")
    c.run("sudo systemctl restart dnsmasq")


if wifi_stations_config:
    print("[STEP6] CONFIGURE WIFI STATIONS")
    dnsmasq_conf_stations = ""
    for station in wifi_stations_config:
      if "wlan_MAC_ADDR" in station and "wlan_IP" in station:
            dnsmasq_conf_stations+=f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n"
    
    if dnsmasq_conf_stations:
        with open("dnsmasq.conf", "a") as f:
          f.write(dnsmasq_conf_stations)

        c = Connection(host=wifi_ap_config['eth_IP'], user="root", connect_kwargs={"password": "123456"})
        c.put("dnsmasq.conf", "/etc/dnsmasq.conf")
        c.run("sudo systemctl restart dnsmasq")