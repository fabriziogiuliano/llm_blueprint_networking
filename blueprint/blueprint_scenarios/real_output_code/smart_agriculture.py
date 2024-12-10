Based on the provided SAMPLE PYTHON CODE and the TEST BLUEPRINT, here is the generated PYTHON CODE to set up the network configuration for the "Smart Agriculture" blueprint. This code will create the necessary LoRaWAN devices and gateways as specified in the NETWORK CONFIGURATION.

python
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
from dotenv import load_dotenv

load_dotenv()
server = os.getenv('CHIRPSTACK_URL')  # Adjust this to your ChirpStack instance
api_token = os.getenv('CHIRPSTACK_API_TOKEN')  # Obtain from ChirpStack
tenant_id = os.getenv('CHIRPSTACK_TENANT_ID') #this is a tenant ID for fabrizio.giuliano@unipa.it

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

# STEP1 create the application
print("[STEP1] CREATE APPLICATION")
dict_application_id = get_dict(str(CreateApplication(name="Smart Agriculture", tenant_id=tenant_id)))
application_id = dict_application_id["id"]

# STEP2 Create Gateway
print("[STEP2] CREATE GATEWAY")
CreateGateway(
    gateway_id="05b0da50148fd6b1",
    name="GW-RAK-ROOF",
    description="Gateway Building 9",
    tenant_id=tenant_id
)

# STEP3 create Device profile
print("[STEP3] CREATE DEVICE PROFILE")
dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="Smart_Agriculture_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
device_profile_id = dict_device_profile_id["id"]
print(device_profile_id)

# STEP4 create all LoRaWAN devices described in the blueprint
print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")

lora_devices = [
    {"dev_eui": "54c3b79eed93ec98", "name": "FMS-001", "application_key": "8BD59A2C514C9EBDAF7E335E78A5E5B4"},
    {"dev_eui": "14c3a79eed93ec98", "name": "SMS-001", "application_key": "9CD59A3D514C9EBDAF7E335E78A5E5B5"},
    {"dev_eui": "14c3a79eed93ec98", "name": "WS-001", "application_key": "9CD59A3D514C9EBDAF7E335E78A5E5B6"},
    {"dev_eui": "14c3a79eed93ec98", "name": "IC-001", "application_key": "9CD59A3D514C9EBDAF7E335E78A5E5B7"},
    {"dev_eui": "1411a79e1193ec98", "name": "DR-001", "application_key": "9CD59A3D514C9EBDAF7E335E78A5E5B8"}
]

for device in lora_devices:
    print(CreateDevice(
        dev_eui=device["dev_eui"],
        name=device["name"],
        application_id=application_id,
        device_profile_id=device_profile_id,
        application_key=device["application_key"],
        skip_fcnt_check=False,
        is_disabled=False
    ))

# WIFI PART
# ... TBD


This code follows the steps outlined in the SAMPLE PYTHON CODE to create an application, gateway, device profile, and devices based on the TEST BLUEPRINT and NETWORK CONFIGURATION provided. The WiFi part is left as "TBD" since the configuration details for WiFi AP and stations are not provided.