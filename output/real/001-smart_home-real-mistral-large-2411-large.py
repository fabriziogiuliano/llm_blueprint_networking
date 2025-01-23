
import json
import os

from fabric import Connection

def configure_hostapd(ap):
    hostapd_conf = f"""
    interface=wlan0
    ssid={ap['ssid']}
    wpa=2
    wpa_passphrase={ap['wpa_passphrase']}
    wpa_key_mgmt={ap['wpa_key_mgmt']}
    rsn_pairwise=CCMP
    """
    with open('/etc/hostapd/hostapd.conf', 'w') as file:
        file.write(hostapd_conf)

def configure_dnsmasq(ap, stations):
    dnsmasq_conf = f"""
    interface=wlan0
    dhcp-range=192.168.1.10,192.168.1.250,255.255.255.0,12h
    dhcp-option=3,{ap['wlan_IP']}
    dhcp-option=6,{ap['wlan_IP']}
    """
    for station in stations:
        dnsmasq_conf += f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n"

    with open('/etc/dnsmasq.conf', 'w') as file:
        file.write(dnsmasq_conf)

def configure_wifi_ap(ap, stations):
    configure_hostapd(ap)
    configure_dnsmasq(ap, stations)
    os.system('systemctl restart hostapd')
    os.system('systemctl restart dnsmasq')

def configure_lora_devices(blueprint):
    if 'lora_devices' in blueprint:
        for device in blueprint['lora_devices']:
            # Add your LoRa device configuration code here
            pass

def main():
    with open('TEST_BLUEPRINT.json', 'r') as file:
        blueprint = json.load(file)

    if 'wifi_ap' in blueprint and blueprint['wifi_ap']:
        ap = blueprint['wifi_ap'][0]
        stations = blueprint['wifi_stations']
        configure_wifi_ap(ap, stations)

    configure_lora_devices(blueprint)

if __name__ == "__main__":
    main()
