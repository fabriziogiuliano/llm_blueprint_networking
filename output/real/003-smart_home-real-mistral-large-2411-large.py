
import json
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
    v = v.replace('"', '').strip("\n")
    return {k: v}

def setup_wifi_ap(ap_config):
    hostapd_conf = f"""
    interface=wlan0
    ssid={ap_config['ssid']}
    wpa=2
    wpa_passphrase={ap_config['wpa_passphrase']}
    wpa_key_mgmt={ap_config['wpa_key_mgmt']}
    rsn_pairwise=CCMP
    """
    with open('/etc/hostapd/hostapd.conf', 'w') as f:
        f.write(hostapd_conf)

    dnsmasq_conf = f"""
    interface=wlan0
    dhcp-range=192.168.1.10,192.168.1.250,255.255.255.0,12h
    """
    with open('/etc/dnsmasq.conf', 'a') as f:
        f.write(dnsmasq_conf)

def setup_wifi_stations(stations):
    with open('/etc/dnsmasq.conf', 'a') as f:
        for station in stations:
            f.write(f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n")

def main():
    with open('TEST_BLUEPRINT.json', 'r') as file:
        blueprint = json.load(file)

    wifi_aps = blueprint['network_devices'].get('wifi_access_points', [])
    wifi_stations = blueprint['network_devices'].get('wifi_stations', [])

    if wifi_aps:
        for ap in wifi_aps:
            setup_wifi_ap(ap)

    if wifi_stations:
        setup_wifi_stations(wifi_stations)

    print("[STEP1] CREATE APPLICATION")
    dict_application_id = get_dict(str(CreateApplication(name="TEST-BLUEPRINT", tenant_id=tenant_id)))
    application_id = dict_application_id["id"]

    print("[STEP3] CREATE DEVICE PROFILE")
    dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
    device_profile_id = dict_device_profile_id["id"]

    print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")
    for station in wifi_stations:
        CreateDevice(
            dev_eui=station['wlan_MAC_ADDR'].replace(':', ''),
            name=station['station_id'],
            application_id=application_id,
            device_profile_id=device_profile_id,
            application_key="8BD59A2C514C9EBDAF7E335E78A5E5B5",
            skip_fcnt_check=False,
            is_disabled=False
        )

if __name__ == "__main__":
    main()
