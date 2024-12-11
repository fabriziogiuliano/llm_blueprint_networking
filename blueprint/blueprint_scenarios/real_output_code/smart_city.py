python
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

def CreateApplication(name, tenant_id):
    client = api.ApplicationServiceStub(channel)
    req = api.CreateApplicationRequest()
    req.application.name = name
    req.application.tenant_id = tenant_id
    resp = client.Create(req, metadata=auth_token)
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

def get_dict(str_application_id):
    k, v = str_application_id.split(': ')
    v = v.replace('"','').strip("\n")
    return {k: v}

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
    dhcp-range=192.168.1.10,192.168.1.250,12h
    """

    for station in wifi_stations:
        dnsmasq_conf += f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n"

    with Connection(host=ap_config['eth_IP'], user='root', connect_kwargs={"password": "123456"}) as c:
        c.put(io.StringIO(hostapd_conf), '/etc/hostapd/hostapd.conf')
        c.put(io.StringIO(dnsmasq_conf), '/etc/dnsmasq.conf')
        c.run('systemctl restart hostapd')
        c.run('systemctl restart dnsmasq')

def configure_wifi_station(station_config):
    with Connection(host=station_config['eth_IP'], user=station_config['user'], connect_kwargs={"password": station_config['password']}) as c:
        c.run(f'nmcli dev wifi connect {station_config["ssid"]} password {station_config["wpa_passphrase"]}')

# STEP1 create the application
print("[STEP1] CREATE APPLICATION")
dict_application_id = get_dict(str(CreateApplication(name="Small Smart City", tenant_id=tenant_id)))
application_id = dict_application_id["id"]

# STEP2 Create Gateway
print("[STEP2] CREATE GATEWAY")
CreateGateway(
    gateway_id="05b0da50148fd6b1",
    name="GW-001",
    description="Gateway City Hall",
    tenant_id=tenant_id
)

# STEP3 create Device profile
print("[STEP3] CREATE DEVICE PROFILE")
dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="SMART_CITY_EU868_1.0.3", tenant_id=tenant_id)))
device_profile_id = dict_device_profile_id["id"]

# STEP4 create LoRaWAN Devices
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

# STEP5 configure WiFi AP
wifi_ap_config = {
    "id": "AP-001",
    "ssid": "SmartCityAP",
    "wpa_passphrase": "12345678",
    "wpa_key_mgmt": "WPA-PSK",
    "wlan_IP": "192.168.1.1",
    "eth_IP": "10.8.8.16"
}
configure_wifi_ap(wifi_ap_config)

# STEP6 configure WiFi Stations
wifi_stations = [
    {
        "id": "STA-001",
        "ssid": "SmartCityAP",
        "wpa_passphrase": "12345678",
        "wpa_key_mgmt": "WPA-PSK",
        "wlan_IP": "192.168.1.101",
        "wlan_MAC_ADDR": "08:5B:28:2B:9E:74",
        "eth_IP": "10.8.8.17",
        "user": "root",
        "password": "123456"
    }
]
for station in wifi_stations:
    configure_wifi_station(station)
