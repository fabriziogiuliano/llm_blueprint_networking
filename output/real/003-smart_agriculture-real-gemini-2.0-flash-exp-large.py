
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
import json
from fabric import Connection

#URL API CHIRPSTACK: https://www.chirpstack.io/docs/chirpstack/api/api.html


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
  #req.application_id = 'e9a065a4-73a0-437e-9711-69de6db90f14'
  req.limit = 1000 
  resp = client.List(req, metadata=auth_token)

  return resp


def ListDevices(application_id):
  client = api.DeviceServiceStub(channel)
  req = api.ListDevicesRequest()
  #req.tenant_id = '52f14cd4-c6f1-4fbd-8f87-4025e1d49242'
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
  #resp = None
  return resp

def GetApplicationID(name, tenant_id):
  client = api.ApplicationServiceStub(channel)
  req = api.CreateApplicationRequest()
  req = api.GetApp
  req.i
  req.application.name = name
  req.application.tenant_id = tenant_id
  resp = client.Create(req, metadata=auth_token)
  #resp = None
  return resp


def CreateDeviceProfile(name, tenant_id, region="EU868", mac_version="LORAWAN_1_0_3", revision ="RP002_1_0_3", supports_otaa=True):  
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
  req.device_profile.name=name
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
  req.id = id #"5f80f68b-0655-495f-94be-c7d72fa4b755"
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
  #req.device.tags.update({})    
  resp = client.Create(req, metadata=auth_token)
  
  #CreateDeviceKeysRequest
  req = api.CreateDeviceKeysRequest()
  req.device_keys.dev_eui = dev_eui
  req.device_keys.nwk_key = application_key
  req.device_keys.app_key = application_key
  resp = client.CreateKeys(req, metadata=auth_token)
  
  return resp  

def DeleteDevice(device_id):
  client = api.DeviceServiceStub(channel)
  req = api.DeleteDeviceRequest()
  req.dev_eui=device_id
  resp = client.Delete(req,metadata=auth_token)
  return resp

def CreateGateway(gateway_id, name, tenant_id, description=None, location=None):
  client = api.GatewayServiceStub(channel)
  req = api.CreateGatewayRequest()
  req.gateway.gateway_id=gateway_id
  req.gateway.name = name
  req.gateway.description =description
  #req.gateway.location=location
  req.gateway.tenant_id=tenant_id
  req.gateway.stats_interval=60
  resp = client.Create(req, metadata=auth_token)
  return resp
  
  
def DeleteGateway(gateway_id):
  client = api.GatewayServiceStub(channel)
  req = api.DeleteGatewayRequest()
  req.gateway_id=gateway_id  
  resp = client.Delete(req, metadata=auth_token)
  return resp  

def get_dict(str_application_id):
    k, v = str_application_id.split(': ')    
    v = v.replace('"','').strip("\n")
    # Crea il dizionario
    return {k: v}

def GetDeviceProfile(id):    
  client = api.DeviceProfileServiceStub(channel)
  req = api.GetDeviceProfileRequest()
  req.id = id #
  resp = client.Get(req, metadata=auth_token)
  return resp

def DeleteApplication(id):
  try:
    client = api.ApplicationServiceStub(channel)
    req = api.DeleteApplicationRequest()
    req.id=id
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

def configure_access_point(ap_config):
    hostapd_conf_path = "hostapd.conf"
    dnsmasq_conf_path = "dnsmasq.conf"

    with open(hostapd_conf_path, "w") as f:
        f.write("interface=" + ap_config["interface"] + "\n")
        f.write("driver=nl80211\n")
        f.write("ssid=" + ap_config["ssid"] + "\n")
        f.write("hw_mode=g\n")
        f.write("channel=" + str(ap_config["channel"]) + "\n")
        f.write("macaddr_acl=0\n")
        f.write("auth_algs=1\n")
        f.write("ignore_broadcast_ssid=0\n")
        f.write("wpa=2\n")
        f.write("wpa_passphrase=" + ap_config["password"] + "\n")
        f.write("wpa_key_mgmt=WPA-PSK\n")
        f.write("rsn_pairwise=CCMP\n")
    
    with open(dnsmasq_conf_path, "w") as f:
      f.write("interface=" + ap_config["interface"] + "\n")
      f.write("dhcp-range=" + ap_config["dhcp_start"] + "," + ap_config["dhcp_end"] + ",12h\n")
      f.write("dhcp-option=option:router," + ap_config["router"]+ "\n")
      for station in ap_config.get("stations", []):
        f.write(f"dhcp-host={station['wlan_mac_addr']},{station['wlan_ip']}\n")
      
    
    c = Connection(ap_config["hostname"])
    c.put(hostapd_conf_path, "/tmp/hostapd.conf")
    c.put(dnsmasq_conf_path, "/tmp/dnsmasq.conf")
    c.run("sudo cp /tmp/hostapd.conf /etc/hostapd/hostapd.conf")
    c.run("sudo cp /tmp/dnsmasq.conf /etc/dnsmasq.conf")
    c.run("sudo systemctl restart hostapd")
    c.run("sudo systemctl restart dnsmasq")
    c.run("sudo systemctl enable hostapd")
    c.run("sudo systemctl enable dnsmasq")

#print(ListGateways(tenant_id='52f14cd4-c6f1-4fbd-8f87-4025e1d49242'))

#print(ListApplications(tenant_id))
#print(ListDevices(application_id='e9a065a4-73a0-437e-9711-69de6db90f14'))
#print(GetDeviceProfile(id='3c8f08c9-b6ee-40f9-bfde-0ff7a0b292fc')) 
#print(ListDeviceProfiles(tenant_id=tenant_id))

#print()

# Load the test blueprint from the JSON file
with open('test_blueprint.json', 'r') as f:
    test_blueprint = json.load(f)

#STEP1 create the application
print("[STEP1] CREATE APPLICATION")

dict_application_id = get_dict(str(CreateApplication(name="TEST-BLUEPRINT",tenant_id=tenant_id)))
application_id=dict_application_id["id"]

#STEP2 Create Gateway
print("[STEP2] CREATE GATEWAY")
if test_blueprint["network_devices"].get("lora_gateways"):
  for gw in test_blueprint["network_devices"]["lora_gateways"]:
    print(DeleteGateway(gw["unique_id"]))
    CreateGateway(
      gateway_id=gw["unique_id"],
      name=gw["gateway_name"],
      description=gw["gateway_description"],
      tenant_id=tenant_id
    )


#STEP3 create Device profile:
print("[STEP3] CREATE DEVICE PROFILE")

dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0",mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
device_profile_id=dict_device_profile_id["id"]
print(device_profile_id)

print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")

#STEP4 create Device:
#EXAMPLE FOR ONE DEVICE
if test_blueprint["network_devices"].get("lora_endpoints"):
  for device in test_blueprint["network_devices"]["lora_endpoints"]:
    print(CreateDevice(
        dev_eui=device["device_eui"],
        name=device["device_name"],
        application_id=application_id,
        device_profile_id=device_profile_id,
        application_key=device["security_key"],
        skip_fcnt_check=False,
        is_disabled=False
    ))

#STEP 5 Configure WiFi Access Point
print("[STEP5] Configure WiFi Access Point")
if "access_points" in test_blueprint["network_devices"]:
    for ap_config in test_blueprint["network_devices"]["access_points"]:
        configure_access_point(ap_config)
