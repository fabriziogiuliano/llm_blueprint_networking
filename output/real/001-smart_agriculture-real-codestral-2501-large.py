
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

def CreateGateway(gateway_id, name, tenant_id, description=None):
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

def configure_wifi_ap(wifi_nodes):
    if not wifi_nodes:
        return

    with open('/etc/hostapd/hostapd.conf', 'w') as hostapd_conf:
        hostapd_conf.write("""
        interface=wlan0
        driver=nl80211
        ssid=MyWiFiNetwork
        hw_mode=g
        channel=7
        wmm_enabled=0
        macaddr_acl=0
        auth_algs=1
        ignore_broadcast_ssid=0
        wpa=2
        wpa_passphrase=MyWiFiPassword
        wpa_key_mgmt=WPA-PSK
        rsn_pairwise=CCMP
        """)

    with open('/etc/dnsmasq.conf', 'w') as dnsmasq_conf:
        dnsmasq_conf.write("""
        interface=wlan0
        dhcp-range=192.168.1.10,192.168.1.100,12h
        """)

    for node in wifi_nodes:
        mac_addr = node['mac_addr']
        ip_addr = node['ip_addr']
        with open('/etc/dnsmasq.conf', 'a') as dnsmasq_conf:
            dnsmasq_conf.write(f"dhcp-host={mac_addr},{ip_addr}\n")

def main():
    with open('TEST BLUEPRINT.json') as f:
        blueprint = json.load(f)

    # Create Application
    dict_application_id = get_dict(str(CreateApplication(name=blueprint['name'], tenant_id=tenant_id)))
    application_id = dict_application_id["id"]

    # Create Device Profile
    dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
    device_profile_id = dict_device_profile_id["id"]

    # Create LoRa Gateways
    for gateway in blueprint['components']['lora_gateways']:
        CreateGateway(
            gateway_id=gateway['gateway_id'],
            name=gateway['name'],
            description=gateway['description'],
            tenant_id=tenant_id
        )

    # Create LoRa Devices
    for device in blueprint['components']['lora_devices']:
        CreateDevice(
            dev_eui=device['dev_eui'],
            name=device['name'],
            application_id=application_id,
            device_profile_id=device_profile_id,
            application_key=device['application_key'],
            skip_fcnt_check=False,
            is_disabled=False
        )

    # Configure WiFi Access Point
    wifi_nodes = blueprint.get('wifi_nodes', [])
    configure_wifi_ap(wifi_nodes)

if __name__ == "__main__":
    main()
