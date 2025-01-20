
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
from fabric import Connection
from invoke import exceptions

# URL API CHIRPSTACK: https://www.chirpstack.io/docs/chirpstack/api/api.html

from dotenv import load_dotenv
load_dotenv()
server = os.getenv('CHIRPSTACK_URL')  # Adjust this to your ChirpStack instance
api_token = os.getenv('CHIRPSTACK_API_TOKEN')  # Obtain from ChirpStack
tenant_id = os.getenv('CHIRPSTACK_TENANT_ID')  # this is a tenant ID for fabrizio.giuliano@unipa.it

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

def get_dict(str_application_id):
    k, v = str_application_id.split(': ')
    v = v.replace('"','').strip("\n")
    return {k: v}

# Configure WiFi Access Point
ap_config = {
    "id": "AP-001",
    "ssid": "SmartHomeAPMyExperiment",
    "wpa_passphrase": "12345678",
    "wpa_key_mgmt": "WPA-PSK",
    "wlan_IP": "192.168.1.1",
    "eth_IP": "10.8.8.16"
}

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
wpa_pairwise=TKIP
rsn_pairwise=CCMP
"""

try:
    with Connection(host=ap_config["eth_IP"], user="root", connect_kwargs={"password": "123456"}) as c:

        # Configure hostapd
        c.put(hostapd_conf, "/etc/hostapd/hostapd.conf")
        c.run("systemctl unmask hostapd")
        c.run("systemctl enable hostapd")
        c.run("systemctl start hostapd")

        # Configure dnsmasq
        dnsmasq_conf = f"""
interface=wlan0
dhcp-range=192.168.1.2,192.168.1.254,255.255.255.0,12h
dhcp-option=3,192.168.1.1
dhcp-option=6,192.168.1.1
server=8.8.8.8
log-queries
log-dhcp
listen-address=127.0.0.1
        """
        
        
        # Configure WiFi Stations
        stations = [
            {"id": "ST-001", "ssid": "SmartHomeAPMyExperiment", "wpa_passphrase": "12345678", "wpa_key_mgmt": "WPA-PSK", "wlan_IP": "192.168.1.103", "wlan_MAC_ADDR": "61:5F:64:5E:90:EB", "eth_IP": "10.8.8.17", "user": "root", "password": "123456"},
            {"id": "SLB-001", "ssid": "SmartHomeAPMyExperiment", "wpa_passphrase": "12345678", "wpa_key_mgmt": "WPA-PSK", "wlan_IP": "192.168.1.102", "wlan_MAC_ADDR": "60:36:1E:9A:0A:0C", "eth_IP": "10.8.8.18", "user": "root", "password": "123456"},
            {"id": "SDL-001", "ssid": "SmartHomeAPMyExperiment", "wpa_passphrase": "12345678", "wpa_key_mgmt": "WPA-PSK", "wlan_IP": "192.168.1.101", "wlan_MAC_ADDR": "39:9F:51:CD:F7:08", "eth_IP": "10.8.8.19", "user": "root", "password": "123456"},
            {"id": "SSC-001", "ssid": "SmartHomeAPMyExperiment", "wpa_passphrase": "12345678", "wpa_key_mgmt": "WPA-PSK", "wlan_IP": "192.168.1.100", "wlan_MAC_ADDR": "0B:E3:41:0A:33:B7", "eth_IP": "10.8.8.20", "user": "root", "password": "123456"}
        ]
        
        for station in stations:
            dnsmasq_conf += f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n"

        c.put(dnsmasq_conf, "/etc/dnsmasq.conf")
        c.run("systemctl restart dnsmasq")
        
        for station in stations:
            with Connection(host=station["eth_IP"], user=station["user"], connect_kwargs={"password": station["password"]}) as c_station:
                wpa_supplicant_conf = f"""
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=US

network={{
    ssid="{station['ssid']}"
    psk="{station['wpa_passphrase']}"
    key_mgmt={station['wpa_key_mgmt']}
}}
"""
                c_station.put(wpa_supplicant_conf, "/etc/wpa_supplicant/wpa_supplicant.conf")

                c_station.run("wpa_cli -i wlan0 reconfigure")

                # Set static IP
                c_station.run(f"ip addr add {station['wlan_IP']}/24 dev wlan0")
                c_station.run(f"ip link set wlan0 up")
                
except exceptions.GroupException as e:
    print("One or more connections failed:")
    for result in e.result:
        print(f"- {result.host}: {result.reason}")
except Exception as e:
    print(f"An error occurred: {e}")
```