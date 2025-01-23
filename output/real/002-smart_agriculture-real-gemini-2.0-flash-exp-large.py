
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import json
from fabric import Connection

from dotenv import load_dotenv
load_dotenv()
server = os.getenv('CHIRPSTACK_URL')  # Adjust this to your ChirpStack instance
api_token = os.getenv('CHIRPSTACK_API_TOKEN')  # Obtain from ChirpStack
tenant_id = os.getenv('CHIRPSTACK_TENANT_ID') #this is a tenant ID for fabrizio.giuliano@unipa.it
wifi_ap_interface = os.getenv('WIFI_AP_INTERFACE')
wifi_ap_ssid = os.getenv('WIFI_AP_SSID')
wifi_ap_passphrase = os.getenv('WIFI_AP_PASSPHRASE')
dnsmasq_conf_path = os.getenv('DNSMASQ_CONF_PATH')
hostapd_conf_path = os.getenv('HOSTAPD_CONF_PATH')


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
  req = api.CreateApplicationRequest()
  req = api.GetApp
  req.i
  req.application.name = name
  req.application.tenant_id = tenant_id
  resp = client.Create(req, metadata=auth_token)
  return resp


def CreateDeviceProfile(name, tenant_id, region="EU868", mac_version="LORAWAN_1_0_3", revision ="RP002_1_0_3", supports_otaa=True):  
  client = api.DeviceProfileServiceStub(channel)
  
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
  req.id = id
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
  req.dev_eui=device_id
  resp = client.Delete(req,metadata=auth_token)
  return resp

def CreateGateway(gateway_id, name, tenant_id, description=None, location=None):
  client = api.GatewayServiceStub(channel)
  req = api.CreateGatewayRequest()
  req.gateway.gateway_id=gateway_id
  req.gateway.name = name
  req.gateway.description =description
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

def configure_hostapd(interface, ssid, passphrase, conf_path):
    config = f"""
interface={interface}
driver=nl80211
ssid={ssid}
hw_mode=g
channel=6
ieee80211n=1
wmm_enabled=1
ht_capab=[HT40][SHORT-GI-20][DSSS_CCK-40]
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
wpa_passphrase={passphrase}
"""
    with open(conf_path, 'w') as f:
        f.write(config)

def configure_dnsmasq(conf_path, fixed_ips=[]):
    config = f"""
interface={wifi_ap_interface}
dhcp-range=192.168.4.100,192.168.4.250,12h
"""
    for host, ip in fixed_ips:
      config += f"dhcp-host={host},{ip}\n"
    
    with open(conf_path, 'w') as f:
        f.write(config)
        

def setup_wifi_ap(interface, ssid, passphrase, hostapd_conf_path, dnsmasq_conf_path, fixed_ips=[]):
    configure_hostapd(interface, ssid, passphrase, hostapd_conf_path)
    configure_dnsmasq(dnsmasq_conf_path, fixed_ips)
    
    c = Connection('localhost')
    c.sudo(f'systemctl stop hostapd')
    c.sudo(f'systemctl stop dnsmasq')
    c.sudo(f'systemctl enable hostapd')
    c.sudo(f'systemctl enable dnsmasq')
    c.sudo(f'systemctl restart hostapd')
    c.sudo(f'systemctl restart dnsmasq')


def main():
  
  with open('test_blueprint.json', 'r') as f:
      blueprint = json.load(f)
  
  lora_gateways = blueprint.get('network_devices', {}).get('lora_gateways', [])
  lora_endpoints = blueprint.get('network_devices', {}).get('lora_endpoints', [])
  
  #STEP1 create the application
  print("[STEP1] CREATE APPLICATION")

  dict_application_id = get_dict(str(CreateApplication(name="TEST-BLUEPRINT",tenant_id=tenant_id)))
  application_id=dict_application_id["id"]
  
  if lora_gateways:
      #STEP2 Create Gateway
      print("[STEP2] CREATE GATEWAY")
      for gw in lora_gateways:
         print(DeleteGateway(gw['unique_id']))
         CreateGateway(
          gateway_id=gw['unique_id'],
          name=gw['gateway_name'],
          description=gw['gateway_description'],
          tenant_id=tenant_id
        )
  if lora_endpoints:
    #STEP3 create Device profile:
    print("[STEP3] CREATE DEVICE PROFILE")

    dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0",mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
    device_profile_id=dict_device_profile_id["id"]
    print(device_profile_id)
    
    print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")
    #STEP4 create Device:
    for device in lora_endpoints:
        print(CreateDevice(
            dev_eui=device['device_eui'],
            name=device['device_name'],
            application_id=application_id,
            device_profile_id=device_profile_id,
            application_key=device['security_key'],
            skip_fcnt_check=False,
            is_disabled=False
        ))
  
  wifi_access_points = blueprint.get('network_devices', {}).get('wifi_access_points',[])
  wifi_nodes = blueprint.get('network_devices', {}).get('wifi_nodes',[])
    
  if wifi_access_points:
      fixed_ips_list=[]
      for node in wifi_nodes:
          fixed_ips_list.append((node['wlan_mac_address'], node['wlan_ip']))
      setup_wifi_ap(wifi_ap_interface, wifi_ap_ssid, wifi_ap_passphrase, hostapd_conf_path, dnsmasq_conf_path, fixed_ips_list)    

if __name__ == "__main__":
    main()
