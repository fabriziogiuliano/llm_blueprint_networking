
import os
import json
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

def setup_network(blueprint_file):
    with open(blueprint_file, 'r') as file:
        blueprint = json.load(file)

    scenario_name = blueprint["scenario_name"]
    print(f"[STEP1] CREATE APPLICATION FOR {scenario_name}")
    dict_application_id = get_dict(str(CreateApplication(name=scenario_name, tenant_id=tenant_id)))
    application_id = dict_application_id["id"]

    if "lora_gateways" in blueprint["network_devices"]:
        print("[STEP2] CREATE GATEWAYS")
        for gateway in blueprint["network_devices"]["lora_gateways"]:
            CreateGateway(
                gateway_id=gateway["unique_id"],
                name=gateway["gateway_name"],
                description=gateway["gateway_description"],
                tenant_id=tenant_id
            )

    print("[STEP3] CREATE DEVICE PROFILE")
    dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
    device_profile_id = dict_device_profile_id["id"]

    if "lora_endpoints" in blueprint["network_devices"]:
        print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")
        for device in blueprint["network_devices"]["lora_endpoints"]:
            CreateDevice(
                dev_eui=device["device_eui"],
                name=device["device_name"],
                application_id=application_id,
                device_profile_id=device_profile_id,
                application_key=device["security_key"],
                skip_fcnt_check=False,
                is_disabled=False
            )

    if "wifi_nodes" in blueprint["network_devices"]:
        print("[STEP5] CONFIGURE WIFI ACCESS POINT")
        with open("/etc/hostapd/hostapd.conf", "w") as hostapd_file:
            hostapd_file.write("interface=wlan0\n")
            hostapd_file.write("driver=nl80211\n")
            hostapd_file.write("ssid=MyAP\n")
            hostapd_file.write("hw_mode=g\n")
            hostapd_file.write("channel=7\n")
            hostapd_file.write("wmm_enabled=0\n")
            hostapd_file.write("macaddr_acl=0\n")
            hostapd_file.write("auth_algs=1\n")
            hostapd_file.write("ignore_broadcast_ssid=0\n")
            hostapd_file.write("wpa=2\n")
            hostapd_file.write("wpa_passphrase=12345678\n")
            hostapd_file.write("wpa_key_mgmt=WPA-PSK\n")
            hostapd_file.write("rsn_pairwise=CCMP\n")

        with open("/etc/dnsmasq.conf", "a") as dnsmasq_file:
            for station in blueprint["network_devices"]["wifi_nodes"]:
                dnsmasq_file.write(f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n")

setup_network('TEST_BLUEPRINT.json')
