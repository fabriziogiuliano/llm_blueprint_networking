To set up the network from the input JSON file `TEST BLUEPRINT` while adhering to the given requirements, we need to follow these steps:

1. Parse the JSON file to extract the necessary information.
2. Configure the WiFi Access Point using the Fabric framework, `hostapd.conf`, and `dnsmasq.conf`.
3. Configure all LoRa devices described in the `TEST BLUEPRINT`.
4. Add fixed IP configurations for each station in `dnsmasq.conf`.

Below is the Python code that accomplishes these tasks:


import os
import sys
import json
import traceback
import grpc
from chirpstack_api import api
from dotenv import load_dotenv

load_dotenv()
server = os.getenv('CHIRPSTACK_URL')  # Adjust this to your ChirpStack instance
api_token = os.getenv('CHIRPSTACK_API_TOKEN')  # Obtain from ChirpStack
tenant_id = os.getenv('CHIRPSTACK_TENANT_ID')  # This is a tenant ID for fabrizio.giuliano@unipa.it

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

def configure_wifi_ap(ssid, password):
    # Configure hostapd.conf
    hostapd_conf = f"""
    interface=wlan0
    driver=nl80211
    ssid={ssid}
    hw_mode=g
    channel=7
    wmm_enabled=0
    macaddr_acl=0
    auth_algs=1
    ignore_broadcast_ssid=0
    wpa=2
    wpa_passphrase={password}
    wpa_key_mgmt=WPA-PSK
    rsn_pairwise=CCMP
    """
    with open('/etc/hostapd/hostapd.conf', 'w') as f:
        f.write(hostapd_conf)

    # Configure dnsmasq.conf
    dnsmasq_conf = """
    interface=wlan0
    dhcp-range=192.168.1.10,192.168.1.50,12h
    """
    with open('/etc/dnsmasq.conf', 'w') as f:
        f.write(dnsmasq_conf)

def add_fixed_ip_configurations(stations):
    with open('/etc/dnsmasq.conf', 'a') as f:
        for station in stations:
            f.write(f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n")

def main():
    with open('TEST_BLUEPRINT.json', 'r') as f:
        blueprint = json.load(f)

    # Step 1: Create Application
    print("[STEP1] CREATE APPLICATION")
    dict_application_id = get_dict(str(CreateApplication(name=blueprint['name'], tenant_id=tenant_id)))
    application_id = dict_application_id["id"]

    # Step 2: Create Gateway
    print("[STEP2] CREATE GATEWAY")
    for gateway in blueprint['components']['lora_gateways']:
        CreateGateway(
            gateway_id=gateway['gateway_id'],
            name=gateway['name'],
            description=gateway['description'],
            tenant_id=tenant_id
        )

    # Step 3: Create Device Profile
    print("[STEP3] CREATE DEVICE PROFILE")
    dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
    device_profile_id = dict_device_profile_id["id"]

    # Step 4: Create All LoRaWAN Devices described in the blueprint
    print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")
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

    # Step 5: Configure WiFi Access Point if there are WiFi nodes or AP
    if 'wifi_nodes' in blueprint['components'] or 'wifi_ap' in blueprint['components']:
        configure_wifi_ap(ssid="SmartAgriculture", password="smartagri123")

    # Step 6: Add fixed IP configurations for each station
    stations = [
        {"wlan_MAC_ADDR": "00:11:22:33:44:55", "wlan_IP": "192.168.1.10"},
        {"wlan_MAC_ADDR": "00:11:22:33:44:66", "wlan_IP": "192.168.1.11"}
    ]
    add_fixed_ip_configurations(stations)

if __name__ == "__main__":
    main()


### Explanation:
1. **Parsing the JSON File**: The JSON file `TEST_BLUEPRINT.json` is read and parsed to extract the necessary information.
2. **Creating the Application**: The application is created using the `CreateApplication` function.
3. **Creating the Gateway**: The gateway is created using the `CreateGateway` function.
4. **Creating the Device Profile**: The device profile is created using the `CreateDeviceProfile` function.
5. **Creating LoRaWAN Devices**: All LoRaWAN devices described in the blueprint are created using the `CreateDevice` function.
6. **Configuring WiFi Access Point**: If there are WiFi nodes or AP, the WiFi Access Point is configured using the `configure_wifi_ap` function.
7. **Adding Fixed IP Configurations**: Fixed IP configurations for each station are added to `dnsmasq.conf` using the `add_fixed_ip_configurations` function.

### Note:
- Ensure that the `TEST_BLUEPRINT.json` file is in the same directory as the script.
- Adjust the SSID and password in the `configure_wifi_ap` function as needed.
- The `stations` list should be populated with the actual MAC addresses and IPs of the stations in your network.