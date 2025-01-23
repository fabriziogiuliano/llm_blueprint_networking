
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


def setup_wifi_ap(ap_config):
    try:
        ap_id = ap_config["id"]
        ssid = ap_config["ssid"]
        wpa_passphrase = str(ap_config["wpa_passphrase"])
        wpa_key_mgmt = ap_config["wpa_key_mgmt"]
        wlan_ip = ap_config["wlan_IP"]
        eth_ip = ap_config["eth_IP"]
        
        hostapd_conf = f"""
        interface=wlan0
        driver=nl80211
        ssid={ssid}
        hw_mode=g
        channel=7
        ieee80211n=1
        wmm_enabled=1
        ht_capab=[HT40][SHORT-GI-20][DSSS_CCK-40]
        macaddr_acl=0
        auth_algs=1
        ignore_broadcast_ssid=0
        wpa=2
        wpa_key_mgmt={wpa_key_mgmt}
        wpa_pairwise=TKIP
        rsn_pairwise=CCMP
        wpa_passphrase={wpa_passphrase}
        """
        
        dnsmasq_conf = f"""
        interface=wlan0
        dhcp-range={wlan_ip.rsplit('.', 1)[0]}.100,{wlan_ip.rsplit('.', 1)[0]}.200,255.255.255.0,12h
        dhcp-option=option:router,{wlan_ip}
        """

        with open("/tmp/hostapd.conf", "w") as f:
            f.write(hostapd_conf)
        with open("/tmp/dnsmasq.conf", "w") as f:
            f.write(dnsmasq_conf)

        c = Connection(host='localhost')
        c.sudo("ip addr add {}/24 dev wlan0".format(wlan_ip))
        c.sudo("systemctl stop hostapd")
        c.sudo("systemctl stop dnsmasq")
        c.sudo("cp /tmp/hostapd.conf /etc/hostapd/hostapd.conf")
        c.sudo("cp /tmp/dnsmasq.conf /etc/dnsmasq.conf")
        c.sudo("systemctl enable hostapd")
        c.sudo("systemctl enable dnsmasq")
        c.sudo("systemctl restart hostapd")
        c.sudo("systemctl restart dnsmasq")
        print(f"WiFi AP {ap_id} configured successfully.")
        return True

    except Exception as e:
        print(f"Error setting up WiFi AP {ap_id}: {e}")
        return False

def setup_wifi_stations(stations_config):
    try:
        dnsmasq_conf_additions = ""
        for station in stations_config:
             wlan_mac = station["wlan_MAC_ADDR"]
             wlan_ip = station["wlan_IP"]
             dnsmasq_conf_additions += f"dhcp-host={wlan_mac},{wlan_ip}\n"
            
        if dnsmasq_conf_additions:
            c = Connection(host='localhost')
            c.sudo(f"echo '{dnsmasq_conf_additions}' >> /etc/dnsmasq.conf")
            c.sudo("systemctl restart dnsmasq")
            print("WiFi Stations configured in dnsmasq.conf.")
        return True

    except Exception as e:
        print(f"Error configuring WiFi Stations: {e}")
        return False


def main():
  
    with open("TEST_BLUEPRINT.json", 'r') as f:
      blueprint = json.load(f)

    lora_gateways = blueprint["components"].get("lora_gateways", [])
    lora_devices = blueprint["components"].get("lora_devices", [])
    wifi_aps = blueprint["components"].get("wifi_ap", [])
    wifi_stations = blueprint["components"].get("wifi_stations", [])

    print("[STEP1] CREATE APPLICATION")
    dict_application_id = get_dict(str(CreateApplication(name="TEST-BLUEPRINT",tenant_id=tenant_id)))
    application_id=dict_application_id["id"]

    if lora_gateways:
        print("[STEP2] CREATE LORAWAN GATEWAY(S)")
        for gw_config in lora_gateways:
            gateway_id = gw_config["gateway_id"]
            name = gw_config["name"]
            description = gw_config["description"]
            print(DeleteGateway(gateway_id))
            CreateGateway(gateway_id=gateway_id, name=name, description=description, tenant_id=tenant_id)
        
    if lora_devices:
        print("[STEP3] CREATE DEVICE PROFILE")
        dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0",mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
        device_profile_id=dict_device_profile_id["id"]
        print(device_profile_id)
        print("[STEP4] CREATE ALL LORAWAN DEVICES")
        for device_config in lora_devices:
            dev_eui = device_config["dev_eui"]
            name = device_config["name"]
            application_key = device_config["application_key"]
            print(CreateDevice(
                dev_eui=dev_eui,
                name=name,
                application_id=application_id,
                device_profile_id=device_profile_id,
                application_key=application_key,
                skip_fcnt_check=False,
                is_disabled=False
            ))

    if wifi_aps:
        print("[STEP5] SET UP WIFI ACCESS POINT(S)")
        for ap_config in wifi_aps:
           setup_wifi_ap(ap_config)
    
    if wifi_stations:
        print("[STEP6] SET UP WIFI STATION(S) DNSMASQ")
        setup_wifi_stations(wifi_stations)

if __name__ == "__main__":
    main()
