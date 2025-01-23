
import os
import json
import subprocess
from fabric import Connection
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

def CreateDeviceProfile(name, tenant_id, region="EU868", mac_version="LORAWAN_1_0_3", revision ="RP002_1_0_3", supports_otaa=True):  
  client = api.DeviceProfileServiceStub(channel)
  req = api.CreateDeviceProfileRequest()
  req.device_profile.name=name
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

def get_dict(str_application_id):
    k, v = str_application_id.split(': ')    
    v = v.replace('"','').strip("\n")
    return {k: v}

def DeleteGateway(gateway_id):
    client = api.GatewayServiceStub(channel)
    req = api.DeleteGatewayRequest()
    req.gateway_id=gateway_id
    resp = client.Delete(req, metadata=auth_token)
    return resp  

def DeleteDevice(device_id):
  client = api.DeviceServiceStub(channel)
  req = api.DeleteDeviceRequest()
  req.dev_eui=device_id
  resp = client.Delete(req,metadata=auth_token)
  return resp

def DeleteDviceProfile(id):
  client = api.DeviceProfileServiceStub(channel)
  req = api.DeleteDeviceProfileRequest()
  req.id = id
  resp = client.Delete(req, metadata=auth_token)
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

def configure_ap(ap_config):
    ap_id = ap_config['ap_id']
    ssid = ap_config['ssid']
    wpa_passphrase = str(ap_config['wpa_passphrase'])
    wpa_key_mgmt = ap_config['wpa_key_mgmt']
    wlan_ip = ap_config['wlan_IP']
    eth_ip = ap_config['eth_IP']
    
    hostapd_conf = f"""
interface=wlan0
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
wpa_key_mgmt={wpa_key_mgmt}
wpa_pairwise=TKIP CCMP
rsn_pairwise=CCMP
wpa_passphrase={wpa_passphrase}
"""
    dnsmasq_conf = f"""
interface=wlan0
dhcp-range={wlan_ip},100,150,12h
dhcp-authoritative
"""
    
    with open(f'/tmp/hostapd_{ap_id}.conf', 'w') as f:
        f.write(hostapd_conf)
    with open(f'/tmp/dnsmasq_{ap_id}.conf', 'w') as f:
        f.write(dnsmasq_conf)

    
    subprocess.run(['sudo', 'ip', 'addr', 'add', f'{wlan_ip}/24', 'dev', 'wlan0'], check=True)
    subprocess.run(['sudo', 'systemctl', 'stop', 'hostapd'], check=False)
    subprocess.run(['sudo', 'systemctl', 'stop', 'dnsmasq'], check=False)
    subprocess.run(['sudo', 'hostapd', f'/tmp/hostapd_{ap_id}.conf', '-B'], check=True)
    subprocess.run(['sudo', 'dnsmasq', f'--conf-file=/tmp/dnsmasq_{ap_id}.conf', '-B'], check=True)
    print(f"AP {ap_id} configured with SSID: {ssid}, IP: {wlan_ip}")


def configure_stations(stations_config):
    dnsmasq_conf_stations = ""
    for station in stations_config:
         wlan_mac_addr = station['wlan_MAC_ADDR']
         wlan_ip = station['wlan_IP']
         dnsmasq_conf_stations += f"dhcp-host={wlan_mac_addr},{wlan_ip}\n"

    if dnsmasq_conf_stations:
         with open('/tmp/dnsmasq_stations.conf', 'w') as f:
            f.write(dnsmasq_conf_stations)
         subprocess.run(['sudo', 'dnsmasq', f'--conf-file=/tmp/dnsmasq_stations.conf', '-B'], check=False)   
         print("Stations DNS configurations created.")

if __name__ == "__main__":
    with open('test_blueprint.json', 'r') as f:
        blueprint = json.load(f)

    lora_gateways = blueprint['network_devices'].get('lora_gateways', [])
    lora_endpoints = blueprint['network_devices'].get('lora_endpoints', [])
    wifi_access_points = blueprint['network_devices'].get('wifi_access_points', [])
    wifi_stations = blueprint['network_devices'].get('wifi_stations', [])


    if lora_gateways or lora_endpoints:

        print("[STEP1] CREATE APPLICATION")
        dict_application_id = get_dict(str(CreateApplication(name="TEST-BLUEPRINT",tenant_id=tenant_id)))
        application_id=dict_application_id["id"]
      
        print("[STEP2] CREATE GATEWAY")
        for gw in lora_gateways:
            print(DeleteGateway(gw['unique_id']))
            CreateGateway(
                gateway_id=gw['unique_id'],
                name=gw['gateway_name'],
                description=gw['gateway_description'],
                tenant_id=tenant_id
            )
        
        print("[STEP3] CREATE DEVICE PROFILE")
        dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0",mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
        device_profile_id=dict_device_profile_id["id"]
        print(device_profile_id)

        print("[STEP4] CREATE ALL LORAWAN DEVICES described in the blueprint")
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
    else:
        print("No LoRa devices found in the blueprint.")
    
    if wifi_access_points:
       print("[STEP5] Configure WiFi Access Points")
       for ap_config in wifi_access_points:
          configure_ap(ap_config)
    else:
        print("No WiFi APs found in the blueprint.")
    
    if wifi_stations:
      print("[STEP6] Configure WiFi Stations")
      configure_stations(wifi_stations)
    else:
       print("No WiFi Stations found in the blueprint.")

