
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
    v = v.replace('"', '').strip("\n")
    return {k: v}

def configure_wifi_ap(ap_config):
    with open('/etc/hostapd/hostapd.conf', 'w') as hostapd_conf:
        hostapd_conf.write(f"interface=wlan0\n")
        hostapd_conf.write(f"ssid={ap_config['ssid']}\n")
        hostapd_conf.write(f"wpa_passphrase={ap_config['wpa_passphrase']}\n")
        hostapd_conf.write(f"wpa_key_mgmt={ap_config['wpa_key_mgmt']}\n")

    with open('/etc/dnsmasq.conf', 'w') as dnsmasq_conf:
        dnsmasq_conf.write(f"interface=wlan0\n")
        dnsmasq_conf.write(f"dhcp-range=192.168.1.100,192.168.1.200,12h\n")
        dnsmasq_conf.write(f"address=/{ap_config['ssid']}/{ap_config['wlan_IP']}\n")

def configure_wifi_station(station_config):
    with open('/etc/dnsmasq.conf', 'a') as dnsmasq_conf:
        dnsmasq_conf.write(f"dhcp-host={station_config['wlan_MAC_ADDR']},{station_config['wlan_IP']}\n")

def main():
    with open('TEST BLUEPRINT.json') as f:
        blueprint = json.load(f)

    # Create Application
    dict_application_id = get_dict(str(CreateApplication(name="TEST-BLUEPRINT", tenant_id=tenant_id)))
    application_id = dict_application_id["id"]

    # Create LoRa Gateways
    for gateway in blueprint['network_devices']['lora_gateways']:
        CreateGateway(
            gateway_id=gateway['unique_id'],
            name=gateway['gateway_name'],
            description=gateway['gateway_description'],
            tenant_id=tenant_id
        )

    # Create Device Profile
    dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
    device_profile_id = dict_device_profile_id["id"]

    # Create LoRa Devices
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

    # Configure WiFi Access Points
    for ap in blueprint['network_devices']['wifi_access_points']:
        configure_wifi_ap(ap)

    # Configure WiFi Stations
    for station in blueprint['network_devices']['wifi_stations']:
        configure_wifi_station(station)

if __name__ == "__main__":
    main()
