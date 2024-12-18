
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

def CreateApplication(name, tenant_id):
    client = api.ApplicationServiceStub(channel)
    req = api.CreateApplicationRequest()
    req.application.name = name
    req.application.tenant_id = tenant_id
    resp = client.Create(req, metadata=auth_token)
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

    with Connection(host=ap_config['eth_IP'], user='root', connect_kwargs={"password": "123456"}) as c:
        c.put(hostapd_conf, '/etc/hostapd/hostapd.conf')
        c.put(dnsmasq_conf, '/etc/dnsmasq.conf')
        c.run('systemctl restart hostapd')
        c.run('systemctl restart dnsmasq')

def configure_wifi_station(station_config):
    dnsmasq_conf = f"""
    dhcp-host={station_config['wlan_MAC_ADDR']},{station_config['wlan_IP']}
    """

    with Connection(host=station_config['eth_IP'], user=station_config['user'], connect_kwargs={"password": station_config['password']}) as c:
        c.put(dnsmasq_conf, '/etc/dnsmasq.d/static_hosts.conf')
        c.run('systemctl restart dnsmasq')

# Create the application
print("[STEP1] CREATE APPLICATION")
dict_application_id = get_dict(str(CreateApplication(name="Smart Home", tenant_id=tenant_id)))
application_id = dict_application_id["id"]

# Configure WiFi AP
print("[STEP2] CONFIGURE WIFI AP")
ap_config = {
    "id": "AP-001",
    "ssid": "SmartHomeAPMyExperiment",
    "wpa_passphrase": "12345678",
    "wpa_key_mgmt": "WPA-PSK",
    "wlan_IP": "192.168.1.1",
    "eth_IP": "10.8.8.16"
}
configure_wifi_ap(ap_config)

# Configure WiFi Stations
print("[STEP3] CONFIGURE WIFI STATIONS")
wifi_stations = [
    {
        "id": "ST-001",
        "ssid": "SmartHomeAPMyExperiment",
        "wpa_passphrase": "12345678",
        "wpa_key_mgmt": "WPA-PSK",
        "wlan_IP": "192.168.1.103",
        "wlan_MAC_ADDR": "61:5F:64:5E:90:EB",
        "eth_IP": "10.8.8.17",
        "user": "root",
        "password": "123456"
    },
    {
        "id": "SLB-001",
        "ssid": "SmartHomeAPMyExperiment",
        "wpa_passphrase": "12345678",
        "wpa_key_mgmt": "WPA-PSK",
        "wlan_IP": "192.168.1.102",
        "wlan_MAC_ADDR": "60:36:1E:9A:0A:0C",
        "eth_IP": "10.8.8.18",
        "user": "root",
        "password": "123456"
    },
    {
        "id": "SDL-001",
        "ssid": "SmartHomeAPMyExperiment",
        "wpa_passphrase": "12345678",
        "wpa_key_mgmt": "WPA-PSK",
        "wlan_IP": "192.168.1.101",
        "wlan_MAC_ADDR": "39:9F:51:CD:F7:08",
        "eth_IP": "10.8.8.19",
        "user": "root",
        "password": "123456"
    },
    {
        "id": "SSC-001",
        "ssid": "SmartHomeAPMyExperiment",
        "wpa_passphrase": "12345678",
        "wpa_key_mgmt": "WPA-PSK",
        "wlan_IP": "192.168.1.100",
        "wlan_MAC_ADDR": "0B:E3:41:0A:33:B7",
        "eth_IP": "10.8.8.20",
        "user": "root",
        "password": "123456"
    }
]

for station in wifi_stations:
    configure_wifi_station(station)
