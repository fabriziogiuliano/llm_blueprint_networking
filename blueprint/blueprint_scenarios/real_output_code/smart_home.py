python
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random

from dotenv import load_dotenv
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

def DeleteGateway(gateway_id):
    client = api.GatewayServiceStub(channel)
    req = api.DeleteGatewayRequest()
    req.gateway_id = gateway_id
    resp = client.Delete(req, metadata=auth_token)
    return resp

def get_dict(str_application_id):
    k, v = str_application_id.split(': ')
    v = v.replace('"','').strip("\n")
    return {k: v}

def configure_wifi_ap(ap_config):
    hostapd_conf = f"""
    interface=wlan0
    ssid={ap_config['ssid']}
    hw_mode=g
    channel=7
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

    for station in ap_config['wifi_stations']:
        dnsmasq_conf += f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n"

    with open('/etc/hostapd/hostapd.conf', 'w') as f:
        f.write(hostapd_conf)

    with open('/etc/dnsmasq.conf', 'w') as f:
        f.write(dnsmasq_conf)

    os.system('systemctl restart hostapd')
    os.system('systemctl restart dnsmasq')

def main():
    print("[STEP1] CREATE APPLICATION")
    dict_application_id = get_dict(str(CreateApplication(name="Smart Home", tenant_id=tenant_id)))
    application_id = dict_application_id["id"]

    print("[STEP2] CREATE GATEWAY")
    print(DeleteGateway("05b0da50148fd6b1"))
    CreateGateway(
        gateway_id="05b0da50148fd6b1",
        name="SmartHomeGW",
        description="Gateway for Smart Home",
        tenant_id=tenant_id
    )

    ap_config = {
        "id": "AP-001",
        "ssid": "SmartHomeAPMyExperiment",
        "wpa_passphrase": "12345678",
        "wpa_key_mgmt": "WPA-PSK",
        "wlan_IP": "192.168.1.1",
        "eth_IP": "10.8.8.16",
        "wifi_stations": [
            {
                "id": "ST-001",
                "ssid": "SmartHomeAPMyExperiment",
                "wpa_passphrase": "12345678",
                "wpa_key_mgmt": "WPA-PSK",
                "wlan_IP": "192.168.1.103",
                "wlan_MAC_ADDR": "61:5F:64:5E:90:EB",
                "eth_IP": "10.8.8.17"
            },
            {
                "id": "SLB-001",
                "ssid": "SmartHomeAPMyExperiment",
                "wpa_passphrase": "12345678",
                "wpa_key_mgmt": "WPA-PSK",
                "wlan_IP": "192.168.1.102",
                "wlan_MAC_ADDR": "60:36:1E:9A:0A:0C",
                "eth_IP": "10.8.8.18"
            },
            {
                "id": "SDL-001",
                "ssid": "SmartHomeAPMyExperiment",
                "wpa_passphrase": "12345678",
                "wpa_key_mgmt": "WPA-PSK",
                "wlan_IP": "192.168.1.101",
                "wlan_MAC_ADDR": "39:9F:51:CD:F7:08",
                "eth_IP": "10.8.8.19"
            },
            {
                "id": "SSC-001",
                "ssid": "SmartHomeAPMyExperiment",
                "wpa_passphrase": "12345678",
                "wpa_key_mgmt": "WPA-PSK",
                "wlan_IP": "192.168.1.100",
                "wlan_MAC_ADDR": "0B:E3:41:0A:33:B7",
                "eth_IP": "10.8.8.20"
            }
        ]
    }

    configure_wifi_ap(ap_config)

if __name__ == "__main__":
    main()
