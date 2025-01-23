
import os
import sys
import traceback
import grpc
from chirpstack_api import api
import random
import json
from fabric import Connection, Config
from fabric.group import Group
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

def configure_access_point(ap_config, dnsmasq_config):
    """Configures the WiFi Access Point using Fabric."""
    
    try:
        # Configuration for the target host (replace with your AP's details)
        target_host = ap_config["host"] 
        target_user = ap_config["user"]
        target_password = ap_config["password"]
        
        config = Config(overrides={"sudo": {"password": target_password}})
        
        ap = Connection(
                host=target_host,
                user=target_user,
                config=config
                )
        
        # Create hostapd config
        ap.sudo(f'echo "{ap_config["hostapd_config"]}" > /etc/hostapd/hostapd.conf')
        
        # Create dnsmasq config and add fixed ip configuration
        dnsmasq_conf = dnsmasq_config["dnsmasq_config"]
        for station in dnsmasq_config["stations"]:
              dnsmasq_conf += f'\ndhcp-host={station["wlan_mac_addr"]},{station["wlan_ip"]}'
        ap.sudo(f'echo "{dnsmasq_conf}" > /etc/dnsmasq.conf')

        # Restart services
        ap.sudo("systemctl restart hostapd")
        ap.sudo("systemctl restart dnsmasq")
        print("WiFi Access Point configured successfully.")
        
    except Exception as e:
        print(f"Error configuring WiFi Access Point: {e}")

def load_blueprint(file_path):
    """Loads the network blueprint from a JSON file."""
    with open(file_path, 'r') as f:
        return json.load(f)
    

if __name__ == "__main__":

    blueprint_file = "TEST BLUEPRINT.json"
    blueprint = load_blueprint(blueprint_file)

    # STEP 1: Create Application
    print("[STEP1] CREATE APPLICATION")
    dict_application_id = get_dict(str(CreateApplication(name=blueprint["name"], tenant_id=tenant_id)))
    application_id = dict_application_id["id"]

    # STEP 2: Create LoRa Gateways
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

    # STEP 3: Create Device Profile
    print("[STEP3] CREATE DEVICE PROFILE")
    dict_device_profile_id = get_dict(str(CreateDeviceProfile(name="TEST_OTAA_EU868_1.1.0", mac_version="LORAWAN_1_1_0", revision="RP002_1_0_3", tenant_id=tenant_id)))
    device_profile_id = dict_device_profile_id["id"]
    print(device_profile_id)

    # STEP 4: Create LoRa Devices
    if "lora_devices" in blueprint["components"]:
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

    # STEP 5: Configure WiFi Access Point
    if "wifi_access_point" in blueprint["components"]:
      print("[STEP5] CONFIGURE WIFI ACCESS POINT")
      ap_config = blueprint["components"]["wifi_access_point"]["ap_config"]
      dnsmasq_config_data = blueprint["components"]["wifi_access_point"]["dnsmasq_config"]
      dnsmasq_config = {"dnsmasq_config": dnsmasq_config_data, "stations": []}

      if "wifi_stations" in blueprint["components"]:
        for station in blueprint["components"]["wifi_stations"]:
            dnsmasq_config["stations"].append({"wlan_mac_addr": station["wlan_mac_addr"], "wlan_ip": station["wlan_ip"]})
            
      configure_access_point(ap_config, dnsmasq_config)


**Explanation:**

1.  **Import Libraries:**
    *   `os`, `sys`, `traceback`, `grpc`, `json`: For general functionalities, error handling, gRPC communication, and JSON processing.
    *   `chirpstack_api`: For interacting with ChirpStack.
    *   `fabric`: For managing the WiFi access point.
    *   `dotenv`: For loading environment variables.
2.  **ChirpStack Configuration:**
    *   Loads server URL, API token, and tenant ID from environment variables.
    *   Establishes a gRPC channel for ChirpStack communication.
3.  **ChirpStack API Functions:**
    *   The code includes functions to interact with the ChirpStack API for managing gateways, applications, devices, and device profiles. These functions are used as defined in the original `SAMPLE PYTHON CODE`
4.  **`configure_access_point` Function:**
    *   Takes the access point configuration (`ap_config`) and dnsmasq configuration (`dnsmasq_config`) as input
    *   Creates a `fabric.Connection` to the target host (the AP).
    *   Uses the `sudo` method to write the content of `hostapd_config` to `/etc/hostapd/hostapd.conf`, and `dnsmasq_config` to `/etc/dnsmasq.conf`. It iterates through `dnsmasq_config["stations"]` to add the fixed IP configuration for each device.
    *   Restarts the `hostapd` and `dnsmasq` services.
    *   Includes basic error handling using try-except.
5.  **`load_blueprint` Function:**
    *   Takes the path to the JSON blueprint as input.
    *   Loads the JSON data from the file.
6.  **Main Execution Block:**
    *   **Load Blueprint:** Load the blueprint JSON from `TEST BLUEPRINT.json`.
    *   **Step 1:** Creates an application on ChirpStack with the `name` from blueprint.
    *   **Step 2:** If `lora_gateways` exist in the blueprint, it creates those on ChirpStack.
    *   **Step 3:** Creates a device profile required for the LoRa devices.
    *   **Step 4:** If `lora_devices` exist in the blueprint, it iterates through them and creates them on ChirpStack.
    *   **Step 5:** If `wifi_access_point` exists:
        *   Extracts `ap_config` and `dnsmasq_config`
        *   If `wifi_stations` exist:
            *   Extract the `wlan_mac_addr` and `wlan_ip` for each station
        *   Calls `configure_access_point()` to configure the AP.

**How to Use:**

1.  **Install Libraries:**
    bash
    pip install grpcio grpcio-tools chirpstack-api python-dotenv fabric
    
2.  **Set up environment variables:**
    *   Create a `.env` file and define `CHIRPSTACK_URL`, `CHIRPSTACK_API_TOKEN` and `CHIRPSTACK_TENANT_ID`.
3.  **Create a JSON blueprint file:**
    *   Save the `TEST BLUEPRINT` as `TEST BLUEPRINT.json` in the same directory as the script.
4.  **Ensure the remote server has `hostapd` and `dnsmasq` installed:**
5.  **Run the script:**
    bash
    python your_script_name.py
    

**Key Points:**

*   **Error Handling:** The script includes basic `try-except` blocks to catch potential errors.
*   **Fabric Configuration:** The Fabric configuration should be customized according to your environment, especially the `user` and `password` for the target host.
*   **Blueprint Adaptability:** The script is designed to be flexible and work with the `TEST BLUEPRINT` provided. You can modify the JSON file and the script will automatically set up the network according to your needs.
*   **Comments:** The code is commented to explain each part of the script.

**Note:** Make sure that you have the correct configuration for the WiFi Access Point in your JSON blueprint (user, host, password, hostapd and dnsmasq configurations).