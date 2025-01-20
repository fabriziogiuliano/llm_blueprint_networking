```python
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random

from fabric import Connection, Config
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

def CreateDeviceProfile(name, tenant_id, region="EU868", mac_version="LORAWAN_1_0_3", revision="RP002_1_0_3",
                        supports_otaa=True):
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

def CreateDevice(dev_eui, name, application_id, device_profile_id, application_key, skip_fcnt_check=True,
                 is_disabled=False):
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

print("[STEP1] CREATE APPLICATION")
dict_application_id = get_dict(str(CreateApplication(name="Small Smart City", tenant_id=tenant_id)))
application_id = dict_application_id["id"]

print("[STEP2] CREATE GATEWAY")
print(DeleteGateway("05b0da50148fd6b1"))
CreateGateway(
    gateway_id="05b0da50148fd6b1",
    name="GW-001",
    description="Gateway City Hall",
    tenant_id=tenant_id
)

print("[STEP3] CREATE DEVICE PROFILE")
dict_device_profile_id = get_dict(
    str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3",
                            tenant_id=tenant_id)))
device_profile_id = dict_device_profile_id["id"]
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

print("[STEP5] Configure WiFi AP and Stations")
ap_config = Config(overrides={'sudo': {'password': '123456'}})
ap_conn = Connection(host='10.8.8.16', user='root', config=ap_config)

print("[STEP6] Configuring AP-001")
ap_conn.sudo("echo 'interface=wlan0' > /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'driver=nl80211' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'ssid=SmartCityAP' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'hw_mode=g' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'channel=7' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'wmm_enabled=0' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'macaddr_acl=0' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'auth_algs=1' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'ignore_broadcast_ssid=0' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'wpa=2' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'wpa_passphrase=12345678' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'wpa_key_mgmt=WPA-PSK' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'wpa_pairwise=TKIP' >> /etc/hostapd/hostapd.conf")
ap_conn.sudo("echo 'rsn_pairwise=CCMP' >> /etc/hostapd/hostapd.conf")

ap_conn.sudo("echo 'interface=wlan0' > /etc/dnsmasq.conf")
ap_conn.sudo("echo 'dhcp-range=192.168.1.100,192.168.1.200,255.255.255.0,12h' >> /etc/dnsmasq.conf")
ap_conn.sudo("echo 'dhcp-option=3,192.168.1.1' >> /etc/dnsmasq.conf")
ap_conn.sudo("echo 'dhcp-option=6,192.168.1.1' >> /etc/dnsmasq.conf")
ap_conn.sudo("echo 'server=8.8.8.8' >> /etc/dnsmasq.conf")
ap_conn.sudo("echo 'log-queries' >> /etc/dnsmasq.conf")
ap_conn.sudo("echo 'log-dhcp' >> /etc/dnsmasq.conf")
ap_conn.sudo("echo 'listen-address=127.0.0.1' >> /etc/dnsmasq.conf")
ap_conn.sudo("echo 'expand-hosts' >> /etc/dnsmasq.conf")
ap_conn.sudo("echo 'dhcp-host=08:5B:28:2B:9E:74,192.168.1.101' >> /etc/dnsmasq.conf")

ap_conn.sudo("systemctl restart hostapd")
ap_conn.sudo("systemctl restart dnsmasq")

print("[STEP7] Configuring STA-001")
sta_config = Config(overrides={'sudo': {'password': '123456'}})
sta_conn = Connection(host='10.8.8.17', user='root', config=sta_config)

sta_conn.sudo("wpa_passphrase SmartCityAP 12345678 > /etc/wpa_supplicant/wpa_supplicant-wlan0.conf")
sta_conn.sudo("ifconfig wlan0 192.168.1.101")
sta_conn.sudo("ifup wlan0")

```