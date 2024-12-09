To generate the Python code for the TEST BLUEPRINT based on the provided SAMPLE PYTHON CODE and the TEST BLUEPRINT, we need to integrate the WiFi nodes using the `fabric` library for setting up AP and STA devices. Below is the Python code that accomplishes this:

python
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
from dotenv import load_dotenv
from fabric import Connection

# Load environment variables
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
    req.gateway.gateway_id = gateway_id
    req.gateway.name = name
    req.gateway.description = description
    req.gateway.tenant_id = tenant_id
    req.gateway.stats_interval = 60
    resp = client.Create(req, metadata=auth_token)
    return resp

def get_dict(str_application_id):
    k, v = str_application_id.split(': ')
    v = v.replace('"','').strip("\n")
    return {k: v}

def setup_wifi_ap(ip_address):
    c = Connection(host=ip_address, user='your_username', connect_kwargs={"password": "your_password"})
    c.sudo('apt-get update')
    c.sudo('apt-get install -y hostapd dnsmasq')
    c.sudo('systemctl stop hostapd')
    c.sudo('systemctl stop dnsmasq')
    c.put('hostapd.conf', '/etc/hostapd/hostapd.conf')
    c.put('dnsmasq.conf', '/etc/dnsmasq.conf')
    c.sudo('systemctl start hostapd')
    c.sudo('systemctl start dnsmasq')
    c.sudo('systemctl enable hostapd')
    c.sudo('systemctl enable dnsmasq')

def setup_wifi_sta(ip_address):
    c = Connection(host=ip_address, user='your_username', connect_kwargs={"password": "your_password"})
    c.sudo('apt-get update')
    c.sudo('apt-get install -y wpasupplicant')
    c.sudo('wpa_passphrase YOUR_SSID YOUR_PASSWORD > /etc/wpa_supplicant/wpa_supplicant.conf')
    c.sudo('wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf')
    c.sudo('dhclient wlan0')

# STEP1 create the application
print("[STEP1] CREATE APPLICATION")
dict_application_id = get_dict(str(CreateApplication(name="Small Smart City", tenant_id=tenant_id)))
application_id = dict_application_id["id"]

# STEP2 Create Gateway
print("[STEP2] CREATE GATEWAY")
CreateGateway(
    gateway_id="GW-001",
    name="LoRa Gateway",
    description="Gateway at City Hall",
    tenant_id=tenant_id
)

# STEP3 create Device profile
print("[STEP3] CREATE DEVICE PROFILE")
dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="SMART_CITY_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
device_profile_id = dict_device_profile_id["id"]

# STEP4 create LoRaWAN devices
print("[STEP4] CREATE LORAWAN DEVICES")
CreateDevice(
    dev_eui="IOT-001",
    name="Traffic Management Sensor",
    application_id=application_id,
    device_profile_id=device_profile_id,
    application_key="8BD59A2C514C9EBDAF7E335E78A5E5B5",
    skip_fcnt_check=False,
    is_disabled=False
)
CreateDevice(
    dev_eui="IOT-002",
    name="Waste Management Sensor",
    application_id=application_id,
    device_profile_id=device_profile_id,
    application_key="8AD59A1B514C9EBDAF7E335E78A5E5B4",
    skip_fcnt_check=False,
    is_disabled=False
)

# STEP5 setup WiFi AP
print("[STEP5] SETUP WIFI AP")
setup_wifi_ap(ip_address="192.168.1.1")

# STEP6 setup WiFi STA
print("[STEP6] SETUP WIFI STA")
setup_wifi_sta(ip_address="192.168.1.100")


### Explanation:
1. **Environment Setup**: Load environment variables for ChirpStack API.
2. **ChirpStack API Functions**: Define functions to interact with ChirpStack API for creating applications, gateways, device profiles, and devices.
3. **WiFi Setup Functions**: Define functions to set up WiFi AP and STA devices using the `fabric` library.
4. **Execution Steps**:
   - **STEP1**: Create the application.
   - **STEP2**: Create the LoRa Gateway.
   - **STEP3**: Create the device profile.
   - **STEP4**: Create LoRaWAN devices.
   - **STEP5**: Set up the WiFi AP.
   - **STEP6**: Set up the WiFi STA.

### Note:
- Replace `'your_username'` and `'your_password'` with actual credentials.
- Ensure the `hostapd.conf` and `dnsmasq.conf` files are correctly configured and available in the script's directory.
- Adjust the IP addresses and other parameters as per your network setup.