
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import json
from fabric import Connection

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

def setup_wifi_ap(ap_config):
    host = ap_config['eth_IP']
    user = 'root'
    conn = Connection(host=host, user=user, connect_timeout=10)
    try:
      
        print(f"Configuring WiFi AP {ap_config['ap_id']} on {host}")

        # Generate hostapd.conf
        hostapd_conf = f"""
interface=wlan0
driver=nl80211
ssid={ap_config['ssid']}
hw_mode=g
channel=6
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa={'2' if 'WPA2' in ap_config['wpa_key_mgmt'] else '1'}
wpa_key_mgmt={ap_config['wpa_key_mgmt']}
rsn_pairwise=CCMP
wpa_passphrase={ap_config['wpa_passphrase']}
        """
        conn.run(f"echo '{hostapd_conf}' > /tmp/hostapd.conf")
        conn.run("sudo mv /tmp/hostapd.conf /etc/hostapd/hostapd.conf")

        # Generate dnsmasq.conf
        dnsmasq_conf = f"""
interface=wlan0
dhcp-range={ap_config['wlan_IP'].rsplit('.', 1)[0]}.100,{ap_config['wlan_IP'].rsplit('.', 1)[0]}.254,255.255.255.0,12h
server=8.8.8.8
"""
        conn.run(f"echo '{dnsmasq_conf}' > /tmp/dnsmasq.conf")
        conn.run("sudo mv /tmp/dnsmasq.conf /etc/dnsmasq.conf")

        conn.run("sudo systemctl restart hostapd")
        conn.run("sudo systemctl restart dnsmasq")
        conn.run("sudo ifconfig wlan0 up")
        conn.run(f"sudo ifconfig wlan0 {ap_config['wlan_IP']} netmask 255.255.255.0")

    except Exception as e:
        print(f"Error configuring AP {ap_config['ap_id']}: {e}")
    finally:
      conn.close()

def setup_wifi_station(station_config, dnsmasq_config):
    host = station_config['eth_IP']
    user = station_config['user']
    password = station_config['password']
    conn = Connection(host=host, user=user, connect_kwargs={"password": password}, connect_timeout=10)
    try:
      
        print(f"Configuring WiFi Station {station_config['station_id']} on {host}")

        # Configure wpa_supplicant.conf
        wpa_supplicant_conf = f"""
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1
country=GB

network={{
    ssid="{station_config['ssid']}"
    key_mgmt={station_config['wpa_key_mgmt']}
    psk="{station_config['wpa_passphrase']}"
}}
"""
        conn.run(f"echo '{wpa_supplicant_conf}' > /tmp/wpa_supplicant.conf")
        conn.run("sudo mv /tmp/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf")

        conn.run("sudo systemctl restart wpa_supplicant")
        conn.run("sleep 5") #wait the connection
        conn.run("sudo dhclient wlan0")
        
        conn.run(f"sudo ip address add {station_config['wlan_IP']}/24 dev wlan0")
    
        # Add station to dnsmasq.conf
        dnsmasq_config += f"dhcp-host={station_config['wlan_MAC_ADDR']},{station_config['wlan_IP']}\n"

        
        with open("/tmp/dnsmasq_config.tmp", "w") as file:
          file.write(dnsmasq_config)
        conn.put("/tmp/dnsmasq_config.tmp", "/tmp/dnsmasq_config.tmp")
        
        conn.run("sudo sh -c 'cat /tmp/dnsmasq_config.tmp >> /etc/dnsmasq.conf'")
        conn.run("sudo systemctl restart dnsmasq")
    except Exception as e:
        print(f"Error configuring Station {station_config['station_id']}: {e}")
    finally:
      conn.close()


def main():
    with open('test_blueprint.json', 'r') as f:
        blueprint = json.load(f)

    print("[STEP1] CREATE APPLICATION")
    dict_application_id = get_dict(str(CreateApplication(name="TEST-BLUEPRINT",tenant_id=tenant_id)))
    application_id=dict_application_id["id"]

    print("[STEP2] CREATE GATEWAY")
    for gw in blueprint['network_devices'].get('lora_gateways', []):
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
    for device in blueprint['network_devices'].get('lora_endpoints', []):
        print(CreateDevice(
            dev_eui=device['device_eui'],
            name=device['device_name'],
            application_id=application_id,
            device_profile_id=device_profile_id,
            application_key=device['security_key'],
            skip_fcnt_check=False,
            is_disabled=False
        ))
    
    print("[STEP5] CONFIGURING WIFI ACCESS POINTS")
    for ap in blueprint['network_devices'].get('wifi_access_points', []):
        setup_wifi_ap(ap)
    
    print("[STEP6] CONFIGURING WIFI STATIONS")
    dnsmasq_config=""
    for station in blueprint['network_devices'].get('wifi_stations', []):
        setup_wifi_station(station, dnsmasq_config)


if __name__ == "__main__":
    main()
