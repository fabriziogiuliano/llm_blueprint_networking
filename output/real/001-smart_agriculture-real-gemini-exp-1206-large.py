
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
import json

from dotenv import load_dotenv
from fabric import Connection

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

def GetApplicationID(name, tenant_id):
    client = api.ApplicationServiceStub(channel)
    req = api.ListApplicationsRequest()
    req.tenant_id = tenant_id
    req.limit = 1000
    resp = client.List(req, metadata=auth_token)
    for app in resp.result:
        if app.name == name:
            return app.id
    return None

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

def DeleteDeviceProfile(id):
    client = api.DeviceProfileServiceStub(channel)
    req = api.DeleteDeviceProfileRequest()
    req.id = id
    resp = client.Delete(req, metadata=auth_token)
    return resp

def CreateDevice(dev_eui, name, application_id, device_profile_id, application_key, skip_fcnt_check=True, is_disabled=False):
    client = api.DeviceServiceStub(channel)
    req = api.CreateDeviceRequest()
    req.device.dev_eui = dev_eui
    req.device.name = name
    req.device.description = ""
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
    if location:
        req.gateway.location.latitude = location.latitude
        req.gateway.location.longitude = location.longitude
        req.gateway.location.altitude = location.altitude
        
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
    return {k: v}

def GetDeviceProfile(id):
    client = api.DeviceProfileServiceStub(channel)
    req = api.GetDeviceProfileRequest()
    req.id = id
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

def configure_wifi_ap(ap_details, stations):
    if not ap_details:
        return

    hostapd_conf_content = f"""
interface={ap_details['interface']}
ssid={ap_details['ssid']}
hw_mode=g
channel=7
macaddr_acl=0
ignore_broadcast_ssid=0
auth_algs=1
wpa=2
wpa_passphrase={ap_details['password']}
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
    """

    dnsmasq_conf_content = f"""
interface={ap_details['interface']}
dhcp-range={ap_details['dhcp_range_start']},{ap_details['dhcp_range_end']},12h
    """
    
    for station in stations:
        dnsmasq_conf_content += f"dhcp-host={station['mac']},{station['ip']}\n"

    with Connection(host=ap_details['host'], user=ap_details['user'], connect_kwargs={"password": ap_details['password']}) as c:
        c.sudo("systemctl stop hostapd")
        c.sudo("systemctl stop dnsmasq")
        c.put(hostapd_conf_content, remote="/tmp/hostapd.conf")
        c.sudo("mv /tmp/hostapd.conf /etc/hostapd/hostapd.conf")
        c.put(dnsmasq_conf_content, remote="/tmp/dnsmasq.conf")
        c.sudo("mv /tmp/dnsmasq.conf /etc/dnsmasq.conf")
        c.sudo("systemctl start hostapd")
        c.sudo("systemctl start dnsmasq")

with open("TEST_BLUEPRINT.json", "r") as f:
    blueprint = json.load(f)

print("[STEP1] CREATE APPLICATION")
application_name = blueprint["name"]
application_id = GetApplicationID(name=application_name, tenant_id=tenant_id)
if application_id is None:
    dict_application_id = get_dict(str(CreateApplication(name=application_name, tenant_id=tenant_id)))
    application_id = dict_application_id["id"]

print(f"[INFO] Application ID: {application_id}")

print("[STEP2] CREATE GATEWAY")
for gw in blueprint["components"]["lora_gateways"]:
    location = api.Location()
    location.latitude = gw["latitude"]
    location.longitude = gw["longitude"]
    location.altitude = gw["altitude"]
    
    DeleteGateway(gw["gateway_id"])
    CreateGateway(
        gateway_id=gw["gateway_id"],
        name=gw["name"],
        description=gw["description"],
        tenant_id=tenant_id,
        location=location
    )

print("[STEP3] CREATE DEVICE PROFILE")
dict_device_profile_id = get_dict(
    str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3",
                             tenant_id=tenant_id)))
device_profile_id = dict_device_profile_id["id"]
print(f"[INFO] Device Profile ID: {device_profile_id}")

print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")
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
    
wifi_ap_config = blueprint.get("wifi_ap", None)
wifi_stations = blueprint.get("wifi_stations", [])

configure_wifi_ap(wifi_ap_config, wifi_stations)
