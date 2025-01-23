
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
import json
from fabric import Connection
from fabric import Config

# URL API CHIRPSTACK: https://www.chirpstack.io/docs/chirpstack/api/api.html

from dotenv import load_dotenv

load_dotenv()
server = os.getenv('CHIRPSTACK_URL')  # Adjust this to your ChirpStack instance
api_token = os.getenv('CHIRPSTACK_API_TOKEN')  # Obtain from ChirpStack
tenant_id = os.getenv('CHIRPSTACK_TENANT_ID')  # this is a tenant ID for fabrizio.giuliano@unipa.it

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
    # req.application_id = 'e9a065a4-73a0-437e-9711-69de6db90f14'
    req.limit = 1000
    resp = client.List(req, metadata=auth_token)

    return resp

def ListDevices(application_id):
    client = api.DeviceServiceStub(channel)
    req = api.ListDevicesRequest()
    # req.tenant_id = '52f14cd4-c6f1-4fbd-8f87-4025e1d49242'
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
    # resp = None
    return resp

def GetApplicationID(name, tenant_id):
    client = api.ApplicationServiceStub(channel)
    req = api.CreateApplicationRequest()
    req = api.GetApp
    req.i
    req.application.name = name
    req.application.tenant_id = tenant_id
    resp = client.Create(req, metadata=auth_token)
    # resp = None
    return resp

def CreateDeviceProfile(name, tenant_id, region="EU868", mac_version="LORAWAN_1_0_3", revision="RP002_1_0_3",
                        supports_otaa=True):
    client = api.DeviceProfileServiceStub(channel)

    """
      from: https://pkg.go.dev/github.com/chirpstack/chirpstack/api/go/v4/common#MacVersion
      MacVersion_LORAWAN_1_0_0 MacVersion = 0
      MacVersion_LORAWAN_1_0_1 MacVersion = 1
      MacVersion_LORAWAN_1_0_2 MacVersion = 2
      MacVersion_LORAWAN_1_0_3 MacVersion = 3
      MacVersion_LORAWAN_1_0_4 MacVersion = 4
      MacVersion_LORAWAN_1_1_0 MacVersion = 5
    """
    req = api.CreateDeviceProfileRequest()
    req.device_profile
    req.device_profile.name = name
    req.device_profile.tenant_id = tenant_id
    req.device_profile.region = region
    req.device_profile.reg_params_revision = revision
    req.device_profile.mac_version = mac_version
    req.device_profile.supports_otaa = supports_otaa
    resp = client.Create(req, metadata=auth_token)
    return resp

def DeleteDviceProfile(id):
    client = api.DeviceProfileServiceStub(channel)
    req = api.DeleteDeviceProfileRequest()
    req.id = id  # "5f80f68b-0655-495f-94be-c7d72fa4b755"
    resp = client.Delete(req, metadata=auth_token)
    return resp

def CreateDevice(
        dev_eui,
        name,
        application_id,
        device_profile_id,
        application_key,
        skip_fcnt_check=True,
        is_disabled=False
):
    client = api.DeviceServiceStub(channel)

    req = api.CreateDeviceRequest()
    req.device.dev_eui = dev_eui
    req.device.name = name
    req.device.description = ""
    req.device.application_id = application_id
    req.device.device_profile_id = device_profile_id
    req.device.skip_fcnt_check = skip_fcnt_check
    req.device.is_disabled = is_disabled
    # req.device.tags.update({})
    resp = client.Create(req, metadata=auth_token)

    # CreateDeviceKeysRequest
    req = api.CreateDeviceKeysRequest()
    req.device_keys.dev_eui = dev_eui
    req.device_keys.nwk_key = application_key
    req.device_keys.app_key = application_key
    resp = client.CreateKeys(req, metadata=auth_token)

    return resp

def DeleteDevice(device_id):
    client = api.DeviceServiceStub(channel)
    req = api.DeleteDeviceRequest()
    req.dev_eui = device_id
    resp = client.Delete(req, metadata=auth_token)
    return resp

def CreateGateway(gateway_id, name, tenant_id, description=None, location=None):
    client = api.GatewayServiceStub(channel)
    req = api.CreateGatewayRequest()
    req.gateway.gateway_id = gateway_id
    req.gateway.name = name
    req.gateway.description = description
    # req.gateway.location=location
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
    # Crea il dizionario
    return {k: v}

def GetDeviceProfile(id):
    client = api.DeviceProfileServiceStub(channel)
    req = api.GetDeviceProfileRequest()
    req.id = id  #
    resp = client.Get(req, metadata=auth_token)
    return resp

def DeleteApplication(id):
    try:
        client = api.ApplicationServiceStub(channel)
        req = api.DeleteApplicationRequest()
        req.id = id
        resp = client.Delete(req, metadata=auth_token)
        return resp
    except Exception:
        print(traceback.format_exc())

def ListDeviceProfiles(tenant_id):
    client = api.DeviceProfileServiceStub(channel)
    req = api.ListDeviceProfilesRequest()
    req.tenant_id = tenant_id
    req.limit = 1000
    resp = client.List(req, metadata=auth_token)
    return resp

def configure_wifi_ap(ap_config, stations):
    
    
    hostapd_conf_path = "/etc/hostapd/hostapd.conf"
    dnsmasq_conf_path = "/etc/dnsmasq.conf"

    hostapd_conf_content = f"""
interface={ap_config['interface']}
ssid={ap_config['ssid']}
hw_mode=g
channel={ap_config['channel']}
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase={ap_config['password']}
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
    """

    dnsmasq_conf_content = f"""
interface={ap_config['interface']}
dhcp-range={ap_config['dhcp_range_start']},{ap_config['dhcp_range_end']},12h
    """
    
    for station in stations:
      dnsmasq_conf_content += f"\ndhcp-host={station['mac']},{station['ip']}"

    
    fabric_config = Config(overrides={'sudo': {'password': ap_config['password']}})
    with Connection(host=ap_config['hostname'], user=ap_config['username'], config=fabric_config) as c:        
        
        
        with c.sudo():  
            
            c.put(hostapd_conf_content, hostapd_conf_path)
            c.put(dnsmasq_conf_content, dnsmasq_conf_path)
            c.run("systemctl restart hostapd")
            c.run("systemctl restart dnsmasq")

# print(ListGateways(tenant_id='52f14cd4-c6f1-4fbd-8f87-4025e1d49242'))

# print(ListApplications(tenant_id))
# print(ListDevices(application_id='e9a065a4-73a0-437e-9711-69de6db90f14'))
# print(GetDeviceProfile(id='3c8f08c9-b6ee-40f9-bfde-0ff7a0b292fc'))
# print(ListDeviceProfiles(tenant_id=tenant_id))

# print()

# Load Blueprint from JSON file
blueprint_file = "test_blueprint.json"  # Replace with your blueprint file

try:
    with open(blueprint_file, "r") as f:
        blueprint = json.load(f)
except FileNotFoundError:
    print(f"Error: Blueprint file '{blueprint_file}' not found.")
    sys.exit(1)

# STEP1 create the application
print("[STEP1] CREATE APPLICATION")

dict_application_id = get_dict(
    str(CreateApplication(name=blueprint["name"], tenant_id=tenant_id)))
application_id = dict_application_id["id"]

# STEP2 Create Gateway (if present in the blueprint)
if "lora_gateways" in blueprint["components"]:
    print("[STEP2] CREATE GATEWAY")
    for gw in blueprint["components"]["lora_gateways"]:
        print(DeleteGateway(gw["gateway_id"]))
        CreateGateway(
            gateway_id=gw["gateway_id"],
            name=gw["name"],
            description=gw["description"],
            tenant_id=tenant_id
        )

# STEP3 create Device profile:
print("[STEP3] CREATE DEVICE PROFILE")

dict_device_profile_id = get_dict(
    str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3",
                            tenant_id=tenant_id)))
device_profile_id = dict_device_profile_id["id"]
print(device_profile_id)

print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")

# STEP4 create Device:
if "lora_devices" in blueprint["components"]:
    for device in blueprint["components"]["lora_devices"]:
        print(CreateDevice(
            dev_eui=device["dev_eui"],
            name=device["name"],
            application_id=application_id,
            device_profile_id=device_profile_id,
            application_key=device["application_key"],
            skip_fcnt_check=False,
            is_disabled=False
        ))

# STEP5 Configure WiFi AP (if present in the blueprint)
if "wifi_access_point" in blueprint["components"]:
    print("[STEP5] CONFIGURE WIFI ACCESS POINT")
    ap_config = blueprint["components"]["wifi_access_point"]
    
    stations = []
    if "wifi_stations" in blueprint["components"]:
        stations = blueprint["components"]["wifi_stations"]
        
    configure_wifi_ap(ap_config, stations)
