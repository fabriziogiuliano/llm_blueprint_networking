
import os
import json
from fabric import Connection

# Load the TEST BLUEPRINT
with open('TEST_BLUEPRINT.json') as f:
    blueprint = json.load(f)

# WiFi Access Point Configuration
if 'wifi_ap' in blueprint:
    ap = blueprint['wifi_ap'][0]
    ap_id = ap['id']
    ssid = ap['ssid']
    wpa_passphrase = ap['wpa_passphrase']
    wpa_key_mgmt = ap['wpa_key_mgmt']
    wlan_ip = ap['wlan_IP']
    eth_ip = ap['eth_IP']

    # Configure hostapd.conf
    hostapd_conf = f"""
    interface=wlan0
    driver=nl80211
    ssid={ssid}
    hw_mode=g
    channel=7
    wmm_enabled=0
    macaddr_acl=0
    auth_algs=1
    ignore_broadcast_ssid=0
    wpa=2
    wpa_passphrase={wpa_passphrase}
    wpa_key_mgmt={wpa_key_mgmt}
    rsn_pairwise=CCMP
    """

    with open('/etc/hostapd/hostapd.conf', 'w') as f:
        f.write(hostapd_conf)

    # Configure dnsmasq.conf
    dnsmasq_conf = f"""
    interface=wlan0
    dhcp-range={wlan_ip},192.168.1.254,12h
    dhcp-option=3,{wlan_ip}
    dhcp-option=6,{wlan_ip}
    server=8.8.8.8
    server=8.8.4.4
    log-queries
    log-dhcp
    listen-address={wlan_ip}
    bind-interfaces
    """

    for station in blueprint['wifi_stations']:
        dnsmasq_conf += f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n"

    with open('/etc/dnsmasq.conf', 'w') as f:
        f.write(dnsmasq_conf)

    # Restart services
    conn = Connection(host=eth_ip, user='root', connect_kwargs={'password': '123456'})
    conn.sudo('systemctl restart hostapd')
    conn.sudo('systemctl restart dnsmasq')

# LoRa Devices Configuration
if 'lora_devices' in blueprint:
    from dotenv import load_dotenv
    load_dotenv()
    server = os.getenv('CHIRPSTACK_URL')
    api_token = os.getenv('CHIRPSTACK_API_TOKEN')
    tenant_id = os.getenv('CHIRPSTACK_TENANT_ID')

    import grpc
    from chirpstack_api import api

    channel = grpc.insecure_channel(server)
    auth_token = [("authorization", "Bearer %s" % api_token)]

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

    # Assuming application and device profile are already created
    application_id = 'your_application_id'
    device_profile_id = 'your_device_profile_id'

    for device in blueprint['lora_devices']:
        CreateDevice(
            dev_eui=device['dev_eui'],
            name=device['name'],
            application_id=application_id,
            device_profile_id=device_profile_id,
            application_key=device['application_key']
        )
