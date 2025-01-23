
import os
import json
import grpc
from chirpstack_api import api
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

def configure_hostapd(ap):
    hostapd_conf = f"""
    interface=wlan0
    driver=nl80211
    ssid={ap['ssid']}
    hw_mode=g
    channel=7
    wmm_enabled=0
    macaddr_acl=0
    auth_algs=1
    ignore_broadcast_ssid=0
    wpa=2
    wpa_passphrase={ap['wpa_passphrase']}
    wpa_key_mgmt={ap['wpa_key_mgmt']}
    rsn_pairwise=CCMP
    """
    with open('/etc/hostapd/hostapd.conf', 'w') as file:
        file.write(hostapd_conf)

def configure_dnsmasq(aps, stations):
    dnsmasq_conf = """
    interface=wlan0
    dhcp-range=192.168.1.10,192.168.1.250,12h
    """
    for sta in stations:
        dnsmasq_conf += f"dhcp-host={sta['wlan_MAC_ADDR']},{sta['wlan_IP']}\n"
    with open('/etc/dnsmasq.conf', 'w') as file:
        file.write(dnsmasq_conf)

def setup_network(blueprint):
    print("[STEP1] CREATE APPLICATION")
    dict_application_id = get_dict(str(CreateApplication(name="TEST-BLUEPRINT", tenant_id=tenant_id)))
    application_id = dict_application_id["id"]

    print("[STEP2] CREATE GATEWAYS")
    for gw in blueprint['network_devices']['lora_gateways']:
        CreateGateway(
            gateway_id=gw['unique_id'],
            name=gw['gateway_name'],
            description=gw['gateway_description'],
            tenant_id=tenant_id
        )

    print("[STEP3] CREATE DEVICE PROFILE")
    dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
    device_profile_id = dict_device_profile_id["id"]

    print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")
    for device in blueprint['network_devices']['lora_endpoints']:
        CreateDevice(
            dev_eui=device['device_eui'],
            name=device['device_name'],
            application_id=application_id,
            device_profile_id=device_profile_id,
            application_key=device['security_key'],
            skip_fcnt_check=False,
            is_disabled=False
        )

    if 'wifi_access_points' in blueprint['network_devices']:
        for ap in blueprint['network_devices']['wifi_access_points']:
            configure_hostapd(ap)

    if 'wifi_stations' in blueprint['network_devices']:
        configure_dnsmasq(blueprint['network_devices']['wifi_access_points'], blueprint['network_devices']['wifi_stations'])

if __name__ == "__main__":
    with open('TEST_BLUEPRINT.json', 'r') as file:
        blueprint = json.load(file)
    setup_network(blueprint)
