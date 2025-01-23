
import os
import json
from fabric import Connection

def configure_hostapd(ap_config):
    hostapd_conf = f"""
    interface=wlan0
    driver=nl80211
    ssid={ap_config['ssid']}
    hw_mode=g
    channel=7
    wmm_enabled=0
    macaddr_acl=0
    auth_algs=1
    ignore_broadcast_ssid=0
    wpa=2
    wpa_passphrase={ap_config['wpa_passphrase']}
    wpa_key_mgmt={ap_config['wpa_key_mgmt']}
    rsn_pairwise=CCMP
    """
    with open('/etc/hostapd/hostapd.conf', 'w') as f:
        f.write(hostapd_conf)

def configure_dnsmasq(stations):
    dnsmasq_conf = """
    interface=wlan0
    dhcp-range=192.168.1.50,192.168.1.150,12h
    """
    for station in stations:
        dnsmasq_conf += f"dhcp-host={station['wlan_MAC_ADDR']},{station['wlan_IP']}\n"
    with open('/etc/dnsmasq.conf', 'w') as f:
        f.write(dnsmasq_conf)

def setup_network(blueprint_file):
    with open(blueprint_file, 'r') as f:
        blueprint = json.load(f)

    wifi_aps = blueprint['network_devices']['wifi_access_points']
    wifi_stations = blueprint['network_devices']['wifi_stations']

    if wifi_aps:
        ap_config = wifi_aps[0]
        configure_hostapd(ap_config)
        configure_dnsmasq(wifi_stations)

        # Use Fabric to manage connections
        conn = Connection(host=ap_config['eth_IP'], user='root', connect_kwargs={'password': 'your_password'})
        conn.sudo('systemctl restart hostapd')
        conn.sudo('systemctl restart dnsmasq')

if __name__ == "__main__":
    setup_network('TEST_BLUEPRINT.json')
